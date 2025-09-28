// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <pranan@synaptics.com>
 *
 */

#include <drm/drm_crtc.h>
#include "drm_syna_drv.h"
#include <drm/drm_panel.h>
#include <drm/drm_encoder.h>
#include <drm/drm_modeset_helper_vtables.h>

#include "dsih_displays.h"
#include "dsih_core.h"
#include "dsih_api.h"
#include "includes.h"
#include "vpp_api.h"

/* To accomodate lower pixel clocks, set pixel clock with mulitplier
 * Add Corresponding divider in the Driver
 */
#define PIXEL_CLOCK_MULTIPLIER				4
#define PIXEL_CLOCK_RATE(PIX_CLK)			(PIX_CLK * PIXEL_CLOCK_MULTIPLIER * 1000)

typedef struct dsi_desc_t {
	int    		outformat;
	int    		bpp;
	int    		byteclock;
	unsigned char	lpcmd;
	unsigned char	non_continuous_clock;
	unsigned char	lanes;
	unsigned char	eotp_tx;
	unsigned char	eotp_rx;
	unsigned char	h_polarity;
	unsigned char	v_polarity;
	unsigned char	data_polarity;
	int		cmdsize;
	int		htotal;
	unsigned char	video_mode;
	unsigned char	receive_ack;
	unsigned char	is_18_loose;
	unsigned char	color_coding;
	int		chunks;
	int		null_pkt;
} SYNA_DSI_DESC;

typedef struct mipi_dsi_info_t {
	SYNA_DSI_DESC synaDsiInfo;
	struct mipi_dsi_dev *dsi_dev;
	int is_dsi_host_configured;
} MIPI_DSIH_INFO;

static int lcdc_rgb_swap;

static const char *compatible_name[] = {
		"syna,drm-lcdc",
		"syna,drm-dsi"
};

static void syna_dsi_host_config(MIPI_DSIH_INFO *pMipiDsiInfo)
{
	avio_fastlogo_info display_info;

	if (!pMipiDsiInfo->is_dsi_host_configured) {
		pMipiDsiInfo->is_dsi_host_configured = 1;
		dsi_host_shutdown(0);
	}

	display_info = avio_get_fastlogo_status();

	if (!display_info.u.status)
		dsi_platform_init(pMipiDsiInfo->dsi_dev, 0, VIDEO_MODE, pMipiDsiInfo->synaDsiInfo.lanes);
}

static void syna_update_dsi_info(MIPI_DSIH_INFO *pMipiDsiInfo)
{
	struct mipi_dsi_dev *dsi_dev = pMipiDsiInfo->dsi_dev;

	dsi_dev->dpi_video.virtual_channel = 0;
	dsi_dev->dpi_video.display_type = 0;

	dsi_dev->dpi_video.no_of_lanes = pMipiDsiInfo->synaDsiInfo.lanes;
	dsi_dev->dpi_video.video_mode = pMipiDsiInfo->synaDsiInfo.video_mode;
	dsi_dev->dpi_video.receive_ack_packets = pMipiDsiInfo->synaDsiInfo.receive_ack;
	dsi_dev->dpi_video.is_18_loosely = pMipiDsiInfo->synaDsiInfo.is_18_loose;
	dsi_dev->dpi_video.data_en_polarity = pMipiDsiInfo->synaDsiInfo.data_polarity;

	dsi_dev->dpi_video.h_polarity = pMipiDsiInfo->synaDsiInfo.h_polarity;
	dsi_dev->dpi_video.v_polarity = pMipiDsiInfo->synaDsiInfo.v_polarity;
	dsi_dev->dpi_video.color_coding = pMipiDsiInfo->synaDsiInfo.color_coding;

	dsi_dev->dpi_video.eotp_rx_en = pMipiDsiInfo->synaDsiInfo.eotp_rx;
	dsi_dev->dpi_video.eotp_tx_en = pMipiDsiInfo->synaDsiInfo.eotp_tx;

	dsi_dev->dpi_video.non_continuous_clock = pMipiDsiInfo->synaDsiInfo.non_continuous_clock;
	dsi_dev->dpi_video.no_of_chunks = pMipiDsiInfo->synaDsiInfo.chunks;
	dsi_dev->dpi_video.null_packet_size = pMipiDsiInfo->synaDsiInfo.null_pkt;
	dsi_dev->dpi_video.dpi_lp_cmd_en = pMipiDsiInfo->synaDsiInfo.lpcmd;
	dsi_dev->dpi_video.byte_clock = pMipiDsiInfo->synaDsiInfo.byteclock; /* KHz  */
	dsi_dev->dpi_video.h_total_pixels = pMipiDsiInfo->synaDsiInfo.htotal;
}

static int syna_encoder_parse_lcdc_dt(void)
{
	struct device_node *lcdc_node;

	lcdc_node = of_find_compatible_node(NULL, NULL, "syna,drm-lcdc");

	if (!lcdc_node) {
		DRM_ERROR("LCDC node not found \n");
		return -ENODEV;
	}

	of_property_read_u32(lcdc_node, "rgbswap",  &lcdc_rgb_swap);

	return 0;
}

static int syna_encoder_parse_dsi_dt(MIPI_DSIH_INFO *pMipiDsiInfo)
{
	struct device_node *dsi_node;

	dsi_node = of_find_compatible_node(NULL, NULL, "syna,drm-dsi");

	if (!dsi_node) {
		DRM_ERROR("DSI node not found \n");
		return -ENODEV;
	}

	of_property_read_u32(dsi_node, "HTOTAL",  &pMipiDsiInfo->synaDsiInfo.htotal);
	of_property_read_u32(dsi_node, "Byte_clk",  &pMipiDsiInfo->synaDsiInfo.byteclock);
	of_property_read_u32(dsi_node, "bits_per_pixel", &pMipiDsiInfo->synaDsiInfo.bpp);
	of_property_read_u32(dsi_node, "busformat", &pMipiDsiInfo->synaDsiInfo.outformat);
	of_property_read_u8(dsi_node, "Lanes", &pMipiDsiInfo->synaDsiInfo.lanes);
	of_property_read_u8(dsi_node, "Vid_mode", &pMipiDsiInfo->synaDsiInfo.video_mode);
	of_property_read_u8(dsi_node, "Recv_ack", &pMipiDsiInfo->synaDsiInfo.receive_ack);
	of_property_read_u8(dsi_node, "Loosely_18", &pMipiDsiInfo->synaDsiInfo.is_18_loose);
	of_property_read_u8(dsi_node, "H_polarity", &pMipiDsiInfo->synaDsiInfo.h_polarity);
	of_property_read_u8(dsi_node, "V_Polarity", &pMipiDsiInfo->synaDsiInfo.v_polarity);
	of_property_read_u8(dsi_node, "Data_Polarity", &pMipiDsiInfo->synaDsiInfo.data_polarity);
	of_property_read_u8(dsi_node, "Eotp_tx", &pMipiDsiInfo->synaDsiInfo.eotp_tx);
	of_property_read_u8(dsi_node, "Eotp_rx", &pMipiDsiInfo->synaDsiInfo.eotp_rx);
	of_property_read_u8(dsi_node, "non-Continuous_clk", &pMipiDsiInfo->synaDsiInfo.non_continuous_clock);
	of_property_read_u8(dsi_node, "dpi_lp_cmd", &pMipiDsiInfo->synaDsiInfo.lpcmd);
	of_property_read_u8(dsi_node, "Color_coding", &pMipiDsiInfo->synaDsiInfo.color_coding);
	of_property_read_u32(dsi_node, "Chunks", &pMipiDsiInfo->synaDsiInfo.chunks);
	of_property_read_u32(dsi_node, "Null_Pkt", &pMipiDsiInfo->synaDsiInfo.null_pkt);

	return 0;
}

static void
syna_encoder_helper_mode_set(struct drm_encoder *encoder,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;
	SYNA_LCDC_CONFIG lcdcConfig = {0};
	int crtc_index = (encoder->encoder_type== DRM_MODE_ENCODER_DSI) ? 1 : 0;
	struct drm_display_info *disp_info = &dev_priv->connector[crtc_index]->display_info;
	MIPI_DSIH_INFO *pMipiDsiInfo = dev_priv->pMipiDsiInfo;
	struct mipi_dsi_dev *dsi_dev = pMipiDsiInfo ? pMipiDsiInfo->dsi_dev : NULL;
	avio_fastlogo_info display_info;

	if (disp_info->num_bus_formats) {
		lcdcConfig.mode = *disp_info->bus_formats;
		lcdcConfig.bits_per_pixel = disp_info->bpc;
	}

	/*htotal = hsync_len + left_margin + xres + right_margin*/
	lcdcConfig.hsync_len = mode->hsync_end - mode->hsync_start;
	lcdcConfig.right_margin = mode->hsync_start - mode->hdisplay; //FP
	lcdcConfig.xres = mode->hdisplay;
	lcdcConfig.left_margin = mode->htotal - mode->hsync_end;  //BP

	/*vtotal = vsync_len + upper_margin + yres + lower_margin*/
	lcdcConfig.vsync_len = mode->vsync_end - mode->vsync_start;
	lcdcConfig.lower_margin = mode->vsync_start - mode->vdisplay; //FP
	lcdcConfig.yres = mode->vdisplay;
	lcdcConfig.upper_margin = mode->vtotal - mode->vsync_end; //BP
	lcdcConfig.pixclock = mode->clock ;

	display_info = avio_get_fastlogo_status();

	if (crtc_index) {
		if (dsi_dev)
		{
			/* Update the timing info */
			dsi_dev->dpi_video.h_sync_pixels = lcdcConfig.hsync_len;
			dsi_dev->dpi_video.h_back_porch_pixels = lcdcConfig.left_margin;
			dsi_dev->dpi_video.v_sync_lines = lcdcConfig.vsync_len;
			dsi_dev->dpi_video.v_back_porch_lines = lcdcConfig.upper_margin;
			dsi_dev->dpi_video.h_active_pixels = lcdcConfig.xres;
			dsi_dev->dpi_video.v_active_lines = lcdcConfig.yres;
			dsi_dev->dpi_video.pixel_clock = lcdcConfig.pixclock;
			dsi_dev->dpi_video.v_total_lines = lcdcConfig.yres +
											lcdcConfig.lower_margin +
											lcdcConfig.vsync_len +
											lcdcConfig.upper_margin;

			/* Configure DSI host */
			syna_dsi_host_config(pMipiDsiInfo);
		}
		if (!display_info.u.status)
			VPP_Clock_Set_Rate_Ext(PIXEL_CLOCK_RATE(lcdcConfig.pixclock));
	} else {
		lcdcConfig.rgb_swap = lcdc_rgb_swap;
		if (!display_info.u.status)
			VPP_Clock_Set_Rate(PIXEL_CLOCK_RATE(lcdcConfig.pixclock));
	}

	if(dev_priv->connector[crtc_index] && dev_priv->panel[crtc_index])
		drm_panel_prepare(dev_priv->panel[crtc_index]);

	syna_vpp_load_config(crtc_index, &lcdcConfig);
}

static void syna_encoder_helper_enable(struct drm_encoder *encoder)
{
	int crtc_index = (encoder->encoder_type == DRM_MODE_ENCODER_DSI) ? 1 : 0;
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;

	if(dev_priv->connector[crtc_index] && dev_priv->panel[crtc_index])
		drm_panel_enable(dev_priv->panel[crtc_index]);
}

static void syna_encoder_helper_disable(struct drm_encoder *encoder)
{
	int crtc_index = (encoder->encoder_type == DRM_MODE_ENCODER_DSI) ? 1 : 0;
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;
	MIPI_DSIH_INFO *pMipiDsiInfo = dev_priv->pMipiDsiInfo;

	if(dev_priv->connector[crtc_index] && dev_priv->panel[crtc_index]) {
		drm_panel_disable(dev_priv->panel[crtc_index]);

		if(encoder->encoder_type == DRM_MODE_ENCODER_DSI) {
			dsi_host_shutdown(1);

			/* retrigger DSI configuration -- during resume */
			pMipiDsiInfo->is_dsi_host_configured = 0;
		}
	}
}

static void syna_encoder_destroy(struct drm_encoder *encoder)
{
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;
	MIPI_DSIH_INFO *pMipiDsiInfo = dev_priv->pMipiDsiInfo;

	if (encoder == NULL) {
		DRM_ERROR("%s: encoder is NULL!!\n", __func__);
		return;
	}
	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	drm_encoder_cleanup(encoder);

	kfree(pMipiDsiInfo->dsi_dev);
	kfree(pMipiDsiInfo);
	kfree(encoder);
}

static const struct drm_encoder_helper_funcs syna_encoder_helper_funcs = {
	.mode_set = syna_encoder_helper_mode_set,
	.enable = syna_encoder_helper_enable,
	.disable = syna_encoder_helper_disable,
};

static const struct drm_encoder_funcs syna_encoder_funcs = {
	.destroy = syna_encoder_destroy,
};

struct drm_encoder *syna_encoder_create(struct drm_device *dev,
		ENUM_VOUT_CONNECTOR vout_id, ENUM_CPCB_ID cpcb_id, int possible_crtc_mask)
{
	struct drm_encoder *encoder;
	struct syna_drm_private *dev_priv = dev->dev_private;
	struct platform_device *pdev = to_platform_device(dev->dev);
	int err;
	int encoder_type = (cpcb_id == 0) ? \
						DRM_MODE_ENCODER_DPI :\
						DRM_MODE_ENCODER_DSI;
	MIPI_DSIH_INFO *pMipiDsiInfo;
	struct mipi_dsi_dev *dsi_dev;

	if (!of_device_is_available(of_find_compatible_node(NULL, NULL, compatible_name[cpcb_id])))
		return ERR_PTR(-ENODEV);

	if (encoder_type == DRM_MODE_ENCODER_DSI) {
		dev_priv->pMipiDsiInfo =  devm_kzalloc(&pdev->dev, sizeof(MIPI_DSIH_INFO), GFP_KERNEL);
		if (!dev_priv->pMipiDsiInfo) {
			DRM_ERROR("Allocate memory failed %ld\n", sizeof(MIPI_DSIH_INFO));
			return ERR_PTR(-ENOMEM);
		}

		dsi_dev = devm_kzalloc(&pdev->dev, sizeof(struct mipi_dsi_dev), GFP_KERNEL);

		if (!dsi_dev) {
			DRM_ERROR("Allocate memory failed %ld\n", sizeof(struct mipi_dsi_dev));
			return ERR_PTR(-ENOMEM);
		}

		pMipiDsiInfo = dev_priv->pMipiDsiInfo;
		pMipiDsiInfo->dsi_dev = dsi_dev;

		err = syna_encoder_parse_dsi_dt(pMipiDsiInfo);
		if (err) {
			DRM_ERROR("DSI Encoder info not available\n");
			return ERR_PTR(err);
		}
		mipi_dsi_init(dsi_dev);
		syna_update_dsi_info(pMipiDsiInfo);

		dsi_register_device(dsi_dev);

		/* Initialise the bootup config for DSI to initialised */
		pMipiDsiInfo->is_dsi_host_configured = 1;
	} else {
		syna_encoder_parse_lcdc_dt();
	}

	encoder = kzalloc(sizeof(*encoder), GFP_KERNEL);
	if (!encoder)
		return ERR_PTR(-ENOMEM);

	err = drm_encoder_init(dev, encoder, &syna_encoder_funcs,
					encoder_type, NULL);
	if (err) {
		DRM_ERROR("Failed to initialise encoder - %d", vout_id);
		kfree(encoder);
		return ERR_PTR(err);
	}

	drm_encoder_helper_add(encoder, &syna_encoder_helper_funcs);

	/*
	 * This is a bit field that's used to determine which
	 * CRTCs can drive this encoder.
	 */
	encoder->possible_crtcs = possible_crtc_mask;

	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	return encoder;
}

void drm_syna_encoder_suspend(struct syna_drm_private *dev_priv,
							bool suspend)
{
	int panelId;

	for (panelId = 0; panelId < MAX_PANELS; panelId++) {
		if (dev_priv->connector[panelId] && dev_priv->panel[panelId]) {
			if (suspend)
				drm_panel_unprepare(dev_priv->panel[panelId]);
			else
				drm_panel_prepare(dev_priv->panel[panelId]);
		}
	}
}
