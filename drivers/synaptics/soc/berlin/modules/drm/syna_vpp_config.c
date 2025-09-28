// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand <pranan@synaptics.com>
 *
 */

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "drm_syna_drv.h"

#include "vpp_api.h"
#include "syna_vpp.h"
#include "syna_drm_priv.h"
#include "syna_vpp_config.h"

static int syna_encoder_parse_dsi_dt(struct syna_drm_private *dev_priv, vpp_config_params* pMipiConfig)
{
	int ret;
	struct device_node *dsi_node;
	VPP_MIPI_LOAD_CONFIG *pLoadcfg;
	VPP_MIPI_CMD_HEADER  *pHeader;
	VPP_MIPI_CONFIG_PARAMS *pResCfg;

	dsi_node = of_find_compatible_node(NULL, NULL, "syna,drm-dsi");

	if (!dsi_node) {
		DRM_ERROR("DSI node not found \n");
		return -ENODEV;
	} else {
		DRM_DEBUG_DRIVER("DSI node found\n");
	}

	pMipiConfig->vpp_resinfo_shm_handle.size = VPP_SHM_4K_ALIGN_ROUNDUP(sizeof(VPP_MIPI_CONFIG_PARAMS));
	ret = VPP_MEM_AllocateMemory(dev_priv->mem_list, VPP_MEM_TYPE_DMA,
			&pMipiConfig->vpp_resinfo_shm_handle, 0);
	if (ret != 0) {
		DRM_ERROR("%s %d  gem alloc failed!\n", __func__, __LINE__);
		goto EXIT;
	}

	pMipiConfig->vpp_dsi_info_shm_handle.size = VPP_SHM_4K_ALIGN_ROUNDUP(sizeof(VPP_MIPI_LOAD_CONFIG));
	ret = VPP_MEM_AllocateMemory(dev_priv->mem_list, VPP_MEM_TYPE_DMA,
			&pMipiConfig->vpp_dsi_info_shm_handle, 0);
	if (ret != 0) {
		DRM_ERROR("%s %d  gem alloc failed!\n", __func__, __LINE__);
		goto EXIT_FREE_RESINFO;
	}

	pResCfg = pMipiConfig->vpp_resinfo_shm_handle.k_addr;
	pLoadcfg = pMipiConfig->vpp_dsi_info_shm_handle.k_addr;
	pLoadcfg->vppMipiCfgPA = (ARCH_PTR_TYPE)pMipiConfig->vpp_resinfo_shm_handle.p_addr;

	of_property_read_u32(dsi_node, "NO_OF_RESID",  &pLoadcfg->noOfresID);
	of_property_read_u32(dsi_node, "DSI_RES", (UINT32*) &pResCfg->initparams.resId);

	of_property_read_u32(dsi_node, "ACTIVE_WIDTH", &pResCfg->infoparams.resInfo.active_width);
	of_property_read_u32(dsi_node, "HFP", &pResCfg->infoparams.resInfo.hfrontporch);
	of_property_read_u32(dsi_node, "HSYNCWIDTH", &pResCfg->infoparams.resInfo.hsyncwidth);
	of_property_read_u32(dsi_node, "HBP", &pResCfg->infoparams.resInfo.hbackporch);
	of_property_read_u32(dsi_node, "ACTIVE_HEIGHT", &pResCfg->infoparams.resInfo.active_height);
	of_property_read_u32(dsi_node, "VFP", &pResCfg->infoparams.resInfo.vfrontporch);
	of_property_read_u32(dsi_node, "VSYNCWIDTH", &pResCfg->infoparams.resInfo.vsyncwidth);
	of_property_read_u32(dsi_node, "VBP", &pResCfg->infoparams.resInfo.vbackporch );

	of_property_read_u32(dsi_node, "TYPE", &pResCfg->infoparams.resInfo.type);
	of_property_read_u32(dsi_node, "SCAN", &pResCfg->infoparams.resInfo.scan);
	of_property_read_u32(dsi_node, "FRAME_RATE", &pResCfg->infoparams.resInfo.frame_rate);
	of_property_read_u32(dsi_node, "FLAG_3D", &pResCfg->infoparams.resInfo.flag_3d);
	of_property_read_u32(dsi_node, "FREQ", &pResCfg->infoparams.resInfo.freq);
	of_property_read_u32(dsi_node, "PTS_PER_4", &pResCfg->infoparams.resInfo.pts_per_cnt_4);

	of_property_read_u8(dsi_node, "VB_MIN", &pResCfg->infoparams.tgParams.tg_vb_min);
	of_property_read_u8(dsi_node, "HB_MIN", &pResCfg->infoparams.tgParams.tg_hb_min);
	of_property_read_u8(dsi_node, "V_OFF", &pResCfg->infoparams.tgParams.tg_size_v_off_p);
	of_property_read_u8(dsi_node, "H_OFF", &pResCfg->infoparams.tgParams.tg_size_h_off_p);
	of_property_read_u8(dsi_node, "HB_VOP_OFF", &pResCfg->infoparams.tgParams.tg_hb_vop_off);
	of_property_read_u8(dsi_node, "VB_VOP_OFF", &pResCfg->infoparams.tgParams.tg_vb0_vop_off);
	of_property_read_u8(dsi_node, "HB_BE", &pResCfg->infoparams.tgParams.tg_hb_be);
	of_property_read_u8(dsi_node, "VB_BE", &pResCfg->infoparams.tgParams.tg_vb0_be);
	of_property_read_u8(dsi_node, "VB_FP", &pResCfg->infoparams.tgParams.tg_vb_fp);
	of_property_read_u8(dsi_node, "HB_FP", &pResCfg->infoparams.tgParams.tg_hb_fp);
	of_property_read_u32(dsi_node, "PIXEL_CLOCK", &pResCfg->infoparams.tgParams.pixel_clock);

	of_property_read_u32(dsi_node, "HTOTAL",  &pResCfg->infoparams.resInfo.width);
	of_property_read_u32(dsi_node, "Byte_clk",  &pResCfg->initparams.byte_clock);
	of_property_read_u8(dsi_node, "Lanes", &pResCfg->initparams.no_of_lanes);
	of_property_read_u8(dsi_node, "Vid_mode", &pResCfg->initparams.video_mode);
	of_property_read_u8(dsi_node, "Recv_ack", &pResCfg->initparams.receive_ack_packets);
	of_property_read_u8(dsi_node, "Loosely_18", &pResCfg->initparams.is_18_loosely);
	of_property_read_u8(dsi_node, "H_polarity", &pResCfg->initparams.h_polarity);
	of_property_read_u8(dsi_node, "V_Polarity", &pResCfg->initparams.v_polarity);
	of_property_read_u8(dsi_node, "Data_Polarity", &pResCfg->initparams.data_en_polarity);
	of_property_read_u8(dsi_node, "Eotp_tx", &pResCfg->initparams.eotp_tx_en);
	of_property_read_u8(dsi_node, "Eotp_rx",  &pResCfg->initparams.eotp_rx_en);
	of_property_read_u8(dsi_node, "non-Continuous_clk", &pResCfg->initparams.non_continuous_clock);
	of_property_read_u8(dsi_node, "dpi_lp_cmd", &pResCfg->initparams.dpi_lp_cmd_en);
	of_property_read_u8(dsi_node, "Color_coding", &pResCfg->initparams.color_coding);
	of_property_read_u32(dsi_node, "Chunks", &pResCfg->initparams.no_of_chunks);
	of_property_read_u32(dsi_node, "Null_Pkt", (UINT32*) &pResCfg->initparams.null_packet_size);
	of_property_read_u8(dsi_node, "Data_Lane_Polarity", &pResCfg->initparams.data_lane_polarity);
	of_property_read_u8(dsi_node, "virtual_chan", &pResCfg->initparams.virtual_channel);
	of_property_read_u8(dsi_node, "Clk_Lane_Polarity", &pResCfg->initparams.clk_lane_polarity);

	pResCfg->infoparams.resInfo.height = pResCfg->infoparams.resInfo.active_height +
        pResCfg->infoparams.resInfo.vfrontporch +
        pResCfg->infoparams.resInfo.vsyncwidth +
        pResCfg->infoparams.resInfo.vbackporch;

	pResCfg->vppMipiCmd.bufsize = of_property_count_u8_elems(dsi_node, "command");

	if (pResCfg->vppMipiCmd.bufsize) {
		pMipiConfig->vpp_cmdinfo_shm_handle.size = VPP_SHM_4K_ALIGN_ROUNDUP(pResCfg->vppMipiCmd.bufsize);

		ret = VPP_MEM_AllocateMemory(dev_priv->mem_list, VPP_MEM_TYPE_DMA,
				&pMipiConfig->vpp_cmdinfo_shm_handle, 0);
		if (ret != 0) {
			DRM_ERROR("%s %d  gem alloc failed!\n", __func__, __LINE__);
			goto EXIT_FREE_DSI;
		}

		of_property_read_u8_array(dsi_node, "command",
			((UINT8*) pMipiConfig->vpp_cmdinfo_shm_handle.k_addr) + MIPI_CMD_HEADER_SIZE,
			pResCfg->vppMipiCmd.bufsize);

		pHeader = (VPP_MIPI_CMD_HEADER *) pMipiConfig->vpp_cmdinfo_shm_handle.k_addr;
		pHeader->cmd_type = VPP_CMD_TYPE_INIT;
		pHeader->cmd_size = pResCfg->vppMipiCmd.bufsize;

		/* Update the PA for the driver*/
		pResCfg->vppMipiCmd.pcmd = (ARCH_PTR_TYPE)pMipiConfig->vpp_cmdinfo_shm_handle.p_addr;
	}

	pMipiConfig->mipi_config_params =  (void *) pMipiConfig->vpp_dsi_info_shm_handle.k_addr;
	pMipiConfig->mipi_resinfo_params = (void *) pMipiConfig->vpp_resinfo_shm_handle.k_addr;

	VPP_MEM_FlushCache(dev_priv->mem_list, &pMipiConfig->vpp_dsi_info_shm_handle, 0);
	VPP_MEM_FlushCache(dev_priv->mem_list, &pMipiConfig->vpp_resinfo_shm_handle, 0);
	VPP_MEM_FlushCache(dev_priv->mem_list, &pMipiConfig->vpp_cmdinfo_shm_handle, 0);

	of_node_put(dsi_node);

	return 0;

EXIT_FREE_DSI:
	VPP_MEM_FreeMemory(dev_priv->mem_list, VPP_MEM_TYPE_DMA,
		&pMipiConfig->vpp_dsi_info_shm_handle);

EXIT_FREE_RESINFO:
	VPP_MEM_FreeMemory(dev_priv->mem_list, VPP_MEM_TYPE_DMA,
		&pMipiConfig->vpp_resinfo_shm_handle);

EXIT:
	return -ENOMEM;
}

int syna_read_config(struct syna_drm_private *dev_priv)
{
	struct drm_device *dev = dev_priv->dev;
	struct platform_device *pdev = to_platform_device(dev->dev);
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	struct device_node *hdmitx_node;

	if (of_property_read_u32(np, "frame-size-ndx", &dev_priv->vpp_config_param.frame_size_ndx))
		dev_priv->vpp_config_param.frame_size_ndx = 0;

	if (of_property_read_u32(np, "disp1-res-id", &dev_priv->vpp_config_param.disp1_res_id))
		dev_priv->vpp_config_param.disp1_res_id = RES_1080P60;

	if (of_property_read_u32(np, "disp1-type", &dev_priv->vpp_config_param.disp1_out_type))
		dev_priv->vpp_config_param.disp1_out_type = VOUT_HDMI;

	if ((dev_priv->vpp_config_param.disp1_out_type != VOUT_HDMI) && \
		(dev_priv->vpp_config_param.disp1_out_type != VOUT_DSI)) {
		DRM_ERROR("Unsupported display out type [%d]\n",
			dev_priv->vpp_config_param.disp1_out_type);
		return -EINVAL;
	}

	if (of_property_read_u32(np, "disp-mode", &dev_priv->vpp_config_param.display_mode))
		dev_priv->vpp_config_param.display_mode = VPP_VOUT_SINGLE_MODE_PRI;

	if ((dev_priv->vpp_config_param.display_mode < VPP_VOUT_SINGLE_MODE_PRI) ||\
		(dev_priv->vpp_config_param.display_mode >= VPP_VOUT_MODE_MAX))	{
		DRM_ERROR("Unsupported display Mode [%d]\n",
			dev_priv->vpp_config_param.display_mode);
		return -EINVAL;
	}

	if (of_property_read_u32(np, "disp1-bitdepth", &dev_priv->vpp_config_param.disp1_bit_depth))
		dev_priv->vpp_config_param.disp1_bit_depth = OUTPUT_BIT_DEPTH_8BIT;

	if (of_property_read_u32(np, "disp1-colorformat", &dev_priv->vpp_config_param.disp1_colorformat))
		dev_priv->vpp_config_param.disp1_colorformat = OUTPUT_COLOR_FMT_RGB888;

	if (of_property_read_u32(np, "disp2-res-id", &dev_priv->vpp_config_param.disp2_res_id))
		dev_priv->vpp_config_param.disp2_res_id = RES_DSI_CUSTOM;

	if (of_property_read_u32(np, "disp2-type", &dev_priv->vpp_config_param.disp2_out_type))
		dev_priv->vpp_config_param.disp2_out_type = VOUT_DSI;

	dev_priv->vpp_config_param.frame_rate = 60;
	dev_priv->vpp_config_param.enable_frame_buf_copy = 0;
	dev_priv->vpp_config_param.fb_count = 3;
	dev_priv->vpp_config_param.callback = NULL;
	dev_priv->vpp_config_param.data = NULL;

	syna_read_config_priv(&dev_priv->vpp_config_param);
	ret = syna_encoder_parse_dsi_dt(dev_priv, (vpp_config_params*) &dev_priv->vpp_config_param);

	if (ret) {
		if (dev_priv->vpp_config_param.display_mode == VPP_VOUT_SINGLE_MODE_PRI)
			ret = 0;
		else
			DRM_ERROR("Failed to Parse DSI DT node %d\n", ret);
	}

	hdmitx_node = of_get_child_by_name(np, "hdmi_tx");
	if (hdmitx_node) {
		if (of_device_is_available(hdmitx_node))
			dev_priv->vpp_config_param.hdmitx_enable = true;
		of_node_put(hdmitx_node);
	} else {
		DRM_DEBUG_DRIVER("hdmitx node not found\n");
	}

	return ret;
}
