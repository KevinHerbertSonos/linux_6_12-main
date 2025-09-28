// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#ifndef _HAL_VPP_NTZ_H_
#define _HAL_VPP_NTZ_H_
/** hal_vpp_ntz.h for non TZ
 */

#include <linux/bitfield.h>
#include "hal_vpp.h"

#ifdef __cplusplus
extern "C" {
#endif

int NTZ_MV_VPP_Init(void);
int NTZ_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew, void **pold);
int NTZ_MV_VPP_DeInit(void);
int NTZ_MV_VPPOBJ_SetPlaneMute(int planeID, int mute);
int NTZ_MV_VPPOBJ_Suspend(int enable);
int NTZ_MV_VPPOBJ_RecycleFrames(int planeID);
int NTZ_MV_VPPOBJ_Stop(void);
int NTZ_MV_VPPOBJ_Destroy(void);
int NTZ_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts);
int NTZ_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock);
int NTZ_MV_VPP_LoadConfigTable(ENUM_VOUT_ID voutid, int Id, void *pConfig);
int NTZ_MV_VPPOBJ_DisplayFrame(int planeID, void *frame);
int NTZ_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc);
int NTZ_MV_VPPOBJ_Create(int base_addr, int *handle);
int NTZ_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg,
	const int *pzorder_cfg, const int *pvoutport_cfg, const int *pfeature_cfg);
int NTZ_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth);
int NTZ_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept);
int NTZ_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr);
int NTZ_MV_VPPOBJ_SetDisplayMode(int planeID, int mode);
int NTZ_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win);
int NTZ_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr, bool isFromISR);
int NTZ_MV_VPPOBJ_Reset(void);
int NTZ_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam);
int NTZ_MV_VPPOBJ_SetHdmiTxControl(int enable);
INT NTZ_MV_VPPOBJ_SetFormat (INT cpcbID, VPP_DISP_OUT_PARAMS *pDispParams);
int NTZ_MV_VPPOBJ_GetDispOutParams(VPP_DISP_OUT_PARAMS *pdispParams, int size);
#ifdef __cplusplus
}
#endif
/** _HAL_VPP_NTZ_H_
 */
#endif // _HAL_VPP_NTZ_H_
/** ENDOFFILE: hal_vpp_ntz.h
 */
