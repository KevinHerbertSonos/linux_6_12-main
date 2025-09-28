// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */

#define SYNA_FORMAT_SIZE_GFX  ARRAY_SIZE(plane_supported_formats_gfx)

#define SYNA_GET_PLANE_SUPPORTED_FORMAT(plane_id) plane_supported_formats_gfx

static const uint32_t plane_supported_formats_gfx[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_RGB888,
};

static const uint32_t plane_supported_formats_size[] = {
	//PLANE_GFX0
	SYNA_FORMAT_SIZE_GFX,

	//PLANE_GFX1
	SYNA_FORMAT_SIZE_GFX,
};

static const char *plane_name[] = {
	"PLANE_GFX0",
	"PLANE_GFX1",
};
