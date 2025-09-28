// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Jie Xi <jie.xi@synaptics.com>
 *
 */

#define pr_fmt(fmt) "[berlin_bm kernel driver] " fmt

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/sched/task.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/utsname.h>
#include <linux/mod_devicetable.h>
#include "berlin_meta.h"
#include "heap_extra.h"
#include "uapi/bm.h"
#include <uapi/linux/dma-buf.h>
#include "ptm_wrapper.h"
#include <linux/delay.h>
#include "kernel_compatibility.h"

#if !IS_ENABLED(CONFIG_OPTEE)
#include "tz_driver.h"
#endif

#define BERLIN_BM_DEVICE_NAME "berlin-bm"

struct berlin_bm_device {
	struct device *dev;
	struct miscdevice mdev;
	struct rb_root ptes;
	struct rw_semaphore pte_lock;
	struct list_head pte_list;
	struct ptm_context ptm;
	struct dentry *debug_root;
	struct dentry *pte_debug_root;
};

struct berlin_bm_client {
	struct berlin_bm_device *dev;
	const char *name;
	struct task_struct *task;
	pid_t pid;
};

union bm_ioctl_arg {
	struct bm_fb_data fb;
	struct bm_pt_data pt;
};

#define FREE_PT_TRY_COUNT     10
#define FREE_PT_TRY_INTERVAL  10

static struct berlin_bm_device bm_dev;

static inline bool is_cust_dmabuf(struct dma_buf *dmabuf)
{
	unsigned long flags;

	if (dma_buf_get_flags(dmabuf, &flags))
		return false;

	if (flags & DMA_BUF_FLAG_CUST_HEAP)
		return true;

	return false;
}

static struct berlin_meta *dmabuf_get_meta(struct dma_buf *dmabuf)
{
	unsigned long dmabuf_flags;
	berlin_meta_t *data;

	if (dma_buf_get_flags(dmabuf, &dmabuf_flags))
		return NULL;

	if (dmabuf_flags & DMA_BUF_FLAG_CUST_HEAP) {
		data = (berlin_meta_t *) (*(unsigned long *)(dmabuf->priv));
	} else
		return NULL;
	data->pte.dmabuf_flags = dmabuf_flags;

	return data;
}

struct berlin_meta *bm_fetch_meta(struct dma_buf *dmabuf)
{
	return dmabuf_get_meta(dmabuf);
}
EXPORT_SYMBOL_NS(bm_fetch_meta, SYNA_BM);

static void print_pte_node(struct seq_file *s, int i, struct berlin_pte_node *pn)
{
	struct page *page;
	phys_addr_t phys;
	size_t size;
	struct dma_buf_attachment *attach;
	struct sg_table *sg_table;

	attach = dma_buf_attach(pn->dmabuf, bm_dev.dev);
	if (IS_ERR(attach)) {
		pr_err("attach dmabuf %p failed\n", pn->dmabuf);
		return;
	}

	attach->dma_map_attrs |= DMA_ATTR_SKIP_CPU_SYNC;
	sg_table = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sg_table)) {
                dma_buf_detach(pn->dmabuf, attach);
		pr_err("map dmabuf %p failed\n", pn->dmabuf);
		return;
	}

	page = sg_page(sg_table->sgl);

	phys = PFN_PHYS(page_to_pfn(page));
	size = pn->dmabuf->size;
	seq_printf(s, "|%4d|%4ld|%8d|%16s|%8d|%16s|%8llx|%8zx|%8llx|%8zx|\n",
		   i, atomic_long_read(&pn->dmabuf->file->f_count),
		   pn->pid, pn->task_comm,
		   pn->tid, pn->thread_name,
		   phys, size,
		   pn->phy_addr_pt, pn->len_pt);

	dma_buf_unmap_attachment(attach, sg_table, DMA_BIDIRECTIONAL);
	dma_buf_detach(pn->dmabuf, attach);
}

static int debug_pte_show(struct seq_file *s, void *unused)
{
	struct berlin_pte_node *pn;
	int i = 0;

	seq_puts(s, "List of allocated PTE entries\n");
	seq_puts(s, "|No  |ref |  pid   |  process_name  |   tid  |   thread_name  |physaddr|  size  |pte addr|pte size|\n");
	seq_puts(s, "-----------------------------------------------------------------------------------------------------------\n");

	down_write(&bm_dev.pte_lock);
	list_for_each_entry(pn, &bm_dev.pte_list, list) {
		print_pte_node(s, i, pn);
		i++;
	}
	up_write(&bm_dev.pte_lock);

	seq_puts(s, "-----------------------------------------------------------------------------------------------------------\n");

	return 0;
}

static int debug_pte_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_pte_show, inode->i_private);
}

static const struct file_operations debug_pte_fops = {
	.open = debug_pte_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef BM_PAGE_DEBUG
static void print_buf_pages(struct sg_table *sg_table)
{
	phys_addr_t pphy = 0, cphy = 0;
	struct sg_page_iter iter;
	int i = 0;

	for_each_sg_page(sg_table->sgl, &iter, sg_table->nents, 0) {
		p = sg_page_iter_page(&iter);
		cphy = PFN_PHYS(page_to_pfn(p));
		if (pphy == 0 || cphy - pphy != PAGE_SIZE) {
			if (i != 0) {
				pr_debug("total %d\n", i);
				pr_debug("phy addr: %pa\n", &cphy);
				i = 1;
			} else {
				i++;
			}
			pphy = cphy;
		}
	}
	pr_debug("total %d\n", i);
}
#endif

static int create_pte(struct dma_buf *dmabuf, u32 flags,
		      struct bm_fb_param *fb_param,
		      struct bm_pt_param *pt_param)
{
	int ret = 0;
	phys_addr_t *addr_array = NULL;
	struct scatterlist *sg;
	struct page *p;
	size_t len;
	int i = 0;
	struct dma_buf_attachment *attach;
	struct sg_table *sg_table;
#if IS_ENABLED(CONFIG_OPTEE)
	struct tee_shm *handle = NULL;

	handle = bm_dev.ptm.sg_shm;
	addr_array = handle->kaddr;
#else
	TEEC_SharedMemory *handle = NULL;

	handle = bm_dev.ptm.sg_shm;
	addr_array = handle->buffer;
#endif

	attach = dma_buf_attach(dmabuf, bm_dev.dev);
	if (IS_ERR(attach)) {
		pr_err("attach dmabuf %p failed\n", dmabuf);
		return PTR_ERR(attach);
	}

	attach->dma_map_attrs |= DMA_ATTR_SKIP_CPU_SYNC;
	sg_table = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sg_table)) {
		pr_err("map dmabuf %p failed\n", dmabuf);
		goto free2;
	}
	pr_debug("number of sg_table: %d, flags: %x\n",
		sg_table->nents, flags);
	pr_debug("buffer size: 0x%lx\n", dmabuf->size);
	len = sg_table->nents * 2 * sizeof(phys_addr_t);

	if (len > PTM_MAX_SG_SHM) {
		pr_err("fail to allocate addr array, %zx exceeds %zx\n",
			len, PTM_MAX_SG_SHM);
		ret = -ENOMEM;
		goto free1;
	}


	for_each_sg(sg_table->sgl, sg, sg_table->nents, i) {
		p = sg_page(sg);
		addr_array[2 * i] = PFN_PHYS(page_to_pfn(p));
		addr_array[2 * i + 1] = sg->length / PAGE_SIZE;
		pr_debug("[%d]start phy addr: %pa\n",
			  i, &addr_array[2 * i]);
		pr_debug("page number: %pa\n", &addr_array[2 * i + 1]);
	}
#ifdef BM_PAGE_DEBUG
	print_buf_pages(sg_table);
#endif
	pr_debug("sg list copy");

	/* data write back would be prevented by SZC, remove invalidate cache here */

	ret = ptm_wrapper_construct_pt(&bm_dev.ptm, handle, len,
				       pt_param, flags, fb_param);
	if (ret) {
		pr_err("fail to construct pt: %x\n", ret);
		/*
		 * return normal kernel error code instead of
		 * TEEC error code directly
		 */
		ret = -EINVAL;
	} else
		pr_debug("pt constructed %llx %llx", pt_param->phy_addr, pt_param->mem_id);

free1:
	dma_buf_unmap_attachment(attach, sg_table, DMA_BIDIRECTIONAL);
free2:
	dma_buf_detach(dmabuf, attach);
	return ret;
}

static struct berlin_pte_node *create_pte_node(struct dma_buf *dmabuf,
					       u32 flags,
					       struct bm_fb_param *fb_param,
					       struct bm_pt_param *pt_param)
{
	struct berlin_meta *data;
	int ret;

	data = dmabuf_get_meta(dmabuf);
	if (!data)
		return ERR_PTR(-EINVAL);

	ret = create_pte(dmabuf, flags, fb_param, pt_param);
	if (ret)
		return ERR_PTR(ret);

	data->pte.dmabuf = dmabuf;
	data->pte.flags = flags;
	data->pte.phy_addr_pt = pt_param->phy_addr;
	data->pte.len_pt = pt_param->len;
	data->pte.mem_id = pt_param->mem_id;
	data->pte.is_pte = 1;

	get_task_comm(data->pte.task_comm, current->group_leader);
	data->pte.pid = task_tgid_vnr(current);
	strncpy(data->pte.thread_name, current->comm, TASK_COMM_LEN);
	data->pte.tid = task_pid_vnr(current);

	pr_debug("%s %d %p ret %x\n", __func__, data->pte.is_pte, dmabuf, ret);
	return &(data->pte);
}

static int get_pte_node(struct dma_buf *dmabuf, struct bm_pt_param *pt_param)
{
	struct berlin_meta *data;
	int ret;

	data = dmabuf_get_meta(dmabuf);
	if (!data)
		return -EINVAL;

	if (data->pte.is_pte) {
		pt_param->phy_addr = data->pte.phy_addr_pt;
		pt_param->len = data->pte.len_pt;
		pt_param->mem_id = data->pte.mem_id;
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

int bm_create_pt(struct dma_buf *dmabuf, u32 flags,
		 struct bm_fb_param *fb_param, struct bm_pt_param *pt_param)
{
	struct berlin_pte_node *pn;
	int ret;

	if (!is_cust_dmabuf(dmabuf))
		return -EBADF;

	down_write(&bm_dev.pte_lock);
	ret = get_pte_node(dmabuf, pt_param);
	if (ret) {
		pn = create_pte_node(dmabuf, flags, fb_param, pt_param);
		if (IS_ERR(pn)) {
			ret = PTR_ERR(pn);
		} else {
			list_add(&pn->list, &bm_dev.pte_list);
			ret = 0;
		}
	}
	up_write(&bm_dev.pte_lock);

	return ret;
}
EXPORT_SYMBOL_NS(bm_create_pt, SYNA_BM);

static int bm_alloc_pt(int fd, u32 flags, struct bm_fb_param *fb_param,
		       struct bm_pt_param *pt_param)
{
	struct dma_buf *dmabuf;
	int ret;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("failed to get dmabuf from fd %d: %ld\n", fd,
		       PTR_ERR(dmabuf));
		return PTR_ERR(dmabuf);
	}

	ret = bm_create_pt(dmabuf, flags, fb_param, pt_param);
	if (ret) {
		if (ret == -EBADF) {
			pr_err("fd(%d) is not the cust heap type\n", fd);
		}
	} else
		pr_debug("pte allocated %p, pt %pa, mem id %llx\n",
			dmabuf, &pt_param->phy_addr, pt_param->mem_id);
	/*
	 * pt lifecycle same as fd, not inc the ref to
	 * simplify user and transparency
	 */
	dma_buf_put(dmabuf);
	return ret;
}

int bm_fetch_pt(struct dma_buf *dmabuf, struct bm_pt_param *pt_param)
{
	int ret;

	if (!is_cust_dmabuf(dmabuf))
		return -EBADF;

	down_read(&bm_dev.pte_lock);
	ret = get_pte_node(dmabuf, pt_param);
	up_read(&bm_dev.pte_lock);

	return ret;
}
EXPORT_SYMBOL_NS(bm_fetch_pt, SYNA_BM);

static int bm_get_pt(int fd, struct bm_pt_param *pt_param)
{
	struct dma_buf *dmabuf;
	int ret;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("failed to get dmabuf from fd %d: %ld\n", fd,
		       PTR_ERR(dmabuf));
		return PTR_ERR(dmabuf);
	}

	ret = bm_fetch_pt(dmabuf, pt_param);
	if (ret) {
		if (ret == -EBADF) {
			pr_err("fd(%d) is not the cust heap type\n", fd);
		} else {
			pr_err("no pte found ret %d\n", ret);
		}
	}
	dma_buf_put(dmabuf);

	return ret;
}

static int pte_node_destroy_by_callback(struct berlin_pte_node *pn)
{
	int ret = 0;

	pr_debug("start free pte node %pa, dma buf %p, ref %ld\n",
		&pn->phy_addr_pt, pn->dmabuf,
		atomic_long_read(&pn->dmabuf->file->f_count));

	down_write(&bm_dev.pte_lock);
	list_del(&pn->list);
	up_write(&bm_dev.pte_lock);
	pn->try_count = 0;
	do {
		if (pn->phy_addr_pt)
			ret = ptm_wrapper_free_pt(&bm_dev.ptm,
						  pn->phy_addr_pt,
						  pn->len_pt);
		if (ret) {
			pr_err("fail to free pt: %pa. error returned: %x\n",
				&pn->phy_addr_pt, ret);
			pr_err("CPU: %d PID: %d Comm: %.20s %s %.*s\n",
				raw_smp_processor_id(), current->pid, current->comm,
				init_utsname()->release,
				(int)strcspn(init_utsname()->version, " "),
				init_utsname()->version);

			if (++pn->try_count == FREE_PT_TRY_COUNT) {
				pr_crit("still cannot free the pt, memory leaked\n");
				BUG_ON(1);
			}
			msleep(FREE_PT_TRY_INTERVAL);
		} else {
			pr_debug("pt freed trying %d times, addr %pa\n",
				pn->try_count, &pn->phy_addr_pt);
			break;
		}
	} while (pn->try_count < FREE_PT_TRY_COUNT);

	return ret;
}

static int bm_callback(struct dma_buf *dmabuf)
{
	struct berlin_meta *data = NULL;
	int ret = 0;

	data = dmabuf_get_meta(dmabuf);
	if (!data)
		return -EINVAL;

	pr_debug("%s is_pte %d dmabuf %p\n", __func__, data->pte.is_pte, dmabuf);

	if (data->pte.is_pte)
		ret = pte_node_destroy_by_callback(&data->pte);
	return ret;
}

static struct berlin_bm_client *berlin_bm_client_create(const char *name)
{
	struct berlin_bm_client *client;
	struct task_struct *task;
	pid_t pid;

	if (!name) {
		pr_err("%s: Name cannot be null\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	get_task_struct(current->group_leader);
	task_lock(current->group_leader);
	pid = task_pid_nr(current->group_leader);
	/*
	 * don't bother to store task struct for kernel threads,
	 * they can't be killed anyway
	 */
	if (current->group_leader->flags & PF_KTHREAD) {
		put_task_struct(current->group_leader);
		task = NULL;
	} else {
		task = current->group_leader;
	}
	task_unlock(current->group_leader);

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		goto err_put_task_struct;

	client->dev = &bm_dev;
	client->task = task;
	client->pid = pid;
	client->name = kstrdup(name, GFP_KERNEL);
	if (!client->name)
		goto err_free_client;

	return client;

err_free_client:
	kfree(client);
err_put_task_struct:
	if (task)
		put_task_struct(current->group_leader);
	return ERR_PTR(-ENOMEM);
}

static void berlin_bm_client_destroy(struct berlin_bm_client *client)
{
	if (client->task)
		put_task_struct(client->task);
	kfree(client->name);
	kfree(client);
}

static int bm_release(struct inode *inode, struct file *file)
{
	struct berlin_bm_client *client = file->private_data;

	pr_info("client %s closed\n", client->name);
	berlin_bm_client_destroy(client);
	return 0;
}

static int bm_open(struct inode *inode, struct file *file)
{
	struct berlin_bm_client *client;
	char debug_name[64];

	snprintf(debug_name, 64, "%u", task_pid_nr(current->group_leader));
	client = berlin_bm_client_create(debug_name);
	if (IS_ERR(client))
		return PTR_ERR(client);
	pr_info("client %s opened\n", client->name);
	file->private_data = client;

	return 0;
}

static long bm_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	int ret = 0;
	unsigned int dir;
	struct berlin_bm_client *client = filp->private_data;
	union bm_ioctl_arg data;

	dir = _IOC_DIR(cmd);
	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (!(dir & _IOC_WRITE))
		memset(&data, 0, sizeof(data));
	else if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
		return -EFAULT;

	pr_debug("ioctl cmd: %x fd: %d pid: %d\n", _IOC_NR(cmd), data.fb.fd, client->pid);
	switch (cmd) {
	case BM_IOC_ALLOC_PT:
		ret = bm_alloc_pt(data.fb.fd,
				  data.fb.flags,
				  &data.fb.fb_param,
				  &data.fb.pt_param);
		break;
	case BM_IOC_GET_PT:
		ret = bm_get_pt(data.pt.fd, &data.pt.pt_param);
		break;
	default:
		return -EINVAL;
	}
	if (ret)
		pr_info("cmd: %x fd: %d pid: %d\n", _IOC_NR(cmd), data.fb.fd, client->pid);

	if (dir & _IOC_READ) {
		if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)))
			return -EFAULT;
	}

	return ret;
}

static const struct file_operations berlin_bm_ops = {
	.open = bm_open,
	.release = bm_release,
	.unlocked_ioctl = bm_ioctl,
	.compat_ioctl = bm_ioctl,
	.owner = THIS_MODULE,
};

static int bm_init(void)
{
	int ret;

	pr_info("berlin bm init\n");
	bm_dev.mdev.minor = MISC_DYNAMIC_MINOR;
	bm_dev.mdev.name = BERLIN_BM_DEVICE_NAME;
	bm_dev.mdev.fops = &berlin_bm_ops;
	bm_dev.mdev.parent = NULL;
	ret = misc_register(&bm_dev.mdev);
	if (ret) {
		pr_err("error in register berlin bm device: %d\n", ret);
		return ret;
	}

	init_rwsem(&bm_dev.pte_lock);
	INIT_LIST_HEAD(&bm_dev.pte_list);
	ret = ptm_wrapper_init(&bm_dev.ptm, bm_dev.dev);
	if (ret) {
		pr_err("error in ptm init: %d\n", ret);
		goto error_ptm_init;
	}

	bm_dev.debug_root = debugfs_create_dir("berlin_bm", NULL);
	bm_dev.pte_debug_root = debugfs_create_file("pte", 0664,
				bm_dev.debug_root, (void *)&bm_dev,
				&debug_pte_fops);

	heap_extra_reg_free_cb(bm_callback);
	return 0;

error_ptm_init:
	misc_deregister(&bm_dev.mdev);
	return ret;
}

static void bm_exit(void)
{
	pr_info("berlin bm exit\n");
	misc_deregister(&bm_dev.mdev);
	ptm_wrapper_exit(&bm_dev.ptm);
	debugfs_remove_recursive(bm_dev.pte_debug_root);
	debugfs_remove_recursive(bm_dev.debug_root);
}

static int bm_probe(struct platform_device *pdev)
{
	int ret;

#if !IS_ENABLED(CONFIG_OPTEE)
	/* Defer probe since there is dependency of tzd */
	if (!tzd_get_kernel_dev_file())
		return -EPROBE_DEFER;
#endif

	bm_dev.dev = &pdev->dev;
	ret = bm_init();
	if (ret) {
		pr_err("bm_init failed\n");
		return ret;
	}
	pr_debug("%s OK\n", __func__);

	return ret;
}

static RET_TYPE bm_remove(struct platform_device *pdev)
{
	bm_exit();
	pr_debug("%s OK\n", __func__);
	RETURN_VALUE;
}

static const struct of_device_id bm_match[] = {
	{.compatible = "syna,berlin-bm",},
	{},
};
MODULE_DEVICE_TABLE(of, bm_match);

static struct platform_driver bm_driver = {
	.probe = bm_probe,
	.remove = bm_remove,
	.driver = {
		.name = "amp-bm",
		.of_match_table = bm_match,
	},
};

module_platform_driver(bm_driver);

MODULE_AUTHOR("synaptics");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("berlin bm module");
MODULE_IMPORT_NS(DMA_BUF);
