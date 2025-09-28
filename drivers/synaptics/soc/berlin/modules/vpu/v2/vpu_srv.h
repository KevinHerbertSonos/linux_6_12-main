/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2021 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
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

#ifndef _SYNA_VPU_SERVICE_H_
#define _SYNA_VPU_SERVICE_H_

#include "vpu_common.h"

bool vpu_srv_push_to_pending(struct syna_vpu_srv *node,
			     struct syna_vcodec_ctx *session);

void vpu_srv_remove_from_pending(struct syna_vpu_srv *node,
				 struct syna_vcodec_ctx *session);

void vpu_srv_prepare_to_run(struct syna_vpu_srv *node,
			    struct syna_vcodec_ctx *session);

int vpu_srv_release_out(struct syna_vpu_srv *node,
			struct syna_vcodec_ctx *session,
			struct syna_vcodec_ctx **next_session);

struct syna_vcodec_ctx* vpu_srv_schedule_pending(struct syna_vpu_srv *node);
void vpu_srv_schedule_no_yield(struct syna_vpu_srv *node);

/* NOTE: it won't disable irq, so don't call it in irq or softirq context */
bool vpu_srv_clear_pending_isr(struct syna_vpu_srv *node);
bool vpu_srv_has_pending_isr(struct syna_vpu_srv *node);

int vpu_srv_wait_isr(struct syna_vpu_srv *node, unsigned int timeout_ms);
/* Hand the hardware over to the pending jobs */
void vpu_srv_isr_done(struct syna_vpu_srv *node);

void *syna_srv_create(struct syna_vpu_dev *vpu);
void syna_srv_destroy(struct syna_vpu_srv **node);
#endif
