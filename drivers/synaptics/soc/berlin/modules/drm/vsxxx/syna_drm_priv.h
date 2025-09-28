// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */
#if !defined(__SYNA_DRM_PRIV_H__)
#define __SYNA_DRM_PRIV_H__

#define MAX_CRTC	MAX_NUM_CPCBS
#define VOUT_DEVICE VOUT_DSI

#define VPP_BUILD_IN_FRAME_ENABLE
#define VPP_BUILD_IN_FRAME_GFX_WIDTH    720
#define VPP_BUILD_IN_FRAME_GFX_HEIGHT   480
#define VPP_BUILD_IN_FRAME_VID_WIDTH    720
#define VPP_BUILD_IN_FRAME_VID_HEIGHT   480
#define VPP_BUILD_IN_FRAME_GFX_NULL_WIDTH    8
#define VPP_BUILD_IN_FRAME_GFX_NULL_HEIGHT   8

typedef enum __VPP_BUILD_IN_FRAME_TYPE__ {
	VPP_BUILD_IN_FRAME_TYPE_GFX,
	VPP_BUILD_IN_FRAME_TYPE_VID,
	VPP_BUILD_IN_FRAME_TYPE_GFX_NULL,
	VPP_BUILD_IN_FRAME_TYPE_MAX,
} VPP_BUILD_IN_FRAME_TYPE;

#define VPP_GET_PLANE_ROTATE_INDX(INDX, PLANE)	\
			{ \
				if ((PLANE == PLANE_PIP) ||\
					(PLANE == PLANE_GFX1)) \
					INDX = PLANE; \
			}

void syna_push_buildin_frame(u32 plane);
VPP_BUILD_IN_FRAME_TYPE syna_get_buidin_frame_type(ENUM_PLANE_ID plane_id);
struct drm_encoder *syna_tmds_encoder_create(struct drm_device *dev,
					ENUM_VOUT_CONNECTOR vout_id, ENUM_CPCB_ID cpcb_id);
int syna_dsi_panel_send_cmd (unsigned int cmdsize, unsigned char *pcmd);
#endif /* !defined(__SYNA_DRM_PRIV_H__) */
