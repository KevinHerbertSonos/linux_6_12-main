// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef _VPP_INTRS_H_
#define _VPP_INTRS_H_

#include "avioDhub.h"

static unsigned int vpp_intrs [] = {
	avioDhubSemMap_vpp128b_vpp_inr0,
	avioDhubSemMap_vpp128b_vpp_inr1,
};

#define VPP_DHUB_VOP1_INTR avioDhubSemMap_vpp128b_vpp_inr0
#define VPP_DHUB_VOP2_INTR avioDhubSemMap_vpp128b_vpp_inr1

#define VPP_SIGNAL_VSYNC(MODE, INTRNO, VSYNC_SEM, VSYNC_SEM1)	\
			do {\
				if (VPP_DHUB_VOP1_INTR == INTRNO)\
					up(VSYNC_SEM);\
				else if (VPP_DHUB_VOP2_INTR == INTRNO)\
					up(VSYNC_SEM1);\
			} while(0);

#endif //_VPP_INTRS_H_
