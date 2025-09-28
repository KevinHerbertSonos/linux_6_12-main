// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 */

#ifndef __VPP_PRIV_VSXXX
#define __VPP_PRIV_VSXXX
/* definition of VPP video outputs */
typedef enum {
	FIRST_VOUT     = 0,
	VOUT_HDMI      = 0,
	VOUT_HD        = 1,
	VOUT_SD        = 2,
	VOUT_DSI       = 3,
	MAX_NUM_VOUTS
} ENUM_VOUT_ID;

/* definition of VPP video connectors */
typedef enum {
	FIRST_VOUT_CONNECTOR   = 0,
	VOUT_CONNECTOR_HDMI    = 0,
	VOUT_CONNECTOR_DSI     = 1,
	MAX_NUM_VOUT_CONNECTORS
} ENUM_VOUT_CONNECTOR;

/* definition of VPP input planes */
typedef enum {
	FIRST_PLANE  = 0,
	PLANE_MAIN   = 0,
	PLANE_PIP    = 1,
	PLANE_GFX1   = 2,
#ifdef VPP_ALLOW_ALL_PLANES
#ifndef USE_DOLPHIN
	//USE_PLATYPUS
	PLANE_OVP_EL = 3,
#else
	PLANE_GFX2   = 3,
#endif
#ifndef USE_DOLPHIN
	PLANE_VMX     = PLANE_OVP_EL,
#else
	PLANE_AUX     = 4,
	PLANE_OVP_EL  = 5,
	PLANE_MAIN_EL = 6,
	PLANE_VMX     = 7,
#endif
#endif
	MAX_NUM_PLANES
} ENUM_PLANE_ID;

#endif
