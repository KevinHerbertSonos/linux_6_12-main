// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#ifndef _VPP_FB_H_
#define _VPP_FB_H_

#include "vpp_mem.h"
#include "vpp_config.h"
//#include "vpp_defines.h"

int MV_VPP_FB_Init(VPP_MEM_LIST *mem_list);
void MV_VPP_FB_Deinit(void);
void MV_VPP_FB_ConfigParams(vpp_config_params* param);
void MV_VPP_FB_DisplayFrame(VPP_MEM *shm_handle, int x,
			int y, int w, int h, int stride);
void MV_VPP_FB_GetInputFrameSize(int *width, int *height);

#endif //_VPP_FB_H_
