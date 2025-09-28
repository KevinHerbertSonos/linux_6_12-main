// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Synaptics Incorporated
 *
 *
 * Author: Lijun Fan <Lijun.Fan@synaptics.com>
 *
 */

#include <drm/drm_crtc.h>
#include <drm/drm_encoder.h>
#include <drm/drm_modeset_helper_vtables.h>
#include "drm_syna_drv.h"
#include "vpp_api.h"
#include <drm/drm_panel.h>

static void syna_tmds_encoder_helper_dpms(struct drm_encoder *encoder, int mode)
{
}

static bool
syna_tmds_encoder_helper_mode_fixup(struct drm_encoder *encoder,
					const struct drm_display_mode *mode,
					struct drm_display_mode *adjusted_mode)
{
	return true;
}

static void syna_tmds_encoder_helper_prepare(struct drm_encoder *encoder)
{
	struct syna_drm_private *dev_priv = (struct syna_drm_private *)encoder->dev->dev_private;

	if ((encoder->encoder_type == DRM_MODE_ENCODER_DPI) && dev_priv->panel[0])
		drm_panel_prepare(dev_priv->panel[0]);
}

static void syna_tmds_encoder_helper_enable(struct drm_encoder *encoder)
{
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;

	if ((encoder->encoder_type == DRM_MODE_ENCODER_DPI) && dev_priv->panel[0])
		drm_panel_enable(dev_priv->panel[0]);
}

static void syna_tmds_encoder_helper_disable(struct drm_encoder *encoder)
{
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;

	if ((encoder->encoder_type == DRM_MODE_ENCODER_DPI) && dev_priv->panel[0])
		drm_panel_disable(dev_priv->panel[0]);
}

static void syna_tmds_encoder_helper_commit(struct drm_encoder *encoder)
{
}

static void
syna_tmds_encoder_helper_mode_set(struct drm_encoder *encoder,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	VPP_DISP_OUT_PARAMS dispParams;
	int res_id;
	int cpcb = CPCB_1;
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;

	// Always use CPCB_1 for correct displaymode
	MV_VPP_GetDispOutParams(cpcb, &dispParams);

	if (encoder->encoder_type == DRM_MODE_ENCODER_TMDS) {
		if (dev_priv->modeset_enabled == false) {
			DRM_ERROR("skip hdmi modeset to (%d), handled internally\n",res_id);
			return;
		}

		res_id = MV_VPP_GetResIndex(mode->hdisplay, mode->vdisplay,
				mode->flags & DRM_MODE_FLAG_INTERLACE, mode->clock,
				drm_mode_vrefresh(mode));

		dispParams.uiResId = res_id;
		dispParams.uiDispId = VOUT_HDMI;
	} else if (encoder->encoder_type == DRM_MODE_ENCODER_DPI) {
#ifdef USE_DOLPHIN
		if (dispParams.uiDisplayMode == VPP_VOUT_DUAL_MODE_PIP) {
			// For Dual Display mode Fetch info with CPCB_2
			cpcb = CPCB_2;
			MV_VPP_GetDispOutParams(cpcb, &dispParams);
			dispParams.uiDisplayMode = VPP_VOUT_DUAL_MODE_PIP;
		}
#endif

		dispParams.uiResId = RES_DSI_CUSTOM;
		dispParams.uiDispId = VOUT_DSI;
		dispParams.uiBitDepth = OUTPUT_BIT_DEPTH_8BIT;
		dispParams.uiColorFmt = OUTPUT_COLOR_FMT_RGB888;
		dispParams.iPixelRepeat = 1;
	}

	MV_VPP_SetDisplayResolution(cpcb, dispParams, 1);
}

static void syna_tmds_encoder_destroy(struct drm_encoder *encoder)
{
	struct syna_drm_private *dev_priv = encoder->dev->dev_private;
	int vout_id = (encoder->encoder_type == DRM_MODE_ENCODER_TMDS ?
				VOUT_CONNECTOR_HDMI : VOUT_CONNECTOR_DSI);

	if (encoder == NULL) {
		DRM_ERROR("%s: encoder is NULL!!\n", __func__);
		return;
	}
	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	drm_encoder_cleanup(encoder);

	kfree(encoder);
	dev_priv->encoder[vout_id] = NULL;
}

static const struct drm_encoder_helper_funcs syna_tmds_encoder_helper_funcs = {
	.dpms = syna_tmds_encoder_helper_dpms,
	.mode_fixup = syna_tmds_encoder_helper_mode_fixup,
	.prepare = syna_tmds_encoder_helper_prepare,
	.enable = syna_tmds_encoder_helper_enable,
	.disable = syna_tmds_encoder_helper_disable,
	.commit = syna_tmds_encoder_helper_commit,
	.mode_set = syna_tmds_encoder_helper_mode_set,
};

static const struct drm_encoder_funcs syna_tmds_encoder_funcs = {
	.destroy = syna_tmds_encoder_destroy,
};

struct drm_encoder *syna_tmds_encoder_create(struct drm_device *dev,
		ENUM_VOUT_CONNECTOR vout_id, ENUM_CPCB_ID cpcb_id)
{
	struct drm_encoder *encoder;
	int err;
	int encoder_type = (vout_id == VOUT_CONNECTOR_HDMI ?
				DRM_MODE_ENCODER_TMDS : DRM_MODE_ENCODER_DPI);

	encoder = kzalloc(sizeof(*encoder), GFP_KERNEL);
	if (!encoder)
		return ERR_PTR(-ENOMEM);

	err = drm_encoder_init(dev, encoder, &syna_tmds_encoder_funcs,
					encoder_type, NULL);
	if (err) {
		DRM_ERROR("Failed to initialise encoder - %d", vout_id);
		kfree(encoder);
		return ERR_PTR(err);
	}
	drm_encoder_helper_add(encoder, &syna_tmds_encoder_helper_funcs);

	/*
	 * This is a bit field that's used to determine which
	 * CRTCs can drive this encoder.
	 */
	encoder->possible_crtcs = 0x1 << cpcb_id;

	DRM_DEBUG_DRIVER("[ENCODER:%d:%s]\n", encoder->base.id, encoder->name);

	return encoder;
}

void drm_syna_encoder_suspend(struct syna_drm_private *dev_priv,
							bool suspend)
{
	/* Normal mode : Suspend/Resume handled by TA
	 * Recovery flow (/non-amp): TBD here */
}
