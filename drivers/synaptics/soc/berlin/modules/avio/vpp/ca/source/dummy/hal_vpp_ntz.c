// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#include "hal_vpp.h"
#include "hal_vpp_ntz.h"

int __weak NTZ_MV_VPP_Init(void)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_DisplayFrame(int planeID, void *frame)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew, void **pold)
{
	return 0;
}

int __weak NTZ_MV_VPP_DeInit(void)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_SetPlaneMute(int planeID, int mute)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_Suspend(int enable)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_RecycleFrames(int planeID)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_Stop(void)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_Destroy(void)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	return 0;
}

int __weak NTZ_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock)
{
	return 0;
}

int __weak NTZ_MV_VPP_LoadConfigTable(ENUM_VOUT_ID voutid, int Id, void *pConfig)
{
	return 0;
}
