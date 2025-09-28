/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023, Synaptics Incorporated
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/firmware.h>

#include "vpu_common.h"
#include "vpu_fw_data.h"
#include "vpu_fw.h"

static const uuid_t TAVmeta_UUID = UUID_INIT(0x1316a183, 0x894d, 0x43fe,
					     0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1,
					     0x03, 0xf0);

static int fw_debug = 0;
module_param(fw_debug, int, 0660);
MODULE_PARM_DESC(fw_debug, "Firmware debug level");

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	return (ver->impl_id == TEE_IMPL_ID_OPTEE);
}

/*******************************************************************
 * VPU Wrap API
*******************************************************************/
int syna_vpu_open_tz_session(struct syna_vpu_dev *vpu)
{
	int ret;
	struct tee_param param[4];
	struct tee_ioctl_open_session_arg sess_arg;

	vpu->tee_ctx =
	    tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(vpu->tee_ctx)) {
		pr_err("fail to initialize optee context\n");
		return -ENODEV;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = vpu->variant->hw_type;
	param[0].u.value.b = 2;

	memset(&sess_arg, 0, sizeof(sess_arg));
	memcpy(sess_arg.uuid, TAVmeta_UUID.b, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 1;

	ret = tee_client_open_session(vpu->tee_ctx, &sess_arg, param);
	if (ret < 0 || sess_arg.ret != 0) {
		pr_err
		    ("tee_client_open_session failed with tee_ctx 0x%px code 0x%x origin 0x%x",
		     vpu->tee_ctx, ret, sess_arg.ret);
		ret = -EINVAL;
		return ret;
	}

	vpu->tee_session = sess_arg.session;
	return ret;
}

int syna_vpu_close_tz_session(struct syna_vpu_dev *vpu)
{
	tee_client_close_session(vpu->tee_ctx, vpu->tee_session);
	tee_client_close_context(vpu->tee_ctx);

	vpu->tee_ctx = NULL;
	vpu->tee_session = 0;

	return 0;
}

int syna_vpu_tee_alloc(VPU_TEEC_Context * tee_ctx, u32 len, void **shm)
{
	struct tee_shm *shmTmp;

	shmTmp = tee_shm_alloc_kernel_buf(tee_ctx, len);
	if (IS_ERR(shmTmp)) {
		pr_err("fail to allocate share memory: size 0x%x\n", len);
		return ENOMEM;
	}
	shmTmp->size = len;

	*shm = shmTmp;

	return 0;
}

int syna_vpu_tee_release(void *shm)
{
	if (shm) {
		tee_shm_free(shm);
	} else {
		pr_err("invalid shm %px\n", shm);
	}
	return 0;
}

void *syna_vpu_get_ctrl_shm_buffer(struct syna_vcodec_ctx *ctx)
{
	return ctx->ctrl_shm->kaddr;
}

void *syna_vpu_get_cfg_shm_buffer(struct syna_vcodec_ctx *ctx)
{
	return ctx->cfg_shm->kaddr;
}

uintptr_t syna_vpu_get_ctrl_shm_paddr(struct syna_vcodec_ctx * ctx)
{
	return ctx->ctrl_shm->paddr;
}

uintptr_t syna_vpu_get_cfg_shm_paddr(struct syna_vcodec_ctx * ctx)
{
	return ctx->cfg_shm->paddr;
}

int syna_vpu_fw_probe(struct device *dev)
{
	return 0;
}

int syna_vpu_get_hwdata(struct syna_vpu_dev *vpu)
{
	int ret = 0;
	struct tee_shm *shm = NULL;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;
	struct syna_vpu_fw_data *fw_data = &vpu->fw_data;
	unsigned int len = ALIGN(sizeof(struct syna_vpu_fw_data), SZ_4K);

	shm = tee_shm_alloc_kernel_buf(vpu->tee_ctx, len);
	if (IS_ERR(shm)) {
		pr_err("fail to allocate share memory: size 0x%x\n", len);
		return -ENOMEM;
	}
	shm->size = len;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[0].u.memref.shm = shm;
	param[0].u.memref.size = len;
	param[0].u.memref.shm_offs = 0;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[1].u.value.a = 0x0;
	param[1].u.value.b = 0xdeadbeef;

	arg.func = VMETA_GET_CONST_V2;
	arg.num_params = 2;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret || param[1].u.value.b) {
		pr_err("failed to command VMETA TA: 0x%08x, 0x%08llx\n",
		       ret, param[1].u.value.b);
		ret = -EIO;
		goto failed_tee;
	}

	memcpy(fw_data, shm->kaddr, sizeof(struct syna_vpu_fw_data));
	fw_data->fw_api_ver = param[1].u.value.a;

failed_tee:
	tee_shm_free(shm);
	return ret;
}

int syna_vpu_set_log_level(struct syna_vpu_dev *vpu)
{
	static int cur_level = 0;
	int ret = 0;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;

	if (cur_level == fw_debug)
		return 0;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = fw_debug;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[1].u.value.a = 0xdeadbeef;

	arg.func = VPU_SET_DEBUG_LEVEL_V2;
	arg.num_params = 2;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret)
		return ret;

	cur_level = fw_debug;
	return 0;
}

/* tz_vpu_create_instance
	IN params0: ctrlshm addr, ctrlshm size
	OUT params1: result
*/
static int tz_vpu_create_instance(int cmd, struct syna_vcodec_ctx *ctx)
{
	int ret;
	struct syna_vpu_ctrl *ctrl;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;
	struct syna_vpu_dev *vpu = ctx->vpu;

	memset(param, 0, sizeof(param));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = ctx->ctrl_shm;
	param[0].u.memref.size = ctx->ctrl_shm->size;
	param[0].u.memref.shm_offs = 0;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[1].u.value.a = 0xdeadbeef;

	arg.func = cmd;
	arg.num_params = 2;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret || param[1].u.value.a) {
		pr_err("can't create inst cmd %d ret:0x%x(0x%llx)",
		       cmd, ret, param[1].u.value.a);
		ret = param[1].u.value.a;
	} else {
		ctrl = syna_vpu_get_ctrl_shm_buffer(ctx);
	}

	return ret;
}

/**tz_vpu_stream_cmd1
 * IN params[0] ctrl tzva, ctrl size
 * OUT prams[1] result
*/
static int tz_vpu_stream_cmd1(int cmd, struct syna_vcodec_ctx *ctx)
{
	int ret = 0;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;
	struct syna_vpu_dev *vpu = ctx->vpu;

	memset(param, 0, sizeof(param));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = ctx->ctrl_shm;
	param[0].u.memref.size = ctx->ctrl_shm->size;
	param[0].u.memref.shm_offs = 0;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[1].u.value.a = 0xdeadbeef;

	arg.func = cmd;
	arg.num_params = 2;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret) {
		pr_err("stmcmd %d failed: %08x\n", cmd, ret);
		param[1].u.value.a = ret;
	}

	return param[1].u.value.a;
}

/**tz_vpu_stream_cmd2
 * IN  params[0] ctrl
 * IN  params[1] strm cfg
 * OUT prams[2] result
*/
static int tz_vpu_stream_cmd2(int cmd, struct syna_vcodec_ctx *ctx)
{
	int ret = 0;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;
	struct syna_vpu_dev *vpu = ctx->vpu;

	memset(param, 0, sizeof(param));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = ctx->ctrl_shm;
	param[0].u.memref.size = ctx->ctrl_shm->size;
	param[0].u.memref.shm_offs = 0;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[1].u.memref.shm = ctx->cfg_shm;
	param[1].u.memref.size = ctx->cfg_shm->size;
	param[1].u.memref.shm_offs = 0;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[2].u.value.a = 0xdeadbeef;

	arg.func = cmd;
	arg.num_params = 3;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret || param[2].u.value.a)
		return ret;

	return param[2].u.value.a;
}

/** tz_vpu_buffer_cmd
 * IN  params[0] ctrl
 * IN  params[1] type, index
 * OUT prams[2] result
*/
static int tz_vpu_buffer_cmd(int cmd, struct syna_vcodec_ctx *ctx,
			     uint32_t type, uint32_t index)
{
	int ret;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;
	struct syna_vpu_dev *vpu = ctx->vpu;

	memset(param, 0, sizeof(param));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = ctx->ctrl_shm;
	param[0].u.memref.size = ctx->ctrl_shm->size;
	param[0].u.memref.shm_offs = 0;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].u.value.a = type;
	param[1].u.value.b = index;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[2].u.value.a = 0xdeadbeef;

	arg.func = cmd;
	arg.num_params = 3;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret || param[2].u.value.a) {
		pr_err
		    ("can't put %d buffer into fw queue(type %d): 0x%x(0x%llx)",
		     index, type, ret, param[2].u.value.a);
		ret = param[2].u.value.a;
	}

	return ret;
}

/*******************************************************************
 * VPU Common API
*******************************************************************/

int syna_vpu_open(struct syna_vpu_dev *vpu, u32 memid, u32 size)
{
	int ret;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;

	memset(param, 0, sizeof(param));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = memid;
	param[0].u.value.b = size;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[1].u.value.a = 0xdeadbeef;

	arg.func = VMETA_OPEN_V2;
	arg.num_params = 2;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret || param[1].u.value.a) {
		pr_err("[vpu]failed to initialize VPU HW: 0x%08x, 0x%08llx\n",
		       ret, param[1].u.value.a);
		ret = -EIO;
	}

	return ret;
}

int syna_vpu_close(struct syna_vpu_dev *vpu)
{
	int ret;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[0].u.value.a = 0xdeadbeef;

	arg.func = VMETA_CLOSE_V2;
	arg.num_params = 1;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);

	return ret;
}

int syna_vpu_register_buf(struct syna_vcodec_ctx *ctx, u32 buftype, u32 idx)
{
	return tz_vpu_buffer_cmd(VPU_REG_BUF_V2, ctx, buftype, idx);
}

int syna_vpu_unregister_buf(struct syna_vcodec_ctx *ctx, u32 buftype, u32 idx)
{
	return tz_vpu_buffer_cmd(VPU_UNREG_BUF_V2, ctx, buftype, idx);
}

int syna_vpu_ctx_switch_inst(struct syna_vcodec_ctx *old,
			     struct syna_vcodec_ctx *successor)
{
	struct syna_vpu_dev *vpu;
	struct tee_param param[4];
	struct tee_ioctl_invoke_arg arg;
	int ret;

	if (!(old || successor))
		return -EINVAL;

	memset(param, 0, sizeof(param));
	vpu = successor ? successor->vpu : old->vpu;

	if (old) {
		param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		param[0].u.memref.shm = old->ctrl_shm;
		param[0].u.memref.size = old->ctrl_shm->size;
		param[0].u.memref.shm_offs = 0;
	} else {
		param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	}

	if (successor) {
		param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
		param[1].u.memref.shm = successor->ctrl_shm;
		param[1].u.memref.size = successor->ctrl_shm->size;
		param[1].u.memref.shm_offs = 0;
	} else {
		param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	}

	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[2].u.value.a = 0xdeadbeef;

	arg.func = VPU_CTX_SWITCH_INST_v2;
	arg.num_params = 3;
	arg.session = vpu->tee_session;

	ret = tee_client_invoke_func(vpu->tee_ctx, &arg, param);
	if (ret) {
		pr_err("stmcmd VPU_CTX_SWITCH_INST_v2 failed: %08x\n", ret);
		param[2].u.value.a = ret;
	}

	return param[2].u.value.a;
}

/*
 * 		VDEC TA APIS
 */
int syna_vdec_create_instance(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_create_instance(VDEC_CREATE_INSTANCE_V2, ctx);
}

int syna_vdec_destroy_instance(struct syna_vcodec_ctx *ctx)
{
	int ret;

	ret = tz_vpu_stream_cmd1(VDEC_DESTROY_INSTANCE_V2, ctx);
	clear_bit(SYNA_VPU_STATUS_SET_FMT, &ctx->status);

	return ret;
}

int syna_vdec_configure_stream(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd2(VDEC_CONFIG_STREAM_V2, ctx);
}

int syna_vdec_close_stream(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VDEC_RESET_INSTANCE_V2, ctx);
}

int syna_vdec_stream_switch_in(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VDEC_STREAM_SWITCH_IN_V2, ctx);
}

int syna_vdec_stream_switch_out(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VDEC_STREAM_SWITCH_OUT_V2, ctx);
}

int syna_vdec_decode_stream(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VDEC_DECODE_STREAM_V2, ctx);
}

int syna_vdec_drain_pool(struct syna_vcodec_ctx *ctx, u32 buftype)
{
	return tz_vpu_buffer_cmd(VDEC_DRAIN_POOL_V2, ctx,
				 buftype, 0U);
}

int syna_vdec_hw_abort(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VDEC_ABORT_HW_V2, ctx);
}

int syna_vdec_push_disp_buf(struct syna_vcodec_ctx *ctx, uint32_t index)
{
	return tz_vpu_buffer_cmd(VDEC_PUSH_BUFFER_V2, ctx,
				 SYNA_VPU_FW_VDEC_DISP, index);
}

int syna_vdec_push_strm_buf(struct syna_vcodec_ctx *ctx, uint32_t index)
{
	return tz_vpu_buffer_cmd(VDEC_PUSH_BUFFER_V2, ctx,
				 SYNA_VPU_FW_VDEC_STRM, index);
}

int syna_vdec_push_ref_buf(struct syna_vcodec_ctx *ctx, uint32_t index)
{
	return tz_vpu_buffer_cmd(VDEC_PUSH_BUFFER_V2, ctx,
				 SYNA_VPU_FW_VDEC_REF, index);
}

/*********************************************************
	VENC TA APIS
*********************************************************/
int syna_venc_create_instance(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_create_instance(VENC_CREATE_INSTANCE_V2, ctx);
}

int syna_venc_destroy_instance(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VENC_DESTROY_INSTANCE_V2, ctx);
}

int syna_venc_configure_stream(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd2(VENC_CONFIG_STREAM_V2, ctx);
}

int syna_venc_stream_switch_in(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VENC_STREAM_SWITCH_IN_V2, ctx);
}

int syna_venc_stream_switch_out(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VENC_STREAM_SWITCH_OUT_V2, ctx);
}

int syna_venc_close_stream(struct syna_vcodec_ctx *ctx)
{
	int ret;

	if (!test_bit(SYNA_VPU_STATUS_SET_FMT, &ctx->status))
		return 0;

	ret = tz_vpu_stream_cmd1(VENC_CLOSE_STREAM_V2, ctx);
	if (ret) {
		return ret;
	}

	clear_bit(SYNA_VPU_STATUS_SET_FMT, &ctx->status);
	return ret;
}

int syna_venc_encode_stream(struct syna_vcodec_ctx *ctx)
{
	return tz_vpu_stream_cmd1(VENC_ENCODE_STREAM_V2, ctx);
}

int syna_venc_push_video_buf(struct syna_vcodec_ctx *ctx, uint32_t index)
{
	return tz_vpu_buffer_cmd(VENC_PUSH_BUFFER_V2, ctx,
				 SYNA_VPU_FW_VENC_VID, index);
}

int syna_venc_push_es_buf(struct syna_vcodec_ctx *ctx, uint32_t index)
{
	return tz_vpu_buffer_cmd(VENC_PUSH_BUFFER_V2, ctx,
				 SYNA_VPU_FW_VENC_STRM, index);
}

int syna_venc_push_ref_buf(struct syna_vcodec_ctx *ctx, uint32_t index)
{
	return tz_vpu_buffer_cmd(VENC_PUSH_BUFFER_V2, ctx,
				 SYNA_VPU_FW_VENC_RECON, index);
}

int syna_venc_push_ds_buf(struct syna_vcodec_ctx *ctx, uint32_t index)
{
	return tz_vpu_buffer_cmd(VENC_PUSH_BUFFER_V2, ctx,
				 SYNA_VPU_FW_VENC_DS, index);
}
