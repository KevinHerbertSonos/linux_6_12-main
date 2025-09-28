/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_BE_COMMON_H_
#define _ISPSS_BE_COMMON_H_

enum ISPBE_ENUM_FMT {
	ISPSS_SRCFMT_MIN = 0,
	ISPSS_SRCFMT_RGB444 = 0,
	ISPSS_SRCFMT_RGB888,
	ISPSS_SRCFMT_YUV422P,
	ISPSS_SRCFMT_YUV444P,
	ISPSS_SRCFMT_YUV420SP,
	ISPSS_SRCFMT_YUV422SP,
	ISPSS_SRCFMT_YUV422SP_DWA,
	ISPSS_SRCFMT_YUV420SP_DWA,
	ISPSS_SRCFMT_YUV420SP_TILED_V8H8,
	ISPSS_SRCFMT_YUV420SP_TILED_V8H6,
	ISPSS_SRCFMT_YUV420SP_TILED_V6H8,
	ISPSS_SRCFMT_YUV422SP_TILED_V8H8,
	ISPSS_SRCFMT_YUV422SP_TILED_V8H6,
	ISPSS_SRCFMT_YUV422SP_TILED_V6H8,
	ISPSS_SRCFMT_MAX,
};

enum ISPBE_ENUM_BIT_DEPTH {
	ISPSS_BIT_DEPTH_INVALID = -1,
	ISPSS_BIT_DEPTH_8BIT,    /**<  8 bit color depth. */
	ISPSS_BIT_DEPTH_10BIT,   /**< 10 bit color depth. */
};

#define ISPBE_GET_TILE_SIZE(srcfmt, tileHwin, tileVwin) do { \
	if (srcfmt == ISPSS_SRCFMT_YUV420SP_TILED_V8H8 || \
		srcfmt == ISPSS_SRCFMT_YUV422SP_TILED_V8H8) { \
		tileHwin = 8; tileVwin = 8; \
	} \
	else if (srcfmt == ISPSS_SRCFMT_YUV420SP_TILED_V8H6 || \
		srcfmt == ISPSS_SRCFMT_YUV422SP_TILED_V8H6) { \
		tileHwin = 6; tileVwin = 8; \
	} \
	else if (srcfmt == ISPSS_SRCFMT_YUV420SP_TILED_V6H8 || \
		srcfmt == ISPSS_SRCFMT_YUV422SP_TILED_V6H8) { \
		tileHwin = 8; tileVwin = 6; \
	} \
} while (0)

#endif
