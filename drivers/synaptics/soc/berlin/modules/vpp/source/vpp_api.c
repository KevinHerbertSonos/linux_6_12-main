// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include "vpp_api.h"
#include "vpp_isr.h"
#include "hal_dhub_wrap.h"
#include "hal_vpp_wrap.h"
#include "avio_sub_module.h"
#include "vpp_res_info.h"

#include "linux/delay.h"

#define VPP_CPCBTG_RESET_WAIT_TIME_MS	(100)
#define VPP_CPCBTG_RESET_LOOP_DELAY_MS	(10)

vpp_plane_info g_vpp_curr_plane_info[MAX_NUM_PLANES];
static VPP_DISP_OUT_PARAMS curr_disp_res_params[MAX_NUM_CPCBS];
static bool IsCpcbResolutionSet[MAX_NUM_CPCBS];
vpp_config_params vpp_config_param = { 0 };

int MV_VPP_make_frame_data(unsigned int iVideo, unsigned int *pStartAddr,
			  unsigned int uiPicH, unsigned int uiLineV, unsigned int uiWidth,
			  unsigned int uiHeight, unsigned int uiPicA, unsigned int uiPicB)
{
	unsigned int *Ptr;
	unsigned int VLines, VRep, HS, HRep, PixVal, VBlock, HBlock, PixelPerWord;
	int Ret = 0;

	if (!pStartAddr) {
		pr_err("invalid input parameters\n");
		return -1;
	}

	/**
	 * make the whole frame data
	 */
	Ptr = pStartAddr;

	if (iVideo)
		PixelPerWord = 2;
	else
		PixelPerWord = 1;

	VBlock = uiLineV;

	for (VLines = 0; VLines < uiHeight; VLines += VBlock) {
		if ((uiHeight - VLines) < VBlock)
			VBlock = uiHeight - VLines;

		for (VRep = 0; VRep < VBlock; VRep++) {
			HBlock = uiPicH * PixelPerWord;

			for (HS = 0; HS < uiWidth; HS += HBlock) {
				if ((VLines / uiLineV +
						HS/(uiPicH * PixelPerWord))
						& 0x1)
					PixVal = uiPicB;
				else
					PixVal = uiPicA;

				if ((uiWidth - HS) < HBlock)
					HBlock = uiWidth - HS;

				for (HRep = 0; HRep < HBlock/PixelPerWord;
						HRep++)
					*Ptr++ = PixVal;
			}
		}
	}
	return Ret;
}

void __weak MV_VPP_UpdatePlane_Mute(ENUM_PLANE_ID plane_id)
{
	return;
}

void __weak MV_VPP_UpdatePlane_Zorder(int cpcb_id, ENUM_PLANE_ID plane_id)
{
	return;
}

bool IS_VPP_SRCWIN_VALID(VPP_WIN *pSrcWin)
{
	if ((pSrcWin->width > VPP_WIN_MIN_INPUT_WIDTH) &&
		(pSrcWin->height > VPP_WIN_MIN_INPUT_HEIGHT))
			return true;

	return false;
}

bool IS_VPP_DSTWIN_VALID(VPP_WIN *pDstWin)
{
	if ((pDstWin->width > VPP_WIN_MIN_OUTPUT_WIDTH) &&
		(pDstWin->height > VPP_WIN_MIN_OUTPUT_HEIGHT) &&
		(pDstWin->x < VPP_WIN_MAX_OUTPUT_X) &&
		(pDstWin->y < VPP_WIN_MAX_OUTPUT_Y))
			return true;

	return false;
}

int MV_VPP_UpdatePlane_Refwin(ENUM_PLANE_ID plane_id, bool isFromISR)
{
	int ret = MV_VPP_OK;
	VPP_WIN fb_win;
	vpp_plane_info *c_pinfo = &g_vpp_curr_plane_info[plane_id];

	if (c_pinfo->user_ref_win) {
		fb_win.x = c_pinfo->ref_win.x;
		fb_win.y = c_pinfo->ref_win.y;
		fb_win.width  = c_pinfo->ref_win.width;
		fb_win.height = c_pinfo->ref_win.height;
	} else {
		fb_win.x = 0;
		fb_win.y = 0;
		fb_win.width  = c_pinfo->frame_size.width;
		fb_win.height = c_pinfo->frame_size.height;
	}

	pr_debug("Change Refwin: plane-%d/%d, (%d,%d,%d,%d)\n", plane_id, isFromISR,
				fb_win.x, fb_win.y,	fb_win.width, fb_win.height);

	if (IS_VPP_SRCWIN_VALID(&fb_win)) {
		if (isFromISR)
			ret = wrap_MV_VPPOBJ_SetRefWindowFromISR(plane_id, &fb_win);
		else
			ret = wrap_MV_VPPOBJ_SetRefWindow(plane_id, &fb_win);
	}

	return ret;
}

int MV_VPP_UpdatePlane_Dispwin(ENUM_PLANE_ID plane_id, bool isFromISR)
{
	int ret = MV_VPP_OK;
	vpp_plane_info *c_pinfo = &g_vpp_curr_plane_info[plane_id];
	pr_debug("Change Dispwin: plane-%d/%d, (%d,%d,%d,%d), (%d,%d,%d)\n",
				plane_id, isFromISR,
				c_pinfo->disp_win.x, c_pinfo->disp_win.y,
				c_pinfo->disp_win.width, c_pinfo->disp_win.height,
				c_pinfo->win_attr.bgcolor, c_pinfo->win_attr.alpha,
				c_pinfo->win_attr.globalAlphaFlag);

	if (IS_VPP_DSTWIN_VALID(&c_pinfo->disp_win)) {
		ret = wrap_MV_VPPOBJ_ChangeDispWindow(plane_id,
			&c_pinfo->disp_win, &c_pinfo->win_attr, isFromISR);
	}

	return ret;
}

int MV_VPP_UpdatePlaneInfoFromISR(void)
{
	ENUM_PLANE_ID plane_id;
	int ret = MV_VPP_OK;

	for (plane_id = FIRST_PLANE; plane_id < MAX_NUM_PLANES; plane_id++) {
		vpp_plane_info *c_pinfo = &g_vpp_curr_plane_info[plane_id];
		if (IS_VPP_PLANE_INFO_CFLAG_SET(c_pinfo->change_flag, REFWIN)) {
			if ( (ret = MV_VPP_UpdatePlane_Refwin(plane_id, 1)) ==
					 VPP_TA_E_SWSTATEWRONG) {
				pr_debug("%s:%d: MV_VPP_UpdatePlane_Refwin FAILED(%d) :"
				"(%d,%d,%d,%d)\n", __func__, __LINE__, plane_id,
				c_pinfo->ref_win.x, c_pinfo->ref_win.y,
				c_pinfo->ref_win.width, c_pinfo->ref_win.height);
				break;
			}
			VPP_PLANE_INFO_CFLAG_CLEAR(c_pinfo->change_flag, REFWIN);
		}
		if (IS_VPP_PLANE_INFO_CFLAG_SET(c_pinfo->change_flag, DISPWIN)) {
			if ( (ret = MV_VPP_UpdatePlane_Dispwin(plane_id, 1)) ==
					VPP_TA_E_SWSTATEWRONG) {
				pr_debug("%s:%d: MV_VPP_UpdatePlane_Dispwin FAILED(%d) :"
				"(%d,%d,%d,%d)\n", __func__, __LINE__, plane_id,
				c_pinfo->disp_win.x, c_pinfo->disp_win.y,
				c_pinfo->disp_win.width, c_pinfo->disp_win.height);
				break;
			}
			VPP_PLANE_INFO_CFLAG_CLEAR(c_pinfo->change_flag, DISPWIN);
		}
	}

	return ret;
}

void MV_VPP_GetPlaneInfo(ENUM_PLANE_ID plane_id, vpp_plane_info *p_pinfo)
{
	if (plane_id >= FIRST_PLANE && plane_id < MAX_NUM_PLANES)
		memcpy(p_pinfo, &g_vpp_curr_plane_info[plane_id],
				sizeof(vpp_plane_info));
}

void MV_VPP_UpdatePlaneInfo(ENUM_PLANE_ID plane_id, vpp_plane_info *p_pinfo)
{
	vpp_plane_info *c_pinfo = &g_vpp_curr_plane_info[plane_id];

	c_pinfo->cpcb_id = p_pinfo->cpcb_id;
	if (IS_VPP_PLANE_INFO_CFLAG_SET(p_pinfo->change_flag, REFWIN) &&
		(c_pinfo->ref_win.x != p_pinfo->ref_win.x ||
		c_pinfo->ref_win.y != p_pinfo->ref_win.y ||
		c_pinfo->ref_win.width != p_pinfo->ref_win.width ||
		c_pinfo->ref_win.height != p_pinfo->ref_win.height)) {
		pr_debug("%s:%d: user changed ref_win(%d) : (%d,%d,%d,%d)"
				"-> (%d,%d,%d,%d)\n", __func__, __LINE__, plane_id,
				c_pinfo->ref_win.x, c_pinfo->ref_win.y,
				c_pinfo->ref_win.width, c_pinfo->ref_win.height,
				p_pinfo->ref_win.x, p_pinfo->ref_win.y,
				p_pinfo->ref_win.width, p_pinfo->ref_win.height);
		c_pinfo->user_ref_win = 1;
		c_pinfo->ref_win.x = p_pinfo->ref_win.x;
		c_pinfo->ref_win.y = p_pinfo->ref_win.y;
		c_pinfo->ref_win.width = p_pinfo->ref_win.width;
		c_pinfo->ref_win.height = p_pinfo->ref_win.height;
		VPP_PLANE_INFO_CFLAG_SET(c_pinfo->change_flag, REFWIN);
	}

	if (IS_VPP_PLANE_INFO_CFLAG_SET(p_pinfo->change_flag, DISPWIN) &&
		(c_pinfo->disp_win.x != p_pinfo->disp_win.x ||
		c_pinfo->disp_win.y != p_pinfo->disp_win.y ||
		c_pinfo->disp_win.width != p_pinfo->disp_win.width ||
		c_pinfo->disp_win.height != p_pinfo->disp_win.height)) {
		pr_debug("%s:%d: user changed disp_win(%d) : (%d,%d,%d,%d)"
				"-> (%d,%d,%d,%d)\n", __func__, __LINE__, plane_id,
				c_pinfo->disp_win.x, c_pinfo->disp_win.y,
				c_pinfo->disp_win.width, c_pinfo->disp_win.height,
				p_pinfo->disp_win.x, p_pinfo->disp_win.y,
				p_pinfo->disp_win.width, p_pinfo->disp_win.height);
		c_pinfo->user_disp_win = 1;
		c_pinfo->disp_win.x = p_pinfo->disp_win.x;
		c_pinfo->disp_win.y = p_pinfo->disp_win.y;
		c_pinfo->disp_win.width = p_pinfo->disp_win.width;
		c_pinfo->disp_win.height = p_pinfo->disp_win.height;
		VPP_PLANE_INFO_CFLAG_SET(c_pinfo->change_flag, DISPWIN);
	}

	if (IS_VPP_PLANE_INFO_CFLAG_SET(p_pinfo->change_flag, ZPOS) &&
			c_pinfo->zpos != p_pinfo->zpos) {
		pr_debug("%s:%d: user changed zpos(%d:%d) : (%d)->(%d)\n",
				__func__, __LINE__, p_pinfo->cpcb_id, plane_id,
				c_pinfo->zpos, p_pinfo->zpos);
		c_pinfo->zpos = p_pinfo->zpos;
		MV_VPP_UpdatePlane_Zorder(p_pinfo->cpcb_id, plane_id);
	}

	if (IS_VPP_PLANE_INFO_CFLAG_SET(p_pinfo->change_flag, MUTE) &&
			c_pinfo->mute != p_pinfo->mute) {
		pr_debug("%s:%d: user changed mute(%d) : (%d)->(%d)\n",
				__func__, __LINE__, plane_id, c_pinfo->mute, p_pinfo->mute);
		c_pinfo->mute = p_pinfo->mute;
		MV_VPP_UpdatePlane_Mute(plane_id);
	}

	if (IS_VPP_PLANE_INFO_CFLAG_SET(p_pinfo->change_flag, ALPHA) &&
		(c_pinfo->win_attr.alpha != p_pinfo->win_attr.alpha ||
		 c_pinfo->win_attr.globalAlphaFlag != p_pinfo->win_attr.globalAlphaFlag)) {
		pr_debug("%s:%d: user changed disp_win_attr(%d) : (%x,%x,%x)"
				"-> (%x,%x,%x)\n", __func__, __LINE__, plane_id,
				c_pinfo->win_attr.bgcolor, c_pinfo->win_attr.alpha,
				c_pinfo->win_attr.globalAlphaFlag, p_pinfo->win_attr.bgcolor,
				p_pinfo->win_attr.alpha, p_pinfo->win_attr.globalAlphaFlag);
		c_pinfo->win_attr.alpha = p_pinfo->win_attr.alpha;
		c_pinfo->win_attr.globalAlphaFlag = p_pinfo->win_attr.globalAlphaFlag;
		MV_VPP_UpdatePlane_Dispwin(plane_id, 0);
	}

	if (IS_VPP_PLANE_INFO_CFLAG_SET(p_pinfo->change_flag, FRAMESIZE) &&
		(c_pinfo->frame_size.width != p_pinfo->frame_size.width ||
		 c_pinfo->frame_size.height != p_pinfo->frame_size.height)) {
		c_pinfo->frame_size.width = p_pinfo->frame_size.width;
		c_pinfo->frame_size.height = p_pinfo->frame_size.height;
		//Apply/use framesize as refwin until user defined refwin is provided
		if (c_pinfo->user_ref_win == 0)
			VPP_PLANE_INFO_CFLAG_SET(c_pinfo->change_flag, REFWIN);
	}
}

static void MV_VPP_GetFrameSize(ENUM_PLANE_ID plane_id, int *pWidth, int *pHeight)
{
	vpp_plane_info *c_pinfo = &g_vpp_curr_plane_info[plane_id];

	*pWidth = c_pinfo->frame_size.width;
	*pHeight = c_pinfo->frame_size.height;
}

void MV_VPP_GetOutResolutionSize(ENUM_CPCB_ID cpcbID, int *p_width, int *p_height)
{
	VPP_RESOLUTION_DESCRIPTION  ResDesc;
	int res;

	res = wrap_MV_VPPOBJ_GetResolutionDescription(curr_disp_res_params[cpcbID].uiResId, &ResDesc);
	if (!res) {
		*p_width = ResDesc.uiActiveWidth;
		*p_height = ResDesc.uiActiveHeight;
	} else {
		/* Default values to FB size in case of failure */
		MV_VPP_GetFrameSize(PLANE_GFX1, p_width, p_height);
	}
}

void MV_VPP_GetInputFrameSize(ENUM_PLANE_ID plane_id, int *width, int *height)
{
	int res_width, res_height;

	MV_VPP_GetFrameSize(plane_id, width, height);
	MV_VPP_GetOutResolutionSize(CPCB_1, &res_width, &res_height);
	if (res_width < res_height)
		swap(*width, *height);
}

int MV_VPP_GetDispOutParams(int cpcbId, VPP_DISP_OUT_PARAMS* pDisplayOutParams)
{
	int result = MV_VPP_EBADPARAM;

	if (pDisplayOutParams != NULL) {
		memcpy(pDisplayOutParams, &curr_disp_res_params[cpcbId], sizeof(VPP_DISP_OUT_PARAMS));
		result = MV_VPP_OK;
	}

	return result;
}

void MV_VPP_InitDispWinSize(ENUM_PLANE_ID plane_id, int width, int height,
			VPP_WIN_ATTR win_attr)
{
	vpp_plane_info *c_pinfo = &g_vpp_curr_plane_info[plane_id];

	c_pinfo->disp_win.width = width;
	c_pinfo->disp_win.height = height;

	c_pinfo->win_attr.bgcolor = win_attr.bgcolor;
	c_pinfo->win_attr.alpha = win_attr.alpha;
	c_pinfo->win_attr.globalAlphaFlag = win_attr.globalAlphaFlag;
}

int MV_VPP_SetDisplayResolution(ENUM_CPCB_ID cpcbID,
		VPP_DISP_OUT_PARAMS dispParams, int bApply)
{
	int res = MV_VPP_OK;
	int pixel_clock;
	int status = -1;
	int wait_count = VPP_CPCBTG_RESET_WAIT_TIME_MS / VPP_CPCBTG_RESET_LOOP_DELAY_MS;

	if (curr_disp_res_params[cpcbID].uiResId != dispParams.uiResId ||
			curr_disp_res_params[cpcbID].uiDisplayMode != dispParams.uiDisplayMode ||
			curr_disp_res_params[cpcbID].uiBitDepth != dispParams.uiBitDepth ||
			curr_disp_res_params[cpcbID].uiColorFmt != dispParams.uiColorFmt) {

		if (IsCpcbResolutionSet[cpcbID]) {
			/* First : put CPCB TG to reset before setting new timing */
			res = wrap_MV_VPPOBJ_GetBlockStatus(VPP_BLOCK_CPCB_TG, cpcbID, &status);
			if (res) {
				pr_err("%s %d> CPCB Status get failed\n", __FUNCTION__, __LINE__);
				return res;
			}

			if (status != STATUS_INACTIVE) {
				res = dispParams.uiResId;
				dispParams.uiResId = RES_RESET;
				wrap_MV_VPPOBJ_SetFormat(cpcbID, &dispParams);
				dispParams.uiResId = res;

				/* Wait until CPCB TG is reset, otherwise timeout after 100 ms */
				do {
					res = wrap_MV_VPPOBJ_GetBlockStatus(VPP_BLOCK_CPCB_TG, cpcbID, &status);
					if (status == STATUS_INACTIVE)
						break;
					else
						msleep(VPP_CPCBTG_RESET_LOOP_DELAY_MS);

					wait_count--;
				} while (wait_count);
			}

			if (!wait_count) {
				pr_err("%s %d> Reset Failed\n", __FUNCTION__, __LINE__);
				return MV_VPP_EIOFAIL;
			}
		}
		memcpy(&curr_disp_res_params[cpcbID], &dispParams, sizeof(VPP_DISP_OUT_PARAMS));

		if (bApply) {
			IsCpcbResolutionSet[cpcbID] = 1;
			//SetDisplayWindow applied to all planes by SetFormat
			wrap_MV_VPPOBJ_GetCPCBOutputPixelClock(dispParams.uiResId, &pixel_clock);

			if ((MAX_NUM_CPCBS == 1) || \
				((cpcbID == CPCB_1) &&\
				(!IS_MIPI_ONLY_MODE(dispParams.uiDisplayMode))))
					res = VPP_Clock_Set_Rate(pixel_clock*1000);
			else
				res = VPP_Clock_Set_Rate_Ext(pixel_clock*1000);

			res = wrap_MV_VPPOBJ_SetFormat(cpcbID, &dispParams);
			if (res != MV_VPP_OK) {
				pr_err("%s:%d: wrap_MV_VPPOBJ_SetFormat FAILED, error: 0x%x\n",
					__func__, __LINE__, res);
			} else {
				pr_info("%s %d> resiD %d cpcbID %d pixel clock %d\n",
					__FUNCTION__, __LINE__, dispParams.uiResId,
					cpcbID, pixel_clock);
			}
		}
	}

	return res;
}

void MV_VPP_DisplayFrame(int uiPlaneId, int isVideoFormat, VBUF_INFO *pVppDesc)
{
	if (!VPP_Is_Recovery_Mode()) {
		if (isVideoFormat)
			wrap_MV_VPPOBJ_DisplayFrame(uiPlaneId, pVppDesc);
		else
			wrap_MV_VPPOBJ_SetStillPicture(uiPlaneId, pVppDesc);
	} else {
		VPP_VBUF *pVppVbufDesc;
		vpp_plane_info plane_info;

		pVppVbufDesc = pVppDesc->pVppVbufInfo_virt;

		//Update frame-size : substitute in absence of user defined refwin
		plane_info.change_flag = 0;
		plane_info.frame_size.width = pVppVbufDesc->m_content_width;
		plane_info.frame_size.height = pVppVbufDesc->m_content_height;
		VPP_PLANE_INFO_CFLAG_SET(plane_info.change_flag, FRAMESIZE);
		MV_VPP_UpdatePlaneInfo(uiPlaneId, &plane_info);

#ifndef VPP_ENABLE_USE_SET_STILL_PICTURE
		VPP_PushFrameToInputQueue(uiPlaneId,
			VPP_FRAMEQ_MSGT_DISPLAY_FRAME, pVppDesc);
#else
		VPP_PushFrameToInputQueue(uiPlaneId,
			VPP_FRAMEQ_MSGT_STILL_PICTURE, pVppDesc);
#endif
	}
}

int MV_VPP_SetHdmiTxControl(int enable)
{
	return wrap_MV_VPPOBJ_SetHdmiTxControl(enable);
}

static int VPP_Init_Normal_vpp_ta(vpp_config_params vpp_config_params)
{
	VPP_DISP_OUT_PARAMS pdispParams[MAX_NUM_CPCBS];
	unsigned int vppInitParam[2];
	int ret = MV_VPP_OK, i;

	if (!VPP_Is_Recovery_Mode()) {
		vppInitParam[0] = VPP_CA_INIT_MAGIC_NUM;
		vppInitParam[1] = 0;

		ret = wrap_MV_VPP_InitVPPS(TA_UUID_VPP, vppInitParam);

		if (ret) {
			pr_err("%s:%d InitVPPS FAILED, error: 0x%x\n", __func__, __LINE__, ret);
			return -ENODEV;
		}

		// Retrieve following details from VPP TA and initialize disp params
		ret = wrap_MV_VPPOBJ_GetDispOutParams(pdispParams, MAX_NUM_CPCBS * sizeof(VPP_DISP_OUT_PARAMS));
		if (!ret) {
			for (i = CPCB_1; i < MAX_NUM_CPCBS; i++) {
				MV_VPP_SetDisplayResolution(i, pdispParams[i], 0);
			}
		}
	}

	return ret;
}

int __weak wrap_VPP_Init_Recovery(VPP_MEM_LIST *vpp_shm_list,
			int is_ampless_boot, vpp_config_params vpp_config_param)
{
	return 0;
}

void __weak wrap_VPP_DeInit_Recovery(void)
{
	return;
}

int MV_VPP_Init(VPP_MEM_LIST *shm_list, vpp_config_params vpp_config_params)
{
	int res = 0;

	memcpy(&vpp_config_param, &vpp_config_params, sizeof(vpp_config_params));
	if (VPP_Is_Recovery_Mode()) {
		pr_info("MV_VPP_Init - Normal/Recovery - libfastlogo.ta \n");
		res = avio_sub_module_dhub_init();
		if (res) {
			pr_info("%s: dhub open failed: %x\n", __func__, res);
			return res;
		}
		VPP_EnableDhubInterrupt(true);

		res = wrap_VPP_Init_Recovery(shm_list, is_ampless_boot(), vpp_config_params);

		if (!res)
			VPP_CreateISRTask();
	} else {
		pr_info("MV_VPP_Init - Normal - libvpp.ta\n");
		res = VPP_Init_Normal_vpp_ta(vpp_config_params);
	}

	return res;
}

void MV_VPP_Deinit(void)
{
	if (VPP_Is_Recovery_Mode())
		VPP_EnableDhubInterrupt(false);
	wrap_MV_VPPOBJ_Destroy();
	if (VPP_Is_Recovery_Mode()) {
		VPP_StopISRTask();
		wrap_VPP_DeInit_Recovery();
	}

	wrap_MV_VPP_DeInit();
}

int __weak syna_get_res_index(int active_width, int active_height, int scan, int freq, int fps)
{
	return -1;
}

int MV_VPP_GetResIndex(int active_width, int active_height, int scan, int freq, int fps)
{
	return syna_get_res_index(active_width, active_height, scan, freq, fps);
}

RESOLUTION_INFO* __weak syna_get_res_info(int res_index)
{
	return NULL;
}

int MV_VPP_GetResInfo(int res_index, RESOLUTION_INFO *p_res_info)
{
	int ret_val = -1;
	RESOLUTION_INFO *p_temp_info = syna_get_res_info(res_index);

	if (p_temp_info) {
		memcpy(p_res_info, p_temp_info, sizeof(RESOLUTION_INFO));
		ret_val = 0;
	}

	return ret_val;
}

EXPORT_SYMBOL(MV_VPP_Init);
EXPORT_SYMBOL(MV_VPP_DisplayFrame);
EXPORT_SYMBOL(MV_VPP_make_frame_data);
EXPORT_SYMBOL(MV_VPP_GetInputFrameSize);
EXPORT_SYMBOL(MV_VPP_GetOutResolutionSize);
EXPORT_SYMBOL(MV_VPP_SetDisplayResolution);
EXPORT_SYMBOL(MV_VPP_SetHdmiTxControl);
EXPORT_SYMBOL(MV_VPP_GetResIndex);
EXPORT_SYMBOL(MV_VPP_GetResInfo);
EXPORT_SYMBOL(MV_VPP_GetDispOutParams);
EXPORT_SYMBOL(MV_VPP_UpdatePlaneInfo);
EXPORT_SYMBOL(MV_VPP_GetPlaneInfo);
