// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <media/v4l2-device.h>
#include <linux/string.h>
#include <linux/io.h>
#include "ispSS_memmap.h"
#include "com_type.h"
#include "ispMISC.h"
#include "mtr_isp_wrap.h"

#define PER_PAGE_PT_ENTRIES 1024

struct ISPSS_MTR_FixedData {
	int stride;
	int format;
	int bm_shy_bw;
	int bm_shy_pos;
	int buf_stride;
	int pm_enable;
	int shuffle_en;
	int bm_enable;
	int vm_enable;
};

static struct ISPSS_MTR_FixedData gIspMTR_FixedData = {
	.stride = 0,
	.format = 8,
	.bm_shy_bw = 0,
	.bm_shy_pos = 0,
	.buf_stride = 0,
	.pm_enable = 1,
	.shuffle_en = 1,
	.bm_enable = 0,
	.vm_enable = 1
};

static UINT32 ISPSS_MTR_SetMode(int format, int bits_per_pixel, UINT8 *mtr_mode_y,
		UINT8 *mtr_mode_uv)
{
	int ret = ISPSS_MTR_SUCCESS;

	switch (format) {
	case ISP_SRCFMT_YUV420SP_DWA:
		*mtr_mode_y = (UINT8)ISPSS_MTR_MODE_D10B_V1_Y;
		*mtr_mode_uv = (UINT8)ISPSS_MTR_MODE_D10B_V1_UV;
		break;
	case ISP_SRCFMT_YUV420SP:
		if (bits_per_pixel == 8) {
			*mtr_mode_y = (UINT8)ISPSS_MTR_MODE_8B_V1_Y;
			*mtr_mode_uv = (UINT8)ISPSS_MTR_MODE_8B_V1_UV;
		} else if (bits_per_pixel == 10) {
			*mtr_mode_y = (UINT8)ISPSS_MTR_MODE_P10B_V4_Y;
			*mtr_mode_uv = (UINT8)ISPSS_MTR_MODE_P10B_V4_UV;
		}
		break;
	case ISP_SRCFMT_BAYER_12BIT_DWA:
		*mtr_mode_y = (UINT8)ISPSS_MTR_MODE_R12B_V1_RAW;
		*mtr_mode_uv = (UINT8)ISPSS_MTR_MODE_R12B_V1_RAW;
		break;
	case ISP_SRCFMT_BAYER_8BIT:
	case ISP_SRCFMT_BAYER_16BIT:
		*mtr_mode_y = (UINT8)ISPSS_MTR_MODE_R16B_V1_RAW;
		*mtr_mode_uv = (UINT8)ISPSS_MTR_MODE_R16B_V1_RAW;
		break;
	//TODO:: Add support for YUYV and NV16
	default:
		ret = ISPSS_MTR_PARAM_ERROR;
		break;
	}

	return ret;
}

INT32 ISPSS_MTR_ConfigureMtr(struct v4l2_format *f, uint32_t  YBaseAddr,
		uint32_t UVBaseAddr, uint32_t path, void *pBCMBuf)
{
	int ret = ISPSS_MTR_SUCCESS;
	struct ISPSS_MTR_CONFIG_PARAM mtrConfigParam;
	const struct v4l2_format_info *info;
	uint32_t out_width, out_height;
	uint32_t bitDepth = 8;

	if (!((path == ISPSS_MTR_PATH_TDNR0_WR) || (path == ISPSS_MTR_PATH_TDNR1_WR)
		|| (path == ISPSS_MTR_PATH_TDNR0_RD) || (path == ISPSS_MTR_PATH_TDNR1_RD))) {
		if (f == NULL) {
			pr_err("Input parameter is NULL\n");
			return -1;
		}

		info   = v4l2_format_info(f->fmt.pix.pixelformat);
		bitDepth *= info->bpp[0];
	}

	/* Set MTR Object properties */
	memset(&mtrConfigParam, 0, sizeof(struct ISPSS_MTR_CONFIG_PARAM));
	mtrConfigParam.ispMtrObj.path = path;
	mtrConfigParam.ispMtrObj.uiIspMtrKickoff = 0;
	mtrConfigParam.ispMtrInfo.pBcmBuf = pBCMBuf;
	mtrConfigParam.ispMtrObj.base_addr_ispMtr = MEMMAP_ISP_REG_BASE + ISP_MTR_REG_BASE;
	switch (path) {
	case ISPSS_MTR_PATH_MCM0_WR:
	case ISPSS_MTR_PATH_MCM0_RD:
	case ISPSS_MTR_PATH_MCM1_WR:
	case ISPSS_MTR_PATH_MCM1_RD:
	case ISPSS_MTR_PATH_MP0_WR:
	case ISPSS_MTR_PATH_MP1_WR:
	case ISPSS_MTR_PATH_SP2_WR0:
	case ISPSS_MTR_PATH_SP2_WR1:
		out_width = f->fmt.pix_mp.width;
		out_height = f->fmt.pix_mp.height;
		break;
	case ISPSS_MTR_PATH_TDNR0_WR:
	case ISPSS_MTR_PATH_TDNR1_WR:
	case ISPSS_MTR_PATH_TDNR0_RD:
	case ISPSS_MTR_PATH_TDNR1_RD:
		out_width = 3840;
		out_height = 2160;
		mtrConfigParam.ispMtrInfo.m_srcfmt = ISP_SRCFMT_BAYER_16BIT;
		break;
	default:
		pr_err("Path[%d] not supported\n", path);
		ret = ISPSS_MTR_PARAM_ERROR;
		break;
	}
	if (ret != ISPSS_MTR_SUCCESS)
		return ret;

	/* Set global registers for MTR */
	ISPSS_MTR_Set_Global_Registers(&mtrConfigParam.ispMtrObj, mtrConfigParam.ispMtrObj.path, 1);
	// TODO - Revisit when MTR enabled
	//ISPSS_MTR_Reset(&mtrConfigParam.ispMtrObj, NULL);
	/* Set Mtr specific parameters */
	if (!((path == ISPSS_MTR_PATH_TDNR0_WR) || (path == ISPSS_MTR_PATH_TDNR1_WR)
		|| (path == ISPSS_MTR_PATH_TDNR0_RD) || (path == ISPSS_MTR_PATH_TDNR1_RD))) {
		switch (f->fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
			mtrConfigParam.ispMtrInfo.m_srcfmt = ISP_SRCFMT_YUV420SP;
			break;
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV16M:
			mtrConfigParam.ispMtrInfo.m_srcfmt = ISP_SRCFMT_YUV422SP;
			break;
		case V4L2_PIX_FMT_YUYV:
			mtrConfigParam.ispMtrInfo.m_srcfmt = ISP_SRCFMT_YUV422PACK;
			break;
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
			mtrConfigParam.ispMtrInfo.m_srcfmt = ISP_SRCFMT_BAYER_8BIT;
			bitDepth = 8;
			break;
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
			mtrConfigParam.ispMtrInfo.m_srcfmt = ISP_SRCFMT_BAYER_16BIT;
			bitDepth = 16;
			break;
		default:
			pr_err("%s: Format not supported\n", __func__);
			ret = ISPSS_MTR_PARAM_ERROR;
			break;
		}
	}
	if (ret != ISPSS_MTR_SUCCESS)
		return ret;

	/* Setting Y data */
	mtrConfigParam.ispMtrInfo.m_active_width = out_width;
	mtrConfigParam.ispMtrInfo.m_active_height = out_height;
	mtrConfigParam.ispMtrInfo.m_bits_per_pixel = bitDepth;
	mtrConfigParam.ispMtrInfo.m_is_compressed = 0; //MTR support will be added later (CamEngine)
	ISPSS_MTR_SetMode(mtrConfigParam.ispMtrInfo.m_srcfmt, bitDepth,
			&mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MtrrCfgCfg_mode,
			&mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MtrrCfgCfg_mode);

	//TODO:: gIspMTR_FixedData how to do this better in kernel ways??

	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MtrrCfgCfg_stride = gIspMTR_FixedData.stride;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MtrrCfgCfg_format = gIspMTR_FixedData.format;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_bw = gIspMTR_FixedData.bm_shy_bw;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_pos =
		gIspMTR_FixedData.bm_shy_pos;
	mtrConfigParam.ispMtrInfo.m_buf_stride = gIspMTR_FixedData.buf_stride;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MmuCfgPbm_pm_enable =
		gIspMTR_FixedData.pm_enable;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MmuCfgPbm_shuffle_en =
		gIspMTR_FixedData.shuffle_en;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MmuCfgPbm_bm_enable =
		gIspMTR_FixedData.bm_enable;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MmuCfgVm_enable = gIspMTR_FixedData.vm_enable;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_Y.m_MtrrCfgBase_addr = 0;

	/* Setting Cb data */
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MtrrCfgCfg_stride = gIspMTR_FixedData.stride;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MtrrCfgCfg_format = gIspMTR_FixedData.format;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_bw =
		gIspMTR_FixedData.bm_shy_bw;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_pos =
		gIspMTR_FixedData.bm_shy_pos;
	mtrConfigParam.ispMtrInfo.m_buf_stride_UV = gIspMTR_FixedData.buf_stride;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MmuCfgPbm_pm_enable =
		gIspMTR_FixedData.pm_enable;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MmuCfgPbm_shuffle_en =
		gIspMTR_FixedData.shuffle_en;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MmuCfgPbm_bm_enable =
		gIspMTR_FixedData.bm_enable;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MmuCfgVm_enable =
		gIspMTR_FixedData.vm_enable;
	mtrConfigParam.ispMtrInfo.m_mtr_buf_desc_UV.m_MtrrCfgBase_addr = 0;

	ret = ISPSS_MTR_StartThread(&mtrConfigParam, f, YBaseAddr, UVBaseAddr);
	//ISPSS_MTR_PrintInfo(&mtrConfigParam);
	return ret;
}
EXPORT_SYMBOL_GPL(ISPSS_MTR_ConfigureMtr);

INT32 ISPSS_MTR_StartThread(struct ISPSS_MTR_CONFIG_PARAM *mtrConfigParam, struct v4l2_format *f,
		uint32_t  YBaseAddr, uint32_t UVBaseAddr)
{
	int ret = SUCCESS;
	int isYuv = 1;
	uint32_t path = mtrConfigParam->ispMtrObj.path;

	if (f != NULL && (!((path == ISPSS_MTR_PATH_TDNR0_WR) || (path == ISPSS_MTR_PATH_TDNR1_WR)
		|| (path == ISPSS_MTR_PATH_TDNR0_RD) || (path == ISPSS_MTR_PATH_TDNR1_RD)))) {
		switch (f->fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV16M:
			mtrConfigParam->ispMtrInfo.m_pbuf_start = YBaseAddr;
			break;
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
			mtrConfigParam->ispMtrInfo.m_pbuf_start = YBaseAddr;
			isYuv = 0;
			break;
		default:
			pr_err("%s: Format not supported.\n", __func__);
			ret = ISPSS_MTR_PARAM_ERROR;
			break;
		}
	} else {
		mtrConfigParam->ispMtrInfo.m_pbuf_start = YBaseAddr;
		isYuv = 0;
	}

	if (ret != ISPSS_MTR_SUCCESS)
		return ret;

	if (isYuv)
		mtrConfigParam->ispMtrInfo.m_buf_pbuf_start_UV = UVBaseAddr;

	/* Update Register and kickoff */
	ISPSS_MTR_UpdateDesc(&mtrConfigParam->ispMtrObj, &mtrConfigParam->ispMtrInfo,
			mtrConfigParam->ispMtrObj.path);
	if (f != NULL)
		ISPSS_MTR_kickOff(&mtrConfigParam->ispMtrObj, &mtrConfigParam->ispMtrInfo);

	return ret;
}

uint32_t ISPSS_MTR_QOS_Config(int config)
{
	struct ISP_MTR_OBJ mtrObj;

	mtrObj.base_addr_ispMtr = MEMMAP_ISP_REG_BASE + ISP_MTR_REG_BASE;
	return ISPSS_MTR_Config(&mtrObj, config, NULL);
}
EXPORT_SYMBOL(ISPSS_MTR_QOS_Config);

void ISPSS_MTR_PrintInfo(struct ISPSS_MTR_CONFIG_PARAM *mtrConfigParam)
{
	ISPSS_MTR_PrintRegisters(&mtrConfigParam->ispMtrObj);
}

void ISPSS_MTR_Exit(uint32_t path)
{
	struct ISPSS_MTR_CONFIG_PARAM mtrConfigParam;

	memset(&mtrConfigParam, 0, sizeof(struct ISPSS_MTR_CONFIG_PARAM));
	mtrConfigParam.ispMtrObj.uiIspMtrKickoff = 0;
	mtrConfigParam.ispMtrObj.base_addr_ispMtr = MEMMAP_ISP_REG_BASE + ISP_MTR_REG_BASE;

	ISPSS_MTR_Set_Global_Registers(&mtrConfigParam.ispMtrObj, path, 0);
}
EXPORT_SYMBOL_GPL(ISPSS_MTR_Exit);
