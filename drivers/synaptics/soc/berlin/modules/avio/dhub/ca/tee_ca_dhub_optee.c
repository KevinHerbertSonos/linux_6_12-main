// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2024 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/tee_drv.h>

#include "tee_ca_dhub.h"
#include "dhub_cmd.h"
#include "vpp_cmd.h"

#define bTST(x, b)      (((x) >> (b)) & 1)

static int initialized;
static struct tee_context *dhub_teec_ctx;
static u32 dhub_session_id;
static const uuid_t dhub_ta_uuid = UUID_INIT(0x1316a183, 0x894d, 0x43fe,
				0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf3);

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	return (ver->impl_id == TEE_IMPL_ID_OPTEE);
}

int DhubInitialize(void)
{
	int ret;
	struct tee_ioctl_open_session_arg sess_arg;

	if (initialized)
		return 0;

	/* ========================================================================
	 *  [1] Connect to TEE
	 * ========================================================================
	 */
	dhub_teec_ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(dhub_teec_ctx)) {
		pr_err("fail to initialize optee context\n");
		return -ENODEV;
	}

	/* ========================================================================
	 *  [2] Allocate DHUB SHM
	 * ========================================================================
	 */

	/* ========================================================================
	 *  [3] Open session with TEE application
	 * ========================================================================
	 */

	memset(&sess_arg, 0, sizeof(sess_arg));
	memcpy(sess_arg.uuid, dhub_ta_uuid.b, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	ret = tee_client_open_session(dhub_teec_ctx, &sess_arg, NULL);
	if (ret < 0 || sess_arg.ret != 0) {
		pr_err("tee_client_open_session failed with code 0x%x origin 0x%x",
			ret, sess_arg.ret);
		ret = -EINVAL;
		goto out_ctx;
	}

	dhub_session_id = sess_arg.session;

	initialized = true;
	return 0;

out_ctx:
	tee_client_close_context(dhub_teec_ctx);

	return ret;
}

static int Fastlogo_EnableAutoPush(bool enable)
{
	int ret;
	struct tee_ioctl_open_session_arg sess_arg;
	static struct tee_context *dhub_teec_local_ctx;
	static u32 dhub_session_local_id;
	static const uuid_t dhub_ta_uuid = UUID_INIT(0x1316a183, 0x894d, 0x43fe,
				0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf5);
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	/* ========================================================================
	 *[1] Connect to TEE
	 *========================================================================
	 */
	dhub_teec_local_ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(dhub_teec_local_ctx)) {
		pr_err("fail to initialize optee context\n");
		return -ENODEV;
	}

	/* ========================================================================
	 *[2] Open session with TEE application
	 * ========================================================================
	 */
	memset(&sess_arg, 0, sizeof(sess_arg));
	memcpy(sess_arg.uuid, dhub_ta_uuid.b, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	ret = tee_client_open_session(dhub_teec_local_ctx, &sess_arg, NULL);
	if (ret < 0 || sess_arg.ret != 0) {
		pr_err("tee_client_open_session failed with code 0x%x origin 0x%x",
			ret, sess_arg.ret);
		ret = -EINVAL;
		goto out_ctx;
	}

	dhub_session_local_id = sess_arg.session;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = enable;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	memset(&arg, 0, sizeof(arg));
	arg.func = VPP_ENABLEAUTOPUSH;
	arg.num_params = 2;
	arg.session = dhub_session_local_id;

	ret = tee_client_invoke_func(dhub_teec_local_ctx, &arg, param);
	if (ret < 0 || arg.ret != 0) {
		pr_err("fail to invoke command to DHUB TA: ret = %x, TEE err: 0x%x\n", ret, arg.ret);
		ret = -EINVAL;
	} else {
		ret = param[1].u.value.a;
	}

	tee_client_close_session(dhub_teec_local_ctx, dhub_session_local_id);
out_ctx:
	tee_client_close_context(dhub_teec_local_ctx);

	return ret;
}

static int dhub_InvokeCommandHelper(struct tee_param *param,
			   int nParam, UNSG32 dhub_cmd)
{
	int ret;
	struct tee_ioctl_invoke_arg arg;

	if (!initialized)
		return -ENODEV;

	memset(&arg, 0, sizeof(arg));
	arg.func = dhub_cmd;
	arg.num_params = nParam;
	arg.session = dhub_session_id;

	ret = tee_client_invoke_func(dhub_teec_ctx, &arg, param);
	if (ret < 0 || arg.ret != 0) {
		pr_err("fail to invoke command(%x) to DHUB TA: ret = %x,"
				"TEE err: 0x%x\n", dhub_cmd, ret, arg.ret);
		ret = -EINVAL;
	}

	return ret;
}

void DhubEnableAutoPush(bool enable, bool useFastLogoTa)
{
	int ret;
	struct tee_param param[4];

	//If possible use fastlogo.ta to disable autopush, otherwise use dhub.ta
	if (useFastLogoTa) {
		ret = Fastlogo_EnableAutoPush(enable);
		if (ret == 0) {
			printk("AutoPush Disabled using fastlogo.ta : 0x%08x\n", ret);
			return;
		}
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = enable;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 2, DHUB_AUTOPUSH_EN);
	if (!ret)
		ret = param[1].u.value.a;

	return;
}

void DhubFinalize(void)
{
	if (!initialized)
		return;
	initialized = 0;

	tee_client_close_session(dhub_teec_ctx, dhub_session_id);
	tee_client_close_context(dhub_teec_ctx);
}

void tz_DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr,
			   UNSG32 hboSramAddr, int *pdhubHandle,
			   int *dhub_config, SIGN32 numOfChans)
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = cpuId;
	param[0].u.value.b = dHubBaseAddr;
	param[1].u.value.a = hboSramAddr;
	param[1].u.value.b = numOfChans;
	/* clear result */
	param[2].u.value.a = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 3, DHUB_INIT);
	if (!ret)
		ret = param[2].u.value.a;
}

void tz_DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[])
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[0].u.value.a = id;

	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 2, DHUB_CHANNEL_CLEAR);
	if (!ret)
		ret = param[2].u.value.a;
}

UNSG32 tz_dhub_channel_write_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				 SIGN32 id,	/*! Channel ID in $dHubReg ! */
				 UNSG32 addr,	/*! CMD: buffer address ! */
				 SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				 SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				 SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				 SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				 SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				 T64b cfgQ[],	/*! Pass NULL to directly update dHub, or
								 * Pass non-zero to receive programming sequence
								 * in (adr,data) pairs
								 *!
								 */
				 UNSG32 *ptr   /*! Pass in current cmdQ pointer (in 64b word),
								*& receive updated new pointer,
								*Pass NULL to read from HW
								*!
								*/
	)
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = id;
	param[0].u.value.b = addr;
	param[1].u.value.a = id;
	param[1].u.value.b = semOnMTU;

	param[2].u.value.a = chkSemId;
	param[2].u.value.b = updSemId;
	param[3].u.value.a = interrupt;

	/* clear result */
	param[3].u.value.b = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 4, DHUB_CHANNEL_WRITECMD);
	if (!ret)
		ret = param[3].u.value.b;

	return param[3].u.value.b;
}

void tz_dhub_channel_generate_cmd(void *hdl,	/*! Handle to HDL_dhub ! */
				  SIGN32 id,	/*! Channel ID in $dHubReg ! */
				  UNSG32 addr,	/*! CMD: buffer address ! */
				  SIGN32 size,	/*! CMD: number of bytes to transfer ! */
				  SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) ! */
				  SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore ! */
				  SIGN32 updSemId,	/*! CMD: non-zero to update semaphore ! */
				  SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish ! */
				  SIGN32 *pData)
{
	int ret;
	struct tee_param param[4];

	if (!pData)
		return;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;

	param[0].u.value.a = id;
	param[0].u.value.b = addr;
	param[1].u.value.a = id;
	param[1].u.value.b = semOnMTU;

	param[2].u.value.a = chkSemId;
	param[2].u.value.b = updSemId;
	param[3].u.value.a = interrupt;

	/* clear result */
	param[3].u.value.b = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 4, DHUB_CHANNEL_GENERATECMD);
	if (!ret)
		ret = param[3].u.value.b;

	pData[0] = param[0].u.value.a;
	pData[1] = param[0].u.value.b;
}

void tz_semaphore_pop(void *hdl,	/*  Handle to HDL_semaphore */
			  SIGN32 id,	/*  Semaphore ID in $SemaHub */
			  SIGN32 delta)	/*  Delta to pop as a consumer */
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[0].u.value.a = id;
	param[0].u.value.b = delta;

	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 2, DHUB_SEM_POP);
	if (!ret)
		ret = param[2].u.value.a;
}

void tz_semaphore_clr_full(void *hdl,	/*  Handle to HDL_semaphore */
			   SIGN32 id)	/*  Semaphore ID in $SemaHub */
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[0].u.value.a = id;

	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 2, DHUB_SEM_CLR_FULL);
	if (!ret)
		ret = param[2].u.value.a;
}

UNSG32 tz_semaphore_chk_full(void *hdl,	/*Handle to HDL_semaphore */
				 SIGN32 id)	/*Semaphore ID in $SemaHub
							 * -1 to return all 32b of the interrupt status
							 */
{
	int ret;
	struct tee_param param[4];
	UNSG32 d;

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[0].u.value.a = id;

	/* clear result */
	param[1].u.value.a = 0xdeadbeef;
	param[1].u.value.b = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 2, DHUB_SEM_CHK_FULL);
	if (!ret)
		ret = param[2].u.value.a;

	/*(d = value of(sem->ra + RA_SemaHub_full); */
	d = param[1].u.value.a;
	return (id < 0) ? d : bTST(d, id);

}

void tz_semaphore_intr_enable(void *hdl,	/*! Handle to HDL_semaphore ! */
				  SIGN32 id,	/*! Semaphore ID in $SemaHub ! */
				  SIGN32 empty,	/*! Interrupt enable for CPU at condition 'empty' ! */
				  SIGN32 full,	/*! Interrupt enable for CPU at condition 'full' ! */
				  SIGN32 almostEmpty,	/*! Interrupt enable for CPU at condition 'almostEmpty' ! */
				  SIGN32 almostFull,	/*! Interrupt enable for CPU at condition 'almostFull' ! */
				  SIGN32 cpu)	/*! CPU ID (0/1/2) ! */
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = id;
	param[0].u.value.b = empty;
	param[1].u.value.a = full;
	param[1].u.value.b = almostEmpty;

	param[2].u.value.a = almostFull;
	param[2].u.value.b = cpu;

	/* clear result */
	param[3].u.value.a = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 4, DHUB_SEM_INTR_ENABLE);
	if (!ret)
		ret = param[3].u.value.a;
}

void tz_dhub2nd_channel_start_seq(void *hdl, SIGN32 id)
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = id;

	ret = dhub_InvokeCommandHelper(param, 1, DHUB_2ND_CHANNEL_START_SEQ);
}

void tz_dhub2nd_channel_clear_seq(void *hdl, SIGN32 id)
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = id;

	ret = dhub_InvokeCommandHelper(param, 1, DHUB_2ND_CHANNEL_CLEAR_SEQ);
}

int tz_BCM_SCHED_PushCmd(UNSG32 QID, UNSG32 *pCmd, UNSG32 *cfgQ)
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = QID;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.b = pCmd[0];
	param[1].u.value.a = pCmd[1];

	/* clear result */
	param[1].u.value.b = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 2, DHUB_BCM_SCHED_PUSHCMD);
	if (!ret)
		ret = param[1].u.value.b;

	return param[1].u.value.b;
}

void tz_BCM_SCHED_GetEmptySts(UNSG32 QID, UNSG32 *EmptySts)
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = QID;

	/* clear result */
	param[0].u.value.b = 0xdeadbeef;

	ret = dhub_InvokeCommandHelper(param, 2, DHUB_BCM_SCHED_GETEMPTYSTS);
	if (!ret)
		ret = param[0].u.value.b;

	*EmptySts = param[0].u.value.b;
}

void tz_BCM_SCHED_SetMux(UNSG32 QID, UNSG32 TrigEvent)
{
	int ret;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = QID;
	param[0].u.value.b = TrigEvent;

	ret = dhub_InvokeCommandHelper(param, 1, DHUB_BCM_SCHED_SETMUX);
}

EXPORT_SYMBOL(DhubEnableAutoPush);
EXPORT_SYMBOL(DhubInitialize);
EXPORT_SYMBOL(DhubFinalize);
