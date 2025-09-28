// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated
 *
 */

#define pr_fmt(fmt) "[berlin_bm kernel driver] " fmt
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/tee_drv.h>
#include "uapi/bm.h"
#include "ptm_wrapper.h"

static struct tee_context *ptm_teec_ctx;
static u32 ptm_session_id;

static const uuid_t ptm_ta_uuid = UUID_INIT(0x1316a183, 0x894d, 0x43fe,
 						0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x2d);

enum {
	PTM_INVALID,
	PTM_DEBUG,
	PTM_CONSTRUCT_PT,
	PTM_FREE_PT,
	PTM_CMD_MAX
};

struct ptm_ta_param {
	u32 flags;
	struct bm_fb_param param;
};

static int check_ctx_session(struct ptm_context *ptm_ctx)
{
	return 0;
}

static int ptm_wrapper_alloc(struct ptm_context *ptm_ctx, unsigned int len,
		      void **handle)
{
	struct tee_shm *shm;

	shm = tee_shm_alloc_kernel_buf(ptm_ctx->teec_ctx, len);
	if (IS_ERR(shm)) {
		pr_err("fail to allocate share memory: size %x\n", len);
		kfree(shm);
		return -ENOMEM;
	}

	*handle = shm;
	return 0;
}

static void ptm_wrapper_free(void *handle)
{
	tee_shm_free(handle);
}


static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	return (ver->impl_id == TEE_IMPL_ID_OPTEE);
}

int ptm_wrapper_init(struct ptm_context *ptm_ctx, struct device *dev)
{
	int ret;
	struct tee_ioctl_open_session_arg sess_arg;

	ptm_teec_ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(ptm_teec_ctx)) {
		pr_err("fail to initialize optee context\n");
		return -ENODEV;
	}

	ptm_ctx->teec_ctx = ptm_teec_ctx;

	ptm_ctx->ctx_created = 1;

	ptm_teec_ctx->supp_nowait = false;

	/* Open session with PTM Trusted App */

	memset(&sess_arg, 0, sizeof(sess_arg));
	memcpy(sess_arg.uuid, ptm_ta_uuid.b, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	ret = tee_client_open_session(ptm_teec_ctx, &sess_arg, NULL);
	if (ret < 0 || sess_arg.ret != 0) {
		pr_err("tee_client_open_session failed with code 0x%x origin 0x%x",
			ret, sess_arg.ret);
		ret = -EINVAL;
		goto out_ctx;
	}

	ptm_session_id = sess_arg.session;

	ptm_ctx->teec_session = ptm_session_id;

	ptm_ctx->session_opened = 1;
	mutex_init(&ptm_ctx->lock);

	ret = ptm_wrapper_alloc(ptm_ctx, PTM_MAX_SG_SHM,
			(void **)&ptm_ctx->sg_shm);
	if (ret) {
		pr_err("fail to alloc ptm sg shm %zx", PTM_MAX_SG_SHM);
		goto error;
	}
	ret = ptm_wrapper_alloc(ptm_ctx, sizeof(struct ptm_ta_param),
			(void **)&ptm_ctx->param_shm);
	if (ret) {
		pr_err("fail to alloc ptm param shm %zx",
				sizeof(struct ptm_ta_param));
		goto error;
	}
	return ret;

error:
	if (ptm_ctx->sg_shm)
		ptm_wrapper_free(ptm_ctx->sg_shm);
	if (ptm_ctx->param_shm)
		ptm_wrapper_free(ptm_ctx->param_shm);

	tee_client_close_session(ptm_teec_ctx, ptm_session_id);
out_ctx:
	tee_client_close_context(ptm_teec_ctx);

	return ret;
}

void ptm_wrapper_exit(struct ptm_context *ptm_ctx)
{
	if (ptm_ctx->sg_shm)
		ptm_wrapper_free(ptm_ctx->sg_shm);
	if (ptm_ctx->param_shm)
		ptm_wrapper_free(ptm_ctx->param_shm);

	tee_client_close_session(ptm_teec_ctx, ptm_session_id);
	tee_client_close_context(ptm_teec_ctx);
}

int ptm_wrapper_construct_pt(struct ptm_context *ptm_ctx,
			     void *handle, unsigned int len,
			     struct bm_pt_param *pt_param, u32 flags,
			     struct bm_fb_param *fb_param)
{
	int ret = 0;
	struct tee_shm *param_handle = NULL;
	struct ptm_ta_param *p = NULL;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	mutex_lock(&ptm_ctx->lock);
	ret = check_ctx_session(ptm_ctx);
	if (ret != 0)
		goto error;

	param_handle = ptm_ctx->param_shm;
	p = param_handle->kaddr;

	p->flags = flags;
	p->param.fb_type = fb_param->fb_type;
	p->param.uva_param.y_size = fb_param->uva_param.y_size;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = handle;
	param[0].u.memref.size = len;
	param[0].u.memref.shm_offs = 0;

	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].u.memref.shm = param_handle;
	param[1].u.memref.size = sizeof(struct ptm_ta_param);
	param[1].u.memref.shm_offs = 0;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	memset(&arg, 0, sizeof(arg));

	arg.func = PTM_CONSTRUCT_PT;
	arg.num_params = 4;
	arg.session = ptm_ctx->teec_session;

	ret = tee_client_invoke_func(ptm_ctx->teec_ctx, &arg, param);
	if (ret < 0 || arg.ret != 0) {
		pr_err("fail to invoke command to PTM TA: ret = %x, TEE err: 0x%x\n", ret, arg.ret);
		ret = -EINVAL;
	} else {
		//TODO: handle possible 64bit physical address
		pt_param->phy_addr = param[2].u.value.a;
		pt_param->len = param[2].u.value.b;
		pt_param->mem_id = param[3].u.value.a;
	}

error:
	mutex_unlock(&ptm_ctx->lock);

	return ret;
}

int ptm_wrapper_free_pt(struct ptm_context *ptm_ctx,
			phys_addr_t addr, unsigned int len)
{
	int ret = 0;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	mutex_lock(&ptm_ctx->lock);
	memset(param, 0, sizeof(param));

	//TODO: handle possible 64bit physical address
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = (uint32_t)(addr & 0xffffffff);
	param[0].u.value.b = len;

	arg.func = PTM_FREE_PT;
	arg.num_params = 1;
	arg.session = ptm_ctx->teec_session;

	ret = tee_client_invoke_func(ptm_ctx->teec_ctx, &arg, param);
	if (ret < 0 || arg.ret != 0) {
		pr_err("fail to free pt to PTM TA: addr %pa, ret = %x, TEE err: 0x%x\n", &addr, ret, arg.ret);
	}

	mutex_unlock(&ptm_ctx->lock);

	return ret;
}
