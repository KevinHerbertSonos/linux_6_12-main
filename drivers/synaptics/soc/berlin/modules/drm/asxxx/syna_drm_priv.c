// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Shanmugam Ramachandran <Shanmugam.Ramachandran@synaptics.com>
 *
 */

#include "drm_syna_drv.h"

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include "syna_vpp.h"

#include "dsih_displays.h"
#include "dsih_core.h"
#include "dsih_api.h"
#include "includes.h"

static const ENUM_PLANE_ID syna_primary_plane_id[MAX_CRTC] = {
	PLANE_GFX0,
	PLANE_GFX1,
};

int syna_is_scaling_allowed(struct syna_drm_private *dev_priv,
				ENUM_PLANE_ID plane_id, ENUM_CPCB_ID cpcb_id)
{
	//Scaling not supported in asxxx
	return 0;
}

VPP_BUILD_IN_FRAME_TYPE syna_get_buidin_frame_type(ENUM_PLANE_ID plane_id)
{
	return VPP_BUILD_IN_FRAME_TYPE_GFX;
}

void syna_vpp_dev_init_priv(struct drm_device *dev)
{
	/* Buildin frame not required for ASXX - Single Plane/Single CRTC system */
}

void syna_read_config_priv(vpp_config_params *p_vpp_config_param)
{
	p_vpp_config_param->active_planes = (1 << PLANE_GFX0);
	p_vpp_config_param->active_planes |= (1 << PLANE_GFX1);
}

int syna_modeset_createEntries(struct syna_drm_private *dev_priv)
{
	struct drm_device *dev = dev_priv->dev;
	int err = 0;
	unsigned int plane_possible_crtc_mask;
	enum drm_plane_type plane_type;
	int panelId;
	int encoderId;

	for (encoderId = 0, panelId = 0; panelId < MAX_PANELS; panelId++) {
		dev_priv->encoder[panelId] = syna_encoder_create(dev, panelId, panelId, (1 << encoderId));

		if (IS_ERR(dev_priv->encoder[panelId])) {
			DRM_ERROR("failed to create a encoder %d\n", panelId);
		} else {
			dev_priv->connector[panelId] = syna_lcdc_connector_create(dev);

			if (IS_ERR(dev_priv->connector[panelId])) {
				DRM_ERROR("failed to create an Connector %d\n", panelId);
			} else {
				err = drm_connector_attach_encoder(dev_priv->connector[panelId],
								dev_priv->encoder[panelId]);

				if (err) {
					DRM_ERROR
						("failed to attach [ENCODER:%d:%s] to [CONNECTOR:%d:%s] (err=%d)\n",
						dev_priv->encoder[panelId]->base.id,
						dev_priv->encoder[panelId]->name,
						dev_priv->connector[panelId]->base.id,
						dev_priv->connector[panelId]->name, err);
					goto err_syna_modeset_createEntries;
				}
			}

			/* Note: one plane(GFX) per crtc/panel --
			 * so panel & plane are used inter-changeably
			 */
			plane_type = DRM_PLANE_TYPE_PRIMARY;
			plane_possible_crtc_mask = (1 << encoderId);
			dev_priv->plane[panelId] = syna_plane_create(dev,
					plane_possible_crtc_mask, panelId, plane_type);
			if (IS_ERR(dev_priv->plane[panelId])) {
				DRM_ERROR("failed to create a %d plane\n", plane_type);
				err = PTR_ERR(dev_priv->plane[panelId]);
				goto err_syna_modeset_createEntries;
			}

			dev_priv->crtc[panelId] = syna_crtc_create(dev, encoderId, dev_priv->plane[panelId]);
			if (IS_ERR(dev_priv->crtc[panelId])) {
				DRM_ERROR("failed to create a CRTC\n");
				err = PTR_ERR(dev_priv->crtc[panelId]);
				goto err_syna_modeset_createEntries;
			}
			encoderId++;
		}
	}

err_syna_modeset_createEntries:
	return err;
}

int syna_vpp_get_bm_details(struct dma_buf *dma_buf,
		       struct bm_pt_param *pt_param,
		       struct berlin_meta **bm_meta)
{
	return -1;
}

int syna_dsi_panel_send_cmd (unsigned int cmdsize, unsigned char *pcmd)
{
	return dsi_panel_send_cmd(cmdsize, pcmd);
}

void syna_push_buildin_frame(u32 plane)
{
	syna_vpp_push_buildin_null_frame(plane);
}