// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/delay.h>

#include "ispSS_be_scl_apifuncs.h"
#include "ispSS_api_dhub_config.h"
#include "ispSS_be_scl_common.h"
#include "ispbe_err.h"
#include "ispSS_dhub_intr.h"

#include "ispSS_be_frc_scl.h"
#include "ispSS_be_scl_api.h"

#define ISPSS_SCL_BCMQ_ID BCM_SCHED_Q12
#define ISPSS_BE_SCL_BLOCKING_ISR_TIMEOUT_IN_100MS   50
#define GET_SCL_OBJ(index) ((struct ISP_BE_SCL_OBJ *)(g_isp_be_scl->isp_be_scl_obj[index]))
#define ISP_BE_SCL_BASE_ADDR (MEMMAP_ISP_REG_BASE + ISPSS_MEMMAP_DNSCL_REG_BASE)
#define ISPMISC_BASE (MEMMAP_ISP_REG_BASE + ISPSS_MEMMAP_GLB_REG_BASE)
#define CHECK_LIMITS(field, low, high) (((field >= low) && (field <= high)) ? 0 : 1)

static INT g_hwStinit;
static struct ISP_BE_RQST_MSG *g_prvsclRqstMsg;

static struct ISP_BE_SCL *g_isp_be_scl;
static struct ISP_BE_SCL_HW_STRUCT *g_hwObjst;

/*******************************************************************
 * FUNCTION: Initialise SCL
 * PARAMS:
 * RETURN:
 ******************************************************************/
static INT ISPSS_BE_SCL_Init(void)
{
	INT ret = ISPSS_OK;
	SHM_HANDLE shmHandle[2];
	void *sclAddr, *prvRqstMsg;

	if (g_isp_be_scl) {
		BE_SCL_LOGD("DNSCL Already initialized\n");
		return ISPSS_OK;
	}
	ret = ispSS_SHM_Allocate(SHM_NONSECURE, sizeof(struct ISP_BE_SCL), 1024,
			&shmHandle[0], SHM_NONSECURE_CONTIG);
	if (ret != ISPSS_OK) {
		BE_SCL_LOGE("ERRNUM:%d Memory Not allocated for SCL STRUCT\n", ret);
		return ret;
	}
	ispSS_SHM_GetVirtualAddress(shmHandle[0], 0, &(sclAddr));
	g_isp_be_scl = sclAddr;
	if (g_isp_be_scl == NULL) {
		BE_SCL_LOGE("ERR:Memory allocation failed at ISP SCL GBL STRUCT ...\n");
		return ISPSS_ENOMEM;
	}
	memset(g_isp_be_scl, 0, sizeof(struct ISP_BE_SCL));
	g_isp_be_scl->m_sclHwStatus = ISPSS_BE_SCL_HW_STATUS_IDLE;
	g_isp_be_scl->isp_be_scl_handle = shmHandle[0];
	g_isp_be_scl->commit_QId = SCL_COMMIT_QUEUE_ID_12;

	ret = ispSS_SHM_Allocate(SHM_NONSECURE, sizeof(struct ISP_BE_RQST_MSG),
			1024, &shmHandle[1], SHM_NONSECURE_CONTIG);
	if (ret != ISPSS_OK) {
		BE_SCL_LOGE("ERRNUM:%d Memory Not allocated for PRV SCL structure\n", ret);
		return ret;
	}
	ispSS_SHM_GetVirtualAddress(shmHandle[1], 0, &prvRqstMsg);
	g_prvsclRqstMsg = prvRqstMsg;
	if (g_prvsclRqstMsg == NULL) {
		BE_SCL_LOGE("ERR:Memory allocation failed at ISP SCL PRV RQST STRUCT ...\n");
		return ISPSS_ENOMEM;
	}
	memset(g_prvsclRqstMsg, 0, sizeof(struct ISP_BE_RQST_MSG));
	g_isp_be_scl->prv_msg_handle = shmHandle[1];

	//Initialize Output Queue
	isp_rqstq_reset(&(g_isp_be_scl->m_sclOutFrameQ));
	//Initialize Process Queue
	g_isp_be_scl->bcmbufQ.queueCfg.maxQueueLen = 6;
	g_isp_be_scl->bcmbufQ.queueCfg.maxBcmBuf = 2;
	g_isp_be_scl->bcmbufQ.queueCfg.bcmBufSize = BCM_BUFFER_SIZE;
	g_isp_be_scl->bcmbufQ.queueCfg.maxDhubCfgQ = 2;
	g_isp_be_scl->bcmbufQ.queueCfg.dmaCmdSize = DMA_CMD_BUFFER_SIZE;
	ISPSS_BCMBUF_QUEUE_Reset(&g_isp_be_scl->bcmbufQ);
	ISPSS_BCMBUF_QUEUE_Create(&g_isp_be_scl->bcmbufQ);
	BCM_SchedSetMux(BCM_SCHED_Q9, 9);
	mutex_init(&g_isp_be_scl->isr_lock);

	return ISPSS_OK;

}

/*******************************************************************
 * FUNCTION: Create and allocate memory for scalar
 * PARAMS:
 * RETURN:
 ******************************************************************/
static void ISPSS_BE_SCL_ResetClient(INT index, INT priority)
{
	g_isp_be_scl->isp_be_scl_obj[index]->m_iValid = 1;
	g_isp_be_scl->isp_be_scl_obj[index]->base_addr = ISP_BE_SCL_BASE_ADDR;
	g_isp_be_scl->isp_be_scl_obj[index]->objPriority = priority;
	g_isp_be_scl->isp_be_scl_obj[index]->clientID = index;
	g_isp_be_scl->isp_be_scl_obj[index]->objStatus = ISPSS_BE_SCL_OBJ_STATUS_ACTIVE;
	g_isp_be_scl->isp_be_scl_obj[index]->m_queue.InputQCnt = 0;
	g_isp_be_scl->isp_be_scl_obj[index]->m_queue.ProcessQCnt = 0;
	g_isp_be_scl->isp_be_scl_obj[index]->m_queue.OutputQCnt = 0;
	isp_rqstq_reset(&(g_isp_be_scl->isp_be_scl_obj[index]->m_queue.InRqstQ));
	g_isp_be_scl->isp_be_scl_obj[index]->m_queue.OutRqstQ
		= &(g_isp_be_scl->m_sclOutFrameQ);
#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_init(&(g_isp_be_scl->isp_be_scl_obj[index]->m_queue.InRqstQ.isp_rqstq_lock));
#endif

}

static int ISPSS_BE_SCL_ResetConfig(int clientID)
{
	//Load Default Configuration
	ISPSS_FRC_SCL_LoadDefaultVal(g_isp_be_scl->isp_be_scl_obj[clientID],
			ISP_FRC_SCL_OVP_Y, g_hwObjst);
	ISPSS_FRC_SCL_LoadDefaultVal(g_isp_be_scl->isp_be_scl_obj[clientID],
			ISP_FRC_SCL_OVP_UV, g_hwObjst);
	return ISPSS_OK;
}

/*******************************************************************
 * FUNCTION: Create and allocate memory for scalar
 * PARAMS:
 * RETURN:
 ******************************************************************/
static INT ISPSS_BE_SCL_Open(INT *clientID, INT priority)
{
	INT cnt, objCnt, ret = ISPSS_FAIL;
	//static INT mapToClientID[MAX_SCL_CLIENTS];
	SHM_HANDLE shmHandle[4];
	void *sclObjAddr, *hwStSclAddr, *scl_coeffHandle, *scl_coefflayerHandle;

	for (objCnt = 0; objCnt < MAX_SCL_OBJECTS; objCnt++) {
		if (g_isp_be_scl->isp_be_scl_obj[objCnt] == NULL) {
			ret = ispSS_SHM_Allocate(SHM_NONSECURE, sizeof(struct ISP_BE_SCL_OBJ), 1024,
					&g_isp_be_scl->scl_handle[objCnt], SHM_NONSECURE_CONTIG);
			if (ret != 0) {
				BE_SCL_LOGE("Failed to Allocate memory for SCL\n");
				ret = ISPSS_ENOMEM;
				goto e_ISPSS_BE_SCL_Create;
			}
			ispSS_SHM_GetVirtualAddress(g_isp_be_scl->scl_handle[objCnt],
					0, &(sclObjAddr));
			g_isp_be_scl->isp_be_scl_obj[objCnt] = sclObjAddr;
			*clientID = objCnt;
			ISPSS_BE_SCL_ResetClient(objCnt, priority);
			BE_SCL_LOGD("SCL OBJECT CREATED,%d\n", *clientID);
			break;
		} else if (!g_isp_be_scl->isp_be_scl_obj[objCnt]->m_iValid) {
			*clientID = objCnt;
			ISPSS_BE_SCL_ResetClient(objCnt, priority);
			BE_SCL_LOGD("RE-USE SCL OBJECT CREATED,%d\n", *clientID);
			break;
		}
	}
	if (!g_hwStinit) {
		ret = ispSS_SHM_Allocate(SHM_NONSECURE, sizeof(struct ISP_BE_SCL_HW_STRUCT),
				1024, &shmHandle[1], SHM_NONSECURE_CONTIG);
		if (ret) {
			BE_SCL_LOGE("Memory allocation failed at SCL init...\n");
			return ISPSS_ENOMEM;
		}
		ispSS_SHM_GetVirtualAddress(shmHandle[1], 0, &(hwStSclAddr));
		g_hwObjst = hwStSclAddr;
		memset(g_hwObjst, 0, sizeof(struct ISP_BE_SCL_HW_STRUCT));
		g_hwObjst->hw_handle = shmHandle[1];

		/*DHUB ID INITIALIZATION */
		g_hwObjst->dmaDhubID = (UINT64)&ISPSS_TSB_dhubHandle;
		g_hwObjst->dmaRID[0] = ispDhubChMap_TSB_Dnscl_R0;
		g_hwObjst->dmaRID[1] = ispDhubChMap_TSB_Dnscl_R1;
		g_hwObjst->dmaWID[0] = ispDhubChMap_TSB_Dnscl_W0;
		g_hwObjst->dmaWID[1] = ispDhubChMap_TSB_Dnscl_W1;
		/* Begin to allocate memory for SCL Coeff */
		for (cnt = FIRST_SCALAR_COEFF; cnt <= SCALAR_COEFF_MAIN_MAX; cnt++) {
			int sclLayer;
			int numOfSclLayers = 0;

			if (SCALAR_COEFF_MAIN_MIN <= cnt && SCALAR_COEFF_MAIN_MAX >= cnt)
				numOfSclLayers = ISP_FRC_SCL_MAIN_LAY_MAX;

			if (!g_hwObjst->scl_coeffs[cnt]) {
				ret = ispSS_SHM_Allocate(SHM_NONSECURE,
						(ISP_FRC_COEFFTAB_BCMBUF_SIZE * numOfSclLayers),
						1024, &shmHandle[2], SHM_NONSECURE_CONTIG);
				if (ret) {
					BE_SCL_LOGE("SCL Coeff Mem alloc failed,%s,%d\n",
						__func__, __LINE__);
					ret = ISPSS_ENOMEM;
					goto e_ISPSS_BE_SCL_Create;
				}
				ispSS_SHM_GetVirtualAddress(shmHandle[2], 0, &(scl_coeffHandle));
				g_hwObjst->scl_coeffs[cnt] = scl_coeffHandle;
				g_hwObjst->scl_coeffs_handle[cnt] = shmHandle[2];
			}
			for (sclLayer = 0; sclLayer < numOfSclLayers; sclLayer++) {
				if (!g_hwObjst->cust_scl_coeffs[sclLayer][cnt]) {
					ret = ispSS_SHM_Allocate(SHM_NONSECURE,
							ISP_FRC_SCL_COEFF_TAB_SIZE,
							1024, &shmHandle[3], SHM_NONSECURE_CONTIG);
					if (ret) {
						BE_SCL_LOGE("SCL Coeff Mem alloc failed,%s,%d\n",
							__func__, __LINE__);
						ret = ISPSS_ENOMEM;
						goto e_ISPSS_BE_SCL_Create;
					}
					ispSS_SHM_GetVirtualAddress(shmHandle[3], 0,
							&(scl_coefflayerHandle));
					memset(scl_coefflayerHandle, 0, 1024);
					g_hwObjst->cust_scl_coeffs[sclLayer][cnt] =
						scl_coefflayerHandle;
					g_hwObjst->cust_scl_coeffs_handle[sclLayer][cnt] =
						shmHandle[3];
				}
			}
		}
		/* End to allocate memory for SCL Coeff */
		g_hwStinit = 1;
		BE_SCL_LOGD("SCL COEEFICENT MEMORY CREATION SUCCESS\n");
	}
	ret = ISPSS_BE_SCL_ResetConfig(*clientID);
	BE_SCL_LOGD("SCL LOAD DEFAULT CONFIG SUCCESS\n");
e_ISPSS_BE_SCL_Create:
	return ret;
}


/*******************************************************************
 * FUNCTION: Configure SCL
 * PARAMS:
 * RETURN:
 ******************************************************************/
static INT ISPSS_BE_SCL_CONFIG(INT clientID, struct BCMBUF *pbcmbuf)
{
	int ret = ISPSS_OK;

	/* Initial Configuration For DNSCL*/
	ret = ISPSS_BE_SCL_initalConfig(g_isp_be_scl->isp_be_scl_obj[clientID],
			g_hwObjst, pbcmbuf);
	if (ret == ISPSS_OK)
		return ISPSS_OK;
	else
		return ISPSS_FAIL;

}

/*******************************************************************
 * FUNCTION: Validate the inputs
 * PARAMS: psclRqstMsg - Request message
 * RETURN: ISPSS_OK - succeed
 *         ISPSS_EBADPARAM - Bad parameter
 *******************************************************************/
static INT ISPSS_BE_SCL_ValidateRqstMsg(struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	HRESULT Ret = ISPSS_OK;
	struct ISP_BE_RQST_MSG *rqstMsg = psclRqstMsg;

	if (psclRqstMsg == NULL) {
		Ret = ISPSS_EBADPARAM;
		goto e_ISP_BE_SCL_ValidateRqstMsg;
	}
	if (CHECK_LIMITS(rqstMsg->in_frame_fmt, ISPSS_SRCFMT_MIN, ISPSS_SRCFMT_MAX)) {
		Ret = ISPSS_EINVALID_SRCFMT;
		BE_SCL_LOGD("in_frame_fmt = %d\n", rqstMsg->in_frame_fmt);
		goto e_ISP_BE_SCL_ValidateRqstMsg;
	}
	if ((CHECK_LIMITS(rqstMsg->active.height, 0, 2160)) ||
		(CHECK_LIMITS(rqstMsg->active.width, 0, 3840))) {
		Ret = ISPSS_EINVALID_RES;
		goto e_ISP_BE_SCL_ValidateRqstMsg;
	}

	if ((CHECK_LIMITS(rqstMsg->outwin.height, 0, 1080)) ||
		(CHECK_LIMITS(rqstMsg->outwin.width, 0, 1920))) {
		Ret = ISPSS_EINVALID_RES;
		goto e_ISP_BE_SCL_ValidateRqstMsg;
	}
e_ISP_BE_SCL_ValidateRqstMsg:
	return Ret;
}

/*********************************************************************
 * FUNCTION:Close client
 * PARAMS:
 * RETURN:
 ********************************************************************/
static INT  ISPSS_BE_SCL_Close(INT clientID)
{
	struct ISP_BE_SCL_OBJ *isp_be_scl_obj;
	int Ret = ISPSS_OK;
	INT objCnt;
	bool bFound = false;

	if (g_isp_be_scl->m_sclHwStatus == ISPSS_BE_SCL_HW_STATUS_RUNNING) {
		Ret = ISPSS_EHARDWAREBUSY;
		goto e_DNSCL_Close;
	}
	isp_be_scl_obj = GET_SCL_OBJ(clientID);
	if (isp_be_scl_obj == NULL) {
		BE_SCL_LOGE("%s: NO DNS client found for this\n", __func__);
		Ret = ISPSS_ENODEV;
		goto e_DNSCL_Close;
	}
	if (isp_be_scl_obj->m_queue.OutputQCnt != 0) {
		BE_SCL_LOGE("%s: The output queue is not empty %d\n", __func__,
				isp_be_scl_obj->m_queue.OutputQCnt);
		Ret = ISPSS_EQNOTEMPTY;
		goto e_DNSCL_Close;
	}

	g_isp_be_scl->isp_be_scl_obj[clientID]->m_iValid = 0;

	for (objCnt = 0; objCnt < MAX_SCL_OBJECTS; objCnt++) {
		if (g_isp_be_scl->isp_be_scl_obj[clientID]->m_iValid)
			bFound = true;
	}
	/* Disable MMU/MTR clients when no SCL client running */
	if (bFound == false) {
		ISPSS_BE_MTR_Set_Global_Registers(ISPSS_BE_DNSCL_RD, false);
		ISPSS_BE_MTR_Set_Global_Registers(ISPSS_BE_DNSCL_WR, false);
		isp_rqstq_reset(&(g_isp_be_scl->m_sclOutFrameQ));
	}

	pr_debug("%s: with instance %d\n", __func__, clientID);

e_DNSCL_Close:
	return Ret;
}

/*********************************************************************
 * FUNCTION: Free allocated memory for scalar
 * PARAMS:
 * RETURN:
 ********************************************************************/
static INT ISPSS_BE_SCL_Destroy(void)
{
	struct ISP_BE_SCL_OBJ *isp_be_scl_obj;
	INT i, cnt, Ret = ISPSS_OK;

	if (g_isp_be_scl->m_sclHwStatus == ISPSS_BE_SCL_HW_STATUS_RUNNING) {
		Ret = ISPSS_EHARDWAREBUSY;
		goto e_DNSCL_Destroy;
	}

	for (i = 0; i < MAX_SCL_OBJECTS; i++) {
		isp_be_scl_obj = GET_SCL_OBJ(i);
		if (isp_be_scl_obj) {
			if (isp_be_scl_obj->m_iValid) {
				BE_SCL_LOGE("Close is not called for id = %d\n", i);
				Ret = ISPSS_EHARDWAREBUSY;
				goto e_DNSCL_Destroy;
			} else {
				ispSS_SHM_Release(g_isp_be_scl->scl_handle[i]);
				isp_be_scl_obj = NULL;
			}
		}
	}

	if (g_hwStinit) {
		for (cnt = FIRST_SCALAR_COEFF; cnt <= SCALAR_COEFF_MAIN_MAX; cnt++) {
			int sclLayer;
			int numOfSclLayers = 0;

			if (SCALAR_COEFF_MAIN_MIN <= cnt && SCALAR_COEFF_MAIN_MAX >= cnt)
				numOfSclLayers = ISP_FRC_SCL_MAIN_LAY_MAX;

			for (sclLayer = 0; sclLayer < numOfSclLayers; sclLayer++) {
				if (g_hwObjst->cust_scl_coeffs[sclLayer][cnt]) {
					ispSS_SHM_Release(
					g_hwObjst->cust_scl_coeffs_handle[sclLayer][cnt]);
					g_hwObjst->cust_scl_coeffs_handle[sclLayer][cnt] = 0;
				}
			}
			if (g_hwObjst->scl_coeffs[cnt]) {
				ispSS_SHM_Release(g_hwObjst->scl_coeffs_handle[cnt]);
				g_hwObjst->scl_coeffs_handle[cnt] = 0;
			}
		}
		/* End to allocate memory for SCL Coeff */
		ispSS_SHM_Release(g_hwObjst->hw_handle);
		g_hwObjst = NULL;
		g_hwStinit = 0;
		BE_SCL_LOGD("SCL COEEFICENT MEMORY Destroy SUCCESS\n");
	}

	ISPSS_BCMBUF_QUEUE_Destroy(&g_isp_be_scl->bcmbufQ);

	mutex_destroy(&g_isp_be_scl->isr_lock);
	if (g_prvsclRqstMsg) {
		ispSS_SHM_Release(g_isp_be_scl->prv_msg_handle);
		g_prvsclRqstMsg = NULL;
	}

	ispSS_SHM_Release(g_isp_be_scl->isp_be_scl_handle);
	g_isp_be_scl = NULL;

e_DNSCL_Destroy:
	return Ret;
}

static void printMsg(struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	if (psclRqstMsg == NULL) {
		BE_SCL_LOGE("Invalid Message request\n");
		return;
	}

	ISPSS_SCLDBG("psclRqstMsg = 0x%08lx\n", (unsigned long)psclRqstMsg);
	ISPSS_SCLDBG("IVres = %d\n", psclRqstMsg->active.height);
	ISPSS_SCLDBG("IHRes = %d\n", psclRqstMsg->active.width);
	ISPSS_SCLDBG("OVRes = %d\n", psclRqstMsg->outwin.height);
	ISPSS_SCLDBG("OHRes = %d\n", psclRqstMsg->outwin.width);

	ISPSS_SCLDBG("in_frame_bit_depth = %d\n", psclRqstMsg->in_frame_bit_depth);
	ISPSS_SCLDBG("out_frame_bit_depth = %d\n", psclRqstMsg->out_frame_bit_depth);
	ISPSS_SCLDBG("in_frame_fmt = %d\n", psclRqstMsg->in_frame_fmt);
	ISPSS_SCLDBG("out_frame_fmt = %d\n", psclRqstMsg->out_frame_fmt);

	ISPSS_SCLDBG("inAddr Y = 0x%08lx\n", (unsigned long)psclRqstMsg->InputFrameAddr_Y);
	ISPSS_SCLDBG("in Y Size= %d\n", psclRqstMsg->in_frame_Ysize);
	ISPSS_SCLDBG("inAddr UV = 0x%08lx\n", (unsigned long)psclRqstMsg->InputFrameAddr_UV);
	ISPSS_SCLDBG("in UV Size= %d\n", psclRqstMsg->in_frame_UVsize);

	ISPSS_SCLDBG("OutAddr Y = 0x%08lx\n", (unsigned long) psclRqstMsg->OutputFrameAddr_Y);
	ISPSS_SCLDBG("Out Y Size= %d\n", psclRqstMsg->out_frame_Ysize);
	ISPSS_SCLDBG("OutAddr UV = 0x%08lx\n", (unsigned long)psclRqstMsg->OutputFrameAddr_UV);
	ISPSS_SCLDBG("Out UV Size= %d\n", psclRqstMsg->out_frame_UVsize);
}

static int fillObjectDetails(struct ISP_BE_RQST_MSG *psclRqstMsg,
		struct ISP_BE_SCL_OBJ *isp_scl_obj)
{
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->contentWin.height
		= psclRqstMsg->content.height;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->contentWin.width
		= psclRqstMsg->content.width;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->inputWin.x
		= psclRqstMsg->active.x;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->inputWin.y
		= psclRqstMsg->active.y;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->inputWin.height
		= psclRqstMsg->active.height;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->inputWin.width
		= psclRqstMsg->active.width;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->outputWin.height
		= psclRqstMsg->outwin.height;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->outputWin.width
		= psclRqstMsg->outwin.width;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->in_bit_depth
		= psclRqstMsg->in_frame_bit_depth;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->output_bit_depth
		= psclRqstMsg->out_frame_bit_depth;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->inSrcFrmt
		= psclRqstMsg->in_frame_fmt;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->outSrcFrmt
		= psclRqstMsg->out_frame_fmt;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->ovp_win_updated   = 1;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->inAddr_y
		= (UINT64)psclRqstMsg->InputFrameAddr_Y;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->inAddr_UV
		=  (UINT64)psclRqstMsg->InputFrameAddr_UV;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->outAddr_Y
		=  (UINT64)psclRqstMsg->OutputFrameAddr_Y;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->outAddr_UV
		=  (UINT64)psclRqstMsg->OutputFrameAddr_UV;
	g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID]->ovp_win_stride
		=  psclRqstMsg->out_frame_Ystride;

	return ISPSS_OK;
}

/*******************************************************************
 * FUNCTION: Clear cache for BCM buffers
 *
 *
 * PARAMS:   pstsclRqstMsg - Request message
 * RETURN:   ISPSS_OK - succeed
 ********************************************************************/
static void ISPSS_BE_SCL_BCM_ClearCache(struct ISP_BE_RQST_MSG *pstsclRqstMsg)
{
	struct ISP_BE_BCM *pSclBcmBuff =  &(pstsclRqstMsg->bcmBuf);

	ispSS_SHM_CleanCache(pSclBcmBuff->shmHandle[0].hShm, 0, sizeof(struct BCMBUF));
	ispSS_SHM_CleanCache(pSclBcmBuff->shmHandle[1].hShm, 0, sizeof(struct BCMBUF));
	ispSS_SHM_CleanCache(pSclBcmBuff->shmHandle[2].hShm, 0, sizeof(struct DHUB_CFGQ));
	ispSS_SHM_CleanCache(pSclBcmBuff->shmHandle[3].hShm, 0, sizeof(struct DHUB_CFGQ));
}

/*******************************************************************
 * FUNCTION: Submit the request message to hardware
 *           If WaitforIntr is set then this function will pole
 *           for H/W semaphore to be set.
 * PARAMS:   pstsclRqstMsg - Request message
 sclObj - DNSCL object
 * RETURN:   ISPSS_OK - succeed
 ********************************************************************/
static INT ISPSS_BE_SCL_SubmitToHardWare(struct ISP_BE_RQST_MSG *pstsclRqstMsg,
		struct ISP_BE_SCL_OBJ *sclObj)
{
	HRESULT Ret = ISPSS_OK;
	struct ISP_BE_BCM *pSclBcmBuff =  &(pstsclRqstMsg->bcmBuf);

	ISPSS_SCLDBG("Submitting BCM Buffer to HardWare\n");
	/* Clear Cache for BCM buffers */
	ISPSS_BE_SCL_BCM_ClearCache(pstsclRqstMsg);
	Ret = ISPSS_BE_SCL_SubmitHW(pSclBcmBuff, g_isp_be_scl->commit_QId);
	/*Application Wait for the EOF interrupt Status*/
	if (pstsclRqstMsg->WaitforIntr) {
		int timeoutCount = ISPSS_BE_SCL_BLOCKING_ISR_TIMEOUT_IN_100MS;

		while (timeoutCount) {
			Ret = ISPSS_BE_SCL_FrameInterrupt(pstsclRqstMsg);
			if (ISPSS_BE_SCL_FrameInterrupt(pstsclRqstMsg)) {
				pstsclRqstMsg->FrameState = 1;
				goto exit;
			} else
				//wait for 100ms before polling again
				mdelay(100);

			timeoutCount--;
		}
	}
exit:
	return Ret;
}

/*******************************************************************
 * FUNCTION: Copy BCM buffer from BCMQ
 * PARAMS: pBcmBuf - BCM buffer
 *         pBcmBufQ - BCM Queue
 * RETURN: ISPSS_SCL_OK - succeed
 *
 ********************************************************************/
static void ISPSS_BE_SCL_CopyFromBcmBufQ(struct ISP_BE_BCM *pBcmBuf,
		struct ISPSS_BCMBUF_ITEM *pBcmBufQ)
{
	int i;

	pBcmBuf->bcm_buf = &pBcmBufQ->pBcmBuf[ISPSS_BCMBUF_ITEM_PROGRAM_BCM_NDX];
	pBcmBuf->clear_bcm_buf =  &pBcmBufQ->pBcmBuf[ISPSS_BCMBUF_ITEM_CLEAR_BCM_NDX];
	pBcmBuf->dma_cfgQ = &pBcmBufQ->pCfgQ[ISPSS_CFGQ_ITEM_DHUB_CFGQ_NDX];
	pBcmBuf->final_bcm_cfgQ =  &pBcmBufQ->pCfgQ[ISPSS_CFGQ_ITEM_FINAL_CGQ_NDX];

	pBcmBuf->shmHandle[0].hShm =
		pBcmBufQ->pBcmBuf[ISPSS_BCMBUF_ITEM_CLEAR_BCM_NDX].handle;
	pBcmBuf->shmHandle[1].hShm =
		pBcmBufQ->pBcmBuf[ISPSS_BCMBUF_ITEM_PROGRAM_BCM_NDX].handle;
	pBcmBuf->shmHandle[2].hShm =
		pBcmBufQ->pCfgQ[ISPSS_CFGQ_ITEM_DHUB_CFGQ_NDX].handle;
	pBcmBuf->shmHandle[3].hShm =
		pBcmBufQ->pCfgQ[ISPSS_CFGQ_ITEM_FINAL_CGQ_NDX].handle;
	for (i = 0; i < 4; i++) {
		ispSS_SHM_GetVirtualAddress(pBcmBuf->shmHandle[i].hShm, 0,
				&(pBcmBuf->shmHandle[i].addr));
		ispSS_SHM_GetPhysicalAddress(pBcmBuf->shmHandle[i].hShm, 0,
				&(pBcmBuf->shmHandle[i].phy_addr));
	}
}

/*********************************************************************
 * FUNCTION: Prepare frame for scalar processing
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
static INT ISPSS_BE_SCL_PrepareRequest(struct ISP_BE_RQST_MSG *psclRqstMsg,
		struct ISP_BE_SCL_OBJ *sclObj)
{
	INT Ret = ISPSS_OK;
	/* Select sub register programming buffer */
	if ((!psclRqstMsg->bcm_prepared)) {
		ISPSS_BCMBUF_QUEUE_Element_Select(&g_isp_be_scl->bcmbufQ,
			psclRqstMsg->pCurrBcmBuf, CPCB_1);
		ISPSS_BE_SCL_CopyFromBcmBufQ(psclRqstMsg->pBcmBuf, psclRqstMsg->pCurrBcmBuf);
		if (psclRqstMsg->out_mtrMode)
			ISPSS_BE_MTR_Set_Global_Registers_bcm(psclRqstMsg->bcmBuf.bcm_buf,
				ISPSS_BE_DNSCL_WR);

		if (psclRqstMsg->in_mtrMode)
			ISPSS_BE_MTR_Set_Global_Registers_bcm(psclRqstMsg->bcmBuf.bcm_buf,
				ISPSS_BE_DNSCL_RD);

		ISPSS_BE_MTR_Set_Dhub_Channel_Scaling(psclRqstMsg->bcmBuf.bcm_buf,
				psclRqstMsg->in_mtrMode, psclRqstMsg->out_mtrMode);

		ISPSS_BE_SCL_CONFIG(psclRqstMsg->m_BuffID, psclRqstMsg->bcmBuf.bcm_buf);
		/*Start frame for SCL processing*/
		Ret = ISPSS_BE_SCL_Start(psclRqstMsg, sclObj, g_hwObjst);
		/*Prepare and Generate Final BCM CFQ before enabling bcm prepared */
		ISPSS_BCMBUF_To_CFGQ(psclRqstMsg->bcmBuf.clear_bcm_buf,
			psclRqstMsg->bcmBuf.final_bcm_cfgQ);
		ISPSS_BCMBUF_To_CFGQ(psclRqstMsg->bcmBuf.bcm_buf,
			psclRqstMsg->bcmBuf.final_bcm_cfgQ);
		ISPSS_CFGQ_To_CFGQ(psclRqstMsg->bcmBuf.dma_cfgQ,
			psclRqstMsg->bcmBuf.final_bcm_cfgQ);
		psclRqstMsg->FrameState = 0; //Actual H/W processing not done yet
		psclRqstMsg->bcm_prepared = 1;  //BCM preparation done
	}
	return Ret;
}

/*********************************************************************
 * FUNCTION: Push a frame for scalar processing
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
static INT ISPSS_BE_SCL_PushRequest(INT clientID, struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	HRESULT Ret = ISPSS_OK;
	struct ISP_BE_SCL_OBJ *isp_scl_obj = g_isp_be_scl->isp_be_scl_obj[psclRqstMsg->m_BuffID];

	ISPSS_DEBUG_LOG_SCL("Entry\n");

	/*Print Filled Request Structure */
	printMsg(psclRqstMsg);

	/*Validate All the parameters are correct*/
	Ret = ISPSS_BE_SCL_ValidateRqstMsg(psclRqstMsg);
	if (Ret != ISPSS_OK) {
		BE_SCL_LOGE("ERRNUM:%d ,%s ,%d\n", Ret, __func__, __LINE__);
		Ret = ISPSS_EBADPARAM;
		goto e_SCLPushRequest;
	}
	Ret = fillObjectDetails(psclRqstMsg, isp_scl_obj);

	psclRqstMsg->pBcmBuf = &(psclRqstMsg->bcmBuf); //Else Copy from pCurrBcmBuf fails

	psclRqstMsg->bcm_prepared = 0;
	psclRqstMsg->pCurrBcmBuf = NULL;
	if (ISPSS_BCMBUF_QUEUE_pop_and_commit(&g_isp_be_scl->bcmbufQ,
			(void **)&psclRqstMsg->pCurrBcmBuf)) {
		ISPSS_SCLDBG_BCMQ("%s:%d, BCMBUFITEM %p allocated Successfully\n",
			__func__, __LINE__, psclRqstMsg->pCurrBcmBuf);
	} else {
		ISPSS_SCLDBG_BCMQ("%s:%d, BCMBUFITEM allocation unsuccessful\n",
			__func__, __LINE__);
		Ret = ISPSS_EBCMBUFFULL;
		goto e_SCLPushRequest;
	}

	/* 2. If BCM is already prepread don't prepare it again
	 * else prepare request for new/changed request
	 */
	if ((!psclRqstMsg->bcm_prepared))
		ISPSS_BE_SCL_PrepareRequest(psclRqstMsg, isp_scl_obj);

	/* 3. If H/W is still processing then push the frame into
	 * client queue and return immediately
	 */
	if (psclRqstMsg->SubmitHw) {
		mutex_lock(&g_isp_be_scl->isr_lock);
		Ret = ISPSS_BE_SCL_SubmitToHardWare(psclRqstMsg, isp_scl_obj);
		if (Ret == ISPSS_OK) {
			g_isp_be_scl->m_sclHwStatus = ISPSS_BE_SCL_HW_STATUS_RUNNING;
			if (isp_rqstq_push(&g_isp_be_scl->m_sclOutFrameQ, psclRqstMsg)) {
				isp_scl_obj->m_queue.OutputQCnt++;
			} else {
				BE_SCL_LOGE("%s: Failed to push into Queue\n", __func__);
				Ret = ISPSS_EOUTQFULL;
			}
			ISPSS_BCMBUF_QUEUE_push(&g_isp_be_scl->bcmbufQ, psclRqstMsg->pCurrBcmBuf);
		} else {
			BE_SCL_LOGE("%s: Failed to submit request to BCM\n", __func__);
		}
		mutex_unlock(&g_isp_be_scl->isr_lock);
	}
e_SCLPushRequest:
	ISPSS_DEBUG_LOG_SCL("Exit\n");

	return Ret;
}

/*********************************************************************
 * FUNCTION: Process ISR request
 * PARAMS:
 * RETURN:   ISPSS_OK
 ********************************************************************/
static INT ISPSS_BE_SCL_ProcessISR(UINT32 intrNum, void *arg)
{
	ISPSS_DEBUG_LOG_SCL("Entry\n");

	if (g_isp_be_scl->IntrHandler)
		(*g_isp_be_scl->IntrHandler)(g_isp_be_scl->IntCnt++,
			g_isp_be_scl->IntrHandlerArgs);

	ISPSS_DEBUG_LOG_SCL("Exit\n");

	return ISPSS_OK;
}

/*********************************************************************
 * FUNCTION: Pop a frame for scalar processing
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
static INT ISPSS_BE_SCL_PopRequest(struct ISP_BE_RQST_MSG **psclRqstMsg)
{
	//Pop Request from Queue
	struct ISP_BE_RQST_MSG *rqstMsg = NULL;
	struct ISP_BE_SCL_OBJ *isp_scl_obj = NULL;
	int ret = ISPSS_OK;


	mutex_lock(&g_isp_be_scl->isr_lock);
	if (g_isp_be_scl->m_sclHwStatus == ISPSS_BE_SCL_HW_STATUS_IDLE)
		goto err;
	if (isp_rqstq_pop(&(g_isp_be_scl->m_sclOutFrameQ), (void **)&rqstMsg)) {
		isp_rqstq_pop_commit(&(g_isp_be_scl->m_sclOutFrameQ));
		isp_scl_obj = GET_SCL_OBJ(rqstMsg->m_BuffID);
		isp_scl_obj->m_queue.OutputQCnt--;
		g_isp_be_scl->m_sclHwStatus = ISPSS_BE_SCL_HW_STATUS_IDLE;
	} else {
		BE_SCL_LOGE("%s:No frames in out queue\n", __func__);
		ret = ISPSS_ENOMEM;
		goto err;
	}
	if (!psclRqstMsg) {
		BE_SCL_LOGE("%s:Invalid parameter\n", __func__);
		ret = ISPSS_EBADPARAM;
		goto err;
	}
err:
	*psclRqstMsg = rqstMsg;
	mutex_unlock(&g_isp_be_scl->isr_lock);
	return ret;
}

/***********************************************
 * FUNCTION: Reset Down scalar
 *
 * PARAMS: void
 * RETURN: ISPSS_DWP_OK - succeed
 ***********************************************/
INT ISPSS_BE_SCL_Reset(struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	UINT32 RegAddr = 0;
	T32CLKRST_FuncReset funcReset;

	RegAddr = ISPMISC_BASE + RA_IspMISC_clkrst + RA_CLKRST_FuncReset;
	/* DNSCL func reset */
	ISPSS_REG_READ32(RegAddr, &funcReset);
	funcReset.uFuncReset_sclReset = 0x1;
	ISPSS_BCMBUF_Write(psclRqstMsg->bcmBuf.clear_bcm_buf, RegAddr, funcReset.u32);
	funcReset.uFuncReset_sclReset = 0x0;
	ISPSS_BCMBUF_Write(psclRqstMsg->bcmBuf.clear_bcm_buf, RegAddr, funcReset.u32);
	return ISPSS_OK;
}

/*********************************************************
 * FUNCTION: Ge the number of frame waiting in the input Q
 * PARAMS: iClientId - Client ID
 *         puiFramesWaiting - No of frames waiting in input Q
 * RETURN: ISPSS_OK - succeed
 *         ISPSS_EBADPARAM - Invalid client
 *
 ********************************************************/
INT ISPSS_BE_DNSCL_GetNoOfFramesWaiting(INT iClientId, UINT32 *puiFramesWaiting)
{
	HRESULT iResult = ISPSS_OK;

	if (iClientId >= 0 && iClientId < MAX_SCL_OBJECTS &&
		g_isp_be_scl->isp_be_scl_obj[iClientId]->m_iValid) {
		mutex_lock(&g_isp_be_scl->isr_lock);
		*puiFramesWaiting = g_isp_be_scl->isp_be_scl_obj[iClientId]->m_queue.InputQCnt;
		mutex_unlock(&g_isp_be_scl->isr_lock);
	} else {
		iResult = ISPSS_EBADPARAM;
	}
	return iResult;
}

/*********************************************************
 * FUNCTION: Get the number of frame waiting in the queues
 * PARAMS: frameWaitCnt - wait count struct
 * RETURN: ISPSS_OK - succeed
 *         ISPSS_EBADPARAM - Invalid client
 *
 ********************************************************/
INT ISPSS_BE_DNSCL_GetNoOfFramesWaiting_ClientQ(struct ISPBE_GET_FRAMEQ_WAIT_CNT *frameWaitCnt)
{
	HRESULT iResult = ISPSS_OK;
	INT index;
	INT iClientId = frameWaitCnt->iClientId;

	if (frameWaitCnt ==  NULL) {
		BE_SCL_LOGE("%s:invalid parameter\n", __func__);
		iResult = ISPSS_EBADPARAM;
		goto frame_cnt_ret;
	}
	frameWaitCnt->noOfFramesWaitingInputQ = 0;
	frameWaitCnt->noOfFramesWaitingProcessQ = 0;
	frameWaitCnt->noOfFramesWaitingOutputQ = 0;

	mutex_lock(&g_isp_be_scl->isr_lock);

	if (iClientId >= 0 && iClientId < MAX_SCL_OBJECTS) {
		if (g_isp_be_scl->isp_be_scl_obj[iClientId]->m_iValid) {
			frameWaitCnt->noOfFramesWaitingInputQ =
				g_isp_be_scl->isp_be_scl_obj[iClientId]->m_queue.InputQCnt;
			frameWaitCnt->noOfFramesWaitingProcessQ =
				g_isp_be_scl->isp_be_scl_obj[iClientId]->m_queue.ProcessQCnt;
			frameWaitCnt->noOfFramesWaitingOutputQ =
				g_isp_be_scl->isp_be_scl_obj[iClientId]->m_queue.OutputQCnt;
		} else {
			iResult = ISPSS_EBADPARAM;
		}
	} else {
		/* clientID is invlaid get sum of all client count */
		for (index = 0; index < MAX_SCL_OBJECTS; index++) {
			if (g_isp_be_scl->isp_be_scl_obj[index]->m_iValid == 1) {
				frameWaitCnt->noOfFramesWaitingInputQ +=
					g_isp_be_scl->isp_be_scl_obj[index]->m_queue.InputQCnt;
				frameWaitCnt->noOfFramesWaitingProcessQ +=
					g_isp_be_scl->isp_be_scl_obj[index]->m_queue.ProcessQCnt;
				frameWaitCnt->noOfFramesWaitingOutputQ +=
					g_isp_be_scl->isp_be_scl_obj[index]->m_queue.OutputQCnt;
			}
		}
	}

	mutex_unlock(&g_isp_be_scl->isr_lock);

frame_cnt_ret:
	return iResult;
}

void ISPSS_BE_DNSCL3_Probe(struct ISPBE_CA_DRV_CTX *drv_ctx)
{
	drv_ctx->fops.module_init = ISPSS_BE_SCL_Init;
	drv_ctx->fops.module_destroy = ISPSS_BE_SCL_Destroy;
	drv_ctx->fops.module_open = ISPSS_BE_SCL_Open;
	drv_ctx->fops.module_close = ISPSS_BE_SCL_Close;
	drv_ctx->fops.module_ProcessISR = ISPSS_BE_SCL_ProcessISR;
	drv_ctx->fops.module_PushRequest = ISPSS_BE_SCL_PushRequest;
	drv_ctx->fops.module_PopRequest = ISPSS_BE_SCL_PopRequest;
	drv_ctx->fops.module_GetNoOfFramesWaiting = ISPSS_BE_DNSCL_GetNoOfFramesWaiting;
	drv_ctx->sem_id = ISP_DHUBSEM_TSB_Scldn_intr1;
}
EXPORT_SYMBOL(ISPSS_BE_DNSCL3_Probe);
