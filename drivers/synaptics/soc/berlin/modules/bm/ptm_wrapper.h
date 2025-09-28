// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated
 *
 * Author: Jie Xi <Jie.Xi@synaptics.com>
 *
 */

#ifndef __PTM_WRAPPER_H__
#define __PTM_WRAPPER_H__

#if IS_ENABLED(CONFIG_OPTEE)
#include <linux/tee_drv.h>

struct ptm_context {
	struct mutex lock;
	int ctx_created;
	int session_opened;
	struct tee_context *teec_ctx;
	int teec_session;
	struct tee_shm *sg_shm;
	struct tee_shm  *param_shm;
};

#else
#include "tee_client_api.h"

struct ptm_context {
	struct mutex lock;
	int ctx_created;
	int session_opened;
	TEEC_Context teec_ctx;
	TEEC_Session teec_session;
	TEEC_SharedMemory *sg_shm;
	TEEC_SharedMemory *param_shm;
};

#endif

/*
 * buffer size = sg_table->nents * 2 * sizeof(phys_addr_t)
 * MAX nents is 30M (max YUV frame buffer)/ 4K (least page size)
 * 30M/4K*2*8 = 120K
 */
#define PTM_MAX_SG_SHM (2 * sizeof(phys_addr_t) * 7680)


int ptm_wrapper_init(struct ptm_context *ptm_ctx, struct device *dev);
void ptm_wrapper_exit(struct ptm_context *ptm_ctx);

int ptm_wrapper_construct_pt(struct ptm_context *ptm_ctx,
			     void *addr, unsigned int len,
			     struct bm_pt_param *pt_param,
			     u32 flags,
			     struct bm_fb_param *fb_param);

int ptm_wrapper_free_pt(struct ptm_context *ptm_ctx,
			phys_addr_t addr,
			unsigned int len);

#endif
