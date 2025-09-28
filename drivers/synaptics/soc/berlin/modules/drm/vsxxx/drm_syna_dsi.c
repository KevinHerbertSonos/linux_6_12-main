// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Shanmugam Ramachandran <Shanmugam.Ramachandran@synaptics.com>
 *
 */

#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_modes.h>
#include <video/videomode.h>
#include <drm/drm_panel.h>

#include "drm_syna_drv.h"

#define PANEL_MAX_WIDTH     3840
#define PANEL_MAX_HEIGHT    2160

MODULE_LICENSE("Dual MIT/GPL");

static int syna_dsi_add_extra_modes(struct drm_connector *connector)
{
	int num_modes = 0;
	struct syna_drm_private *dev_priv = connector->dev->dev_private;

	dev_priv->panel[0] = of_drm_find_panel(of_find_compatible_node(NULL, NULL, "syna,drm-dsi"));

	if (!IS_ERR(dev_priv->panel[0])) {
		drm_panel_get_modes(dev_priv->panel[0], connector);
		num_modes++;
	} else {
		connector->status = connector_status_disconnected;
	}

	return num_modes;
}

static int syna_dsi_connector_helper_get_modes(struct drm_connector *connector)
{
	int num_modes;

	num_modes = syna_dsi_add_extra_modes(connector);

	drm_mode_sort(&connector->probed_modes);

	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s] found %d modes\n",
			 connector->base.id, connector->name, num_modes);

	return num_modes;
}

static int syna_dsi_connector_helper_mode_valid(struct drm_connector *connector,
						struct drm_display_mode *mode)
{
	struct syna_drm_private *dev_priv = connector->dev->dev_private;
	VPP_MIPI_CONFIG_PARAMS *pvpp_mipi_param = dev_priv->vpp_config_param.mipi_resinfo_params;
	RESOLUTION_INFO *pres_info_params = &pvpp_mipi_param->infoparams.resInfo;

	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		return MODE_NO_INTERLACE;
	else if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
		return MODE_NO_DBLESCAN;

	if (pres_info_params->active_width == mode->hdisplay &&
			pres_info_params->active_height == mode->vdisplay)
		return MODE_OK;
	else
		return MODE_ERROR;
}

static void syna_dsi_connector_destroy(struct drm_connector *connector)
{
	struct syna_drm_private *dev_priv = connector->dev->dev_private;
	ENUM_VOUT_CONNECTOR vout_id =
		connector->connector_type == DRM_MODE_CONNECTOR_DVID ?
			  VOUT_CONNECTOR_HDMI : VOUT_CONNECTOR_DSI;

	if (!dev_priv) {
		DRM_ERROR("%s %d  device private is NULL!!\n",
			  __func__, __LINE__);
		return;
	}
	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n",
			 connector->base.id, connector->name);

	drm_connector_cleanup(connector);

	kfree(connector);
	dev_priv->connector[vout_id] = NULL;
}

static void syna_dsi_connector_force(struct drm_connector *connector)
{
}

static struct drm_connector_helper_funcs syna_dsi_connector_helper_funcs = {
	.get_modes = syna_dsi_connector_helper_get_modes,
	.mode_valid = syna_dsi_connector_helper_mode_valid,
};

static const struct drm_connector_funcs syna_dsi_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = syna_dsi_connector_destroy,
	.force = syna_dsi_connector_force,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.dpms = drm_helper_connector_dpms,
};

struct drm_connector *syna_dsi_connector_create(struct drm_device *dev)
{
	struct drm_connector *connector;

	connector = kzalloc(sizeof(*connector), GFP_KERNEL);
	if (!connector)
		return ERR_PTR(-ENOMEM);

	drm_connector_init(dev, connector, &syna_dsi_connector_funcs,
			   DRM_MODE_CONNECTOR_DSI);
	drm_connector_helper_add(connector, &syna_dsi_connector_helper_funcs);

	connector->dpms = DRM_MODE_DPMS_OFF;
	connector->interlace_allowed = false;
	connector->doublescan_allowed = false;
	connector->display_info.subpixel_order = SubPixelHorizontalRGB;

	DRM_DEBUG_DRIVER("[CONNECTOR:%d:%s]\n", connector->base.id,
			 connector->name);

	return connector;
}
