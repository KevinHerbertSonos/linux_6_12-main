// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Synaptics Incorporated
 *
 */

#if !defined(__VPP_RES_INFO_H__)
#define __VPP_RES_INFO_H__

#include "hal_vpp.h"
#include "vpp_api.h"
#include "vpp_defines.h"

int syna_get_res_index(int active_width, int active_height, int scan, int freq, int fps);
RESOLUTION_INFO* syna_get_res_info(int res_index);

#endif
