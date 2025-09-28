// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/export.h>
#include <linux/delay.h>
#include "hal_vpp.h"
#include "hal_vpp_tz.h"
#include "hal_vpp_ntz.h"
#include "hal_vpp_wrap.h"
#include "tee_ca_vpp.h"
#include "drv_dhub.h"
#include "drv_vpp.h"

#define HDMITX_HPD_DEBOUNCE 60

static int curr_sink_conn_state = -1;

//Callback to wait for vpp vsync interrupt
static wait_for_vpp_vsync_cb_func wait_for_vpp_vsync_cb;

//Callback to wait for HDMI connection status change
static wait_for_hdmi_hpd_cb_func wait_for_hdmi_hpd_cb;

void wrap_MV_VPP_RegisterWaitForVppVsyncCb(wait_for_vpp_vsync_cb_func cb_func)
{
	wait_for_vpp_vsync_cb = cb_func;
}

void wrap_MV_VPP_RegisterWaitForHdmiHpd(wait_for_hdmi_hpd_cb_func cb_func)
{
	wait_for_hdmi_hpd_cb = cb_func;
}

int wrap_MV_VPP_InitVPPS(ENUM_TA_UUID_TYPE uuidType, unsigned int *vpp_init_parm)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled) {
		if (vpp_init_parm != NULL)
			retVal = TZ_MV_VPP_InitVPPS(uuidType, vpp_init_parm);
	} else {
		retVal = NTZ_MV_VPP_Init( );
	}

	return retVal;
}

int wrap_MV_VPPOBJ_GetCPCBOutputResolution(int cpcbID, int *pResID)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled) {
		if (pResID)
			retVal = TZ_MV_VPPOBJ_GetCPCBOutputResolution(cpcbID, pResID);
	}

	return retVal;
}

int wrap_MV_VPPOBJ_GetResolutionDescription(int ResId, VPP_RESOLUTION_DESCRIPTION *pResDesc)
{
	int retVal = 0;
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if(hDhubCtx->isTeeEnabled) {
		if (pResDesc)
			retVal = TZ_MV_VPPOBJ_GetResolutionDescription(ResId, pResDesc);
	}

	return retVal;
}

int wrap_MV_VPP_GetCurrentHDCPVersion(int *pHDCPVersion)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled ) {
		if (pHDCPVersion)
			retVal = TZ_MV_VPP_GetCurrentHDCPVersion(pHDCPVersion);
	}

	return retVal;
}

int wrap_MV_VPPOBJ_SetStillPicture(int planeID, void *pnew)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;
	void **pold = 0;

	if (hDhubCtx->isTeeEnabled) {
		if (pnew)
			retVal = TZ_MV_VPPOBJ_SetStillPicture(planeID, pnew, pold);
	} else {
		if (pnew)
			retVal = NTZ_MV_VPPOBJ_SetStillPicture(planeID, pnew, pold);
	}

	return retVal;
}

int wrap_MV_VPP_WaitVsync(int Id) {
	int retVal = 0;

	if (wait_for_vpp_vsync_cb)
		retVal = wait_for_vpp_vsync_cb(Id);

	return retVal;
}

int wrap_MV_VPP_Init(VPP_INIT_PARM *vpp_init_parm)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled) {
		if (vpp_init_parm != NULL)
			retVal = TZ_MV_VPP_Init(TA_UUID_FASTLOGO, vpp_init_parm);
	} else {
		retVal = NTZ_MV_VPP_Init();
	}

	return retVal;
}

int wrap_MV_VPP_DeInit(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPP_DeInit();
	else
		retVal = NTZ_MV_VPP_DeInit();

	return retVal;
}

int wrap_MV_VPPOBJ_Create(int base_addr, int *handle)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Create(base_addr, handle);

	return retVal;
}

int wrap_MV_VPPOBJ_Config(const int *pvinport_cfg, const int *pdv_cfg,
	const int *pzorder_cfg, const int *pvoutport_cfg, const int *pfeature_cfg)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled) {
		if (pvinport_cfg && pdv_cfg && pzorder_cfg && pvoutport_cfg)
			retVal = TZ_MV_VPPOBJ_ObjConfig(pvinport_cfg, pdv_cfg, pzorder_cfg,
						pvoutport_cfg, pfeature_cfg);
		else
			retVal = TZ_MV_VPPOBJ_Config(pvinport_cfg, pdv_cfg, pzorder_cfg,
						pvoutport_cfg, pfeature_cfg);
    }

	return retVal;
}

int wrap_MV_VPPOBJ_SetCPCBOutputResolution(int cpcbID, int resID, int bit_depth)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetCPCBOutputResolution(cpcbID, resID, bit_depth);

	return retVal;
}

int wrap_MV_VPPOBJ_SetHdmiVideoFmt(int color_fmt, int bit_depth, int pixel_rept)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetHdmiVideoFmt(color_fmt, bit_depth, pixel_rept);

	return retVal;
}

int wrap_MV_VPPOBJ_OpenDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_OpenDispWindow(planeID, win, attr);

	return retVal;
}

int wrap_MV_VPPOBJ_SetDisplayMode(int planeID, int mode)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetDisplayMode(planeID, mode);

	return retVal;
}

int wrap_MV_VPPOBJ_DisplayFrame(int planeID, void *frame)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_DisplayFrame(planeID, frame);
	else
		retVal = NTZ_MV_VPPOBJ_DisplayFrame(planeID, frame);

	return retVal;
}

int wrap_MV_VPPOBJ_SetRefWindow(int planeID, VPP_WIN *win)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetRefWindow(planeID, win);

	return retVal;
}

int wrap_MV_VPPOBJ_SetRefWindowFromISR(int planeID, VPP_WIN *win)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetRefWindowFromISR(planeID, win);

	return retVal;
}

int wrap_MV_VPPOBJ_ChangeDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr, bool isFromISR)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_ChangeDispWindow(planeID, win, attr, isFromISR);

	return retVal;
}

int wrap_MV_VPPOBJ_SetPlaneMute(int planeID, int mute)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetPlaneMute(planeID, mute);

	return retVal;
}

int wrap_MV_VPPOBJ_ChangeZOrder(int handle, int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;
	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_ChangeZOrder(cpcbID, pZorderCtrl);
	return retVal;
}

int wrap_MV_VPPOBJ_GetZOrder(int handle, int cpcbID, VPP_ZORDER_CTRL *pZorderCtrl)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;
	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_GetZOrder(cpcbID, pZorderCtrl);
	return retVal;
}

int wrap_MV_VPPOBJ_Suspend(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Suspend(enable);

	return retVal;
}

int wrap_MV_VPPOBJ_SetHdmiTxControl(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SetHdmiTxControl(enable);

	return retVal;
}

int wrap_MV_VPPOBJ_RecycleFrames(int planeID)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int frame = 0;

	if (hDhubCtx->isTeeEnabled)
		frame = TZ_MV_VPPOBJ_RecycleFrames(planeID);

	return frame;
}

int wrap_MV_VPPOBJ_Stop(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Stop();

	return retVal;
}

int wrap_MV_VPPOBJ_Reset(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Reset();

	return retVal;
}

int wrap_MV_VPPOBJ_Destroy(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_Destroy();

	return retVal;
}

int wrap_MV_VPPOBJ_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_IsrHandler(MsgId, IntSts);
	else
		retVal = NTZ_MV_VPPOBJ_IsrHandler(MsgId, IntSts);

	return retVal;
}

int wrap_MV_VPPOBJ_SemOper(int cmd_id, int sem_id, int *pParam)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_SemOper(cmd_id, sem_id, pParam);

	return retVal;
}

int wrap_MV_VPPOBJ_EnableHdmiAudioFmt(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_EnableHdmiAudioFmt(enable);

	return retVal;
}

int wrap_MV_VPPOBJ_GetHDMIRawEdid(VPP_HDMI_RAW_EDID *pRawEdid)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_GetHDMIRawEdid(pRawEdid);

	return retVal;
}

int wrap_MV_VPPOBJ_InvokePassShm_Helper(void *pBuffer, unsigned int shmCmdId,
		unsigned int sBufferSize)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	int retVal = 0;

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_InvokePassShm_Helper(pBuffer, shmCmdId, sBufferSize);

	return retVal;
}


INT wrap_MV_VPPOBJ_SetFormat (INT cpcbID, VPP_DISP_OUT_PARAMS *pDispParams)
{
	HRESULT Ret = MV_VPP_OK;
	UINT32 pDispParamsData[VPP_SETFORMAT_SIZE];
	DHUB_CTX *hDhubCtx = \
			(DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	memcpy(&pDispParamsData[0], &cpcbID, sizeof(INT));
	memcpy(&pDispParamsData[1], pDispParams, sizeof(VPP_DISP_OUT_PARAMS));
	if (hDhubCtx->isTeeEnabled)
		Ret = TZ_MV_VPPOBJ_InvokePassShm_Helper(pDispParamsData,
												VPP_SETFORMAT,
												VPP_SETFORMAT_SIZE);

	return Ret;
}

int wrap_MV_VPPOBJ_GetCPCBOutputPixelClock(int resID, int *pixel_clock)
{
	HRESULT Ret = MV_VPP_OK;

	Ret = TZ_MV_VPPOBJ_GetCPCBOutputPixelClock(resID, pixel_clock);
	return Ret;
}

int wrap_MV_VPPOBJ_GetDispOutParams(VPP_DISP_OUT_PARAMS *pdispParams, int size)
{
	return  TZ_MV_VPPOBJ_GetDispOutParams(pdispParams, size);
}

void wrap_MV_VPP_MIPI_Reset(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	void *hVppCtx = (void *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_VPP);

	if (hDhubCtx->isTeeEnabled)
		drv_mipi_reset(hVppCtx, enable);
}

void wrap_MV_VPP_HDMITX_5v_set(int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
	void *hVppCtx = (void *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_VPP);

	if (hDhubCtx->isTeeEnabled)
		drv_hdmitx_5v_set(hVppCtx, enable);
}

int wrap_MV_VPP_LoadConfigTable(ENUM_VOUT_ID voutid, int Id, void *pConfig)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (!hDhubCtx->isTeeEnabled)
		NTZ_MV_VPP_LoadConfigTable(voutid, Id, pConfig);
	else
		TZ_MV_VPPOBJ_LoadMipiConfig(pConfig);

	return MV_VPP_OK;
}

int wrap_MV_VPP_iSTeeEnabled(void)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	return hDhubCtx->isTeeEnabled;
}

int wrap_MV_VPPOBJ_SetDispOutParams(VPP_DISP_OUT_PARAMS *pDispParams, int cpcbID)
{
	HRESULT Ret = MV_VPP_OK;
	UINT32 aDispParamsData[VPP_SETFORMAT_SIZE];
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	memcpy(&aDispParamsData[0], &cpcbID, sizeof(INT));
	memcpy(&aDispParamsData[1], pDispParams, sizeof(VPP_DISP_OUT_PARAMS));

	if (hDhubCtx->isTeeEnabled)
		Ret = TZ_MV_VPPOBJ_SetDispOutParams(aDispParamsData, VPP_SETFORMAT_SIZE);

	return  Ret;
}

int wrap_MV_VPPOBJ_GetHPDStatus(unsigned char *pHpdStatus, unsigned char sync)
{
	/* keep local copy synced after HPD to aviod unnecessary TA-CA context switch.
	 * If explicitely requested using sync param, read/sync HPD status from TA
	 * drm core have option to  use polling for HPD status.
	 */
	int retVal = 0;

	if (curr_sink_conn_state < 0) {
		/*force sync the status before first read*/
		sync = 1;
	}

	if (sync) {
		DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);
		if (hDhubCtx->isTeeEnabled) {
			retVal = TZ_MV_VPPOBJ_GetHPDStatus(pHpdStatus);
			if (retVal) {
				avio_error("error getting HDMI device connection status from TA,assume connected ret:%d\n",retVal);
				return retVal;
			}
		}
		avio_debug("setting hdmi connected to : %d\n", *pHpdStatus);
		curr_sink_conn_state = *pHpdStatus;
	} else {
		/* wrap_MV_VPPOBJ_GetHPDStatus is called with sync after vpp init,
		 * so by the time drm checks status, curr_sink_conn_state will be synced
		 */
		*pHpdStatus = curr_sink_conn_state ? true : false;
	}
	return retVal;
}

int wrap_MV_VPP_WaitHdmiConnChange(unsigned char *pSinkConnected)
{
	int retVal = -1;

	if (wait_for_hdmi_hpd_cb)
		retVal = wait_for_hdmi_hpd_cb();

	if (!retVal) {
		/* sync HPD status internally after allowing debouncing period.
		 * in case drm reverts to polling for HPD status, this will help
		 * to minimize ca-ta switch
		 */
		msleep(HDMITX_HPD_DEBOUNCE);
		retVal = wrap_MV_VPPOBJ_GetHPDStatus(pSinkConnected, true);
		if (retVal) {
			avio_error("error getting HDMI device connection status, assume connected\n");
			return retVal;
		}
	}

	/* return 0 if HPD state change happened */
	return retVal;
}

int wrap_MV_VPPOBJ_GetBlockStatus(ENUM_VPP_BLOCK blkId, int blkSubID, int *status)
{
	HRESULT Ret = MV_VPP_OK;
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled)
		Ret = TZ_MV_VPPOBJ_GetBlockStatus(blkId, blkSubID, status);

	return Ret;
}

int wrap_MV_VPPOBJ_GetDispWindow(int planeID, VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	HRESULT Ret = MV_VPP_OK;
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled)
		Ret = TZ_MV_VPPOBJ_GetDispWindow(planeID, win, attr);

	return Ret;
}

int wrap_MV_VPPOBJ_GetRefWindow(int planeID, VPP_WIN *win)
{
	HRESULT Ret = MV_VPP_OK;
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled)
		Ret = TZ_MV_VPPOBJ_GetRefWindow(planeID, win);

	return Ret;
}

int wrap_MV_VPPOBJ_GetHDMISinkFeatureMap(VPP_HDMI_SINK_CAPS *pSinkFeatureMap)
{
	int retVal = 0;
	DHUB_CTX *hDhubCtx = (DHUB_CTX *) avio_sub_module_get_ctx(AVIO_MODULE_TYPE_DHUB);

	if (hDhubCtx->isTeeEnabled)
		retVal = TZ_MV_VPPOBJ_GetHDMISinkFeatureMap(pSinkFeatureMap);

	return retVal;
}

EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetDispWindow);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetRefWindow);
EXPORT_SYMBOL(wrap_MV_VPP_InitVPPS);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetCPCBOutputResolution);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetResolutionDescription);
EXPORT_SYMBOL(wrap_MV_VPP_GetCurrentHDCPVersion);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetStillPicture);
EXPORT_SYMBOL(wrap_MV_VPP_WaitVsync);
EXPORT_SYMBOL(wrap_MV_VPP_Init);
EXPORT_SYMBOL(wrap_MV_VPP_DeInit);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Create);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Config);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetCPCBOutputResolution);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetHdmiVideoFmt);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_OpenDispWindow);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetDisplayMode);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_DisplayFrame);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetRefWindow);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetRefWindowFromISR);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_ChangeDispWindow);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetPlaneMute);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_ChangeZOrder);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetZOrder);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Suspend);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetHdmiTxControl);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_RecycleFrames);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Stop);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Reset);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_Destroy);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_IsrHandler);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SemOper);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_EnableHdmiAudioFmt);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_InvokePassShm_Helper);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetFormat);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetCPCBOutputPixelClock);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetDispOutParams);
EXPORT_SYMBOL(wrap_MV_VPP_MIPI_Reset);
EXPORT_SYMBOL(wrap_MV_VPP_LoadConfigTable);
EXPORT_SYMBOL(wrap_MV_VPP_iSTeeEnabled);
EXPORT_SYMBOL(wrap_MV_VPP_RegisterWaitForVppVsyncCb);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_SetDispOutParams);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetHDMIRawEdid);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetHPDStatus);
EXPORT_SYMBOL(wrap_MV_VPP_RegisterWaitForHdmiHpd);
EXPORT_SYMBOL(wrap_MV_VPP_WaitHdmiConnChange);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetBlockStatus);
EXPORT_SYMBOL(wrap_MV_VPPOBJ_GetHDMISinkFeatureMap);
EXPORT_SYMBOL(wrap_MV_VPP_HDMITX_5v_set);
