// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand <prem.anand@synaptics.com>
 *
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>

#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include "panel/panel.h"

typedef struct panel_timing_info_t {
	unsigned int hact;
	unsigned int hfp;
	unsigned int hsa;
	unsigned int hbp;
	unsigned int vact;
	unsigned int vfp;
	unsigned int vsa;
	unsigned int vbp;
	unsigned int pixelclock;
} PANEL_TIMING_INFO;

typedef struct panel_desc_t {
	PANEL_TIMING_INFO   panel_timing;
	int                 outformat;
	int                 bpp;
} SYNA_PANEL_DESC;

static struct display_timing synaPanelTimings;
static SYNA_PANEL_DESC  	synaPanelInfo;

static const struct of_device_id platform_of_match[] = {
	{
		.compatible = "syna,drm-lcdc",
	},
};

static int syna_panel_lcdc_get_timings (struct drm_panel *panel, unsigned int num_timings,
			   struct display_timing *dptimings)
{
	memcpy(dptimings, &synaPanelTimings, sizeof(struct display_timing));

	return 0;
}

static int syna_panel_lcdc_prepare(struct drm_panel *panel)
{
	pr_info("need  prepare impl for non TFT panels \n");
	return 0;
}

static int syna_panel_lcdc_get_modes(struct drm_panel *panel, struct drm_connector *connector) {

	struct drm_display_mode *dmode;

	dmode = drm_mode_create(connector->dev);
	if (dmode) {
		dmode->hdisplay = synaPanelTimings.hactive.typ;
		dmode->hsync_start = dmode->hdisplay + synaPanelTimings.hfront_porch.typ;
		dmode->hsync_end =  dmode->hsync_start + synaPanelTimings.hsync_len.typ;
		dmode->htotal = dmode->hsync_end + synaPanelTimings.hback_porch.typ;

		dmode->vdisplay =  synaPanelTimings.vactive.typ;
		dmode->vsync_start = dmode->vdisplay + synaPanelTimings.vfront_porch.typ;
		dmode->vsync_end = dmode->vsync_start + synaPanelTimings.vsync_len.typ;
		dmode->vtotal = dmode->vsync_end + synaPanelTimings.vback_porch.typ;
		dmode->clock =  synaPanelTimings.pixelclock.typ ;

		drm_mode_set_name(dmode);
		dmode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;

		drm_mode_probed_add(connector, dmode);

		connector->display_info.bus_formats = &synaPanelInfo.outformat;
		connector->display_info.num_bus_formats = 1;
		connector->display_info.bpc = synaPanelInfo.bpp;
	}

	return 0;
}
static const struct drm_panel_funcs syna_panel_lcdc_funcs = {
	.prepare = syna_panel_lcdc_prepare,
	.get_timings = syna_panel_lcdc_get_timings,
	.get_modes = syna_panel_lcdc_get_modes,
};

int syna_panel_lcdc_init(struct platform_device *pdev)
{
	struct drm_panel *panel;
	int err = 0;
	struct device *lcdc_dev;

	lcdc_dev = kmalloc (sizeof(struct device), GFP_KERNEL);

	lcdc_dev->of_node = of_find_node_by_name(NULL, "lcdc_panel");
	if (!lcdc_dev->of_node) {
		pr_err("panel node not found \n");
		return -ENODEV;
	}

	panel = devm_kzalloc(&pdev->dev, sizeof(struct drm_panel), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	of_property_read_u32(lcdc_dev->of_node, "hact", &synaPanelInfo.panel_timing.hact);
	of_property_read_u32(lcdc_dev->of_node, "hfp", &synaPanelInfo.panel_timing.hfp);
	of_property_read_u32(lcdc_dev->of_node, "hsa", &synaPanelInfo.panel_timing.hsa);
	of_property_read_u32(lcdc_dev->of_node, "hbp", &synaPanelInfo.panel_timing.hbp);

	of_property_read_u32(lcdc_dev->of_node, "vact", &synaPanelInfo.panel_timing.vact);
	of_property_read_u32(lcdc_dev->of_node, "vfp", &synaPanelInfo.panel_timing.vfp);
	of_property_read_u32(lcdc_dev->of_node, "vsa", &synaPanelInfo.panel_timing.vsa);
	of_property_read_u32(lcdc_dev->of_node, "vbp", &synaPanelInfo.panel_timing.vbp);

	of_property_read_u32(lcdc_dev->of_node, "pixclockKhz", &synaPanelInfo.panel_timing.pixelclock);
	of_property_read_u32(lcdc_dev->of_node, "bits_per_pixel", &synaPanelInfo.bpp);
	of_property_read_u32(lcdc_dev->of_node, "busformat", &synaPanelInfo.outformat);

	synaPanelTimings.hactive.min = synaPanelTimings.hactive.typ = \
					synaPanelTimings.hactive.max = synaPanelInfo.panel_timing.hact;
	synaPanelTimings.hfront_porch.min = synaPanelTimings.hfront_porch.typ = \
					synaPanelTimings.hfront_porch.max = synaPanelInfo.panel_timing.hfp;
	synaPanelTimings.hsync_len.min = synaPanelTimings.hsync_len.typ = \
					synaPanelTimings.hsync_len.max = synaPanelInfo.panel_timing.hsa;
	synaPanelTimings.hback_porch.min = synaPanelTimings.hback_porch.typ = \
					synaPanelTimings.hback_porch.max = synaPanelInfo.panel_timing.hbp;

	synaPanelTimings.vactive.min = synaPanelTimings.vactive.typ = \
					synaPanelTimings.vactive.max = synaPanelInfo.panel_timing.vact;
	synaPanelTimings.vfront_porch.min =  synaPanelTimings.vfront_porch.typ = \
					synaPanelTimings.vfront_porch.max = synaPanelInfo.panel_timing.vfp;
	synaPanelTimings.vsync_len.min = synaPanelTimings.vsync_len.typ = \
					synaPanelTimings.vsync_len.max = synaPanelInfo.panel_timing.vsa;
	synaPanelTimings.vback_porch.min = synaPanelTimings.vback_porch.typ = \
					synaPanelTimings.vback_porch.max = synaPanelInfo.panel_timing.vbp;

	synaPanelTimings.pixelclock.min = synaPanelTimings.pixelclock.max = \
					synaPanelTimings.pixelclock.typ = synaPanelInfo.panel_timing.pixelclock;

	drm_panel_init(panel, lcdc_dev, &syna_panel_lcdc_funcs,
	 		       DRM_MODE_CONNECTOR_DPI);

	drm_panel_add(panel);

	return err;
}
