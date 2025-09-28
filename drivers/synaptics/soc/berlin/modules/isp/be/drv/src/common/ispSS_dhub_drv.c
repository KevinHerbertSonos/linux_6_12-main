// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispSS_api_dhub_config.h"
#include "ispbe_err.h"

#define ISP_STRIDE_ALIGNMNET (256)
#define ISP_DHUB_BURST_MMU_ONLY (64)
#define ISP_IS_STRIDE_ALIGN_TO_BURST(stride)  (!((stride) & (ISP_STRIDE_ALIGNMNET-1)))
#define ISP_DHUB_BURST_BW_OPT       0
#define ISP_DHUB_MTU_OPT            dHubChannel_CFG_MTU_256byte
#define ISP_DHUB_MTU_MMU_ONLY       dHubChannel_CFG_MTU_256byte

// Helper Macros for MTR and MMU feature.
#define ISP_BITMASK_MTR  0x01
#define ISP_BITMASK_MMU  0x02
#define IS_MTR_ENABLED(x) ((x) & ISP_BITMASK_MTR)
#define IS_MMU_ENABLED(x) ((x) & ISP_BITMASK_MMU)
#define IS_MMU_ONLY_ENABLED(x) (!IS_MTR_ENABLED(x) &&  IS_MMU_ENABLED(x))

struct ISPSS_DHUB_CONTEXT g_ispssDhubContext;
static int g_ispssDhubCtxInitialized;

void ISPSS_InitDhubContext(UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr)
{
	if (!g_ispssDhubCtxInitialized) {
		g_ispssDhubContext.bcmDhubType = bcmDhubType;
		g_ispssDhubContext.bcmBaseAddr = bcmBaseAddr;
		g_ispssDhubContext.bcmDummyRegAddr = bcmDummyRegAddr;
		g_ispssDhubContext.dhubContextCount = 0;
		g_ispssDhubCtxInitialized = 1;
	}
}

static struct DHUB_channel_config *ISPSS_GetDhubConfig(struct HDL_dhub2d *pDhubHandle)
{
	int i;
	int nCtx = g_ispssDhubContext.dhubContextCount;
	struct DHUB_channel_config *pDhubConfigLocal = NULL;

	for (i = 0; i < nCtx; i++) {
		if (g_ispssDhubContext.contextInfo[i].pDhubHandle == pDhubHandle) {
			pDhubConfigLocal = g_ispssDhubContext.contextInfo[i].pDhubConfig;
			break;
		}
	}

	return pDhubConfigLocal;
}

UINT32 ISPSS_AddDhubContextInfo(struct ISPSS_DHUB_CONTEXT_INFO contextInfo)
{
	int nCtx = g_ispssDhubContext.dhubContextCount;
	struct DHUB_channel_config *pDhubConfigLocal =
		ISPSS_GetDhubConfig(contextInfo.pDhubHandle);

	if ((pDhubConfigLocal == NULL) && (nCtx < ISPSS_DHUB_CONTEXT_INFO_MAX)) {
		g_ispssDhubContext.contextInfo[nCtx] = contextInfo;
		pDhubConfigLocal = contextInfo.pDhubConfig;
		g_ispssDhubContext.dhubContextCount++;
	}

	return (pDhubConfigLocal != NULL);
}

struct ISPSS_DHUB_CONTEXT_INFO *ISPSS_GetDhubContextInfo(int nCtx)
{
	struct ISPSS_DHUB_CONTEXT_INFO *pCtxInfo = NULL;

	if (nCtx < g_ispssDhubContext.dhubContextCount)
		pCtxInfo = &g_ispssDhubContext.contextInfo[nCtx];

	return pCtxInfo;
}

static UNSG32 ISPSS_GetBcmDhubType(void)
{
	return g_ispssDhubContext.bcmDhubType;
}

static UNSG32 ISPSS_GetBcmBaseAddr(void)
{
	return g_ispssDhubContext.bcmBaseAddr;
}

UNSG32 ISPSS_GetBcmDummyRegAddr(void)
{
	return g_ispssDhubContext.bcmDummyRegAddr;
}

UNSG32 getBcmDhubType(void)
{
	return ISPSS_GetBcmDhubType();
}

UNSG32 getBcmBaseAddr(void)
{
	return ISPSS_GetBcmBaseAddr();
}

UNSG32 getBcmDummyRegAddr(void)
{
	return ISPSS_GetBcmDummyRegAddr();
}

/********************************************************************
 *      Function: GetChannelInfo
 *      Description: Get the Dhub configuration of requested channel.
 *      Parameter : pdhubHandle ----- pointer to 2D dhubHandle
 *             IChannel        ----- Channel of the dhub
 *             cfg        ----- Configuration need to be updated here.
 *      Return:  0     ----    Success
 *********************************************************************/
int getDhubChannelInfo(struct HDL_dhub2d *pdhubHandle,
		SIGN32 IChannel, T32dHubChannel_CFG *cfg)
{
	int retVal = 0;
	struct DHUB_channel_config *dhub_config = ISPSS_GetDhubConfig(pdhubHandle);
	//Get the Dhub Config array from the received handle
	if (dhub_config) {
		//Update the MTU, QOS and self loop paramteres.
		cfg->uCFG_MTU = dhub_config[IChannel].chanMtuSize;
		cfg->uCFG_QoS =  dhub_config[IChannel].chanQos;
		cfg->uCFG_selfLoop = dhub_config[IChannel].chanSelfLoop;
	} else {
		retVal = 1;
	}

	return retVal;
}

/*********************************************************************************
 * Function: DhubChannelClear
 * Description: Clear corresponding DHUB channel.
 * Parameter:    hdl - handle to HDL_dhub
 * id - channel ID in dHubReg
 * cfgQ - pass null to directly init dhub, or pass non-zero to receive programming1
 * sequence in (adr, data) pairs
 * Return: void
 *********************************************************************************/
void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[])
{
	UNSG32    cmdID = dhub_id2hbo_cmdQ(id);
	UNSG32    dataID = dhub_id2hbo_data(id);
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	struct HDL_hbo *hbo = &(dhub->hbo);

	/* 1.Software stops the command queue in HBO (please refer to HBO.sxw for details) */
	hbo_queue_enable(hbo, cmdID, 0, cfgQ);
	/* 2.Software stops the channel in dHub by writing zero to  dHubChannel.START.EN */
	dhub_channel_enable(dhub, id, 0, cfgQ);
	/* 3.Software clears the channel in dHub by writing one to  dHubChannel.CLEAR.EN */
	dhub_channel_clear(dhub, id);
	/* 4.Software waits for the register bits
	 * dHubChannel.PENDING.ST and dHubChannel.BUSY.ST to be 0
	 */
	dhub_channel_clear_done(dhub, id);
	/* 5.Software stops and clears the command queue */
	hbo_queue_enable(hbo, cmdID, 0, cfgQ);
	hbo_queue_clear(hbo, cmdID);
	/* 6.Software wait for the corresponding busy bit to be 0 */
	hbo_queue_clear_done(hbo, cmdID);
	/* 7.Software stops and clears the data queue */
	hbo_queue_enable(hbo, dataID, 0, cfgQ);
	hbo_queue_clear(hbo, dataID);
	/* 8.Software wait for the corresponding data Q busy bit to be 0 */
	hbo_queue_clear_done(hbo, dataID);
	/* 9.Software enable dHub and HBO */
	dhub_channel_enable(dhub, id, 1, cfgQ);
	hbo_queue_enable(hbo, cmdID, 1, cfgQ);
	hbo_queue_enable(hbo, dataID, 1, cfgQ);
}

/* To set mtu size and inverse mode  for each a given channel */
static int ISPSS_SetDhubChannelCfg(struct ISPSS_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam,
		INT dmaId, SIGN32 mtuByte)
{
	struct HDL_dhub *dhub =
		(struct HDL_dhub *)&pstCfg2NDDMAParam->pdhubHandle->dhub;
	UINT32 RegAddr;
	T32dHubChannel_CFG  cfg;
	struct DHUB_channel_config *dhub_config;

	//Get the Dhub Config array from the received handle
	if (pstCfg2NDDMAParam->pdhubHandle == &ISPSS_TSB_dhubHandle)
		dhub_config = ISPSS_TSB_config;
	else
		dhub_config = ISPSS_FWR_config;

	switch (mtuByte) {
	case dHubChannel_CFG_MTU_8byte:
		dhub->MTUb[dmaId] = 3; break;
	case dHubChannel_CFG_MTU_32byte:
		dhub->MTUb[dmaId] = 5; break;
	case dHubChannel_CFG_MTU_128byte:
		dhub->MTUb[dmaId] = 7; break;
	case dHubChannel_CFG_MTU_4096byte:
		dhub->MTUb[dmaId] = 12; break;
	}

	cfg.u32 = 0;
	cfg.uCFG_MTU = mtuByte;
	cfg.uCFG_QoS = dhub_config[dmaId].chanQos;
	cfg.uCFG_selfLoop = dhub_config[dmaId].chanSelfLoop;

	switch (pstCfg2NDDMAParam->chan_param.uchInverseScanMode) {
	case 0:
		//Normal Scanning
		cfg.uCFG_hScan = 0; cfg.uCFG_vScan = 0;
		break;
	case 1:
		//Only H inverse scan
		cfg.uCFG_hScan = 1;
		cfg.uCFG_vScan = 0;
		break;
	case 2:
		//Only V inverse Scan
		cfg.uCFG_hScan = 0;
		cfg.uCFG_vScan = 1;
		break;
	case 3:
		//Both HV inverse
		cfg.uCFG_hScan = 1; cfg.uCFG_vScan = 1;
		break;
	default:
		return ISPSS_EBADPARAM;
	}
	RegAddr = dhub->ra + RA_dHubReg_ARR + dmaId*sizeof(SIE_dHubChannel);
	ISPSS_BCMBUF_Write(pstCfg2NDDMAParam->pBcmBuf, RegAddr, cfg.u32);
	dhub_config[dmaId].chanMtuSize = mtuByte;

	return ISPSS_OK;
}

/* calculate 2NDDMA params to support ISPSS_DBUB_BURST_OPT */
UNSG32 ISPSS_DhubCfg2NDDMA(struct ISPSS_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam)
{
	SIGN32 burst, step1, size1, step2, size2;
	SIGN32 mtu;
	UNSG32 RetVal;
	UNSG32 addr;
	UINT32 *CfgQ;

	addr  = pstCfg2NDDMAParam->addr;

	if ((IS_MMU_ENABLED(pstCfg2NDDMAParam->mtrREn) ||
				(IS_MTR_ENABLED(pstCfg2NDDMAParam->mtrREn))) &&
			!ISP_IS_STRIDE_ALIGN_TO_BURST(pstCfg2NDDMAParam->stride))
		return 1;

	if (IS_MTR_ENABLED(pstCfg2NDDMAParam->mtrREn)) {
		burst = (ISP_DHUB_BURST_BW_OPT > 0)
			? ISP_DHUB_BURST_BW_OPT : pstCfg2NDDMAParam->width;
		mtu   = ISP_DHUB_MTU_OPT;
		step2 = 262144;
	} else if (IS_MMU_ONLY_ENABLED(pstCfg2NDDMAParam->mtrREn)) {
		burst = ISP_DHUB_BURST_MMU_ONLY;
		mtu   = ISP_DHUB_MTU_MMU_ONLY;
		step2 = 262144;
	} else {
		/* Overlayed horizontal line on top of video is seen when MTU > 128 and
		 * burst is not multiple of MTU
		 */
		burst = pstCfg2NDDMAParam->width;
		mtu   = dHubChannel_CFG_MTU_128byte;
		addr  = pstCfg2NDDMAParam->addr;
		step2 = pstCfg2NDDMAParam->stride;
	}
	step1 = burst;
	size1 = pstCfg2NDDMAParam->width / burst;
	size2 = pstCfg2NDDMAParam->height;
	ISPSS_SetDhubChannelCfg(pstCfg2NDDMAParam, pstCfg2NDDMAParam->dmaId, mtu);
	CfgQ = pstCfg2NDDMAParam->cfgQ;

	if (pstCfg2NDDMAParam->dmaId == pstCfg2NDDMAParam->chan_param.channelDmaId[0])
		pstCfg2NDDMAParam->chan_param.last_frame_addrY =  (UINT64)(CfgQ + 2);
	if (pstCfg2NDDMAParam->dmaId == pstCfg2NDDMAParam->chan_param.channelDmaId[1])
		pstCfg2NDDMAParam->chan_param.last_frame_addrUV = (UINT64) (CfgQ + 2);
	else
		pstCfg2NDDMAParam->chan_param.last_frame_addrUV = 0;

	if (pstCfg2NDDMAParam->mtrREn == 0)
		RetVal = START_2NDDMA(pstCfg2NDDMAParam->dhubId,
				pstCfg2NDDMAParam->dmaId, addr, burst,
				step1, size1, step2, size2, (T64b *)pstCfg2NDDMAParam->cfgQ);
	else
		RetVal =  START_2NDDMA_MTRR(pstCfg2NDDMAParam->dhubId,
				pstCfg2NDDMAParam->dmaId, addr,
				burst, step1, size1, step2, size2, pstCfg2NDDMAParam->chkSemId,
				pstCfg2NDDMAParam->updSemId, (T64b *)pstCfg2NDDMAParam->cfgQ);

	return RetVal;
}
EXPORT_SYMBOL(ISPSS_DhubCfg2NDDMA);
