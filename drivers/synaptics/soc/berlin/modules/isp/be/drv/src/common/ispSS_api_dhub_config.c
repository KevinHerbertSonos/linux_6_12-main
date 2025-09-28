// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispSS_api_dhub_config.h"

#define MEMMAP_ISPSS_BCM_REG_BASE   (MEMMAP_ISP_REG_BASE + ISPSS_MEMMAP_BCM_REG_BASE)
#define MEMMAP_ISPSS_DUMMY_REG_BASE (MEMMAP_ISP_REG_BASE + ISPSS_MEMMAP_GLB_REG_BASE \
		+ RA_IspMISC_interrupt + RA_Interrupt_BcmInvalidAddr)

struct HDL_dhub2d ISPSS_TSB_dhubHandle;
EXPORT_SYMBOL(ISPSS_TSB_dhubHandle);
struct HDL_dhub2d ISPSS_FWR_dhubHandle;
EXPORT_SYMBOL(ISPSS_FWR_dhubHandle);

#define ISPSS_CMDSZ_64  64
#define ISPSS_CMDSZ_128 128
#define ISPSS_CMDSZ_256 256

#define ISPSS_CONVERT_128BITS_TO_BYTES(n128b)       ((128/8)*n128b)

#define ispDhubChMap_TSB_Tile_W0_SIZE           (ISPSS_CONVERT_128BITS_TO_BYTES(225))
#define ispDhubChMap_TSB_Tile_W1_SIZE           (ISPSS_CONVERT_128BITS_TO_BYTES(150))
#define ispDhubChMap_TSB_Dnscl_R0_SIZE          (ISPSS_CONVERT_128BITS_TO_BYTES(60))
#define ispDhubChMap_TSB_Dnscl_R1_SIZE          (ISPSS_CONVERT_128BITS_TO_BYTES(60))
#define ispDhubChMap_TSB_Dnscl_W0_SIZE          (ISPSS_CONVERT_128BITS_TO_BYTES(90))
#define ispDhubChMap_TSB_Dnscl_W1_SIZE          (ISPSS_CONVERT_128BITS_TO_BYTES(32))
#define ispDhubChMap_TSB_BCM_R1_SIZE            (ISPSS_CONVERT_128BITS_TO_BYTES(32))

#define ispDhubChMap_TSB_Tile_W0_BASE   (ISPSS_TSB_DHUB_BANK0_START_ADDR)
#define ispDhubChMap_TSB_Tile_W1_BASE \
	(ispDhubChMap_TSB_Tile_W0_BASE  + ispDhubChMap_TSB_Tile_W0_SIZE)
#define ispDhubChMap_TSB_Dnscl_W0_BASE \
	(ispDhubChMap_TSB_Tile_W1_BASE  + ispDhubChMap_TSB_Tile_W1_SIZE)
#define ispDhubChMap_TSB_Dnscl_W1_BASE \
	(ispDhubChMap_TSB_Dnscl_W0_BASE + ispDhubChMap_TSB_Dnscl_W0_SIZE)
#define ispDhubChMap_TSB_Dnscl_R0_BASE \
	(ispDhubChMap_TSB_Dnscl_W1_BASE + ispDhubChMap_TSB_Dnscl_W1_SIZE)
#define ispDhubChMap_TSB_Dnscl_R1_BASE \
	(ispDhubChMap_TSB_Dnscl_R0_BASE + ispDhubChMap_TSB_Dnscl_R0_SIZE)
#define ispDhubChMap_TSB_BCM_R1_BASE \
	(ispDhubChMap_TSB_Dnscl_R1_BASE + ispDhubChMap_TSB_Dnscl_R1_SIZE)

struct DHUB_channel_config  ISPSS_TSB_config[ISPSS_TSB_NUM_OF_CHANNELS] = {
	{ ispDhubChMap_TSB_Tile_W0, ispDhubChMap_TSB_Tile_W0_BASE,
		ispDhubChMap_TSB_Tile_W0_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_TSB_Tile_W0_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0xF, 0xF},
	{ ispDhubChMap_TSB_Tile_W1, ispDhubChMap_TSB_Tile_W1_BASE,
		ispDhubChMap_TSB_Tile_W1_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_TSB_Tile_W1_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0xF, 0xF},
	{ ispDhubChMap_TSB_Dnscl_W0, ispDhubChMap_TSB_Dnscl_W0_BASE,
		ispDhubChMap_TSB_Dnscl_W0_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_TSB_Dnscl_W0_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_TSB_Dnscl_W1, ispDhubChMap_TSB_Dnscl_W1_BASE,
		ispDhubChMap_TSB_Dnscl_W1_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_TSB_Dnscl_W1_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_TSB_Dnscl_R0, ispDhubChMap_TSB_Dnscl_R0_BASE,
		ispDhubChMap_TSB_Dnscl_R0_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_TSB_Dnscl_R0_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_TSB_Dnscl_R1, ispDhubChMap_TSB_Dnscl_R1_BASE,
		ispDhubChMap_TSB_Dnscl_R1_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_TSB_Dnscl_R1_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_TSB_BCM_R1, ispDhubChMap_TSB_BCM_R1_BASE,
		ispDhubChMap_TSB_BCM_R1_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_TSB_BCM_R1_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0xF, 0xF},
};

static char *ISPSS_TSB_chanName[] = {
	"Tile_W0",
	"Tile_W1",
	"Dnscl_W0",
	"Dnscl_W1",
	"Dnscl_R0",
	"Dnscl_R1",
	"BCM_R1"
};

static char *ISPSS_TSB_dhubName = "TSB DHUB";

static char *ISPSS_TSB_intrName[] = {
	"CH0_intr",
	"CH1_intr",
	"CH2_intr",
	"CH3_intr",
	"CH4_intr",
	"CH5_intr",
	"CH6_intr",
	"CH7_intr",
	"CH8_intr",
	"CH9_intr",
	"CH10_intr",
	"CH11_intr",
	"CH12_intr",
	"CH13_intr",
	"CH14_intr",
	"CH15_intr",
	"ispIntr",
	"miIntr",
	"mipi0Intr",
	"mipi1Intr",
	"Scldn_intr0",
	"Scldn_intr1",
	"Scldn_intr2",
	"dwrp_intr",
	"rot_intr",
	"tile_intr",
	"bcmInvalidIntr",
	"usr_inr13",
	"usr_inr14",
	"usr_inr15"
};

#define ispDhubChMap_FWR_Dwrp_Matrix_W_SIZE     (ISPSS_CONVERT_128BITS_TO_BYTES(1500))
#define ispDhubChMap_FWR_Dwrp_R0_SIZE           (ISPSS_CONVERT_128BITS_TO_BYTES(375))
#define ispDhubChMap_FWR_Dwrp_R1_SIZE           (ISPSS_CONVERT_128BITS_TO_BYTES(250))
#define ispDhubChMap_FWR_Dwrp_W0_SIZE           (ISPSS_CONVERT_128BITS_TO_BYTES(180))
#define ispDhubChMap_FWR_Dwrp_W1_SIZE           (ISPSS_CONVERT_128BITS_TO_BYTES(120))
#define ispDhubChMap_FWR_Rot_R0_SIZE            (ISPSS_CONVERT_128BITS_TO_BYTES(180))
#define ispDhubChMap_FWR_Rot_R1_SIZE            (ISPSS_CONVERT_128BITS_TO_BYTES(120))
#define ispDhubChMap_FWR_Rot_W0_SIZE            (ISPSS_CONVERT_128BITS_TO_BYTES(180))
#define ispDhubChMap_FWR_Rot_W1_SIZE            (ISPSS_CONVERT_128BITS_TO_BYTES(120))

#define ispDhubChMap_FWR_Dwrp_Matrix_W_BASE (ISPSS_FWR_DHUB_BANK0_START_ADDR)
#define ispDhubChMap_FWR_Dwrp_R0_BASE \
	(ispDhubChMap_FWR_Dwrp_Matrix_W_BASE + ispDhubChMap_FWR_Dwrp_Matrix_W_SIZE)
#define ispDhubChMap_FWR_Dwrp_R1_BASE \
	(ispDhubChMap_FWR_Dwrp_R0_BASE + ispDhubChMap_FWR_Dwrp_R0_SIZE)
#define ispDhubChMap_FWR_Rot_R0_BASE \
	(ispDhubChMap_FWR_Dwrp_R1_BASE + ispDhubChMap_FWR_Dwrp_R1_SIZE)
#define ispDhubChMap_FWR_Rot_R1_BASE \
	(ispDhubChMap_FWR_Rot_R0_BASE + ispDhubChMap_FWR_Rot_R0_SIZE)
#define ispDhubChMap_FWR_Dwrp_W0_BASE \
	(ispDhubChMap_FWR_Rot_R1_BASE + ispDhubChMap_FWR_Rot_R1_SIZE)
#define ispDhubChMap_FWR_Dwrp_W1_BASE \
	(ispDhubChMap_FWR_Dwrp_W0_BASE + ispDhubChMap_FWR_Dwrp_W0_SIZE)
#define ispDhubChMap_FWR_Rot_W0_BASE \
	(ispDhubChMap_FWR_Dwrp_W1_BASE + ispDhubChMap_FWR_Dwrp_W1_SIZE)
#define ispDhubChMap_FWR_Rot_W1_BASE \
	(ispDhubChMap_FWR_Rot_W0_BASE + ispDhubChMap_FWR_Rot_W0_SIZE)

struct DHUB_channel_config  ISPSS_FWR_config[ISPSS_FWR_NUM_OF_CHANNELS] = {
	{ ispDhubChMap_FWR_Dwrp_Matrix_W, ispDhubChMap_FWR_Dwrp_Matrix_W_BASE,
		ispDhubChMap_FWR_Dwrp_Matrix_W_BASE+ISPSS_CMDSZ_256,
		ISPSS_CMDSZ_256, (ispDhubChMap_FWR_Dwrp_Matrix_W_SIZE-ISPSS_CMDSZ_256),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Dwrp_R0, ispDhubChMap_FWR_Dwrp_R0_BASE,
		ispDhubChMap_FWR_Dwrp_R0_BASE+ISPSS_CMDSZ_256,
		ISPSS_CMDSZ_256, (ispDhubChMap_FWR_Dwrp_R0_SIZE-ISPSS_CMDSZ_256),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Dwrp_R1, ispDhubChMap_FWR_Dwrp_R1_BASE,
		ispDhubChMap_FWR_Dwrp_R1_BASE+ISPSS_CMDSZ_256,
		ISPSS_CMDSZ_256, (ispDhubChMap_FWR_Dwrp_R1_SIZE-ISPSS_CMDSZ_256),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Rot_R0, ispDhubChMap_FWR_Rot_R0_BASE,
		ispDhubChMap_FWR_Rot_R0_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_FWR_Rot_R0_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Rot_R1, ispDhubChMap_FWR_Rot_R1_BASE,
		ispDhubChMap_FWR_Rot_R1_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_FWR_Rot_R1_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Dwrp_W0, ispDhubChMap_FWR_Dwrp_W0_BASE,
		ispDhubChMap_FWR_Dwrp_W0_BASE+ISPSS_CMDSZ_256,
		ISPSS_CMDSZ_256, (ispDhubChMap_FWR_Dwrp_W0_SIZE-ISPSS_CMDSZ_256),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Dwrp_W1, ispDhubChMap_FWR_Dwrp_W1_BASE,
		ispDhubChMap_FWR_Dwrp_W1_BASE+ISPSS_CMDSZ_128,
		ISPSS_CMDSZ_128, (ispDhubChMap_FWR_Dwrp_W1_SIZE-ISPSS_CMDSZ_128),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Rot_W0, ispDhubChMap_FWR_Rot_W0_BASE,
		ispDhubChMap_FWR_Rot_W0_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_FWR_Rot_W0_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
	{ ispDhubChMap_FWR_Rot_W1, ispDhubChMap_FWR_Rot_W1_BASE,
		ispDhubChMap_FWR_Rot_W1_BASE+ISPSS_CMDSZ_64,
		ISPSS_CMDSZ_64, (ispDhubChMap_FWR_Rot_W1_SIZE-ISPSS_CMDSZ_64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0x7, 0x7},
};

char *ISPSS_FWR_chanName[] = {
	"Dwrp_Matrix_W",
	"Dwrp_R0",
	"Dwrp_R1",
	"Rot_R0",
	"Rot_R1",
	"Dwrp_W0",
	"Dwrp_W1",
	"Rot_W0",
	"Rot_W1",
};

char *ISPSS_FWR_dhubName = "FWR DHUB";
static struct ISPSS_DHUB_CONTEXT_INFO contextInfo;

/*****************************************************************
 *    Function: DhubInitialization
 *    Description: Initialize DHUB .
 *    Parameter : cpuId ------------- cpu ID
 *             dHubBaseAddr -------------  dHub Base address.
 *             hboSramAddr ----- Sram Address for HBO.
 *             pdhubHandle ----- pointer to 2D dhubHandle
 *             dhub_config ----- configuration of AG
 *             numOfChans     ----- number of channels
 *    Return:        void
 ******************************************************************/
void DhubInitialization(SIGN32 cpuId, UNSG32 dHubBaseAddr, UNSG32 hboSramAddr,
		struct HDL_dhub2d *pdhubHandle, struct DHUB_channel_config *dhub_config,
		SIGN32 numOfChans, enum DHUB_TYPE dHubType)
{
	struct HDL_semaphore *pSemHandle;
	SIGN32 i;
	SIGN32 chanId;
	SIGN32 cmdDiv = 8;

	memset(&contextInfo, 0, sizeof(struct ISPSS_DHUB_CONTEXT_INFO));
	contextInfo.dHubBaseAddr = dHubBaseAddr;
	contextInfo.hboSramAddr = hboSramAddr;
	contextInfo.numOfChans = numOfChans;
	contextInfo.dHubType = dHubType;
	contextInfo.pDhubHandle = pdhubHandle;
	contextInfo.pDhubConfig = dhub_config;
	contextInfo.dhubName = NULL;
	contextInfo.chanName = NULL;
	if (dhub_config == ISPSS_TSB_config) {
		contextInfo.dhubName = ISPSS_TSB_dhubName;
		contextInfo.chanName = ISPSS_TSB_chanName;
		contextInfo.intrName = ISPSS_TSB_intrName;
	} else if (dhub_config == ISPSS_FWR_config) {
		contextInfo.dhubName = ISPSS_FWR_dhubName;
		contextInfo.chanName = ISPSS_FWR_chanName;
		contextInfo.intrName = ISPSS_TSB_intrName;
	}

	ISPSS_AddDhubContextInfo(contextInfo);

	if (dHubType == DHUB_TYPE_128BIT)
		cmdDiv = 16;
	else
		cmdDiv = 8;

	//Initialize HDL_dhub with a $dHub BIU instance.
	dhub2d_hdl(hboSramAddr,            /*!    Base address of dHub.HBO SRAM !*/
			dHubBaseAddr,            /*!    Base address of a BIU instance of $dHub !*/
			pdhubHandle                /*!    Handle to HDL_dhub2d !*/
			);
	//set up semaphore to trigger cmd done interrupt
	//note that this set of semaphores are different from the HBO semaphores
	//the ID must match the dhub ID because they are hardwired.
	pSemHandle = ispss_dhub_semaphore(&pdhubHandle->dhub);

	for (i = 0; i < numOfChans; i++) {
		// Configurate a dHub channel
		// note that in this function,
		// it also configured right HBO channels(cmdQ and dataQ) and semaphores
		chanId = dhub_config[i].chanId;
		{
/* dhub :Handle to HDL_dhub !
 * chanId: Channel ID in $dHubReg !
 * dhub_config[i].chanCmdBase: UNSG32 baseCmd, Channel FIFO base address (byte address) for cmdQ
 * dhub_config[i].chanDataBase: UNSG32 baseData, Channel FIFO base address (byte address) for dataQ
 * dhub_config[i].chanCmdSize/cmdDiv: SIGN32 depthCmd, Channel FIFO depth for cmdQ, in 64b word
 * dhub_config[i].chanDataSize/cmdDiv: SIGN32 depthData, Channel FIFO depth for dataQ, in 64b word
 * dhub_config[i].chanMtuSize: See 'dHubChannel.CFG.MTU', 0/1/2 for 8/32/128 bytes
 * dhub_config[i].chanQos: See 'dHubChannel.CFG.QoS'
 * dhub_config[i].chanSelfLoop: See 'dHubChannel.CFG.selfLoop'
 * dhub_config[i].chanEnable: 0 to disable, 1 to enable
 * 0, Pass NULL to directly init dHub, or Pass non-zero
 * to receive programming sequence
 * in (adr,data) pairs
 */
			dhub_channel_cfg(&pdhubHandle->dhub,
					chanId,
					dhub_config[i].chanCmdBase,
					dhub_config[i].chanDataBase,
					dhub_config[i].chanCmdSize/cmdDiv,
					dhub_config[i].chanDataSize/cmdDiv,
					dhub_config[i].chanMtuSize,
					dhub_config[i].chanQos,
					dhub_config[i].chanSelfLoop,
					dhub_config[i].chanEnable,
					0);
			// setup interrupt for channel chanId
			//configure the semaphore depth to be 1
			semaphore_cfg(pSemHandle, chanId, 1, 0);
		}
	}
}

void ISPSS_DhubInit(SIGN32 cpuId)
{
	static int alreadyInitialized;

	if (!alreadyInitialized) {
		ISPSS_REG_WRITE32(0xF9140000, 0);

		ISPSS_InitDhubContext(ISPSS_DHUB_128bBCM_TYPE_64b,
				MEMMAP_ISPSS_BCM_REG_BASE, MEMMAP_ISPSS_DUMMY_REG_BASE);

		DhubInitialization(cpuId, ISPSS_TSB_DHUB_BASE, ISPSS_TSB_HBO_SRAM_BASE,
				&ISPSS_TSB_dhubHandle, ISPSS_TSB_config,
				ISPSS_TSB_NUM_OF_CHANNELS, DHUB_TYPE_128BIT);
		DhubInitialization(cpuId, ISPSS_FWR_DHUB_BASE, ISPSS_FWR_HBO_SRAM_BASE,
				&ISPSS_FWR_dhubHandle, ISPSS_FWR_config,
				ISPSS_FWR_NUM_OF_CHANNELS, DHUB_TYPE_128BIT);

		alreadyInitialized = 1;
	}
}
