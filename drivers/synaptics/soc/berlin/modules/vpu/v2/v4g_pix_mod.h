// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __V4G_PIX_MOD_H__
#define __V4G_PIX_MOD_H__

#include <drm/drm_fourcc.h>

#if defined(__cplusplus)
extern "C" {
#endif
/* Vendor Ids: */
#define DRM_FORMAT_MOD_VENDOR_SYNAPTICS 0x0b

/*
 * Synaptics VideoSmart modifiers
 *
 * Tiles could be arranged in Groups of Tiles (GOTs), it is a small tile
 * within a tile. GOT size and layout varies based on platform and
 * performance concern. When the compression is applied, it is possible
 * that we would have two tile type in the GOT, these parameters can't
 * tell the secondary tile type.
 *
 * Besides, an 8 size 4 bytes arrary (32 bytes) would be need to store
 * some compression parameters for a compression meta data plane.
 *
 *       Macro
 * Bits  Param Description
 * ----  ----- -----------------------------------------------------------------
 *
 *  7:0  f     Scan direction description.
 *
 *               0 = Invalid
 *               1 = V4, the scan would always start from vertical for 4 pixel
 *                   then move back to the start pixel of the next horizontal
 *                   direction.
 *               2 = Reserved for future use.
 *
 * 15:8  m     The times of pattern repeat in the right angle direction from
 *             the first scan direction.
 *
 * 19:16 p     The padding bits after the whole scan, could be zero.
 *
 * 20:20 g     GOT packing flag.
 *
 * 23:21 -     Reserved for future use.  Must be zero.
 *
 * 27:24 h     log2(horizontal) of bytes, in GOTs.
 *             MMU page request width value. H1 or H2 after 256 bytes layer.
 *
 * 31:28 v     log2(vertical) of bytes, in GOTs.
 *             MMU page request height value. V1, V2, V4, V8, V16 or V32
 *             after 256 bytes layer.
 *
 * 35:32 -     Reserved for future use.  Must be zero.
 *
 * 36:36 c     Compression flag.
 *
 * 55:37 -     Reserved for future use.  Must be zero.
 *
 */

#define DRM_FORMAT_MOD_SYNA_V4_TILED		fourcc_mod_code(SYNAPTICS, 1)

#define DRM_FORMAT_MOD_SYNA_MTR_LINEAR_2D(f, m, p, g, h, v, c) \
	fourcc_mod_code(SYNAPTICS, ((__u64)((f) & 0xff) | \
				 ((__u64)((m) & 0xff) << 8) | \
				 ((__u64)((p) & 0xf) << 16) | \
				 ((__u64)((g) & 0x1) << 20) | \
				 ((__u64)((h) & 0xf) << 24) | \
				 ((__u64)((v) & 0xf) << 28) | \
				 ((__u64)((c) & 0x1) << 36)))

/* https://confluence.synaptics.com/display/~hguan/Pixel+Formats+In+VXG+Video+Codecs */
/* B8V4H1 */
#define DRM_FORMAT_MOD_SYNA_V4H1 \
	DRM_FORMAT_MOD_SYNA_MTR_LINEAR_2D(1, 1, 0, 0, 0, 0, 0)

/* B10V4H3 */
#define DRM_FORMAT_MOD_SYNA_V4H3P8 \
	DRM_FORMAT_MOD_SYNA_MTR_LINEAR_2D(1, 3, 8, 0, 0, 0, 0)

/* MTR B8V4H64 */
#define DRM_FORMAT_MOD_SYNA_V4H1_64L4_COMPRESSED \
	DRM_FORMAT_MOD_SYNA_MTR_LINEAR_2D(1, 1, 0, 1, 6, 2, 1)

/* MTR B10V4H3H16 */
#define DRM_FORMAT_MOD_SYNA_V4H3P8_64L4_COMPRESSED \
	DRM_FORMAT_MOD_SYNA_MTR_LINEAR_2D(1, 3, 8, 1, 6, 2, 1)

/* MTR B8V4H64H2V32 */
#define DRM_FORMAT_MOD_SYNA_V4H1_128L128_COMPRESSED \
	DRM_FORMAT_MOD_SYNA_MTR_LINEAR_2D(1, 1, 0, 1, 7, 7, 1)

/* MTR B10V4H3P8H16H2V32 */
#define DRM_FORMAT_MOD_SYNA_V4H3P8_128L128_COMPRESSED \
	DRM_FORMAT_MOD_SYNA_MTR_LINEAR_2D(1, 3, 8, 1, 7, 7, 1)

#if defined(__cplusplus)
}
#endif

#endif /* __V4G_PIX_MOD_H__ */
