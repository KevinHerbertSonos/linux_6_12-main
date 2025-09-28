// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispbe_err.h"
#include "ispSS_clkrst.h"
#include "common.h"

#include <linux/kthread.h>

#define ISPSS_INVALID_MODULE ISPBE_MODULE_MAX
#define IS_VALID_MODULE(module)		\
		((module >= ISPBE_MODULE_MIN && \
		  module < ISPBE_MODULE_MAX) ? 1:0)
#define IS_VALID_SEMID(DhubSem_Id)		\
	((DhubSem_Id >= ISP_DHUBSEM_TSB_MIN && \
	  DhubSem_Id < ISP_DHUBSEM_TSB_MAX) ? 1:0)

static struct ISPBE_CA_CTX g_ispbe_ca_ctx;

static int ISPBE_GetModuleFromIntrNum(unsigned int intrNum)
{
	int module = ISPBE_MODULE_MAX;

	switch (intrNum) {
	case ISP_DHUBSEM_TSB_Scldn_intr0:
	case ISP_DHUBSEM_TSB_Scldn_intr1:
	case ISP_DHUBSEM_TSB_Scldn_intr2:
		module = ISPBE_MODULE_DNSCL3;
		break;
	case ISP_DHUBSEM_TSB_dwrp_intr:
		module = ISPBE_MODULE_DEWARP;
		break;
	case ISP_DHUBSEM_TSB_rot_intr:
		module = ISPBE_MODULE_ROT;
		break;
	case ISP_DHUBSEM_TSB_tile_intr:
		module = ISPBE_MODULE_TILER;
		break;
	case ISP_DHUBSEM_TSB_ispIntr:
	case ISP_DHUBSEM_TSB_miIntr:
	case ISP_DHUBSEM_TSB_mipi0Intr:
	case ISP_DHUBSEM_TSB_mipi1Intr:
	case ISP_DHUBSEM_TSB_bcmInvalidIntr:
	case ISP_DHUBSEM_TSB_usr_inr13:
	case ISP_DHUBSEM_TSB_usr_inr14:
	case ISP_DHUBSEM_TSB_usr_inr15:
	default:
		module = ISPBE_MODULE_MAX;
		break;
	}

	return module;
}

static int ISPBE_ProcessISR(unsigned int intrNum, void *pArgs)
{
	struct ISPBE_DRIVER_CB *intr_cb;
	struct ISPBE_CA_DRV_CTX *drv_ctx;
	int module = ISPBE_GetModuleFromIntrNum(intrNum);
	struct ISPBE_INTR_CB *cbPostParam, *cbPreParam;
	CC_MSG_t *stMsg;
	UINT8 clientID;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	intr_cb = &g_ispbe_ca_ctx.intr_cb[intrNum];
	drv_ctx = (module >= ISPBE_MODULE_MAX) ? NULL : &g_ispbe_ca_ctx.drv_ctx[module];

	if (intr_cb->pre_IntrCB || intr_cb->post_IntrCB) {
		//handle module that can do pop_request
		//So invoke pre/post callback; pre for pushRqst, post for popRsqt

		//Pass interrupt value to ISP core callback
		if ((intrNum == ISP_DHUBSEM_TSB_ispIntr) || (intrNum == ISP_DHUBSEM_TSB_miIntr)) {
			stMsg = (CC_MSG_t *) pArgs;
			if ((stMsg != NULL) && (intr_cb->pre_param != NULL)) {
				cbPreParam = (struct ISPBE_INTR_CB *)intr_cb->pre_param;
				cbPreParam->intrStatus = stMsg->m_Param1;
			}

			if ((stMsg != NULL) && (intr_cb->post_param != NULL)) {
				cbPostParam = (struct ISPBE_INTR_CB *)intr_cb->post_param;
				cbPostParam->intrStatus = stMsg->m_Param1;
			}
		}

		if (intr_cb->pre_IntrCB)
			(*intr_cb->pre_IntrCB)(intrNum, intr_cb->pre_param);

		if (drv_ctx && drv_ctx->fops.module_ProcessISR)
			drv_ctx->fops.module_ProcessISR(intrNum, pArgs);

		if (intr_cb->post_IntrCB)
			(*intr_cb->post_IntrCB)(intrNum, intr_cb->post_param);

	} else if (drv_ctx) {
		//handle module that can't do pop_request
		//So do pop here and send the request back in the callback associate with client
		if (drv_ctx->fops.module_ProcessISR)
			drv_ctx->fops.module_ProcessISR(intrNum, pArgs);

		if (drv_ctx->fops.module_PopRequest) {
			struct ISP_BE_RQST_MSG *pstRqstMsg = NULL;

			drv_ctx->fops.module_PopRequest(&pstRqstMsg);

			if (pstRqstMsg) {
				clientID = pstRqstMsg->m_BuffID;
				if (clientID < ISPBE_CLIENT_MAX && drv_ctx->client_cb[clientID])
					drv_ctx->client_cb[clientID](pstRqstMsg);
			}
		}
	}

	return 0;
}

static int ISPBE_ISR_Handler_Task(void)
{
	CC_MSG_t msg;
	int rc;

	while (!g_ispbe_ca_ctx.isrTaskExitFlag) {
		rc = ispss_wait_dhub_sem_intr(&msg);
		if (!rc)
			ISPBE_ProcessISR(msg.m_Param2, &msg);
	}
	return 0;
}

void ISPSS_CA_ClockGateSharedResources(BOOL state)
{
	struct ISPBE_CA_CLKGT_CTX *clkgate_ctx = NULL;


	clkgate_ctx = &g_ispbe_ca_ctx.clkgate_ctx;
	mutex_lock(&clkgate_ctx->ispbe_clkgate_mutex);
	if (state == ISPSS_CLK_STATE_ENABLE) {
		if (clkgate_ctx->ispbe_clkgate_shared_refcnt == 0) {
			ISPSS_CLKRST_ISPMiscModuleSetClockGateState(ISPMISC_MODULE_BCM,
					ISPSS_CLKRST_CLK_STATE_ENABLE);
			ISPSS_CLKRST_ISPMiscModuleSetClockGateState(ISPMISC_MODULE_DHUB_TSB,
					ISPSS_CLKRST_CLK_STATE_ENABLE);
			ISPSS_CLKRST_ISPMiscModuleSetClockGateState(ISPMISC_MODULE_DHUB_FWR,
					ISPSS_CLKRST_CLK_STATE_ENABLE);
		}
		clkgate_ctx->ispbe_clkgate_shared_refcnt++;
	} else if (state == ISPSS_CLK_STATE_DISABLE) {
		if (clkgate_ctx->ispbe_clkgate_shared_refcnt > 0)
			clkgate_ctx->ispbe_clkgate_shared_refcnt -= 1;
		if (clkgate_ctx->ispbe_clkgate_shared_refcnt == 0) {
			ISPSS_CLKRST_ISPMiscModuleSetClockGateState(ISPMISC_MODULE_DHUB_FWR,
					ISPSS_CLKRST_CLK_STATE_DISABLE);
			ISPSS_CLKRST_ISPMiscModuleSetClockGateState(ISPMISC_MODULE_DHUB_TSB,
					ISPSS_CLKRST_CLK_STATE_DISABLE);
			ISPSS_CLKRST_ISPMiscModuleSetClockGateState(ISPMISC_MODULE_BCM,
					ISPSS_CLKRST_CLK_STATE_DISABLE);
		}
	}
	mutex_unlock(&clkgate_ctx->ispbe_clkgate_mutex);
}
EXPORT_SYMBOL(ISPSS_CA_ClockGateSharedResources);

INT ISPBE_CA_Initialize(void)
{
	HRESULT Ret = S_OK;
	static int is_created;
	int module;

	/* Create memory mapping - isp dev open and then DHUB-Init */
	if (ispss_create_iomap() == -EINVAL) {
		Ret = -EINVAL;
		goto error1;
	}

	if (!is_created)
		memset(&g_ispbe_ca_ctx, 0, sizeof(struct ISPBE_CA_CTX));

	if (is_created) {
		Ret = -1;
		goto error1;
	} else {
		is_created = 1;
		g_ispbe_ca_ctx.is_created = 1;
	}

	/* Initialize DHUB */
	ispss_dhub_init();

	/* Initialize BCM */
	ISPSS_BCMBUF_Init();

	/* Create ISR Handler task */
	g_ispbe_ca_ctx.isrTaskExitFlag = 0;

	g_ispbe_ca_ctx.ispbe_isr_handler_task = kthread_run((void *)ISPBE_ISR_Handler_Task,
			NULL, "ISPBE ISR Handler task");

	for (module = ISPBE_MODULE_MIN; module < ISPBE_MODULE_MAX; ++module)
		g_ispbe_ca_ctx.clkgate_ctx.ispbe_clkgate_module_refcnt[module] = 0;

	g_ispbe_ca_ctx.clkgate_ctx.ispbe_clkgate_shared_refcnt = 0;
	mutex_init(&g_ispbe_ca_ctx.clkgate_ctx.ispbe_clkgate_mutex);

	ISPSS_CA_ClockGateSharedResources(ISPSS_CLK_STATE_DISABLE);
	for (module = ISPBE_MODULE_MIN; module < ISPBE_MODULE_MAX; ++module)
		ISPSS_CLKRST_ISPBeModuleSetClockGateState(module, ISPSS_CLKRST_CLK_STATE_DISABLE);

error1:
	return Ret;
}

INT ISPBE_CA_DeInitialize(void)
{
	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	ISPSS_BCMBUF_DeInit();

	ispss_destroy_iomap();

	g_ispbe_ca_ctx.isrTaskExitFlag = 1;
	ispss_enable_dhub_sem_intr(ISP_DHUBSEM_TSB_Scldn_intr1, 0);
	kthread_stop(g_ispbe_ca_ctx.ispbe_isr_handler_task);

	g_ispbe_ca_ctx.is_created = 0;
	mutex_destroy(&g_ispbe_ca_ctx.clkgate_ctx.ispbe_clkgate_mutex);
	return 0;
}

INT ISPBE_MODULE_Init(enum ISPBE_MODULES module, struct ISPBE_CA_DRV_CTX module_ctx,
		ispbe_client_cb_t client_cb)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;
	struct ISPBE_CA_CLKGT_CTX *clkgate_ctx = NULL;

	if (!g_ispbe_ca_ctx.is_created)
		return -EPROBE_DEFER;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];
	clkgate_ctx = &g_ispbe_ca_ctx.clkgate_ctx;

	memcpy(&drv_ctx->fops, &module_ctx.fops, sizeof(struct ISPBE_DRIVER_OPS));
	drv_ctx->sem_id = module_ctx.sem_id;

	if (drv_ctx->fops.module_init != NULL) {
		ispbe_module_init module_initi = drv_ctx->fops.module_init;

		Ret =  module_initi();

		if (module == ISPBE_MODULE_TILER) {
			mutex_lock(&clkgate_ctx->ispbe_clkgate_mutex);
			//TODO verify with tiler
			ISPSS_CA_ClockGateSharedResources(ISPSS_CLK_STATE_ENABLE);
			if (clkgate_ctx->ispbe_clkgate_module_refcnt[module] == 0) {
				ISPSS_CLKRST_ISPBeModuleSetClockGateState(module,
						ISPSS_CLKRST_CLK_STATE_ENABLE);
				Ret = ispss_enable_dhub_sem_intr(drv_ctx->sem_id, 1);
			}
			clkgate_ctx->ispbe_clkgate_module_refcnt[module] += 1;
			mutex_unlock(&clkgate_ctx->ispbe_clkgate_mutex);

			//TODO Fix Client ID ?
			drv_ctx->client_cb[0] = client_cb;
		}
	} else {
		pr_err("%s: module_init fops doesn't exist !!\n", __func__);
		Ret = ISPSS_EBADCALL;
	}

	return Ret;
}
EXPORT_SYMBOL(ISPBE_MODULE_Init);

INT ISPBE_MODULE_Destroy(enum ISPBE_MODULES module)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;
	struct ISPBE_CA_CLKGT_CTX *clkgate_ctx = NULL;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];
	clkgate_ctx = &g_ispbe_ca_ctx.clkgate_ctx;

	if (drv_ctx->fops.module_destroy != NULL) {
		Ret = drv_ctx->fops.module_destroy();
		if (module == ISPBE_MODULE_TILER) {
			mutex_lock(&clkgate_ctx->ispbe_clkgate_mutex);
			if (clkgate_ctx->ispbe_clkgate_module_refcnt[module] > 0)
				clkgate_ctx->ispbe_clkgate_module_refcnt[module] -= 1;
			if (clkgate_ctx->ispbe_clkgate_module_refcnt[module] == 0) {
				Ret = ispss_enable_dhub_sem_intr(drv_ctx->sem_id, 0);
				ISPSS_CLKRST_ISPBeModuleSetClockGateState(module,
						ISPSS_CLKRST_CLK_STATE_DISABLE);
			}
			mutex_unlock(&clkgate_ctx->ispbe_clkgate_mutex);
			ISPSS_CA_ClockGateSharedResources(ISPSS_CLK_STATE_DISABLE);
		}
	} else {
		Ret = ISPSS_EBADCALL;
	}

	return Ret;
}
EXPORT_SYMBOL(ISPBE_MODULE_Destroy);

INT ISPBE_MODULE_Open(enum ISPBE_MODULES module, INT *clientID,
					INT priority, ispbe_client_cb_t client_cb)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;
	struct ISPBE_CA_CLKGT_CTX *clkgate_ctx = NULL;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];
	clkgate_ctx = &g_ispbe_ca_ctx.clkgate_ctx;

	if (drv_ctx->fops.module_open != NULL) {
		Ret = drv_ctx->fops.module_open(clientID, priority);
		if (Ret == S_OK)
			drv_ctx->client_cb[*clientID] = client_cb;

		ISPSS_CA_ClockGateSharedResources(ISPSS_CLK_STATE_ENABLE);

		mutex_lock(&clkgate_ctx->ispbe_clkgate_mutex);
		if (clkgate_ctx->ispbe_clkgate_module_refcnt[module] == 0) {
			ISPSS_CLKRST_ISPBeModuleSetClockGateState(module,
					ISPSS_CLKRST_CLK_STATE_ENABLE);
			Ret = ispss_enable_dhub_sem_intr(drv_ctx->sem_id, 1);
		}

		clkgate_ctx->ispbe_clkgate_module_refcnt[module] += 1;
		mutex_unlock(&clkgate_ctx->ispbe_clkgate_mutex);

	} else
		Ret = ISPSS_EBADCALL;

	return Ret;
}
EXPORT_SYMBOL(ISPBE_MODULE_Open);

INT ISPBE_MODULE_Close(enum ISPBE_MODULES module, INT clientID)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;
	struct ISPBE_CA_CLKGT_CTX *clkgate_ctx = NULL;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];
	clkgate_ctx = &g_ispbe_ca_ctx.clkgate_ctx;

	if (drv_ctx->fops.module_close != NULL) {
		Ret = drv_ctx->fops.module_close(clientID);

		if (Ret == ISPSS_EHARDWAREBUSY)
			return Ret;

		mutex_lock(&clkgate_ctx->ispbe_clkgate_mutex);

		if (clkgate_ctx->ispbe_clkgate_module_refcnt[module] > 0)
			clkgate_ctx->ispbe_clkgate_module_refcnt[module] -= 1;

		if (clkgate_ctx->ispbe_clkgate_module_refcnt[module] == 0) {
			Ret = ispss_enable_dhub_sem_intr(drv_ctx->sem_id, 0);
			ISPSS_CLKRST_ISPBeModuleSetClockGateState(module,
					ISPSS_CLKRST_CLK_STATE_DISABLE);
		}
		mutex_unlock(&clkgate_ctx->ispbe_clkgate_mutex);
		ISPSS_CA_ClockGateSharedResources(ISPSS_CLK_STATE_DISABLE);
	} else
		Ret = ISPSS_EBADCALL;

	return Ret;
}
EXPORT_SYMBOL(ISPBE_MODULE_Close);

INT ISPBE_MODULE_PushRequest(enum ISPBE_MODULES module, INT clientID,
				struct ISP_BE_RQST_MSG *pstRqstMsg)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];

	if (drv_ctx->fops.module_PushRequest != NULL) {
		//Ensure clientID is stored in bufferID for later use
		pstRqstMsg->m_BuffID = clientID;

		Ret = drv_ctx->fops.module_PushRequest(clientID, pstRqstMsg);
	} else
		Ret = ISPSS_EBADCALL;

	return Ret;
}
EXPORT_SYMBOL(ISPBE_MODULE_PushRequest);

/* Don't we need client ID, even though output Q is same for all clients ?*/
INT ISPBE_MODULE_PopRequest(enum ISPBE_MODULES module, INT clientID,
			struct ISP_BE_RQST_MSG **pstRqstMsg)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];

	if (drv_ctx->fops.module_PopRequest != NULL)
		Ret = drv_ctx->fops.module_PopRequest(pstRqstMsg);
	else
		Ret = ISPSS_EBADCALL;

	return Ret;
}

INT ISPBE_MODULE_ReleaseRequest(enum ISPBE_MODULES module, INT iClientId,
		struct ISP_BE_RQST_MSG **pstRqstMsg)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];

	if (drv_ctx->fops.module_ReleaseRequest != NULL)
		Ret = drv_ctx->fops.module_ReleaseRequest(iClientId, pstRqstMsg);
	else
		Ret = ISPSS_EBADCALL;

	return Ret;
}

INT ISPBE_MODULE_Ioctl(enum ISPBE_MODULES module, enum ISP_BE_IOCTL_CMD ioctl_cmd, void *param)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];

	if (drv_ctx->fops.module_ioctl != NULL)
		Ret = drv_ctx->fops.module_ioctl(ioctl_cmd, param);
	else
		Ret = ISPSS_EBADCALL;

	return Ret;
}
EXPORT_SYMBOL(ISPBE_MODULE_Ioctl);

INT ISPBE_MODULE_GetNoOfFramesWaiting(enum ISPBE_MODULES module,
			INT iClientId, UINT32 *puiFramesWaiting)
{
	HRESULT Ret = S_OK;
	struct ISPBE_CA_DRV_CTX *drv_ctx;

	if (!g_ispbe_ca_ctx.is_created)
		return -1;

	if (!IS_VALID_MODULE(module))
		return ISPSS_INVALID_MODULE;

	drv_ctx = &g_ispbe_ca_ctx.drv_ctx[module];

	if (drv_ctx->fops.module_GetNoOfFramesWaiting != NULL)
		Ret = drv_ctx->fops.module_GetNoOfFramesWaiting(iClientId, puiFramesWaiting);
	else
		Ret = ISPSS_EBADCALL;

	return Ret;
}
