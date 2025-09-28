/*
 * drivers/staging/dspg/coma/coma.c - the cordless manager
 *
 * The cordless manager is responsible for maintaining the main cfifo between
 * Linux and cordless which is used for loading and unloading cordless and for
 * registration of other services (e.g., debug output, VoIP/RTP, etc.)
 *
 * Copyright (C) 2007 NXP Semiconductors
 * Copyright (C) 2008 - 2016 DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <asm/uaccess.h>
#include <linux/coma/cfifo.h>
#include <linux/coma/coma.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/coma/coma-alsa.h>
#include <linux/coma/coma-sharedmem.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/sched/types.h>
#include <linux/string.h>
#include <uapi/linux/sched/types.h>

#include "coma-signaling.h"
#include "coma-tdm.h"
#include "coma-voice.h"
#include "coma-dvf9a.h"

#include "cmsg-coma.h"

#define MAX_SERVICES CONFIG_DSPG_COMA_MAX_SERVICES

struct coma_service {
	const char *name;
	void *arg;
	struct cfifo __rcu *l2c;
	struct cfifo __rcu *c2l;
	atomic_t received_bytes;
	atomic_t sent_bytes;
	coma_process_cb_t process_message;
	coma_remove_cb_t remove;
	coma_suspend_cb_t suspend;
	unsigned int sched_policy;
	struct sched_param kparam;
	struct task_struct *thread;
	wait_queue_head_t wq;
	atomic_t update;
};

static int setup_result;
static DECLARE_COMPLETION(setup_reply);
static DECLARE_COMPLETION(remove_reply);
static DEFINE_MUTEX(coma_mutex);
static unsigned long active_services[BITS_TO_LONGS(MAX_SERVICES)];
static struct coma_service services[MAX_SERVICES];
static struct workqueue_struct *receive_wq;
static struct device *css_dev;

/*
 * Statistics
 */
static ssize_t show_services(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	unsigned int i = 1;
	ssize_t len = 0;

	mutex_lock(&coma_mutex);
	for_each_set_bit_from(i, active_services, MAX_SERVICES) {
		len += snprintf(buf + len, PAGE_SIZE - len, "%d", i);
		if (services[i].name)
			len += snprintf(buf + len, PAGE_SIZE - len,
				" %s", services[i].name);

		len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	}
	mutex_unlock(&coma_mutex);

	return len;
}

static ssize_t show_cfifo(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	unsigned int i = 1;
	ssize_t len = 0;

	mutex_lock(&coma_mutex);
	for_each_set_bit_from(i, active_services, MAX_SERVICES) {
		struct cfifo *l2c, *c2l;

		l2c = rcu_dereference_protected(services[i].l2c,
			mutex_is_locked(&coma_mutex));
		c2l = rcu_dereference_protected(services[i].c2l,
			mutex_is_locked(&coma_mutex));

		len += snprintf(buf + len, PAGE_SIZE - len, "%d %u %s %u %s\n",
				i, cfifo_size(l2c),
				cfifo_empty(l2c) ? "empty" : "filled",
				cfifo_size(c2l),
				cfifo_empty(c2l) ? "empty" : "filled");
	}
	mutex_unlock(&coma_mutex);

	return len;
}

static ssize_t show_stats(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int i = 1;
	ssize_t len = 0;

	mutex_lock(&coma_mutex);
	for_each_set_bit_from(i, active_services, MAX_SERVICES) {
		len += snprintf(buf + len, PAGE_SIZE - len,
			"%d %d %d\n", i,
			atomic_read(&services[i].received_bytes),
			atomic_read(&services[i].sent_bytes));
	}
	mutex_unlock(&coma_mutex);

	return len;
}

static ssize_t reset_stats(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned long id;

	id = simple_strtoul(buf, NULL, 0);
	if (id == 0 || id >= MAX_SERVICES)
		return -EINVAL;

	atomic_set(&services[id].received_bytes, 0);
	atomic_set(&services[id].sent_bytes, 0);

	return count;
}

static ssize_t show_priority(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	unsigned int i = 1;
	ssize_t len = 0;

	mutex_lock(&coma_mutex);

	for_each_set_bit_from(i, active_services, MAX_SERVICES) {
		len += snprintf(buf + len, PAGE_SIZE - len, "%d", i);
		if (len >= PAGE_SIZE)
			break;

		if (services[i].name) {
			len += snprintf(buf + len, PAGE_SIZE - len,
			" %s, ", services[i].name);
			if (len >= PAGE_SIZE)
				break;

			len += snprintf(buf + len, PAGE_SIZE - len,
			" Policy: %d,", services[i].sched_policy);
			if (len >= PAGE_SIZE)
				break;

			len += snprintf(buf + len, PAGE_SIZE - len,
			" Priority: %d", services[i].kparam.sched_priority);
			if (len >= PAGE_SIZE)
				break;
		}
		len += snprintf(buf + len, PAGE_SIZE - len, "\n");
		if (len >= PAGE_SIZE)
			break;
	}

	/*If the buffer is exhausted, display error to user
	len is anyways PAGE_SIZE, we can return as it is*/
	if (len >= PAGE_SIZE)
		pr_err("coma service priority contents are truncated! \n");

	mutex_unlock(&coma_mutex);
	return len;
}

static ssize_t set_priority(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	char *service, *priorityStr, *policyStr, *parseStr, *origStr;
	int priority, policy;
	int ret = -EINVAL;
	unsigned int i = 1;

	if((origStr = kstrdup(buf, GFP_KERNEL)) == NULL)
		return -ENOMEM;

	/* Assign to new pointer for parsing */
	parseStr = origStr;

	/*
	 * We expect value in the format "service:policy:priority"
	 * For missing information, we return default -EINVAL
	 */
	if((service = strsep(&parseStr,":")) == NULL)
		goto set_priority_out_free_str;

	if((policyStr = strsep(&parseStr,":")) == NULL)
		goto set_priority_out_free_str;

	if((priorityStr = strsep(&parseStr,":")) == NULL)
		goto set_priority_out_free_str;

	priority = simple_strtoul(priorityStr,  NULL, 0);
	policy = simple_strtoul(policyStr, NULL, 0);

	/*
	 * No need to validate the priority/policy as the sched_setscheduler
	 * will do it
	 */
	mutex_lock(&coma_mutex);
	for_each_set_bit_from(i, active_services, MAX_SERVICES) {
		if (strcmp(services[i].name, service) == 0) {
			/*
			 * Found the service, set the priority/policy &
			 * return count
			 */
			struct sched_param param;

			param.sched_priority = priority;

			ret = sched_setscheduler(services[i].thread, policy,
						 &param);
			if (ret) {
				pr_err("%s: sched_setscheduler Error %d: \n",
				       services[i].name, ret);
				goto set_priority_out_unlock;
			}
			services[i].kparam.sched_priority = priority;
			services[i].sched_policy = policy;
			/*
			 * Set is successful, need to return "count" bytes
			 * back
			 */
			ret = count;
			goto set_priority_out_unlock;
		}
	}
set_priority_out_unlock:
	mutex_unlock(&coma_mutex);
set_priority_out_free_str:
	kfree(origStr);
	return ret;
}

static struct device_attribute coma_attrs[] = {
__ATTR(services,	S_IRUGO,		show_services,	0),
__ATTR(cfifo,		S_IRUGO,		show_cfifo,	0),
__ATTR(statistics,	S_IRUGO | S_IWUSR,	show_stats,	reset_stats),
__ATTR(priority,	S_IRUGO | S_IWUSR,	show_priority,	set_priority),
};

static int coma_sysfs_init(struct device *dev)
{
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(coma_attrs); i++) {
		ret = device_create_file(dev, &coma_attrs[i]);
		if (ret)
			goto err;
	}

	return 0;
err:
	while (--i >= 0)
		device_remove_file(dev, &coma_attrs[i]);
	pr_err("coma: failed creating sysfs file %d\n", ret);
	return ret;
}

static void coma_sysfs_release(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(coma_attrs); i++)
		device_remove_file(dev, &coma_attrs[i]);
}

/*
 * CMsg handling
 */

struct cmsg *coma_cmsg_alloc(unsigned int service, int type,
			     unsigned int params_size,
			     unsigned int payload_size)
{
	struct cfifo *cfifo;
	struct cmsg *p = ERR_PTR(-ENODEV);

	rcu_read_lock();
	cfifo = rcu_dereference(services[service].l2c);
	if (!cfifo)
		goto out_err;

	p = cfifo_request(cfifo, sizeof(*p) + params_size + payload_size);
	if (IS_ERR(p))
		goto out_err;

	p->type = type;
	p->params_size = params_size;
	p->payload_size = payload_size;

	/* not quite accurate to do it here but who cares... */
	atomic_add(params_size + payload_size, &services[service].sent_bytes);

	/* keep on holding RCU read lock */
	return p;

out_err:
	rcu_read_unlock();
	return p;
}
EXPORT_SYMBOL(coma_cmsg_alloc);

int coma_cmsg_commit(unsigned int service)
{
	struct cfifo *cfifo;
	int ret = -ENODEV;

	/* the RCU read lock is still held from coma_cmsg_alloc() */

	cfifo = rcu_dereference(services[service].l2c);
	if (!cfifo)
		goto out;

	ret = cfifo_commit(cfifo);
	if (ret == 0)
		coma_signal(service);

out:
	rcu_read_unlock();
	return ret;
}
EXPORT_SYMBOL(coma_cmsg_commit);

void coma_cmsg_abort(unsigned int service)
{
	/*
	 * Nothing to do in the CFIFO. The caller can invoke coma_cmsg_alloc()
	 * again for the next message and it will happily reuse the just
	 * allocated space.
	 */

	rcu_read_unlock();
}
EXPORT_SYMBOL(coma_cmsg_abort);

int coma_cmsg_send(unsigned int service, int type,
		   void *params, unsigned int params_size,
		   void *payload, unsigned int payload_size)
{
	struct cmsg *cmsg;

	cmsg = coma_cmsg_alloc(service, type, params_size, payload_size);
	if (IS_ERR(cmsg))
		return PTR_ERR(cmsg);

	if (params)
		memcpy(cmsg_params(cmsg), params, params_size);

	if (payload)
		memcpy(cmsg_payload(cmsg), payload, payload_size);

	return coma_cmsg_commit(service);
}
EXPORT_SYMBOL(coma_cmsg_send);

/*
 * message handling
 */

static int coma_receive_kthread(void *data)
{
	struct coma_service *service = (struct coma_service *)data;
	coma_process_cb_t cb;
	struct cfifo *cfifo;
	struct cmsg *cmsg;
	int len;
	void *arg;
	int ret;

	ret = sched_setscheduler(current, service->sched_policy,
				 &service->kparam);
	if (ret) {
		pr_err("could not set scheduling parameters %s\n",
		       ret == -EINVAL? "<invalid argument>":
		       ret == -EPERM? "<permission denied>": "");
	}

	do {
		wait_event_interruptible(service->wq,
					 kthread_should_stop() ||
					 atomic_read(&service->update) == 1);

		if (kthread_should_stop())
			break;

		if (atomic_read(&service->update) != 1)
			continue;

		atomic_set(&service->update, 0);

		cb = service->process_message;
		if (!cb)
			continue;

		rcu_read_lock();
		cfifo = rcu_dereference(service->c2l);
		if (!cfifo) {
			rcu_read_unlock();
			continue;
		}
		cfifo_ref(cfifo); /* make sure CFIFO doesn't disappear */
		rcu_read_unlock();

		arg = service->arg;
		while ((len = cfifo_get(cfifo, (void **)&cmsg)) > 0) {
			cb(arg, cmsg);
			cfifo_processed(cfifo);

			atomic_add(cmsg->params_size + cmsg->payload_size,
			&service->received_bytes);
		}

		cfifo_unref(cfifo);
	} while (!kthread_should_stop());

	return 0;
}

void coma_receive(unsigned int i)
{
	if (test_bit(i, active_services)) {
		atomic_set(&services[i].update, 1);
		wake_up_interruptible(&services[i].wq);
	}
}

/*
 * service (de)registration
 */

static int
coma_create_coma_message(enum cmsg_coma_types type,
                         union cmsg_coma_params *params)
{
	return coma_cmsg_send(0, type, params, sizeof(*params), NULL, 0);
}

int coma_register(const char *name, unsigned int size, void *arg,
		  coma_process_cb_t process_message, coma_remove_cb_t remove,
		  coma_suspend_cb_t suspend)
{
	struct cfifo *l2c, *c2l;
	union cmsg_coma_params params;
	int id, ret;

	if (strlen(name) >= 16)
		return -EINVAL;

	mutex_lock(&coma_mutex);

	if (!test_bit(0, active_services)) {
		ret = -ENODEV;
		goto done;
	}

	/* service already registered? */
	for_each_set_bit(id, active_services, MAX_SERVICES) {
		if (strcmp(services[id].name, name) == 0) {
			ret = -EBUSY;
			goto done;
		}
	}

	/* find free service number, starting from 1 (0 == COMA service) */
	id = find_next_zero_bit(active_services, MAX_SERVICES, 1);
	if (id >= MAX_SERVICES) {
		ret = -EMFILE;
		goto done;
	}

	/* allocate FIFOs */
	l2c = cfifo_alloc(css_dev, size, NULL, 0);
	if (!l2c) {
		ret = -ENOMEM;
		goto done;
	}
	c2l = cfifo_alloc(css_dev, size, NULL, 0);
	if (!c2l) {
		cfifo_unref(l2c);
		ret = -ENOMEM;
		goto done;
	}

	/*
	 * Already mark the service as active because once we send the
	 * registration message we might immediately get messages from the
	 * cordless domain.
	 */
	services[id].name = kstrdup(name, GFP_KERNEL);
	services[id].arg = arg;
	rcu_assign_pointer(services[id].l2c, l2c);
	rcu_assign_pointer(services[id].c2l, c2l);
	services[id].process_message = process_message;
	services[id].remove = remove;
	services[id].suspend = suspend;
	atomic_set(&services[id].received_bytes, 0);
	atomic_set(&services[id].sent_bytes, 0);

	init_waitqueue_head(&services[id].wq);
	atomic_set(&services[id].update, 1);

	services[id].sched_policy = SCHED_NORMAL;
	services[id].kparam.sched_priority = 0;

	/* spawn coma receive thread */
	services[id].thread = kthread_run(coma_receive_kthread, &services[id],
					  name);
	if (IS_ERR(services[id].thread)) {
		ret = -ENOMEM;
		goto err;
	}

	wmb();

	/* register on cordless domain */
	params.request_register.id = id;
	strcpy(params.request_register.name, name);
	params.request_register.l2c = cfifo_phys(l2c);
	params.request_register.c2l = cfifo_phys(c2l);
	ret = coma_create_coma_message(CMSG_COMA_REQUEST_REGISTER, &params);
	if (ret)
		goto err;

	/* wait for reply from cordless domain */
	ret = wait_for_completion_timeout(&setup_reply, HZ);
	if (ret <= 0) {
		if (ret == 0)
			ret = -ETIMEDOUT;
		goto err;
	}

	if (setup_result != 0) {
		ret = -ECONNREFUSED;
		goto err;
	}

	set_bit(id, active_services);

	ret = id;
	goto done;

err:
	rcu_assign_pointer(services[id].l2c, NULL);
	rcu_assign_pointer(services[id].c2l, NULL);
	services[id].process_message = NULL;
	services[id].remove = NULL;
	services[id].suspend = NULL;
	kfree(services[id].name);
	services[id].name = NULL;

	/*
	 * No need to synchronize with RCU here: nobody knows about the CFIFOs
	 * yet.
	 */
	cfifo_unref(c2l);
	cfifo_unref(l2c);
done:
	mutex_unlock(&coma_mutex);
	return ret;
}
EXPORT_SYMBOL(coma_register);

int coma_deregister(unsigned int id)
{
	struct cfifo *l2c, *c2l;
	union cmsg_coma_params params;
	int ret = 0;

	if (id >= MAX_SERVICES)
		return -EINVAL;

	mutex_lock(&coma_mutex);

	if (!test_bit(id, active_services))
		goto done;

	l2c = rcu_dereference_protected(services[id].l2c,
		mutex_is_locked(&coma_mutex));
	c2l = rcu_dereference_protected(services[id].c2l,
		mutex_is_locked(&coma_mutex));

	/*
	 * Only clear FIFO pointers first. Some other thread might still be
	 * writing into a CFIFO, so we have to wait for RCU until it is safe to
	 * free them.
	 */
	clear_bit(id, active_services);;
	wmb();
	rcu_assign_pointer(services[id].l2c, NULL);
	rcu_assign_pointer(services[id].c2l, NULL);
	services[id].process_message = NULL;
	services[id].remove = NULL;
	services[id].suspend = NULL;
	kfree(services[id].name);
	services[id].name = NULL;

	/* Unregister from cordless domain. Wait for reply if message could be
	 * sent. */
	params.request_deregister.id = id;
	ret = coma_create_coma_message(CMSG_COMA_REQUEST_DEREGISTER, &params);
	if (ret == 0) {
		if (wait_for_completion_timeout(&remove_reply, HZ) == 0)
			pr_warn("coma deregister timed out!\n");
	}

	kthread_stop(services[id].thread);

	/* synchronize with RCU and then safely free the CFIFos */
	synchronize_rcu();
	cfifo_unref(c2l);
	cfifo_unref(l2c);

done:
	mutex_unlock(&coma_mutex);
	return ret;
}
EXPORT_SYMBOL(coma_deregister);

static void coma_process_message(void *arg, struct cmsg *cmsg)
{
	union cmsg_coma_params *params = cmsg_params(cmsg);

	switch(cmsg->type) {
	case CMSG_COMA_REPLY_REGISTER:
		setup_result = params->reply_register.result;
		complete(&setup_reply);
		break;

	case CMSG_COMA_REPLY_DEREGISTER:
		complete(&remove_reply);
		break;
	}
}

/*
 * start-up and release
 */

void coma_init_services(void)
{
	tdm_service_init();
	voice_init(css_dev);
	dvf9a_coma_service_init();
	alsa_service_init();
	sharedmem_coma_service_init();
}
EXPORT_SYMBOL(coma_init_services);


int coma_suspend(void)
{
	int i;
	int ret = 0;

	mutex_lock(&coma_mutex);
	for_each_set_bit(i, active_services, MAX_SERVICES) {
		if (services[i].suspend)
			ret = services[i].suspend();
		if (ret)
			break;
	}

	mutex_unlock(&coma_mutex);
	return ret;
}
EXPORT_SYMBOL(coma_suspend);

int coma_setup(struct device *dev, dma_addr_t *l2c_phys, dma_addr_t *c2l_phys)
{
	int ret;
	struct cfifo *l2c, *c2l;

	/* guard against concurrent coma_register() calls */
	mutex_lock(&coma_mutex);

	css_dev = dev;

	receive_wq = alloc_workqueue("coma", 0, 0);
	if (!receive_wq) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	l2c = cfifo_alloc(dev, 1024, NULL, 0);
	if (!l2c) {
		ret = -ENOMEM;
		goto out_free_wq;
	}

	c2l = cfifo_alloc(dev, 1024, NULL, 0);
	if (!c2l) {
		ret = -ENOMEM;
		goto out_free_l2c;
	}

	memset(services, 0, sizeof(services));
	services[0].name = kstrdup("coma", GFP_KERNEL);
	rcu_assign_pointer(services[0].l2c, l2c);
	rcu_assign_pointer(services[0].c2l, c2l);
	services[0].process_message = coma_process_message;
	init_waitqueue_head(&services[0].wq);
	atomic_set(&services[0].update, 0);

	services[0].sched_policy = SCHED_NORMAL;
	services[0].kparam.sched_priority = 0;

	/* spawn coma receive thread */
	services[0].thread = kthread_run(coma_receive_kthread, &services[0],
					 "coma");
	if (IS_ERR(services[0].thread)) {
		ret = -ENOMEM;
		goto out_free_l2c;
	}
	wmb();
	set_bit(0, active_services);

	ret = coma_signaling_init();
	if (ret)
		goto out_free_c2l;

	ret = coma_sysfs_init(dev);
	if (ret)
		goto out_sig_exit;

	*l2c_phys = cfifo_phys(l2c);
	*c2l_phys = cfifo_phys(c2l);

	mutex_unlock(&coma_mutex);

	return 0;

out_sig_exit:
	coma_signaling_exit();
out_free_c2l:
	clear_bit(0, active_services);
	kfree(services[0].name);
	memset(services, 0, sizeof(services));
	cfifo_unref(c2l);
out_free_l2c:
	cfifo_unref(l2c);
out_free_wq:
	flush_workqueue(receive_wq);
	destroy_workqueue(receive_wq);
out_unlock:
	mutex_unlock(&coma_mutex);
	return ret;
}
EXPORT_SYMBOL(coma_setup);

void coma_release(struct device *dev)
{
	int i;

	/* tear down all services forcefully */
	mutex_lock(&coma_mutex);

	coma_sysfs_release(dev);
	coma_signaling_exit();
	flush_workqueue(receive_wq);
	destroy_workqueue(receive_wq);

	for_each_set_bit(i, active_services, MAX_SERVICES) {
		struct cfifo *l2c, *c2l;

		kthread_stop(services[i].thread);

		l2c = rcu_dereference_protected(services[i].l2c,
			mutex_is_locked(&coma_mutex));
		c2l = rcu_dereference_protected(services[i].c2l,
			mutex_is_locked(&coma_mutex));

		/*
		 * Only clear FIFO pointers first. Some other thread might
		 * still be writing into a CFIFO, so we have to wait for RCU
		 * until it is safe to free them.
		 */
		clear_bit(i, active_services);
		rcu_assign_pointer(services[i].l2c, NULL);
		rcu_assign_pointer(services[i].c2l, NULL);
		kfree(services[i].name);

		/* synchronize with RCU and then safely free the CFIFos */
		synchronize_rcu();
		cfifo_unref(c2l);
		cfifo_unref(l2c);

		if (services[i].remove)
			services[i].remove(services[i].arg);
	}
	memset(services, 0, sizeof(services));
	mutex_unlock(&coma_mutex);
}
EXPORT_SYMBOL(coma_release);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
