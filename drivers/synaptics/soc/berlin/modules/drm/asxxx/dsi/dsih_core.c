// SPDX-License-Identifier: GPL-2.0
/**
 * Copyright (C) 2018 Synopsys, Inc.
 *
 * @file dsih_core.c
 * @brief Synopsys MIPI DSI driver API
 * included as a part of Synopsys MIPI DSI Host controller driver
 *
 * @author Luis Oliveira <luis.oliveira@synopsys.com>
 */

#include "includes.h"
#include "dsih_displays.h"
#include "dsih_api.h"
#include "dsih_dphy.h"
#include "dsih_hal.h"
//#include "mipi_hal.h"
#include "avioGbl.h"

uint16_t mipi_dsi_init_param(struct mipi_dsi_dev *dev);
/**
* @short Init DSI parameters
* @param[in] dev MIPI DSI device
* @return none
*/
uint16_t mipi_dsi_init_param(struct mipi_dsi_dev *dev)
{
	dphy_t *phy = &dev->phy;
	dsih_dpi_video_t *video = &dev->dpi_video;
	dsih_cmd_mode_video_t *edpi_video = &dev->cmd_mode_video;
	uint16_t ret;
	avio_fastlogo_info display_info;

	mipi_dbg_print(MIPI_INFO,"%s:DSI initialization\n", FUNC_NAME);
#ifdef GEN_3
	phy->reference_freq = 25000; /* [KHz] */
#else
	phy->reference_freq = 27000; /* [KHz] */
#endif
	if (dev == NULL) {
		mipi_dbg_print(MIPI_ERROR,"Dev Null\n");
		return MIPI_RET(ENODEV);
	}
	phy->base = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_DPHYTX;
	phy->pHandle = dev->pHandle;

	dev->max_lanes = 4;
	dev->max_bta_cycles = 4095;
	dev->color_mode_polarity = 1;
	dev->shut_down_polarity = 1;

	display_info = avio_get_fastlogo_status();

	if (!display_info.u.status) {
		mipi_dsih_presp_timeout_low_power_write(dev, 0);
		mipi_dsih_presp_timeout_low_power_read(dev, 0);
		mipi_dsih_presp_timeout_high_speed_write(dev, 0);
		mipi_dsih_presp_timeout_high_speed_read(dev, 0);
		mipi_dsih_presp_timeout_bta(dev, 8);

		/*
		* Open instance first
		* - to make sure addresses and other
		* attributes are correct
		*/
		ret = mipi_dsih_open(dev, COLOR_CODE_24BIT);

		if (ret != TRUE)
			return ret;
	}
	/* initialise DPI video params */
	video->no_of_lanes = 1;
	video->non_continuous_clock = 0;
	video->virtual_channel = 0;
	video->video_mode = 4;  /* invalid value */
	video->byte_clock = 0;  /* KHz  */
	video->pixel_clock = 0; /* dpi_clock KHz */
	video->is_18_loosely = 0;
	video->h_polarity = 0;
	video->h_active_pixels = 0; /* hadr */
	video->h_sync_pixels = 0;
	video->h_back_porch_pixels = 0; /* hbp */
	video->h_total_pixels = 0;		/* hfp */
	video->v_polarity = 0;
	video->v_active_lines = 0; /* vadr */
	video->v_sync_lines = 0;
	video->v_back_porch_lines = 0; /* vbp */
	video->v_total_lines = 0;	  /* vfp */
	/* as of 1.20a */
	video->max_hs_to_lp_cycles = 50;	  /* value for max freq */
	video->max_lp_to_hs_cycles = 153;	 /* value for max freq */
	video->max_clk_hs_to_lp_cycles = 66;  /* value for max freq */
	video->max_clk_lp_to_hs_cycles = 181; /* value for max freq */

	/* Initialise eDPI params */

	edpi_video->virtual_channel = 0;
	edpi_video->lp = 0;
	edpi_video->te = 0;
	edpi_video->bta = 0;
	edpi_video->h_start = 0;
	edpi_video->h_active_pixels = 0;
	edpi_video->packet_size = 0;
	edpi_video->v_start = 0;
	edpi_video->v_active_lines = 0;

	return TRUE;
}

/**
* @short Start DSI platform
* @param[in] dev MIPI DSI device
* @param[in] display Type of display
* @param[in] video_mode Video mode or command mode
* @param[in] lanes number of lanes
* @return none
*/
void dsi_platform_init(struct mipi_dsi_dev *dev, int display, int video_mode,
					   int lanes)
{
	switch (video_mode) {
		case COMMAND_MODE:
			/* command mode */
			mipi_dbg_print(MIPI_DEBUG,"%s:Command Mode\n", FUNC_NAME);
			if (!pre_command_mode(dev, display, lanes)) {
				mipi_dbg_print(MIPI_DEBUG,"%s:eDPI Video\n", FUNC_NAME);
				if (!mipi_dsih_edpi_video(dev))
					mipi_dbg_print(MIPI_ERROR,
							"error configuring video\n");

				copy_edpi_param_changes(&dev->cmd_mode_video,
						&dev->cmd_mode_video_old);
			}
			break;
		case VIDEO_MODE:
			/* video mode */
			mipi_dbg_print(MIPI_DEBUG,"%s:Video Mode\n", FUNC_NAME);
			if (!pre_video_mode(dev, display, lanes)) {
				mipi_dbg_print(MIPI_DEBUG,"%s:DPI Video\n", FUNC_NAME);
				mipi_dbg_print(MIPI_DEBUG,"%s:Using %d lanes\n", FUNC_NAME, lanes);
				if (mipi_dsih_dpi_video(dev))
					mipi_dbg_print(MIPI_ERROR,
							"error configuring video\n");

				copy_dpi_param_changes(&dev->dpi_video,
						&dev->dpi_video_old);
			}
			pre_video_mode(dev, display, lanes);
			break;
		default:
			mipi_dbg_print(MIPI_ERROR,"Invalid mode\n");
			break;
	}
	mipi_dsih_reset_controller(dev);
}

/**
 * @short Initialization routine - Entry point of the driver
 * @param[in] pdev pointer to the platform device structure
 * @return 0 on success and a negative number on failure
 * Refer to Linux errors.
 */
int mipi_dsi_init(struct mipi_dsi_dev *dev)
{
	int ret = 0;

	mipi_dbg_print(MIPI_INFO,"****************************************\n");
	mipi_dbg_print(MIPI_INFO,"%s:Installing SNPS MIPI DSI module\n", FUNC_NAME);
	mipi_dbg_print(MIPI_INFO,"****************************************\n");

	mipi_dbg_print(MIPI_INFO,"%s:Device registration\n", FUNC_NAME);
	if (!dev) {
		mipi_dbg_print(MIPI_ERROR,"%s:Could not allocated mipi_dsi_dev\n", FUNC_NAME);
		return MIPI_RET(ENOMEM);
	}
	dev->device_name = "MIPI_DSI";
	mipi_dbg_print(MIPI_INFO,"%s:Driver's name '%s'\n", FUNC_NAME, dev->device_name);

	// Map memory blocks
	mipi_dbg_print(MIPI_DEBUG,"%s:Map memory blocks\n", FUNC_NAME);
	dev->core_addr = MEMMAP_AVIO_REG_BASE + AVIO_MEMMAP_AVIO_GBL_BASE + RA_avioGbl_MEMMAP_MIPI;

	//INIT
	mipi_dsi_init_param(dev);
	dev->phy.is_g118 = TRUE;

	  /* Un-mask interrupts */
	mipi_dsih_hal_int_mask_0(dev, 0xffffffff);
	mipi_dsih_hal_int_mask_1(dev, 0xffffffff);

	return ret;
}
