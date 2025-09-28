/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023, Synaptics Incorporated
 */

#ifndef __VPU_FIRMWARE_H__
#define __VPU_FIRMWARE_H__

#include <linux/types.h>
#include <linux/device.h>
#include "vpu_common.h"
#include "vpu_tee_wrap.h"

int syna_vpu_open_tz_session(struct syna_vpu_dev *vpu);
int syna_vpu_close_tz_session(struct syna_vpu_dev *vpu);
int syna_vpu_tee_alloc(VPU_TEEC_Context * tee_ctx, u32 len, void **shm);
int syna_vpu_tee_release(void *shm);
int syna_vpu_fw_probe(struct device *dev);
void *syna_vpu_get_ctrl_shm_buffer(struct syna_vcodec_ctx *ctx);
uintptr_t syna_vpu_get_ctrl_shm_paddr(struct syna_vcodec_ctx *ctx);
void *syna_vpu_get_cfg_shm_buffer(struct syna_vcodec_ctx *ctx);
uintptr_t syna_vpu_get_cfg_shm_paddr(struct syna_vcodec_ctx *ctx);

int syna_vpu_open(struct syna_vpu_dev *vpu, u32 memid, u32 size);
int syna_vpu_close(struct syna_vpu_dev *vpu);
int syna_vpu_get_hwdata(struct syna_vpu_dev *vpu);
int syna_vpu_set_log_level(struct syna_vpu_dev *vpu);

int syna_vpu_register_buf(struct syna_vcodec_ctx *ctx, u32 buftype, u32 idx);
int syna_vpu_unregister_buf(struct syna_vcodec_ctx *ctx, u32 buftype, u32 idx);

int syna_vpu_ctx_switch_inst(struct syna_vcodec_ctx *old,
			     struct syna_vcodec_ctx *successor);

int syna_vdec_create_instance(struct syna_vcodec_ctx *ctx);
int syna_vdec_destroy_instance(struct syna_vcodec_ctx *ctx);
int syna_vdec_configure_stream(struct syna_vcodec_ctx *ctx);
int syna_vdec_push_disp_buf(struct syna_vcodec_ctx *ctx, uint32_t index);
int syna_vdec_push_strm_buf(struct syna_vcodec_ctx *ctx, uint32_t index);
int syna_vdec_push_ref_buf(struct syna_vcodec_ctx *ctx, uint32_t index);
int syna_vdec_drain_pool(struct syna_vcodec_ctx *ctx, u32 buftype);
int syna_vdec_decode_stream(struct syna_vcodec_ctx *ctx);
int syna_vdec_stream_switch_in(struct syna_vcodec_ctx *ctx);
int syna_vdec_stream_switch_out(struct syna_vcodec_ctx *ctx);
int syna_vdec_close_stream(struct syna_vcodec_ctx *ctx);
int syna_vdec_hw_abort(struct syna_vcodec_ctx *ctx);

int syna_venc_create_instance(struct syna_vcodec_ctx *ctx);
int syna_venc_destroy_instance(struct syna_vcodec_ctx *ctx);
int syna_venc_configure_stream(struct syna_vcodec_ctx *ctx);
int syna_venc_stream_switch_in(struct syna_vcodec_ctx *ctx);
int syna_venc_stream_switch_out(struct syna_vcodec_ctx *ctx);
int syna_venc_close_stream(struct syna_vcodec_ctx *ctx);
int syna_venc_encode_stream(struct syna_vcodec_ctx *ctx);
int syna_venc_push_video_buf(struct syna_vcodec_ctx *ctx, uint32_t index);
int syna_venc_push_es_buf(struct syna_vcodec_ctx *ctx, uint32_t index);
int syna_venc_push_ref_buf(struct syna_vcodec_ctx *ctx, uint32_t index);
int syna_venc_push_ds_buf(struct syna_vcodec_ctx *ctx, uint32_t index);

/* __VPU_FIRMWARE_H__ */
#endif
