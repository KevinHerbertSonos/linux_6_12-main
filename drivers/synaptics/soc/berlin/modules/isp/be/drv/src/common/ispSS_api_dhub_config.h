/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ISPSS_API_DHUB_CONFIG_H__
#define __ISPSS_API_DHUB_CONFIG_H__

#include "ispSS_common.h"
#include "ispSS_memmap.h"
#include "ispDhub.h"
#include "ispSS_api_dhub.h"
#include "ispSS_dhub_drv.h"
#include "ispSS_reg.h"
#include "ispSS_bcmbuf.h"
#include "global.h"
#include "ispSS_api_dhub.h"

#define ISPSS_TSB_DHUB_BASE          (MEMMAP_ISP_REG_BASE + \
		ISPSS_MEMMAP_TSB_DHUB_REG_BASE + RA_ispDhubTSB_dHub0)
#define ISPSS_TSB_HBO_SRAM_BASE      (MEMMAP_ISP_REG_BASE + \
		ISPSS_MEMMAP_TSB_DHUB_REG_BASE + RA_ispDhubTSB_tcm0)
#define ISPSS_TSB_NUM_OF_CHANNELS    (ispDhubChMap_TSB_BCM_R1+1)

#define ISPSS_TSB_DHUB_BANK0_START_ADDR     ispDhubTcmMap_ispDhubTSB_BANK0_START_ADDR

#define ISPSS_FWR_DHUB_BASE          (MEMMAP_ISP_REG_BASE + \
		ISPSS_MEMMAP_FWR_DHUB_REG_BASE + RA_ispDhubFWR_dHub0)
#define ISPSS_FWR_HBO_SRAM_BASE      (MEMMAP_ISP_REG_BASE + \
		ISPSS_MEMMAP_FWR_DHUB_REG_BASE + RA_ispDhubFWR_tcm0)
#define ISPSS_FWR_NUM_OF_CHANNELS    (ispDhubChMap_FWR_Rot_W1+1)

#define ISPSS_FWR_DHUB_BANK0_START_ADDR     ispDhubTcmMap_ispDhubFWR_BANK0_START_ADDR

enum ISPSS_CHIP_GLOBAL_RESET_CTRL {
	ISPSS_CHIP_GLOBAL_RESET_CTRL_DISABLE = 0,
	ISPSS_CHIP_GLOBAL_RESET_CTRL_ENABLE,
};

extern struct HDL_dhub2d ISPSS_TSB_dhubHandle;
extern struct DHUB_channel_config  ISPSS_TSB_config[];
extern struct HDL_dhub2d ISPSS_FWR_dhubHandle;
extern struct DHUB_channel_config  ISPSS_FWR_config[];

/****************************************************************************************
 *    Function: DhubInitialization
 *    Description: Initialize DHUB .
 *    Parameter : cpuId ------------- cpu ID
 *             dHubBaseAddr -------------  dHub Base address.
 *             hboSramAddr ----- Sram Address for HBO.
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             dhub_config ----- configuration of AG
 *             numOfChans     ----- number of channels
 *    Return:        void
 ****************************************************************************************
 */
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr,
		struct HDL_dhub2d *pdhubHandle, struct DHUB_channel_config *dhub_config,
		SIGN32 numOfChans, enum DHUB_TYPE dHubType);

void ISPSS_DhubInit(SIGN32 cpuId);

#endif //__ISPSS_API_DHUB_CONFIG_H__
