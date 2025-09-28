// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/export.h>
#include <linux/delay.h>
#include "hal_vpp_wrap.h"
#include "hal_vpp_ntz.h"
#include "vpp_defines.h"
#include "syna_lcdc_dev.h"

int NTZ_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew, void **pold)
{
	VBUF_INFO *pVppDesc = pnew;
	VPP_VBUF *pvbufinfo = pVppDesc->pVppVbufInfo_virt;

	switch (pvbufinfo->m_srcfmt) {
		case SRCFMT_ARGB32:
			pvbufinfo->m_srcfmt = SYNA_LCDC_PIXFMT_ARGB32;
		break;

		case SRCFMT_ARGB32_PM:
			pvbufinfo->m_srcfmt = SYNA_LCDC_PIXFMT_ARGB32_PM;
		break;

		case SRCFMT_RGB565:
			pvbufinfo->m_srcfmt = SYNA_LCDC_PIXFMT_RGB565;
		break;

		case SRCFMT_XRGB32:
			pvbufinfo->m_srcfmt = SYNA_LCDC_PIXFMT_ARGB32;
		break;

		default:
			pr_err("unsupported Format %d\n",pvbufinfo->m_srcfmt);
		break;
	}

	syna_lcdc_pushframe(planeID, pvbufinfo);

	return MV_VPP_OK;
}

int NTZ_MV_VPP_Init(void)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPP_DeInit(void)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_Create(int base_addr, int *handle)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg,
	const int *pzorder_cfg, const int *pvoutport_cfg, const int *pfeature_cfg)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_SetDisplayMode(int planeID, int mode)
{
	return MV_VPP_OK;

}

int NTZ_MV_VPPOBJ_DisplayFrame(int planeID, void *frame)
{
	void **pold = 0;

	return NTZ_MV_VPPOBJ_SetStillPicture(planeID, frame, pold);
}

int NTZ_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr, bool isFromISR)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_SetPlaneMute(int planeID, int mute)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_Suspend(int enable)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_SetHdmiTxControl(int enable)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_Stop(void)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_Reset(void)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_RecycleFrames(int planeID)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_Destroy(void)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	syna_lcdc_irq(IntSts);

	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam)
{
	return MV_VPP_OK;
}


INT NTZ_MV_VPPOBJ_SetFormat (INT cpcbID, VPP_DISP_OUT_PARAMS *pDispParams)
{
	return MV_VPP_OK;

}

int NTZ_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock)
{
	return MV_VPP_OK;
}

int NTZ_MV_VPPOBJ_GetDispOutParams(VPP_DISP_OUT_PARAMS *pdispParams, int size)
{
	return  MV_VPP_OK;
}

int NTZ_MV_VPP_LoadConfigTable(ENUM_VOUT_ID voutid, int Id, void *pConfig)
{
	if (VOUT_TFT == voutid) {
		SYNA_LCDC_CONFIG *lcdcconfig = (SYNA_LCDC_CONFIG*) pConfig;

		syna_lcdc_hw_config(Id, (void*)lcdcconfig);
	}
	return MV_VPP_OK;
}
