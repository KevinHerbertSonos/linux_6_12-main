// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/kernel.h>

#include "avio_dhub_cfg.h"
#include "avio_dhub_cfg_prv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "avio_dhub_drv.h"
#include "tee_ca_dhub.h"

#define CPUINDEX    0

HDL_dhub2d AG_dhubHandle;
HDL_dhub2d VPP_dhubHandle;

/*Total size vpp128bDhub_BANK0_SIZE 0x5A00 = 23040*/
#define AVIO_DHUB_MV_R0_SIZE       (10752)
#define AVIO_DHUB_MV_R1_SIZE       (10752)

#define AVIO_DHUB_MV_R0_BASE       (VPP_DHUB_BANK0_START_ADDR)
#define AVIO_DHUB_MV_R1_BASE       (VPP_DHUB_BANK0_START_ADDR + AVIO_DHUB_MV_R0_SIZE)

DHUB_channel_config  LCDC_config[VPP_NUM_OF_CHANNELS] = {
    {avioDhubChMap_vpp128b_LCDC1_R, AVIO_DHUB_MV_R0_BASE,
		AVIO_DHUB_MV_R0_BASE+64, 64, (AVIO_DHUB_MV_R0_SIZE-64),
		dHubChannel_CFG_MTU_256byte, 1, 0, 1, 0xF, 0xF},

    {avioDhubChMap_vpp128b_LCDC2_R, AVIO_DHUB_MV_R1_BASE,
		AVIO_DHUB_MV_R1_BASE+64, 64, (AVIO_DHUB_MV_R1_SIZE-64),
		dHubChannel_CFG_MTU_256byte, 1, 0, 1, 0xF, 0xF},
};

DHUB_channel_config  AG_config[AG_NUM_OF_CHANNELS] = {
	// Bank0
	{avioDhubChMap_aio64b_I2S1_R, AIO_DHUB_I2S1_R_BASE,
		AIO_DHUB_I2S1_R_BASE+32, 32, (AIO_DHUB_I2S1_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S1_W, AIO_DHUB_I2S1_W_BASE,
		AIO_DHUB_I2S1_W_BASE+64, 64, (AIO_DHUB_I2S1_W_SIZE-64),
		dHubChannel_CFG_MTU_128byte, 1, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S2_R, AIO_DHUB_I2S2_R_BASE,
		AIO_DHUB_I2S2_R_BASE+32, 32, (AIO_DHUB_I2S2_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S2_W, AIO_DHUB_I2S2_W_BASE,
		AIO_DHUB_I2S2_W_BASE+32, 32, (AIO_DHUB_I2S2_W_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S3_R, AIO_DHUB_I2S3_R_BASE,
		AIO_DHUB_I2S3_R_BASE+32, 32, (AIO_DHUB_I2S3_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S3_W, AIO_DHUB_I2S3_W_BASE,
		AIO_DHUB_I2S3_W_BASE+32, 32, (AIO_DHUB_I2S3_W_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S4_R, AIO_DHUB_I2S4_R_BASE,
		AIO_DHUB_I2S4_R_BASE+32, 32, (AIO_DHUB_I2S4_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S4_W, AIO_DHUB_I2S4_W_BASE,
		AIO_DHUB_I2S4_W_BASE+32, 32, (AIO_DHUB_I2S4_W_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_BCM_R, AIO_DHUB_BCM_R_BASE,
		AIO_DHUB_BCM_R_BASE+128,  128, (AIO_DHUB_BCM_R_SIZE-128),
		dHubChannel_CFG_MTU_256byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S5_R, AIO_DHUB_I2S5_R_BASE,
		AIO_DHUB_I2S5_R_BASE+32, 32, (AIO_DHUB_I2S5_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_I2S5_W, AIO_DHUB_I2S5_W_BASE,
		AIO_DHUB_I2S5_W_BASE+32, 32, (AIO_DHUB_I2S5_W_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_PDM_W, AIO_DHUB_PDM_W_BASE,
		AIO_DHUB_PDM_W_BASE+32,    32, (AIO_DHUB_PDM_W_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_PDM_R, AIO_DHUB_PDM_R_BASE,
		AIO_DHUB_PDM_R_BASE+32,  32, (AIO_DHUB_PDM_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_DCLASS_R, AIO_DHUB_DCLASS_R_BASE,
		AIO_DHUB_DCLASS_R_BASE+32, 32, (AIO_DHUB_DCLASS_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_DRT_R, AIO_DHUB_DRT_R_BASE,
		AIO_DHUB_DRT_R_BASE+32,    32, (AIO_DHUB_DRT_R_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},

	{avioDhubChMap_aio64b_DRT_W, AIO_DHUB_DRT_W_BASE,
		AIO_DHUB_DRT_W_BASE+32,    32, (AIO_DHUB_DRT_W_SIZE-32),
		dHubChannel_CFG_MTU_128byte, 0, 0, 1, 0xF, 0xF},
};

int drv_dhub_initialize_dhub(void *h_dhub_ctx)
{
	static atomic_t dhub_init_done = ATOMIC_INIT(0);
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;
	avio_fastlogo_info display_info;
	unsigned int channel_init_mask;

	//Allow DHUB initialization only once
	if (atomic_cmpxchg(&dhub_init_done, 0, 1))
		return 0;

	display_info = avio_get_fastlogo_status();

	/* Disable Autopush before initialization of VPP DHUB */
	if (!display_info.u.status)
		wrap_DhubEnableAutoPush(false, true, hDhubCtx->fastlogo_framerate);

	channel_init_mask = display_info.u.status ? 0 : (1 << VPP_NUM_OF_CHANNELS) - 1 ;

	DhubInitialization(DHUB_ID_VPP_DHUB, DHUB_TYPE_128BIT,
			CPUINDEX, hDhubCtx->vpp_dhub_base,
			hDhubCtx->vpp_sram_base, &VPP_dhubHandle,
			LCDC_config, VPP_NUM_OF_CHANNELS,
			DHUB_TYPE_64BIT, hDhubCtx->vpp_bcm_base, 0, channel_init_mask);

	/* Avoid initializing BCM channel when logo is displayed from bootloader
	 * TBD : Root cause need for AGDHUB re-initialization, when AGDHUB
	 * is already initialized in bootloader
	*/
	channel_init_mask = (1 << AG_NUM_OF_CHANNELS) - 1;
	if (display_info.u.status)
		channel_init_mask &= ~(1 << avioDhubChMap_aio64b_BCM_R);

	DhubInitialization(DHUB_ID_AG_DHUB, DHUB_TYPE_64BIT, CPUINDEX, hDhubCtx->ag_dhub_base,
			hDhubCtx->ag_sram_base,
			&AG_dhubHandle, AG_config, AG_NUM_OF_CHANNELS,
			DHUB_TYPE_64BIT, hDhubCtx->vpp_bcm_base, 0, channel_init_mask);

	return 0;
}

void drv_dhub_config_ctx(void *h_dhub_ctx, UNSG32 avio_base)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)h_dhub_ctx;

	hDhubCtx->ag_dhub_base = avio_base +
							AVIO_MEMMAP_AIO64B_DHUB_REG_BASE +
							RA_aio64bDhub_dHub0;

	hDhubCtx->ag_sram_base = avio_base +
							AVIO_MEMMAP_AIO64B_DHUB_REG_BASE +
							RA_aio64bDhub_tcm0;

	hDhubCtx->vpp_bcm_base   = avio_base +
							AVIO_MEMMAP_AVIO_BCM_REG_BASE;

	hDhubCtx->vpp_dhub_base  = avio_base +
		AVIO_MEMMAP_VPP128B_DHUB_REG_BASE + RA_vpp128bDhub_dHub0;

	hDhubCtx->vpp_sram_base  = avio_base + AVIO_MEMMAP_VPP128B_DHUB_REG_BASE +
							RA_vpp128bDhub_tcm0;

	hDhubCtx->avio_gbl_base = avio_base + AVIO_MEMMAP_AVIO_GBL_BASE;
}