// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */

#define SYNA_FORMAT_TYPE_PIP    2
#define SYNA_FORMAT_TYPE_VIDEO  1
#define SYNA_FORMAT_TYPE_GFX    0
#define SYNA_FORMAT_SIZE_VIDEO  ARRAY_SIZE(plane_supported_formats_video)
#define SYNA_FORMAT_SIZE_GFX  ARRAY_SIZE(plane_supported_formats_gfx)
#define SYNA_FORMAT_SIZE_PIP  ARRAY_SIZE(plane_supported_formats_pip)

#define SYNA_GET_PLANE_SUPPORTED_FORMAT(plane_id)                       \
	((plane_supported_formats_type[plane_id] == SYNA_FORMAT_TYPE_PIP) ? \
		plane_supported_formats_pip :                                   \
	(plane_supported_formats_type[plane_id] == SYNA_FORMAT_TYPE_VIDEO ? \
		plane_supported_formats_video : plane_supported_formats_gfx));

static const uint32_t plane_supported_formats_video[] = {
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVYU,
};

static const uint32_t plane_supported_formats_pip[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV21,
};

static const uint32_t plane_supported_formats_gfx[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ABGR8888,
};

static const uint32_t plane_supported_formats_size[] = {
	//PLANE_MAIN
	SYNA_FORMAT_SIZE_VIDEO,
	//PLANE_PIP
	SYNA_FORMAT_SIZE_PIP,
	//PLANE_GFX1
	SYNA_FORMAT_SIZE_GFX,
#ifndef USE_DOLPHIN
	//PLANE_OVL_EL
	SYNA_FORMAT_SIZE_VIDEO,
#else
	//PLANE_GFX2
	SYNA_FORMAT_SIZE_GFX,
#endif
};

static const uint32_t plane_supported_formats_type[] = {
	//PLANE_MAIN
	SYNA_FORMAT_TYPE_VIDEO,
	//PLANE_PIP
	SYNA_FORMAT_TYPE_PIP,
	//PLANE_GFX1
	SYNA_FORMAT_TYPE_GFX,
#ifndef USE_DOLPHIN
	//PLANE_OVL_EL
	SYNA_FORMAT_TYPE_VIDEO,
#else
	//PLANE_GFX2
	SYNA_FORMAT_TYPE_GFX,
#endif
};

static const char *plane_name[] = {
	"PLANE_MAIN",
	"PLANE_PIP",
	"PLANE_GFX1",
#ifndef USE_DOLPHIN
	"PLANE_OVL_EL",
#else
	"PLANE_GFX2",
#endif
};
