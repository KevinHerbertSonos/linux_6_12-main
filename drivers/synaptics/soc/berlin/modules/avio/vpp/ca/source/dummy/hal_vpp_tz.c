// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#include <linux/types.h>
#include "hal_vpp.h"
#include "hal_vpp_tz.h"

int __weak TZ_MV_VPP_InitVPPS(ENUM_TA_UUID_TYPE uuidType, unsigned int *vpp_init_parm)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetCPCBOutputResolution(int cpcbID, int *resID)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc)
{
	return 0;
}

int __weak TZ_MV_VPP_GetCurrentHDCPVersion(int *pHDCPVersion)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew, void **pold)
{
	return 0;
}

int __weak TZ_MV_VPP_Init(ENUM_TA_UUID_TYPE uuidType, VPP_INIT_PARM *vpp_init_parm)
{
	return 0;
}

int __weak TZ_MV_VPP_DeInit(void)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_Create(int base_addr, int *handle)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg,
	       	const int *pzorder_cfg, const int *pvoutport_cfg, const int *pfeature_cfg)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_ObjConfig(const int *pvinport_cfg, const int *pdv_cfg,
		const int *pzorder_cfg, const int *pvoutport_cfg, const int *pfeature_cfg)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetDisplayMode(int planeID, int mode)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_DisplayFrame(int planeID, void *frame)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetRefWindowFromISR(int planeID, VPP_WIN *win)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr, bool isFromISR)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetPlaneMute(int planeID, int mute)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_ChangeZOrder(int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetZOrder(int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_Suspend(int enable)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetHdmiTxControl(int enable)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_RecycleFrames(int planeID)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_Stop(void)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_Reset(void)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_Destroy(void)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_EnableHdmiAudioFmt(int enable)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_InvokePassShm_Helper(void *pBuffer, unsigned int shmCmdId,
	    	unsigned int sBufferSize)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetDispOutParams(VPP_DISP_OUT_PARAMS *pdispParams, int size)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetHDMIRawEdid(VPP_HDMI_RAW_EDID *pRawEdid)
{
	return 0;
}

void __weak drv_mipi_reset(void *h_vpp_ctx, int enable)
{
}

void __weak drv_hdmitx_5v_set(void *h_vpp_ctx, int enable)
{
}

int __weak TZ_MV_VPPOBJ_LoadMipiConfig(VPP_MIPI_LOAD_CONFIG *pConfigParams)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_SetDispOutParams(void *pdispParams, int size)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetHPDStatus(unsigned char *pHpdStatus)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetBlockStatus(ENUM_VPP_BLOCK blkId, int blkSubID, int *status)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetRefWindow(int planeID, VPP_WIN *win)
{
	return 0;
}

int __weak TZ_MV_VPPOBJ_GetHDMISinkFeatureMap(VPP_HDMI_SINK_CAPS *pSinkFeatureMap)
{
        return 0;
}
