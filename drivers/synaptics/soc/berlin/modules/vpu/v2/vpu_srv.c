// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/slab.h>

#include "vpu_srv.h"

struct syna_vpu_srv {
	struct syna_vpu_dev *vpu;
	/* node structure global lock */
	spinlock_t srv_spinlock;
	struct list_head pending;
	struct list_head successor;

	/* service critical time(irq) */
	struct completion ree_irq;

	/* switch in/switch out */
	atomic_t switchpoint;

	/* starts with zero and one for a context takes the device */
	atomic_t wait_for_next;
	struct syna_vcodec_ctx *last_session;
};

/* service queue schedule */
bool vpu_srv_push_to_pending(struct syna_vpu_srv *node,
			     struct syna_vcodec_ctx *session)
{
	unsigned long flags;

	if (atomic_dec_and_test(&node->switchpoint)) {
		reinit_completion(&session->work_done);
		return false;
	} else if (atomic_dec_and_test(&node->wait_for_next)) {
		atomic_inc(&node->switchpoint);
		spin_lock_irqsave(&node->srv_spinlock, flags);

		/* last ctx still holds the device */
		if (session == node->last_session) {
			spin_unlock_irqrestore(&node->srv_spinlock, flags);
			reinit_completion(&session->work_done);
			return false;
		}

		if (session == list_first_entry(&node->successor,
						struct syna_vcodec_ctx,
						service_link)) {
			list_del_init(&session->service_link);
			spin_unlock_irqrestore(&node->srv_spinlock, flags);
			reinit_completion(&session->work_done);
			return false;
		}

		spin_unlock_irqrestore(&node->srv_spinlock, flags);
	} else {
		atomic_inc(&node->switchpoint);
	}

	atomic_inc(&node->wait_for_next);
	spin_lock_irqsave(&node->srv_spinlock, flags);

	if (list_empty(&session->service_link))
		list_add_tail(&session->service_link, &node->pending);

	spin_unlock_irqrestore(&node->srv_spinlock, flags);

	return true;
}

void vpu_srv_remove_from_pending(struct syna_vpu_srv *node,
				 struct syna_vcodec_ctx *session)
{
	unsigned long flags;

	spin_lock_irqsave(&node->srv_spinlock, flags);

	if (session == list_first_entry_or_null(&node->successor,
						struct syna_vcodec_ctx,
						service_link)) {
		atomic_dec(&node->wait_for_next);
		list_del_init(&session->service_link);
		atomic_inc(&node->switchpoint);
	} else {
		list_del_init(&session->service_link);
	}

	spin_unlock_irqrestore(&node->srv_spinlock, flags);
}

void vpu_srv_prepare_to_run(struct syna_vpu_srv *node,
			    struct syna_vcodec_ctx *session)
{
	struct syna_vpu_dev *vpu = node->vpu;
	struct syna_vcodec_ctx *last_session;
	unsigned long flags;

	spin_lock_irqsave(&node->srv_spinlock, flags);
	last_session = node->last_session;
	spin_unlock_irqrestore(&node->srv_spinlock, flags);

	if (last_session == session)
		return;
	else if (last_session)
		vpu->fw_ops->fw_inst_swap(last_session, session);
	else
		vpu->fw_ops->fw_inst_swap(NULL, session);

	spin_lock_irqsave(&node->srv_spinlock, flags);
	node->last_session = session;
	spin_unlock_irqrestore(&node->srv_spinlock, flags);

}

/* It should be only be called under the protection of m2m_dev->job_spinlock */
struct syna_vcodec_ctx* vpu_srv_schedule_pending(struct syna_vpu_srv *node)
{
	struct syna_vpu_dev *vpu = node->vpu;
	struct syna_vcodec_ctx *session;
	unsigned long flags;

	spin_lock_irqsave(&node->srv_spinlock, flags);

	if (list_empty(&node->pending)) {
		/**
		 * no pending context, v4l2 framework could fight for the
		 * token then.
		 */
		spin_unlock_irqrestore(&node->srv_spinlock, flags);
		atomic_inc(&node->switchpoint);
		session = NULL;
	} else {
		session = list_first_entry(&node->pending,
					   struct syna_vcodec_ctx,
					   service_link);
		list_move(&session->service_link, &node->successor);
		atomic_inc(&node->wait_for_next);

		node->last_session = NULL;
		spin_unlock_irqrestore(&node->srv_spinlock, flags);

		vpu->fw_ops->fw_inst_swap(node->last_session, NULL);
	}

	return session;
}

/* It should be only be called under the protection of m2m_dev->job_spinlock */
void vpu_srv_schedule_no_yield(struct syna_vpu_srv *node)
{
	unsigned long flags;

	spin_lock_irqsave(&node->srv_spinlock, flags);
	atomic_inc(&node->wait_for_next);
	spin_unlock_irqrestore(&node->srv_spinlock, flags);
}

static inline struct syna_vcodec_ctx*
vpu_srv_release_schedule_next(struct syna_vpu_srv *node)
{
	struct syna_vcodec_ctx *session;
	unsigned long flags;

	spin_lock_irqsave(&node->srv_spinlock, flags);
	if (list_empty(&node->pending)) {
		spin_unlock_irqrestore(&node->srv_spinlock, flags);
		atomic_inc(&node->switchpoint);
		session = NULL;
	} else {
		session = list_first_entry(&node->pending,
					   struct syna_vcodec_ctx,
					   service_link);
		list_move(&session->service_link, &node->successor);
		atomic_inc(&node->wait_for_next);
		spin_unlock_irqrestore(&node->srv_spinlock, flags);
	}

	return session;
}

int vpu_srv_release_out(struct syna_vpu_srv *node,
			struct syna_vcodec_ctx *session,
			struct syna_vcodec_ctx **next_session)
{
	struct syna_vpu_dev *vpu = node->vpu;
	unsigned long flags;
	int ret;

try_again:
	spin_lock_irqsave(&node->srv_spinlock, flags);
	if (node->last_session == session) {
		if (atomic_dec_and_test(&node->switchpoint)) {
			node->last_session = NULL;
			spin_unlock_irqrestore(&node->srv_spinlock, flags);

			ret = vpu->fw_ops->release(session);

			*next_session = vpu_srv_release_schedule_next(node);
		} else if (atomic_dec_and_test(&node->wait_for_next)) {
			node->last_session = NULL;
			spin_unlock_irqrestore(&node->srv_spinlock, flags);

			ret = vpu->fw_ops->release(session);

			atomic_inc(&node->switchpoint);
			*next_session = vpu_srv_release_schedule_next(node);
		} else {
			atomic_inc(&node->switchpoint);
			atomic_inc(&node->wait_for_next);
			spin_unlock_irqrestore(&node->srv_spinlock, flags);
			goto try_again;
		}
	} else {
		spin_unlock_irqrestore(&node->srv_spinlock, flags);
		ret = vpu->fw_ops->release(session);
		*next_session = NULL;
	}

	return ret;
}

int vpu_srv_wait_isr(struct syna_vpu_srv *node, unsigned int timeout_ms)
{
	unsigned long timeout_jiff;
	unsigned long ret;
	int status = 0;

	timeout_jiff = msecs_to_jiffies(timeout_ms);

	ret =
	    wait_for_completion_interruptible_timeout(&node->ree_irq,
						      timeout_jiff);
	if (-ERESTARTSYS == ret) {
		status = -1;
		pr_err("interrupted by a signal\n");
	} else if (0 == ret) {
		status = -1;
		pr_err("interrupter timeout\n");
	}

	return status;
}

void vpu_srv_isr_done(struct syna_vpu_srv *node)
{
	complete(&node->ree_irq);
}

bool vpu_srv_clear_pending_isr(struct syna_vpu_srv *node)
{
	bool pending;

	if (pending = try_wait_for_completion(&node->ree_irq), pending) {
		spin_lock(&node->srv_spinlock);
		reinit_completion(&node->ree_irq);
		spin_unlock(&node->srv_spinlock);
	}

	return pending;
}

bool vpu_srv_has_pending_isr(struct syna_vpu_srv * node)
{
	return completion_done(&node->ree_irq);
}

void *syna_srv_create(struct syna_vpu_dev *vpu)
{
	struct syna_vpu_srv *node = NULL;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return node;

	node->vpu = vpu;
	spin_lock_init(&node->srv_spinlock);
	INIT_LIST_HEAD(&node->pending);
	INIT_LIST_HEAD(&node->successor);
	init_completion(&node->ree_irq);

	atomic_set(&node->switchpoint, 1);
	atomic_set(&node->wait_for_next, 0);

	return node;
}

void syna_srv_destroy(struct syna_vpu_srv **node)
{
	kfree(*node);
	*node = NULL;
}
