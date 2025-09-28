// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */
#ifndef __VPP_PRIV_MYNA
#define  __VPP_PRIV_MYNA
typedef enum {
	FIRST_VOUT     = 0,
	VOUT_HDMI      = 0,
	VOUT_DSI       = 1,
	VOUT_TFT      = 2,

	MAX_NUM_VOUTS
} ENUM_VOUT_ID;

/* definition of VPP input planes */
typedef enum {
	FIRST_PLANE   = 0,
	PLANE_GFX0    = 0, // Plane Alias for LCDC1
	PLANE_GFX1    = 1,  // Plane Alias for LCDC2

	MAX_NUM_PLANES
} ENUM_PLANE_ID;

typedef enum {
	FIRST_VOUT_CONNECTOR   = 0,
	VOUT_CONNECTOR_TFT     = 0,
	VOUT_CONNECTOR_CPU     = 1,
	VOUT_CONNECTOR_DSI     = 2,
	MAX_NUM_VOUT_CONNECTORS
} ENUM_VOUT_CONNECTOR;

typedef struct SynaLcdcConfig_T {
	unsigned char intf_type;     /*DPI TFT or CPU, DSI Video or CMD*/
	unsigned int bits_per_pixel; /*16, 18 or 24-bits per pixel*/
	unsigned char mode;
	unsigned char ext_te;         /*External tearing Effect*/
	unsigned int te_delay;
	unsigned char iclk;             /*lpclock polarity*/
	unsigned char rgb_swap;
	unsigned char rotation;         /*0, 90, 180, 270*/

	/*htotal = hsync_len + left_margin + xres + right_margin*/
	unsigned int hsync_len;
	unsigned int right_margin; //FP
	unsigned int xres;
	unsigned int left_margin;  //BP

	/*vtotal = vsync_len + upper_margin + yres + lower_margin*/
	unsigned int vsync_len;
	unsigned int lower_margin; //FP
	unsigned int yres;
	unsigned int upper_margin; //BP

	unsigned int refresh;
	/*htotal * vtotal * refresh*/
	unsigned int pixclock;

	/*htotal + hskip*/
	unsigned int hskip;
	/*vtotal + vskip*/
	unsigned int vskip;
} SYNA_LCDC_CONFIG;
#endif