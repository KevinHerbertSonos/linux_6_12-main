// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _VPP_INTRS_H_
#define _VPP_INTRS_H_

#include "avioDhub.h"

static unsigned int vpp_intrs [] = {
	//VOP1/DP1 - VBI, VDE
	avioDhubSemMap_vpp128b_vpp_inr0,
	avioDhubSemMap_vpp128b_vpp_inr6,
	//VOP2/DP2 - VOP_TG, EoF
	avioDhubSemMap_vpp128b_vpp_inr13,
	avioDhubSemMap_vpp128b_vpp_inr14,
	//OVP - uv,y
	avioDhubSemMap_vpp128b_vpp_inr11,
	avioDhubSemMap_vpp128b_vpp_inr12,
	avioDhubSemMap_vpp128b_vpp_inr2,
	avioDhubSemMap_vpp128b_vpp_inr3,
	avioDhubSemMap_vpp128b_vpp_inr4,
	avioDhubSemMap_vpp128b_vpp_inr5,
	avioDhubSemMap_vpp128b_vpp_inr15
};

#define VPP_DHUB_VOP1_INTR avioDhubSemMap_vpp128b_vpp_inr0
#define VPP_DHUB_VOP2_INTR avioDhubSemMap_vpp128b_vpp_inr14
#define VPP_DHUB_HDMITX_HPD_INTR avioDhubSemMap_vpp128b_vpp_inr2

#define VPP_SIGNAL_VSYNC(MODE, INTRNO, VSYNC_SEM, VSYNC_SEM1)	\
			do {\
				if (VPP_DHUB_VOP1_INTR == INTRNO)\
					up(VSYNC_SEM);\
				else if (VPP_DHUB_VOP2_INTR == INTRNO) {\
					if (MODE == VPP_VOUT_DUAL_MODE_PIP)\
						up(VSYNC_SEM1);\
					else\
						up(VSYNC_SEM);\
				}\
			} while(0);

#endif //_VPP_INTRS_H_
