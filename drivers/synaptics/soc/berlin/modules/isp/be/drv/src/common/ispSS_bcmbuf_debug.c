// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#include "ispSS_errorno.h"
#include "ispbe_err.h"
#include "ispSS_bcmbuf.h"
#include "ispSS_reg.h"
#include "ispSS_bcmbuf_debug.h"
#include "com_type.h"
#include "ispDhub.h"
#include "ispSS_api_dhub.h"
#include "ispSS_shm.h"
//#include "ispSS_com.h"
#include "ispSS_common.h"
#include "ispSS_dhub_drv.h"

INT ISPSS_BCMBUF_Raw_DirectWrite(UINT32 *pdata, UINT32 length)
{
	int i;
	int addr, val;
	UINT32 *regValPair = pdata;

	if ((!pdata) || (length < 8))
		return ISPSS_EBADPARAM;

	for (i = 0; i < length; i += 8) {
		val  = *regValPair++;
		addr = *regValPair++;
		ISPSS_REG_WRITE32(addr, val);
	}

	return ISPSS_OK;
}

INT ISPSS_BCMBUF_Raw_LogPrint(UINT64 *pdata, UINT32 length)
{
	int i;
	UINT64 addr, val;
	UINT64 *regValPair = pdata;

	if ((!pdata) || (length < 8))
		return ISPSS_EBADPARAM;

	for (i = 0; i < length; i += 8) {
		addr  = (*regValPair & 0xFFFFFFFF00000000) >> 32;
		val = (*regValPair & 0xFFFFFFFF);
		regValPair++;
		pr_err("0x%08X = 0x%08X\n", (unsigned int)addr, (unsigned int)val);
	}

	return ISPSS_OK;
}

INT ISPSS_BCMBUF_To_Raw(struct BCMBUF *pbcmbuf, UINT64 **start, INT32 *size)
{
	if (!pbcmbuf || !start || !size)
		return ISPSS_EBADPARAM;

	if (pbcmbuf->subID == CPCB_1)
		*start = pbcmbuf->dv1_head;
	else
		*start = pbcmbuf->head;

	*size = (UINT64)pbcmbuf->writer - (UINT64)*start;
	pr_err("BCMBUF(%p) => start : %p, size:%d\n", pbcmbuf, *start, *size);
	return ISPSS_OK;
}

INT ISPSS_BCMBUF_DirectWrite(struct BCMBUF *pbcmbuf)
{
	UINT32 *start;
	INT32 size;
	INT retVal = ISPSS_BCMBUF_To_Raw(pbcmbuf, (UINT64 **) &start, &size);

	if (retVal == ISPSS_OK)
		retVal = ISPSS_BCMBUF_Raw_DirectWrite(start, size);

	return retVal;
}

INT ISPSS_BCMBUF_LogPrint(struct BCMBUF *pbcmbuf)
{
	UINT64 *start;
	INT32 size;
	INT retVal = ISPSS_BCMBUF_To_Raw(pbcmbuf, &start, &size);

	if (retVal == ISPSS_OK)
		retVal = ISPSS_BCMBUF_Raw_LogPrint(start, size);

	return retVal;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_LogPrint);

INT ISPSS_CFGQ_DirectWrite(struct DHUB_CFGQ *pCfgQ)
{
	return ISPSS_BCMBUF_Raw_DirectWrite((UINT32 *)pCfgQ->addr, pCfgQ->len*8);
}

INT ISPSS_CFGQ_LogPrint(struct DHUB_CFGQ *pCfgQ)
{
	pr_info("CFGQ (%p) => start : %p, size:%d\n", pCfgQ, pCfgQ->addr, pCfgQ->len*8);

	return ISPSS_BCMBUF_Raw_LogPrint(pCfgQ->addr, pCfgQ->len*8);
}
EXPORT_SYMBOL_GPL(ISPSS_CFGQ_LogPrint);

static void ISPSS_BCMBUF_DumpDhubConfig_Func(struct ISPSS_DHUB_CONTEXT_INFO *pDhubCtx)
{
	UNSG32 dHubBaseAddr;
	UNSG32 hboSramAddr;
	struct HDL_dhub2d *pdhubHandle;
	struct DHUB_channel_config *dhub_config;
	SIGN32 numOfChans;
	enum DHUB_TYPE dHubType;
	char *dhubName;
	char **chanName;
	char *notDefinedStr = "-NA-";
	SIGN32 i;
	SIGN32 chanId;

	if (!pDhubCtx)
		return;

	dHubBaseAddr = pDhubCtx->dHubBaseAddr;
	hboSramAddr = pDhubCtx->hboSramAddr;
	pdhubHandle = pDhubCtx->pDhubHandle;
	dhub_config = pDhubCtx->pDhubConfig;
	numOfChans = pDhubCtx->numOfChans;
	dHubType = pDhubCtx->dHubType;
	dhubName = pDhubCtx->dhubName;
	chanName = pDhubCtx->chanName;

	pr_debug("DHUB: %s, hboSramAddr : %08X, dHubBaseAddr : %08X\n",
		dhubName ? dhubName : notDefinedStr, hboSramAddr, dHubBaseAddr);
	pr_debug("%18s) \t: chanId CmdBase DataBase CmdSz DataSz MtuSz Qos SelfLoop Enable\n",
			"Sl#(ChannelName        ");

	for (i = 0; i < numOfChans; i++) {
		chanId = dhub_config[i].chanId;
		pr_debug("%2d (%-18s) \t: %8X %8X %8X %8X %8X %8X %4X %8d %6d\n",
				i, chanName ? chanName[chanId] : notDefinedStr,
				dhub_config[i].chanId, dhub_config[i].chanCmdBase,
				dhub_config[i].chanDataBase, dhub_config[i].chanCmdSize,
				dhub_config[i].chanDataSize, dhub_config[i].chanMtuSize,
				dhub_config[i].chanQos, dhub_config[i].chanSelfLoop,
				dhub_config[i].chanEnable);
	}
}

void ISPSS_BCMBUF_DhubConfig_LogPrint(void)
{
	int ndx = 0;

	while (1) {
		struct ISPSS_DHUB_CONTEXT_INFO *pDhubCtx = ISPSS_GetDhubContextInfo(ndx);

		pr_debug("%s:%d: ndx:%d, pDhubCtx:%lx\n", __func__, __LINE__,
				ndx, (unsigned long) pDhubCtx);
		if (pDhubCtx) {
			ISPSS_BCMBUF_DumpDhubConfig_Func(pDhubCtx);
			ndx++;
		} else {
			break;
		}
	}
}

