/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ISPSS_DHUB_DRV_H__
#define __ISPSS_DHUB_DRV_H__

#include "ispSS_common.h"
#include "ispSS_memmap.h"
#include "ispDhub.h"
#include "ispSS_api_dhub.h"
#include "ispSS_reg.h"
#include "ispSS_bcmbuf.h"

#define ISPSS_DBUB_BURST_OPT (256)

#define ISPSS_DHUB_CONTEXT_INFO_MAX  10
#define ISPSS_DHUB_INTR_HANDLER_MAX  32

typedef int (*ISPSS_DHUB_INTR_HANDLER)(UNSG32 intrNum, void *pArgs);

enum ISPSS_DHUB_128bBCM_TYPE {
	ISPSS_DHUB_128bBCM_TYPE_64b,
	ISPSS_DHUB_128bBCM_TYPE_128b,
};

struct ISPSS_DHUB_CFG_2NDDMA_CHANNEL_PARAM {
	//path/channel related information
	SIGN32 uchInverseScanMode;
	//Y & UV channel ID - R0, R1
	int channelDmaId[2];
	int m_mtrR_mode;

	/* Output params */
	//BCM buffer address at which Y & UV start address is stored. Used for early push
	UINT64 last_frame_addrY;
	UINT64 last_frame_addrUV;

	//MTR/Virtual stride computed for given i/p DMA stride
	int m_mtrR_updSemId_Y;
	int m_mtrR_updSemId_UV;
};

struct ISPSS_DHUB_CFG_2NDDMA_FRAME_PARAM {
	//frame buffer related information
	int m_is_compressed;

	//input DMA stride
	int m_buf_stride;
	int m_buf_stride_UV;
};

struct ISPSS_DHUB_CFG_2NDDMA_PARAM {
	/* Input params */
	UINT64 dhubId;
	struct HDL_dhub2d *pdhubHandle;
	struct BCMBUF *pBcmBuf;
	int dmaId;
	UINT64 addr;
	SIGN32 width; /* Number of bytes in a line or tile line */
	SIGN32 stride; /* Number of bytes in a line or tile line */
	SIGN32 height; /* Number of lines or tile lines */
	int *cfgQ;
	int mtrREn;
	int updSemId;
	int chkSemId;

	struct ISPSS_DHUB_CFG_2NDDMA_CHANNEL_PARAM chan_param;
	struct ISPSS_DHUB_CFG_2NDDMA_FRAME_PARAM frame_param;
};

struct DHUB_channel_config {
	SIGN32 chanId;
	UNSG32 chanCmdBase;
	UNSG32 chanDataBase;
	SIGN32 chanCmdSize;
	SIGN32 chanDataSize;
	SIGN32 chanMtuSize;
	SIGN32 chanQos;
	SIGN32 chanSelfLoop;
	SIGN32 chanEnable;
	SIGN32 chanAxQosLO;
	SIGN32 chanAxQosHI;
};

enum DHUB_TYPE {
	DHUB_TYPE_128BIT = 0
};

struct ISPSS_DHUB_CONTEXT_INFO {
	UNSG32 dHubBaseAddr;
	UNSG32 hboSramAddr;
	SIGN32 numOfChans;
	enum DHUB_TYPE dHubType;
	struct HDL_dhub2d *pDhubHandle;
	struct DHUB_channel_config *pDhubConfig;
	int DhubChannelIntrCount[ISPSS_DHUB_INTR_HANDLER_MAX][4];
	ISPSS_DHUB_INTR_HANDLER pIntrHandler[ISPSS_DHUB_INTR_HANDLER_MAX];
	void *pIntrHandlerArgs[ISPSS_DHUB_INTR_HANDLER_MAX];
	char *dhubName;
	char **chanName;
	char **intrName;
};

struct ISPSS_DHUB_CONTEXT {
	struct ISPSS_DHUB_CONTEXT_INFO contextInfo[ISPSS_DHUB_CONTEXT_INFO_MAX];
	UNSG32 dhubContextCount;
	UNSG32 bcmDhubType; //0:64b, 1:128b(Add filler to MSB-64b as they are ignored)
	UNSG32 bcmBaseAddr; //Base address of the BCM register space
	UNSG32 bcmDummyRegAddr;  //Dummy register write to add for odd size BCM buffer
	//clear dhub interrupt after interrupt handling otherwise clear it before handling interrupt
	UNSG32 clearAfterIntHandlerFlag;
	UNSG32 debugLogReg;         //Register holding the debug information of multiple modules
	UNSG32 debugLogMod[32];       //debug information of each of the modules
};

extern struct ISPSS_DHUB_CONTEXT g_ispssDhubContext;

void ISPSS_InitDhubContext(UNSG32 bcmDhubType, UNSG32 bcmBaseAddr, UNSG32 bcmDummyRegAddr);
UINT32 ISPSS_AddDhubContextInfo(struct ISPSS_DHUB_CONTEXT_INFO contextInfo);
UNSG32 ISPSS_GetBcmDummyRegAddr(void);
struct ISPSS_DHUB_CONTEXT_INFO *ISPSS_GetDhubContextInfo(int nCtx);

void DhubChannelClear(void *hdl, SIGN32 id, T64b cfgQ[]);

UNSG32 getBcmDhubType(void);
UNSG32 getBcmBaseAddr(void);
UNSG32 getBcmDummyRegAddr(void);
int getDhubChannelInfo(struct HDL_dhub2d *pdhubHandle, SIGN32 IChannel, T32dHubChannel_CFG *cfg);
UNSG32 ISPSS_DhubCfg2NDDMA(struct ISPSS_DHUB_CFG_2NDDMA_PARAM *pstCfg2NDDMAParam);
#endif // __ISPSS_DHUB_DRV_H__
