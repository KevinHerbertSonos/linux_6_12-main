// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispbe_err.h"
#include "ispSS_be_scl_apifuncs.h"
#include "ispSS_api_dhub_config.h"
#include "ispSS_be_frc_scl.h"
#include "ispSS_be_scl_common.h"
#include "ispSS_bcmbuf_debug.h"

//Internal Macros
#define ISPSS_BE_SCL_SUBMIT_QUEUE
#define IS_OTG_PROG_ENABLE(srcfmt) ((srcfmt == ISPSS_SRCFMT_YUV444P) \
		|| (srcfmt ==  ISPSS_SRCFMT_RGB444) || (srcfmt ==  ISPSS_SRCFMT_YUV422P) \
		|| (srcfmt == ISPSS_SRCFMT_RGB888))
#define MMU_STRIDE_LENGTH 262144
//End Internal Macros
#define bTST(x, b)                    (((x) >> (b)) & 1)

/*******************************************************************
 * FUNCTION: Initial configuration done during driver create/open
 * PARAMS:
 * RETURN:
 ******************************************************************/
INT ISPSS_BE_SCL_initalConfig(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf)
{
	//Configuration of Scalar params
	ISPSS_FRC_SCL_GenerateCoeffTables(isp_scl_obj, hwObjSt);
	/* config FRC-SCLs */
	ISPSS_FRC_SCL_SetDeLrstDelay(isp_scl_obj, ISP_FRC_SCL_OVP_Y, 80, hwObjSt, pbcmbuf);
	ISPSS_FRC_SCL_SetDeLrstDelay(isp_scl_obj, ISP_FRC_SCL_OVP_UV, 80, hwObjSt, pbcmbuf);
	return ISPSS_OK;
}

/*******************************************************************
 * FUNCTION: Set Output TG Params
 * PARAMS:   isp_scl_obj - scl object
 *           width       - Output width
 *           height      - Output Height
 *           pbcmbuf     - Bcm Buffer
 * RETURN:
 ******************************************************************/
static void ISPSS_BE_SCL_SetOTGParams(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		UINT32 width, UINT32 height, struct BCMBUF *pbcmbuf)
{
	T32TG_SIZE tg_size;
	T32TG_HB tg_hb;
	T32TG_HB_CR tg_hb_cr;
	T32TG_HB_CR2 tg_hb_cr2;
	T32TG_VB0 tg_vb0;
	T32TG_VB0_CR tg_vb0_cr;
	T32TG_VB0_CR2 tg_vb0_cr2;
	UINT32 uiRegAddr;

	uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OTG;

	tg_size.uSIZE_Y = height+6;
	tg_size.uSIZE_X = width+60; /* Changed from 20 to 40 to fix 4K SCL issues */
	tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = width+8;
	tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
	tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = height + 1;//3;
	tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 0;//2;
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_SIZE), tg_size.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_HB), tg_hb.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_HB_CR), tg_hb_cr.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_HB_CR2), tg_hb_cr2.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_VB0), tg_vb0.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_VB0_CR), tg_vb0_cr.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_VB0_CR2), tg_vb0_cr2.u32);
}

/*******************************************************************
 * FUNCTION: Set OVPSCL TG  Params For Y and UV
 * PARAMS:   isp_scl_obj - scl object
 *            width       - Input width
 *            height      - Input Height
 *            luma        - Y or UV
 *            pbcmbuf     - Bcm Buffer
 * RETURN:
 ******************************************************************/
static void ISPSS_BE_SCL_SetOVPSCLTGParams(struct ISP_BE_SCL_OBJ *isp_scl_obj, UINT32 width,
		UINT32 height, UINT32 luma, struct BCMBUF *pbcmbuf)
{
	T32TG_SIZE tg_size;
	T32TG_HB tg_hb;
	T32TG_HB_CR tg_hb_cr;
	T32TG_HB_CR2 tg_hb_cr2;
	T32TG_VB0 tg_vb0;
	T32TG_VB0_CR tg_vb0_cr;
	T32TG_VB0_CR2 tg_vb0_cr2;
	UINT32 uiRegAddr;

	if (luma == 1)
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_OTG_Y;
	else
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_OTG_UV;

	tg_size.uSIZE_Y = height+6;
	if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888)
		tg_size.uSIZE_X = width + 10;
	else
		tg_size.uSIZE_X = width + 60; /* Changed from 20 to 60 to fix 4K SCL issues */

	tg_hb.uHB_FE = tg_hb_cr.uHB_CR_FE = tg_hb_cr2.uHB_CR2_FE = width+8;
	tg_hb.uHB_BE = tg_hb_cr.uHB_CR_BE = tg_hb_cr2.uHB_CR2_BE = 7;
	tg_vb0.uVB0_FE = tg_vb0_cr.uVB0_CR_FE = tg_vb0_cr2.uVB0_CR2_FE = height + 1;//3;
	tg_vb0.uVB0_BE = tg_vb0_cr.uVB0_CR_BE = tg_vb0_cr2.uVB0_CR2_BE = 0;//2;
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_SIZE), tg_size.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_HB), tg_hb.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_HB_CR), tg_hb_cr.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_HB_CR2), tg_hb_cr2.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_VB0), tg_vb0.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_VB0_CR), tg_vb0_cr.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_VB0_CR2), tg_vb0_cr2.u32);
}

static int ISPSS_BE_SCL_getAlignX(int src_fmt, int bit_depth, int mtrMode)
{
	int alignX = 1;

	switch (src_fmt) {
	case ISPSS_SRCFMT_YUV420SP:
		if (bit_depth == 8)
			alignX = mtrMode?64:16;
		break;

	case ISPSS_SRCFMT_YUV422SP:
	case ISPSS_SRCFMT_YUV422P:
	case ISPSS_SRCFMT_YUV422SP_DWA:
	case ISPSS_SRCFMT_YUV420SP_DWA:
	case ISPSS_SRCFMT_YUV444P:
	case ISPSS_SRCFMT_RGB888:
	default:
		break;
	}
	return alignX;
}

/*******************************************************************
 * FUNCTION: Set INPUT TG  Params
 * PARAMS:   isp_scl_obj - scl object
 *            width       - Input width
 *            height      - Input Height
 *            pbcmbuf     - Bcm Buffer
 * RETURN:
 ******************************************************************/
static void ISPSS_BE_SCL_SetINTGParams(struct ISP_BE_SCL_OBJ *isp_scl_obj, UINT32 width,
		UINT32 height, struct BCMBUF *pbcmbuf)
{
	T32TG_MAIN_SIZE tg_size;
	T32TG_MAIN_HB tg_hb;
	T32TG_MAIN_HB_Y tg_hb_y;/*T32TG_HB_CR*/
	T32TG_MAIN_HB_C tg_hb_c;/*T32TG_HB_CR2*/
	T32TG_MAIN_HB_OUT tg_hb_out;
	T32TG_MAIN_VB0 tg_vb0;
	T32TG_MAIN_VB0_Y tg_vb0_y;/*T32TG_VB0_CR*/
	T32TG_MAIN_VB0_C tg_vb0_c;
	T32TG_MAIN_VB0_OUT tg_vb0_out;
	UINT32 uiRegAddr;
	int alignX;

	uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_INPTG_OVPDNSCL3;
	alignX = ISPSS_BE_SCL_getAlignX(isp_scl_obj->inSrcFrmt, isp_scl_obj->in_bit_depth,
			isp_scl_obj->in_mtrConfigDesc.m_mtrR_mode);
	if ((isp_scl_obj->inputWin.x % alignX) != 0) {
		isp_scl_obj->tg_crop.leftCrop = (isp_scl_obj->inputWin.x % alignX);
		isp_scl_obj->tg_crop.rightCrop = (alignX - isp_scl_obj->tg_crop.leftCrop);
		width += isp_scl_obj->tg_crop.leftCrop + isp_scl_obj->tg_crop.rightCrop;
	} else {
		isp_scl_obj->tg_crop.leftCrop = 0;
		isp_scl_obj->tg_crop.rightCrop = 0;
	}


	tg_size.uSIZE_Y = height+6;
	tg_size.uSIZE_X = width+60; /* Changed from 20 to 40 to fix 4K SCL issues */

	tg_hb.uHB_FE = width+8;
	tg_hb_y.uHB_Y_FE = tg_hb_c.uHB_C_FE = tg_hb_out.uHB_OUT_FE =
		(width - (isp_scl_obj->tg_crop.rightCrop)) + 8;

	tg_hb.uHB_BE = 7;
	tg_hb_y.uHB_Y_BE = tg_hb_c.uHB_C_BE = tg_hb_out.uHB_OUT_BE =
		(isp_scl_obj->tg_crop.leftCrop + 7);

	tg_vb0.uVB0_FE = tg_vb0_y.uVB0_Y_FE = tg_vb0_c.uVB0_C_FE =
		tg_vb0_out.uVB0_OUT_FE = height + 1;
	tg_vb0.uVB0_BE = tg_vb0_y.uVB0_Y_BE = tg_vb0_c.uVB0_C_BE =
		tg_vb0_out.uVB0_OUT_BE = 0;

	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_SIZE), tg_size.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB), tg_hb.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB_Y), tg_hb_y.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB_C), tg_hb_c.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0), tg_vb0.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0_Y), tg_vb0_y.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0_C), tg_vb0_c.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB_OUT), tg_hb_out.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0_OUT), tg_vb0_out.u32);
}

/*******************************************************************
 * FUNCTION: Set INPUT TG  Params
 * PARAMS:   isp_scl_obj - scl object
 *           width       - Input width
 *           height      - Input Height
 *           pbcmbuf     - Bcm Buffer
 * RETURN:
 ******************************************************************/
void ISPSS_BE_SCL_SetINTGParams_UV(struct ISP_BE_SCL_OBJ *isp_scl_obj, UINT32 width,
		UINT32 height, struct BCMBUF *pbcmbuf)
{
	T32TG_MAIN_SIZE tg_size;
	T32TG_MAIN_HB tg_hb;
	T32TG_MAIN_HB_Y tg_hb_y;
	T32TG_MAIN_HB_C tg_hb_c;
	T32TG_MAIN_HB_OUT tg_hb_out;
	T32TG_MAIN_VB0 tg_vb0;
	T32TG_MAIN_VB0_Y tg_vb0_y;
	T32TG_MAIN_VB0_C tg_vb0_c;
	T32TG_MAIN_VB0_OUT tg_vb0_out;
	UINT32 uiRegAddr;

	uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_INPTG_OVPDNSCL3_UV;

	tg_size.uSIZE_Y = height+6;
	tg_size.uSIZE_X = width+60; /* Changed from 20 to 40 to fix 4K SCL issues */

	tg_hb.uHB_FE = tg_hb_y.uHB_Y_FE = tg_hb_c.uHB_C_FE = tg_hb_out.uHB_OUT_FE = width+8;
	tg_hb.uHB_BE = tg_hb_y.uHB_Y_BE = tg_hb_c.uHB_C_BE = tg_hb_out.uHB_OUT_BE = 7;

	tg_vb0.uVB0_FE = tg_vb0_y.uVB0_Y_FE = tg_vb0_c.uVB0_C_FE =
		tg_vb0_out.uVB0_OUT_FE = height + 1;
	tg_vb0.uVB0_BE = tg_vb0_y.uVB0_Y_BE = tg_vb0_c.uVB0_C_BE =
		tg_vb0_out.uVB0_OUT_BE = 0;

	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_SIZE), tg_size.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB), tg_hb.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB_Y), tg_hb_y.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB_C), tg_hb_c.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0), tg_vb0.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0_Y), tg_vb0_y.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0_C), tg_vb0_c.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_HB_OUT), tg_hb_out.u32);
	ISPSS_BCMBUF_Write(pbcmbuf, (uiRegAddr + RA_TG_MAIN_VB0_OUT), tg_vb0_out.u32);
}

/*******************************************************************
 * FUNCTION: Set Scalar Parameteres
 * PARAMS:   isp_scl_obj - scl object
 *           inputWin    - Window containg input Resolution
 *           outputWin   - Window containg output Resolution
 * RETURN:
 ******************************************************************/
static void ISPSS_BE_SCL_OVPSetSclParams(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_WIN *inputWin,
		struct ISP_BE_SCL_WIN *outputWin, INT32 eIspFrcSclNum,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct ISP_BE_BCM *pSclBcmBuf)
{
	if (eIspFrcSclNum == ISP_FRC_SCL_OVP_Y) {
		isp_scl_obj->scl_res[eIspFrcSclNum].IHRes = inputWin->width;
		isp_scl_obj->scl_res[eIspFrcSclNum].IVRes = inputWin->height;
		isp_scl_obj->scl_res[eIspFrcSclNum].OHRes = outputWin->width;
		isp_scl_obj->scl_res[eIspFrcSclNum].OVRes = outputWin->height;
	} else {
		//For YUV420SP ,Chroma width and height should be divided by 2
		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
			isp_scl_obj->scl_res[eIspFrcSclNum].IHRes = inputWin->width/2;
			isp_scl_obj->scl_res[eIspFrcSclNum].OHRes = outputWin->width/2;
			isp_scl_obj->scl_res[eIspFrcSclNum].IVRes = inputWin->height/2;
			isp_scl_obj->scl_res[eIspFrcSclNum].OVRes = outputWin->height/2;
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) &&
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P)) {
			isp_scl_obj->scl_res[eIspFrcSclNum].IHRes = inputWin->width/2;
			isp_scl_obj->scl_res[eIspFrcSclNum].OHRes = outputWin->width;
			isp_scl_obj->scl_res[eIspFrcSclNum].IVRes = inputWin->height;
			isp_scl_obj->scl_res[eIspFrcSclNum].OVRes = outputWin->height;
			ISPSS_SCLDBG("%s,%d\n", __func__, __LINE__);
		} else if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) {
			//For 422SP, Chroma Width should be divided by 2
			isp_scl_obj->scl_res[eIspFrcSclNum].IHRes = inputWin->width/2;
			isp_scl_obj->scl_res[eIspFrcSclNum].OHRes = outputWin->width/2;
			isp_scl_obj->scl_res[eIspFrcSclNum].IVRes = inputWin->height;
			isp_scl_obj->scl_res[eIspFrcSclNum].OVRes = outputWin->height;
			ISPSS_SCLDBG("%s,%d\n", __func__, __LINE__);
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) &&
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
			isp_scl_obj->scl_res[eIspFrcSclNum].IHRes = inputWin->width;
			isp_scl_obj->scl_res[eIspFrcSclNum].OHRes = outputWin->width/2;
			isp_scl_obj->scl_res[eIspFrcSclNum].IVRes = inputWin->height;
			isp_scl_obj->scl_res[eIspFrcSclNum].OVRes = outputWin->height/2;
			ISPSS_SCLDBG("%s,%d\n", __func__, __LINE__);
		} else {
			isp_scl_obj->scl_res[eIspFrcSclNum].IHRes = inputWin->width;
			isp_scl_obj->scl_res[eIspFrcSclNum].OHRes = outputWin->width;
			isp_scl_obj->scl_res[eIspFrcSclNum].IVRes = inputWin->height;
			isp_scl_obj->scl_res[eIspFrcSclNum].OVRes = outputWin->height;
		}
	}

	isp_scl_obj->scl_ctrl[eIspFrcSclNum].HScalePos = ISP_SCL_HSCALE_AUTO;
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].InputClr = 0;
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].NLEn = 0;
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].I565 = 0;
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].HTapNum = 12;
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].VTapNum = 8;
	/* dynamically update coefficient table */
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].DynamicLoad = 1;
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].ForceSel = hwObjSt->scl_force_sel[eIspFrcSclNum];
	/* In offline case VIP SCL will take care of cropping.
	 * So no need to do cropping on MAIN SCL
	 */
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].leftCrop  = 0;
	isp_scl_obj->scl_ctrl[eIspFrcSclNum].rightCrop = 0;

	ISPSS_FRC_SCL_SetSclCtrlParams(isp_scl_obj, eIspFrcSclNum,
			&isp_scl_obj->scl_res[eIspFrcSclNum],
			&isp_scl_obj->scl_ctrl[eIspFrcSclNum],
			hwObjSt, pSclBcmBuf->bcm_buf);
	ISPSS_FRC_SCL_SetFrcParams(isp_scl_obj, eIspFrcSclNum,
			&isp_scl_obj->scl_res[eIspFrcSclNum],
			hwObjSt, pSclBcmBuf->bcm_buf);
}

void dump_to_hex(const unsigned char *buffer, size_t length)
{
	int i;

	pr_info("Dumping buffer at address %p to hex:\n", buffer);

	for (i = 0; i < length; ++i) {
		pr_info("%02X ", buffer[i]);

		if ((i + 1) % 16 == 0)
			pr_info("\n");
	}

	if (i % 16 != 0)
		pr_info("\n");

	pr_info("End of buffer dump\n");
}

/***********************************************
 * FUNCTION: Submit the BCM buffers to Hardware
 * PARAMS: pstsclRqstMsg - Request message
 * RETURN: ISPSS_OK - succeed
 ***********************************************/
INT ISPSS_BE_SCL_SubmitHW(struct ISP_BE_BCM *pSclBcmBuf, INT commit_QId)
{
	int ret = -1;

#ifdef DUMP_RAW_BCM
	pr_info("bcm_buf:       vaddr: %lx, paddr: %lx\n",
			pSclBcmBuf->shmHandle[0].addr,
			pSclBcmBuf->shmHandle[0].phy_addr);
	dump_to_hex(pSclBcmBuf->shmHandle[0].addr, 4896);
	pr_info("clear_bcm_buf: vaddr: %lx, paddr: %lx\n",
			pSclBcmBuf->shmHandle[1].addr,
			pSclBcmBuf->shmHandle[1].phy_addr);
	dump_to_hex(pSclBcmBuf->shmHandle[1].addr, 4896);
	pr_info("dma_cfgQ:	     vaddr: %lx, paddr: %lx\n",
			pSclBcmBuf->shmHandle[2].addr,
			pSclBcmBuf->shmHandle[2].phy_addr);
	dump_to_hex(pSclBcmBuf->shmHandle[2].addr, 0x320);
	pr_info("final_bcm_cfgQ:vaddr: %lx, paddr: %lx\n",
			pSclBcmBuf->shmHandle[3].addr,
			pSclBcmBuf->shmHandle[3].phy_addr);
	dump_to_hex(pSclBcmBuf->shmHandle[3].addr, 0x320);
#endif

#ifdef ISPSS_BE_SCL_SUBMIT_QUEUE
	// commit to the Q associated with DEWARP interrupt (Qx)
	ISPSS_SCLDBG("%s:%d: Submitting to Q12\n", __func__, __LINE__);
	ret =  ISPSS_BCMDHUB_CFGQ_Commit(pSclBcmBuf->final_bcm_cfgQ,
			CPCB_1, commit_QId, BCM_NON_BLOCKING_WAIT);//12 is generic queue
#else
	ret = ISPSS_BCMDHUB_CFGQ_Commit(pSclBcmBuf->final_bcm_cfgQ, CPCB_1, 13, BCM_BLOCKING_WAIT);
#endif
	return ret;
}

static void ISPSS_BE_SCL_ClientClear(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_BCM *pSclBcmBuf,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt,
		struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	/*Flush OVP Pipe*/
	hwObjSt->ovp_tg_ctrl.utg_ctrl_ovp_flush = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_ovp_flush = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	if (!psclRqstMsg->dnscl.isInputFrmIsp) {
		/*Read Client Clear */
		ISPSS_FE_RD_CLIENT_ClearChannelSCLInstance(pSclBcmBuf->clear_bcm_buf,
				ISP_OVPDNSCLWRAP_RD0);
		ISPSS_FE_RD_CLIENT_ClearChannelSCLInstance(pSclBcmBuf->clear_bcm_buf,
				ISP_OVPDNSCLWRAP_RD1);
	}
	/*Input TG Clear */
	hwObjSt->ovp_cfg0.uCFG0_tg_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_CFG0), hwObjSt->ovp_cfg0.u32);
	hwObjSt->ovp_cfg0.uCFG0_tg_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_CFG0), hwObjSt->ovp_cfg0.u32);

	if (IS_UPSAMPLING_ENABLE(isp_scl_obj->inSrcFrmt, isp_scl_obj->outSrcFrmt)) {
		/*Input UV TG Clear */
		hwObjSt->ovp_cfg9.uCFG9_tg_clear = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG9), hwObjSt->ovp_cfg9.u32);
		hwObjSt->ovp_cfg9.uCFG9_tg_clear = 0;
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG9), hwObjSt->ovp_cfg9.u32);
	}

	/*Clear Input FIFO*/
	hwObjSt->ovp_tg_ctrl.utg_ctrl_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	/*Clear Output TG Y and UV */
	hwObjSt->ovp_tg_ctrl.utg_ctrl_Y_otg_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_Y_otg_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_UV_otg_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_UV_otg_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	/*Clear output fifo */
	hwObjSt->ovp_tg_ctrl.utg_ctrl_Y_Ofifo_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_Y_Ofifo_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_UV_Ofifo_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_UV_Ofifo_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	/*Reset scalar output FIFO */
	if (IS_OTG_PROG_ENABLE(isp_scl_obj->outSrcFrmt)) {
		hwObjSt->ovp_tg_ctrl.utg_ctrl_otg_clear = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
		hwObjSt->ovp_tg_ctrl.utg_ctrl_otg_clear = 0;
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	}

	/*Clear Write client */
	hwObjSt->ovp_tg_ctrl.utg_ctrl_wr0_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_wr0_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_wr1_clear = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_wr1_clear = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl), hwObjSt->ovp_tg_ctrl.u32);
	if (psclRqstMsg->dnscl.isInputFrmIsp) {
		hwObjSt->ovp_cfg0.uCFG0_isp_fifo_clear0 = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG0), hwObjSt->ovp_cfg0.u32);
		hwObjSt->ovp_cfg0.uCFG0_isp_fifo_clear0 = 0;
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG0), hwObjSt->ovp_cfg0.u32);
	}

}

/*********************************************************************
 * FUNCTION: SCL Clients Start
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 *           pSclBcmBuf  - BCM buffer strucrure
 * RETURN:   ISPSS_OK
 ********************************************************************/
static INT ISPSS_BE_SCL_ClientStart(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_BCM *pSclBcmBuf,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt,
		struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	INT Ret = ISPSS_OK;
	/*Start Write client */
	hwObjSt->ovp_tg_ctrl.utg_ctrl_wr0_start = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_wr0_start = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	ISPSS_SCLDBG("Reading from Register:utg_ctrl_wr0_start:%d\n",
			hwObjSt->ovp_tg_ctrl.u32);
	/*Start WR1 Only for 420SP and 422SP Write Case */
	if ((isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
			(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {
		hwObjSt->ovp_tg_ctrl.utg_ctrl_wr1_start = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
		hwObjSt->ovp_tg_ctrl.utg_ctrl_wr1_start = 0;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
		ISPSS_SCLDBG("Reading from Register:utg_ctrl_wr1_start:%d\n",
				hwObjSt->ovp_tg_ctrl.u32);
	}
	/*Start OVPSCL Final OTG */
	if (IS_OTG_PROG_ENABLE(isp_scl_obj->outSrcFrmt)) {
		hwObjSt->ovp_tg_ctrl.utg_ctrl_otg_start = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
		hwObjSt->ovp_tg_ctrl.utg_ctrl_otg_start = 0;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
		ISPSS_SCLDBG("Line:%d Reading from Register:utg_ctrl_start:%d\n",
				__LINE__, hwObjSt->ovp_tg_ctrl.u32);
	}
	/*start output FIFO and output TG for chroma and luma */
	hwObjSt->ovp_tg_ctrl.utg_ctrl_Y_otg_start = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_Y_otg_start = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_UV_otg_start = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_UV_otg_start = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	/*Start OVPSCL IFIFO */
	hwObjSt->ovp_tg_ctrl.utg_ctrl_start = 1;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	hwObjSt->ovp_tg_ctrl.utg_ctrl_start = 0;
	ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
			+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	/*Start Input TG*/
	/*If Input from ISP then enable ISP path and mux else enable read client for MEM TO MEM*/
	if (psclRqstMsg->dnscl.isInputFrmIsp) {
		if (psclRqstMsg->dnscl.isEnableSclBypass == ENABLE_COMPLETE_BYPASSPATH) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x408600);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x408610);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x408600);
		} else {
			if ((isp_scl_obj->output_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x408600);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x408610);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x408600);
			}
		}
	} else {
		/*Bypass path with Read client input */
		if (psclRqstMsg->dnscl.isEnableSclBypass == ENABLE_COMPLETE_BYPASSPATH) {
			ISPSS_SCLDBG("%s,%d,Entering Bypass path\n", __func__, __LINE__);
			if ((isp_scl_obj->in_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x30800);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x30810);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x30800);
			}
			if ((isp_scl_obj->in_bit_depth == 10) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x830800);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x830810);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
					isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG0, 0x830800);
			}
		} else { /*Enable SCL Path */
			if ((isp_scl_obj->in_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
				if ((isp_scl_obj->tg_crop.leftCrop) ||
						(isp_scl_obj->tg_crop.rightCrop)) {
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x233800);
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x233810);
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x233800);
				} else {
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x33800);
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x33810);
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x33800);
				}
			}
			if ((isp_scl_obj->in_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x173840);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x173850);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x173840);
			}
			if ((isp_scl_obj->in_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C40);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C50);
				if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) &&
					(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
							isp_scl_obj->base_addr +
							RA_OVPDNSCLWRAP_CFG0, 0xD73C41);
				} else
					ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
							isp_scl_obj->base_addr +
							RA_OVPDNSCLWRAP_CFG0, 0xD73C40);
			}
			if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888 &&
					isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_RGB888) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C40);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C50);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C40);
			}
			if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888 &&
					isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C40);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C50);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0xD73C40);
			}
			if ((isp_scl_obj->in_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x1C03C00);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x1C03C10);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_CFG0, 0x1C03C00);
			}
			if ((isp_scl_obj->in_bit_depth == 10) &&
					((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
					 (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA))) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x833800);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x833810);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x833800);
			}
			if ((isp_scl_obj->in_bit_depth == 10) &&
					((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
					 (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA))) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x803800);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x803810);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x803800);
			}
			if ((isp_scl_obj->in_bit_depth == 10) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x1403800);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x1403810);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0, 0x1403800);
			}

		}

		if (IS_UPSAMPLING_ENABLE(isp_scl_obj->inSrcFrmt,
					isp_scl_obj->outSrcFrmt)) {
			T32OVPDNSCLWRAP_CFG9 stovpCfg9;
			UINT32 uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG9;

			GA_REG_WORD32_READ(uiRegAddr, &stovpCfg9.u32);
			stovpCfg9.uCFG9_tg_start = 1;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG9, stovpCfg9.u32);
			stovpCfg9.uCFG9_tg_start = 0;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG9, stovpCfg9.u32);
		}

		/*Read Client Enable RD0 and RD1*/
		Ret = ISPSS_FE_RD_CLIENT_EnableChannelSCLInstance(pSclBcmBuf->bcm_buf,
				ISP_OVPDNSCLWRAP_RD0);
		if (Ret != ISPSS_OK) {
			pr_debug("ERR:%s,%d", __func__, __LINE__);
			return Ret;
		}
		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA)) {
			Ret = ISPSS_FE_RD_CLIENT_EnableChannelSCLInstance(pSclBcmBuf->bcm_buf,
					ISP_OVPDNSCLWRAP_RD1);
			if (Ret != ISPSS_OK) {
				pr_debug("ERR:%s,%d", __func__, __LINE__);
				return Ret;
			}
		}
	}
	if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
			(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA) ||
			(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {
		hwObjSt->ovp_tg_ctrl.utg_ctrl_420sp_rd_start = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
		hwObjSt->ovp_tg_ctrl.utg_ctrl_420sp_rd_start = 0;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_tg_ctrl, hwObjSt->ovp_tg_ctrl.u32);
	}

	return Ret;
}

/*********************************************************************
 * FUNCTION: This function is used to configure Decompression based on
 *           input frame
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 *           pSclBcmBuf  - BCM buffer strucrure
 ********************************************************************/
static void ISPSS_BE_ConfigDecompression(struct BCMBUF *pbcmbuf,
		struct ISPSS_BE_MTR *pMtrConfigDesc, int path)
{
	ISPSS_SCLDBG("m_mtrR_mode:%d path:%d\n", pMtrConfigDesc->m_mtrR_mode, path);
	if (pMtrConfigDesc->m_mtrR_mode /*& 1*/) {
		pMtrConfigDesc->m_mtrR_updSemId_Y  |= 0x1; /* Compression mode */
		pMtrConfigDesc->m_mtrR_updSemId_UV |= 0x1; /* Compression mode */
		ISPSS_SCLDBG("%s,%d, m_mtrR_updSemId_Y:%d,m_mtrR_updSemId_UV:%d\n",
				__func__, __LINE__, pMtrConfigDesc->m_mtrR_updSemId_Y,
				pMtrConfigDesc->m_mtrR_updSemId_UV);
	} else {
		pMtrConfigDesc->m_mtrR_updSemId_Y  &= (~0x1); /* Bypass mode */
		pMtrConfigDesc->m_mtrR_updSemId_UV &= (~0x1); /* Bypass mode */
		ISPSS_SCLDBG("%s,%d, m_mtrR_updSemId_Y:%d,m_mtrR_updSemId_UV:%d\n",
				__func__, __LINE__, pMtrConfigDesc->m_mtrR_updSemId_Y,
				pMtrConfigDesc->m_mtrR_updSemId_UV);
	}
	ISPSS_BE_MTR_UpdateDesc(pbcmbuf, pMtrConfigDesc, path);
}

/*********************************************************************
 * FUNCTION: Programming of SCL
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 *           pSclBcmBuf  - BCM buffer strucrure
 * RETURN:   ISPSS_OK
 ********************************************************************/
static void ISPSS_BE_SCL_programSclPipe(struct ISP_BE_BCM *pSclBcmBuf,
		struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt,
		struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	int *cfgQ, *cfgQ_shadow,  *cfgQ_len;
	static int z = 1;
	int alignX = 1;

	struct ISPSS_DHUB_CFG_2NDDMA_PARAM stCfg2NDDMAParam, stCfgRd2NDDMAParam;

	if (isp_scl_obj->ovp_win_updated == 1) {
		cfgQ_shadow = (int *)pSclBcmBuf->dma_cfgQ->addr;
		cfgQ_len = &(pSclBcmBuf->dma_cfgQ->len);

		cfgQ = cfgQ_shadow + *cfgQ_len * 2;

		memset(&stCfg2NDDMAParam, 0, sizeof(struct ISPSS_DHUB_CFG_2NDDMA_PARAM));
		if (psclRqstMsg->dnscl.isEnableSclBypass == ENABLE_COMPLETE_BYPASSPATH) {
			hwObjSt->ovp_cfg8.uCFG8_scl1d_bypass = 1;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_CFG8), hwObjSt->ovp_cfg8.u32);
		} else {
			hwObjSt->ovp_cfg8.uCFG8_scl1d_bypass = 0;
		}
		if (psclRqstMsg->dnscl.isInputFrmIsp) {
			hwObjSt->ovp_cfg0.uCFG0_tgEn_rstEn = 1;
			hwObjSt->ovp_cfg0.uCFG0_tgEof_en  = 1;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_CFG0), hwObjSt->ovp_cfg0.u32);
			hwObjSt->ovp_cfg1.uCFG1_Inpix_tot =
				isp_scl_obj->inputWin.height * isp_scl_obj->inputWin.width;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_CFG1), hwObjSt->ovp_cfg1.u32);
		}
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (
				isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_Y +
				RA_OVPSCL_SCL1D_YC + RA_SCL1D_YC_CFG20), 0x300);
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC +
				RA_SCL1D_YC_CFG20), 0x300);
		//In Pix calculation
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_Y + RA_OVPSCL_scl1d_Inpix),
				isp_scl_obj->inputWin.height*isp_scl_obj->inputWin.width);

		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_scl1d_Inpix),
				isp_scl_obj->inputWin.height*isp_scl_obj->inputWin.width/4);//4
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA)) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_scl1d_Inpix),
				isp_scl_obj->inputWin.height * isp_scl_obj->inputWin.width/2);
		} else {
			//Don't configure scl1d_Inpix For YUV422P/YUV444P/RGB
		}
		//Out Pix Calculation
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_Y + RA_OVPSCL_scl1d_Outpix),
				isp_scl_obj->outputWin.height*isp_scl_obj->outputWin.width);

		if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP) {
			ISPSS_SCLDBG("Enter OUTPIX FOR SCL\n");
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_scl1d_Outpix),
				isp_scl_obj->outputWin.height*isp_scl_obj->outputWin.width/4);
		} else if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP &&
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422SP ||
				 isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P)) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_scl1d_Outpix),
				isp_scl_obj->outputWin.height*isp_scl_obj->outputWin.width/2);
		} else {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_scl1d_Outpix),
				isp_scl_obj->outputWin.height*isp_scl_obj->outputWin.width);
		}
		//Set OVP Scalar Params
		ISPSS_BE_SCL_OVPSetSclParams(isp_scl_obj, &(isp_scl_obj->inputWin),
				&isp_scl_obj->outputWin, ISP_FRC_SCL_OVP_Y, hwObjSt, pSclBcmBuf);
		ISPSS_BE_SCL_OVPSetSclParams(isp_scl_obj, &(isp_scl_obj->inputWin),
				&isp_scl_obj->outputWin, ISP_FRC_SCL_OVP_UV, hwObjSt, pSclBcmBuf);
		if (isp_scl_obj->output_bit_depth == 10) {
			hwObjSt->ovp_y_scl_ctrl.uCTRL_dither_bypass = 1;
			hwObjSt->ovp_uv_scl_ctrl.uCTRL_dither_bypass = 1;
		} else {
			hwObjSt->ovp_y_scl_ctrl.uCTRL_dither_bypass = 1;
			hwObjSt->ovp_uv_scl_ctrl.uCTRL_dither_bypass = 1;
		}

		if (psclRqstMsg->dnscl.isEnableSclBypass == ENABLE_ONLY_1D_BYPASSPATH) {
			hwObjSt->ovp_y_scl_ctrl.uCTRL_scl1d_YC_bypass = 1;
			hwObjSt->ovp_uv_scl_ctrl.uCTRL_scl1d_YC_bypass = 1;
		} else {
			hwObjSt->ovp_y_scl_ctrl.uCTRL_scl1d_YC_bypass = 0;
			hwObjSt->ovp_uv_scl_ctrl.uCTRL_scl1d_YC_bypass = 0;
		}
		hwObjSt->ovp_y_scl_ctrl.uCTRL_scl_rst_en0 = 1;
		hwObjSt->ovp_y_scl_ctrl.uCTRL_scl_rst_en1 = 1;
		hwObjSt->ovp_y_scl_ctrl.uCTRL_scl_rst_en2 = 1;
		hwObjSt->ovp_uv_scl_ctrl.uCTRL_scl_rst_en0 = 1;
		hwObjSt->ovp_uv_scl_ctrl.uCTRL_scl_rst_en1 = 1;
		hwObjSt->ovp_uv_scl_ctrl.uCTRL_scl_rst_en2 = 1;

		if (psclRqstMsg->dnscl.isEnableSclBypass == ENABLE_ONLY_1D_BYPASSPATH) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPSCL_Y +
						RA_OVPSCL_CTRL), hwObjSt->ovp_y_scl_ctrl.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPSCL_UV+RA_OVPSCL_CTRL),
					hwObjSt->ovp_uv_scl_ctrl.u32);
		} else {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPSCL_Y+RA_OVPSCL_CTRL), 0x172);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPSCL_UV+RA_OVPSCL_CTRL), 0x172);
		}
		hwObjSt->ovpsclwrap_ctrl.uCTRL_ovpscl_ictrl_sel = 5;
		hwObjSt->ovpsclwrap_ctrl.uCTRL_clk_en_sel = 0;

		if ((isp_scl_obj->in_bit_depth == 10) &&
				((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				 (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA))) {
			hwObjSt->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 0;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 1;
		}
		if ((isp_scl_obj->in_bit_depth == 10) &&
				((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
				 (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA))) {

			hwObjSt->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 2;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 0;
		}

		if ((isp_scl_obj->in_bit_depth == 8) &&
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {

			hwObjSt->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 1;
		}

		if ((isp_scl_obj->in_bit_depth == 8) &&
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {

			hwObjSt->ovpsclwrap_ctrl.uCTRL_hde_msk_en = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 1;
		}

		if ((isp_scl_obj->in_bit_depth == 8) &&
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P)) {
			hwObjSt->ovpsclwrap_ctrl.uCTRL_hde_msk_en = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 1;
		}
		if ((isp_scl_obj->in_bit_depth == 8) &&
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888)) {
			hwObjSt->ovpsclwrap_ctrl.uCTRL_hde_msk_en = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_luma_data_sel = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_chroma_data_sel = 1;
		}
		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
			hwObjSt->ovpsclwrap_ctrl.uCTRL_420sp_wrbk = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_rdsel_420SP = 1;
		} else {
			hwObjSt->ovpsclwrap_ctrl.uCTRL_420sp_wrbk = 0;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_rdsel_420SP = 0;
		}

		hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOutrdy_sts0_en = 1;

		if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P)
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOutrdy_sts1_en = 1;
		else
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOutrdy_sts1_en = 1;

		if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P)
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOutrdy_sts1_en = 0;
		if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_RGB888)
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOutrdy_sts1_en = 0;

		if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P ||
				isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888) {
			hwObjSt->ovpsclwrap_ctrl.uCTRL_ofifo_sts0_ctrl = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_ofifo_sts1_ctrl = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOfifo_Y_stsCtrl = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOfifo_UV_stsCtrl = 1;
		} else {
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOfifo_Y_stsCtrl = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_ofifo_sts0_ctrl = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_sclOfifo_UV_stsCtrl = 0;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_ofifo_sts1_ctrl = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_hde_initval1 = 0;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_hde_initval1 = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_clken_ctrl0 = 1;
			hwObjSt->ovpsclwrap_ctrl.uCTRL_clken_ctrl1 = 1;
		}
		/*DITHER OVPDNSCL */
		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888)) {
			hwObjSt->ovp_dither.uCFG0_mode = 0;
			hwObjSt->ovp_dither.uCFG0_ctrl = 2;
			hwObjSt->ovp_dither.uCFG0_ycmode = 0;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPDNSCL_DITHER + RA_DITHER_CFG0),
					hwObjSt->ovp_dither.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPSCL_Y +
						RA_OVPSCL_DITHER + RA_DITHER_CFG0),
					hwObjSt->ovp_dither.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
						RA_OVPDNSCLWRAP_OVPSCL_UV +
						RA_OVPSCL_DITHER + RA_DITHER_CFG0),
					hwObjSt->ovp_dither.u32);
		} else {
			hwObjSt->ovp_dither.uCFG0_mode = 3;
			hwObjSt->ovp_dither.uCFG0_ctrl = 0;
			hwObjSt->ovp_dither.uCFG0_ycmode = 1;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPDNSCL_DITHER + RA_DITHER_CFG0),
					hwObjSt->ovp_dither.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPSCL_Y + RA_OVPSCL_DITHER +
						RA_DITHER_CFG0), 0x000);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_DITHER +
						RA_DITHER_CFG0), 0x00);
		}
		hwObjSt->ovp_IntrEn.uINTR_EN_OVPDNSCL_Y_EOF = 1;
		hwObjSt->ovp_IntrEn.uINTR_EN_OVPDNSCL_UV_EOF = 1;
		hwObjSt->ovp_IntrEn.uINTR_EN_OVPDNSCL_otg_EOF = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_INTR_EN), hwObjSt->ovp_IntrEn.u32);
		//CHECK
		//Set OVPSCL_TG Params for Y and UV
		ISPSS_BE_SCL_SetOVPSCLTGParams(isp_scl_obj, isp_scl_obj->outputWin.width,
				isp_scl_obj->outputWin.height, 1, pSclBcmBuf->bcm_buf);
		if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P) {
			ISPSS_BE_SCL_SetOVPSCLTGParams(isp_scl_obj, isp_scl_obj->outputWin.width,
					isp_scl_obj->outputWin.height, 0, pSclBcmBuf->bcm_buf);
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
			ISPSS_BE_SCL_SetOVPSCLTGParams(isp_scl_obj, isp_scl_obj->outputWin.width/2,
					isp_scl_obj->outputWin.height/2, 0, pSclBcmBuf->bcm_buf);//2
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA)) {
			ISPSS_BE_SCL_SetOVPSCLTGParams(isp_scl_obj, isp_scl_obj->outputWin.width/2,
					isp_scl_obj->outputWin.height, 0, pSclBcmBuf->bcm_buf);//2
		} else {
			ISPSS_BE_SCL_SetOVPSCLTGParams(isp_scl_obj, isp_scl_obj->outputWin.width,
					isp_scl_obj->outputWin.height, 0, pSclBcmBuf->bcm_buf);
		}
		/*Set Output TG Parameters */
		if (IS_OTG_PROG_ENABLE(isp_scl_obj->outSrcFrmt))  {
			ISPSS_BE_SCL_SetOTGParams(isp_scl_obj, isp_scl_obj->outputWin.width,
					isp_scl_obj->outputWin.height, pSclBcmBuf->bcm_buf);
		}
		if (psclRqstMsg->dnscl.isInputFrmIsp) {
			hwObjSt->ovp_cfg0.uCFG0_clken_ctrl0 = 1;
			hwObjSt->ovp_cfg0.uCFG0_clken_ctrl1 = 1;
			hwObjSt->ovp_cfg0.uCFG0_clken_ctrl2 = 0;
			hwObjSt->ovp_cfg0.uCFG0_read_Sel = 1;
			hwObjSt->ovp_cfg0.uCFG0_idata_Sel = 1;
		} else {
			hwObjSt->ovp_cfg0.uCFG0_clken_ctrl0 = 0;
			if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) ||
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P) ||
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888)) {
				//Read channel for Chroma is independent
				hwObjSt->ovp_cfg0.uCFG0_clken_ctrl1 = 1;
				hwObjSt->ovp_cfg0.uCFG0_uv_mask_disable = 1;
			} else {
				hwObjSt->ovp_cfg0.uCFG0_clken_ctrl1 = 0;
			}
			hwObjSt->ovp_cfg0.uCFG0_clken_ctrl2 = 1;
			//10 bit YUV420/422SP
			if ((isp_scl_obj->in_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
				hwObjSt->ovp_cfg0.uCFG0_idata_Sel = 4;
			}
			if ((isp_scl_obj->in_bit_depth == 10) &&
					((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
					 (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA))) {
				hwObjSt->ovp_cfg0.uCFG0_idata_Sel = 2;
			}
		}
		hwObjSt->ovp_cfg2.uCFG2_ififo_sts0_ctrl = 1;
		hwObjSt->ovp_cfg2.uCFG2_ififo_sts1_ctrl = 1;
		hwObjSt->ovp_cfg2.uCFG2_fifo_full_ctrlEn = 1;

		if (((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) ||
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P) ||
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888)) &&
				((isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P) ||
				 (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P) ||
				 (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_RGB888))) {
			hwObjSt->ovp_cfg2.uCFG2_UVPack_sel = 0;
		} else {
			hwObjSt->ovp_cfg2.uCFG2_UVPack_sel = 1;
		}
		if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P)
			hwObjSt->ovp_cfg2.uCFG2_swap_regs = 0;
		else
			hwObjSt->ovp_cfg2.uCFG2_swap_regs = 1;
		hwObjSt->ovp_cfg2.uCFG2_ovpdnsclwrap_rst = 1;
		hwObjSt->ovp_cfg2.uCFG2_ovpdnsclwrap_Y_rst = 1;
		hwObjSt->ovp_cfg2.uCFG2_ovpdnsclwrap_UV_rst = 1;
		hwObjSt->ovp_cfg2.uCFG2_ovpdnsclwrap_Y_memlb_rst = 1;
		hwObjSt->ovp_cfg2.uCFG2_ovpdnsclwrap_UV_memlb_rst = 1;
		hwObjSt->ovp_cfg2.uCFG2_ovpdnsclwrap_Y_sbrst = 1;
		hwObjSt->ovp_cfg2.uCFG2_ovpdnsclwrap_UV_sbrst = 1;

		hwObjSt->ovp_cfg8.uCFG8_sclOutrdy_sts0_en = 1;
		hwObjSt->ovp_cfg8.uCFG8_scl_OFifoY_aEmpCtrl = 1;
		hwObjSt->ovp_cfg8.uCFG8_csc_bypass = 1;
		if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) {
			hwObjSt->ovp_cfg8.uCFG8_uvrd_en_mask = 1;
			hwObjSt->ovp_cfg8.uCFG8_uv422_444_sel = 1;
			hwObjSt->ovp_cfg8.uCFG8_dither_bypass = 1;
		}
		if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888) {
			hwObjSt->ovp_cfg8.uCFG8_uvrd_en_mask = 1;
			hwObjSt->ovp_cfg8.uCFG8_uv422_444_sel = 1;
			hwObjSt->ovp_cfg8.uCFG8_dither_bypass = 1;
		}
		if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P)
			hwObjSt->ovp_cfg8.uCFG8_uv422_444_sel = 0;

		if ((isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P) ||
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P) ||
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_RGB888)) {
			hwObjSt->ovp_cfg8.uCFG8_444_sel = 1;
			hwObjSt->ovp_cfg8.uCFG8_sclOutrdy_sts1_en = 0;
			hwObjSt->ovp_cfg8.uCFG8_scl_OFifoUV_aEmpCtrl = 1;
		} else {
			hwObjSt->ovp_cfg8.uCFG8_444_sel = 0;
			hwObjSt->ovp_cfg8.uCFG8_scl_OFifoUV_aEmpCtrl = 0;
			hwObjSt->ovp_cfg8.uCFG8_sclOutrdy_sts1_en = 1;
		}
		if (psclRqstMsg->dnscl.isEnableSclBypass == ENABLE_COMPLETE_BYPASSPATH) {
		} else {
			hwObjSt->ovp_cfg8.uCFG8_scl_OFifoY_aFullCtrl = 0;
			hwObjSt->ovp_cfg8.uCFG8_scl_OFifoUV_aFullCtrl = 0;
		}
		if ((isp_scl_obj->output_bit_depth == 8) &&
				((isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P) ||
				 (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP))) {
			hwObjSt->ovp_cfg8.uCFG8_sp_wrbk_8b = 0;
		}
		if ((isp_scl_obj->output_bit_depth == 10) &&
				((isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P) ||
				 (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP))) {
			hwObjSt->ovp_cfg8.uCFG8_sp_wrbk_8b = 1;
		}
		if ((isp_scl_obj->output_bit_depth == 8) &&
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P)) {
			hwObjSt->ovp_cfg8.uCFG8_pack_wrbk = 0;
		}
		if ((isp_scl_obj->output_bit_depth == 8) &&
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_RGB888)) {
			hwObjSt->ovp_cfg8.uCFG8_pack_wrbk = 0;
		}
		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888)) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG0), 0xD73C40);
		} else if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG0), 0x173840);
		} else {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG0), hwObjSt->ovp_cfg0.u32);
		}
		if (psclRqstMsg->dnscl.isEnableSclBypass == ENABLE_COMPLETE_BYPASSPATH) {
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG8), hwObjSt->ovp_cfg8.u32);
		} else {
			if ((isp_scl_obj->output_bit_depth == 8) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG8), 0xf40);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CTRL), 0x6805b7a);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG2), hwObjSt->ovp_cfg2.u32);
			} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) &&
					(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_CFG8),
						hwObjSt->ovp_cfg8.u32);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
							+ RA_OVPDNSCLWRAP_CTRL), 0x604537F);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
							+ RA_OVPDNSCLWRAP_CFG2), 0x1FE7);
			} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888) &&
					(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_RGB888)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						(isp_scl_obj->base_addr +
						 RA_OVPDNSCLWRAP_CFG8), 0x18df0);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						(isp_scl_obj->base_addr +
						 RA_OVPDNSCLWRAP_CTRL), 0x604536F);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						(isp_scl_obj->base_addr +
						 RA_OVPDNSCLWRAP_CFG2), 0x1FE7);
			} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888) &&
					(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						(isp_scl_obj->base_addr +
						 RA_OVPDNSCLWRAP_CFG8), 0x18df0);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						(isp_scl_obj->base_addr +
						 RA_OVPDNSCLWRAP_CTRL), 0x604527F);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
						(isp_scl_obj->base_addr +
						 RA_OVPDNSCLWRAP_CFG2), 0x1FE7);
			} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) &&
					(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
							+ RA_OVPDNSCLWRAP_CFG8), 0xF40);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
							+ RA_OVPDNSCLWRAP_CTRL), 0x6845b7b);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG2), 0x1FEF);
			} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) &&
					(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG8), 0x8D60);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CTRL), 0x684535B);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG2), 0x1FEF);
			} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) &&
					(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG8), 0x8DC0);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CTRL), 0x6845B5F);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG2), 0x1FCF);
			} else if ((isp_scl_obj->output_bit_depth == 10) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG8), 0x1740);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CTRL), 0x6805B3A);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG2), hwObjSt->ovp_cfg2.u32);
			} else if ((isp_scl_obj->in_bit_depth == 10) &&
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG8), 0x1740);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CTRL), 0x6E05B3A);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG2), hwObjSt->ovp_cfg2.u32);
			} else {
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG8), hwObjSt->ovp_cfg8.u32);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CTRL), hwObjSt->ovpsclwrap_ctrl.u32);
				ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
					+ RA_OVPDNSCLWRAP_CFG2), hwObjSt->ovp_cfg2.u32);
			}
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG0), 0x800);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG1), 0x105C4);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG2), 0x1023C);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG3), 0xF3);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG4), 0xAD9);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG5), 0x434);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG6), 0x100a5);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG7), 0x1075b);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG8), 0x800);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG9), 0x40000);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG10), 0x0);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
				+ RA_OVPDNSCLWRAP_OVPDNSCL3_CSC + RA_CSC_C17O24_CFG11), 0x40000);
		}

		ISPSS_FE_WR_CLIENT_ClientConfig(pSclBcmBuf->bcm_buf, ISP_OVPDNSCLWRAP_WR0,
				isp_scl_obj->outSrcFrmt, isp_scl_obj->outputWin.width,
				isp_scl_obj->outputWin.height, isp_scl_obj->output_bit_depth);

		if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP) {
			ISPSS_FE_WR_CLIENT_ClientConfig(pSclBcmBuf->bcm_buf, ISP_OVPDNSCLWRAP_WR1,
				isp_scl_obj->outSrcFrmt, isp_scl_obj->outputWin.width,
				(isp_scl_obj->outputWin.height)/2, isp_scl_obj->output_bit_depth);
		} else if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422SP) {
			ISPSS_FE_WR_CLIENT_ClientConfig(pSclBcmBuf->bcm_buf, ISP_OVPDNSCLWRAP_WR1,
				isp_scl_obj->outSrcFrmt, isp_scl_obj->outputWin.width,
				(isp_scl_obj->outputWin.height), isp_scl_obj->output_bit_depth);
			pr_debug("%s,%d,Enter WR1 CONFIG\n", __func__, __LINE__);
		} else {
			//Don't configure WR1 for YUV422P/YUV444P/RGB444
		}

		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P) &&
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP)) {
			T32BITMAP32_SEL     stBitMap32Sel = {0};
			T32BITMAP32_SEL1    stBitMap32Sel1 = {0};
			T32BITMAP32_SEL2    stBitMap32Sel2 = {0};
			T32BITMAP32_SEL3    stBitMap32Sel3 = {0};
			T32BITMAP32_SEL4    stBitMap32Sel4 = {0};
			T32BITMAP32_SEL5    stBitMap32Sel5 = {0};

			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CTRL), 0x06045b7A);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG2), 0x1FE7);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr
						+ RA_OVPDNSCLWRAP_CFG8), 0xf41);

			stBitMap32Sel.uSEL_BIT_POS0 = 20;
			stBitMap32Sel.uSEL_BIT_POS1 = 21;
			stBitMap32Sel.uSEL_BIT_POS2 = 22;
			stBitMap32Sel.uSEL_BIT_POS3 = 23;
			stBitMap32Sel.uSEL_BIT_POS4 = 24;
			stBitMap32Sel.uSEL_BIT_POS5 = 25;
			stBitMap32Sel1.uSEL_BIT_POS6 = 26;
			stBitMap32Sel1.uSEL_BIT_POS7 = 27;
			stBitMap32Sel1.uSEL_BIT_POS8 = 28;
			stBitMap32Sel1.uSEL_BIT_POS9 = 29;
			stBitMap32Sel1.uSEL_BIT_POS10 = 0;
			stBitMap32Sel1.uSEL_BIT_POS11 = 1;
			stBitMap32Sel2.uSEL_BIT_POS12 = 2;
			stBitMap32Sel2.uSEL_BIT_POS13 = 3;
			stBitMap32Sel2.uSEL_BIT_POS14 = 4;
			stBitMap32Sel2.uSEL_BIT_POS15 = 5;
			stBitMap32Sel2.uSEL_BIT_POS16 = 6;
			stBitMap32Sel2.uSEL_BIT_POS17 = 7;
			stBitMap32Sel3.uSEL_BIT_POS18 = 8;
			stBitMap32Sel3.uSEL_BIT_POS19 = 9;
			stBitMap32Sel3.uSEL_BIT_POS20 = 10;
			stBitMap32Sel3.uSEL_BIT_POS21 = 11;
			stBitMap32Sel3.uSEL_BIT_POS22 = 12;
			stBitMap32Sel3.uSEL_BIT_POS23 = 13;
			stBitMap32Sel4.uSEL_BIT_POS24 = 14;
			stBitMap32Sel4.uSEL_BIT_POS25 = 15;
			stBitMap32Sel4.uSEL_BIT_POS26 = 16;
			stBitMap32Sel4.uSEL_BIT_POS27 = 17;
			stBitMap32Sel4.uSEL_BIT_POS28 = 18;
			stBitMap32Sel4.uSEL_BIT_POS29 = 19;
			stBitMap32Sel5.uSEL_BIT_POS30 = 30;
			stBitMap32Sel5.uSEL_BIT_POS31 = 31;
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
				RA_OVPDNSCLWRAP_BITMAP32 + RA_BITMAP32_SEL), stBitMap32Sel.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
				RA_OVPDNSCLWRAP_BITMAP32 + RA_BITMAP32_SEL1), stBitMap32Sel1.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
				RA_OVPDNSCLWRAP_BITMAP32 + RA_BITMAP32_SEL2), stBitMap32Sel2.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
				RA_OVPDNSCLWRAP_BITMAP32 + RA_BITMAP32_SEL3), stBitMap32Sel3.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
				RA_OVPDNSCLWRAP_BITMAP32 + RA_BITMAP32_SEL4), stBitMap32Sel4.u32);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
				RA_OVPDNSCLWRAP_BITMAP32 + RA_BITMAP32_SEL5), stBitMap32Sel5.u32);
		}
		stCfg2NDDMAParam.mtrREn = 0;
		if (psclRqstMsg->out_mtrMode) {
			ISPSS_SCLDBG(">>>>>>> Out MTR/MMU frame <<<<<<<");
			memset(&isp_scl_obj->out_mtrConfigDesc, 0,
					sizeof(isp_scl_obj->out_mtrConfigDesc));
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_Y.m_MtrrCfgBase_addr =
				psclRqstMsg->out_mtrAddr_Y;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_UV.m_MtrrCfgBase_addr =
				psclRqstMsg->out_mtrAddr_UV;
			isp_scl_obj->out_mtrConfigDesc.m_bits_per_pixel =
				isp_scl_obj->output_bit_depth;
			isp_scl_obj->out_mtrConfigDesc.m_active_height =
				isp_scl_obj->outputWin.height;
			isp_scl_obj->out_mtrConfigDesc.m_content_height =
				isp_scl_obj->outputWin.height;
			isp_scl_obj->out_mtrConfigDesc.m_active_width =
				isp_scl_obj->outputWin.width;
			isp_scl_obj->out_mtrConfigDesc.m_content_width =
				isp_scl_obj->outputWin.width;
			isp_scl_obj->out_mtrConfigDesc.m_srcfmt = isp_scl_obj->outSrcFrmt;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_bw = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_pos = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_pm_enable = 1;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_shuffle_en = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_bm_enable = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgVm_enable = 1;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_bw = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_pos = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_pm_enable = 1;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_shuffle_en = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_bm_enable = 0;
			isp_scl_obj->out_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgVm_enable = 1;
			isp_scl_obj->out_mtrConfigDesc.m_buf_stride = isp_scl_obj->ovp_win_stride;
			isp_scl_obj->out_mtrConfigDesc.m_pbuf_start =
				(void *)isp_scl_obj->outAddr_Y;
			isp_scl_obj->out_mtrConfigDesc.m_buf_pbuf_start_UV =
				(UINT32)isp_scl_obj->outAddr_UV;
			isp_scl_obj->out_mtrConfigDesc.m_mtrR_mode = psclRqstMsg->out_mtrMode;
			ISPSS_BE_ConfigDecompression(pSclBcmBuf->bcm_buf,
					&isp_scl_obj->out_mtrConfigDesc, ISPSS_BE_DNSCL_WR);
		}

		memset(&isp_scl_obj->in_mtrConfigDesc, 0, sizeof(isp_scl_obj->in_mtrConfigDesc));
		if (psclRqstMsg->in_mtrMode) {
			ISPSS_SCLDBG(">>>>>>> In MTR/MMU frame <<<<<<<");
			isp_scl_obj->in_mtrConfigDesc.m_bits_per_pixel =
				isp_scl_obj->in_bit_depth;
			isp_scl_obj->in_mtrConfigDesc.m_active_height  =
				isp_scl_obj->inputWin.height;
			isp_scl_obj->in_mtrConfigDesc.m_content_height =
				isp_scl_obj->inputWin.height;
			isp_scl_obj->in_mtrConfigDesc.m_active_width   =
				isp_scl_obj->inputWin.width;
			isp_scl_obj->in_mtrConfigDesc.m_content_width  =
				isp_scl_obj->contentWin.width;
			isp_scl_obj->in_mtrConfigDesc.m_srcfmt = isp_scl_obj->inSrcFrmt;

			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_Y.m_MtrrCfgBase_addr =
				psclRqstMsg->in_mtrAddr_Y;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_bw = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_shy_pos = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_pm_enable = 1;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_shuffle_en = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgPbm_bm_enable = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_Y.m_MmuCfgVm_enable = 1;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_bw = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_shy_pos = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_pm_enable = 1;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_shuffle_en = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgPbm_bm_enable = 0;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_UV.m_MmuCfgVm_enable = 1;
			isp_scl_obj->in_mtrConfigDesc.m_mtr_buf_desc_UV.m_MtrrCfgBase_addr =
				psclRqstMsg->in_mtrAddr_UV;

			isp_scl_obj->in_mtrConfigDesc.m_buf_stride = isp_scl_obj->in_win_stride;
			isp_scl_obj->in_mtrConfigDesc.m_pbuf_start = (void *)isp_scl_obj->inAddr_y;
			isp_scl_obj->in_mtrConfigDesc.m_buf_pbuf_start_UV =
				(UINT32)isp_scl_obj->inAddr_UV;
			isp_scl_obj->in_mtrConfigDesc.m_mtrR_mode = psclRqstMsg->in_mtrMode;
			ISPSS_BE_ConfigDecompression(pSclBcmBuf->bcm_buf,
					&isp_scl_obj->in_mtrConfigDesc, ISPSS_BE_DNSCL_RD);
		}

		//Program 2NDDMA For Write Client WR0
		stCfg2NDDMAParam.dhubId = hwObjSt->dmaDhubID;
		stCfg2NDDMAParam.pdhubHandle = (struct HDL_dhub2d *)hwObjSt->dmaDhubID;
		stCfg2NDDMAParam.pBcmBuf = pSclBcmBuf->bcm_buf;
		stCfg2NDDMAParam.dmaId  = hwObjSt->dmaWID[0];
		stCfg2NDDMAParam.addr   = isp_scl_obj->outAddr_Y;
		if ((isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {
			stCfg2NDDMAParam.width  = ((isp_scl_obj->outputWin.width *
						isp_scl_obj->output_bit_depth) + 7)/8;
		} else if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422P) {
			stCfg2NDDMAParam.width  = ((isp_scl_obj->outputWin.width *
						isp_scl_obj->output_bit_depth * 2) + 7)/8;
			ISPSS_SCLDBG("%s,%d,width:%d,output_bit_depth:%d\n",
					__func__, __LINE__, stCfg2NDDMAParam.width,
					isp_scl_obj->output_bit_depth);
		} else if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV444P) {
			stCfg2NDDMAParam.width  = ((isp_scl_obj->outputWin.width *
						isp_scl_obj->output_bit_depth * 3) + 7)/8;
			ISPSS_SCLDBG("%s,%d,width:%d,output_bit_depth:%d\n",
					__func__, __LINE__, stCfg2NDDMAParam.width,
					isp_scl_obj->output_bit_depth);
		} else if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_RGB888) {
			stCfg2NDDMAParam.width  =
				((isp_scl_obj->outputWin.width *
				  isp_scl_obj->output_bit_depth * 3) + 7) / 8;
		} else {

		}
		stCfg2NDDMAParam.stride = isp_scl_obj->ovp_win_stride;
		stCfg2NDDMAParam.height = isp_scl_obj->outputWin.height;
		stCfg2NDDMAParam.cfgQ   = cfgQ;
		if (psclRqstMsg->out_mtrMode) {
			stCfg2NDDMAParam.mtrREn   = psclRqstMsg->out_mtrMode;
			stCfg2NDDMAParam.updSemId = 0;
			stCfg2NDDMAParam.chkSemId = 0;
			stCfg2NDDMAParam.addr     = 0;
		} else {
			stCfg2NDDMAParam.mtrREn = 0;
		}
		*cfgQ_len += ISPSS_DhubCfg2NDDMA(&stCfg2NDDMAParam);
		cfgQ = cfgQ_shadow + *cfgQ_len * 2;

		/* Program 2NDDMA For write Client WR1
		 * If Output format is YUV422P/YUV444P/RGB444
		 * No need to Configure DHUB for WR1 client
		 */
		if ((isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422SP)) {
			ISPSS_SCLDBG("Entering DHUB FOR WR1\n");
			stCfg2NDDMAParam.dhubId = hwObjSt->dmaDhubID;
			stCfg2NDDMAParam.pdhubHandle = (struct HDL_dhub2d *)hwObjSt->dmaDhubID;
			stCfg2NDDMAParam.pBcmBuf = pSclBcmBuf->bcm_buf;
			stCfg2NDDMAParam.dmaId  = hwObjSt->dmaWID[1];
			stCfg2NDDMAParam.addr   = isp_scl_obj->outAddr_UV;
			ISPSS_SCLDBG("outAddr_Y:%lx outAddr_UV:%lx\n",
					(unsigned long) isp_scl_obj->outAddr_Y,
					(unsigned long)isp_scl_obj->outAddr_UV);
			stCfg2NDDMAParam.stride = isp_scl_obj->ovp_win_stride;
			stCfg2NDDMAParam.width  = ((isp_scl_obj->outputWin.width *
						isp_scl_obj->output_bit_depth) + 7)/8;
			stCfg2NDDMAParam.stride = isp_scl_obj->ovp_win_stride;
			if (isp_scl_obj->outSrcFrmt == ISPSS_SRCFMT_YUV422SP)
				stCfg2NDDMAParam.height = isp_scl_obj->outputWin.height;
			else
				stCfg2NDDMAParam.height = isp_scl_obj->outputWin.height/2;

			stCfg2NDDMAParam.stride = isp_scl_obj->ovp_win_stride;
			stCfg2NDDMAParam.addr   = isp_scl_obj->outAddr_UV;
			if (psclRqstMsg->out_mtrMode) {
				stCfg2NDDMAParam.mtrREn   = psclRqstMsg->out_mtrMode;
				stCfg2NDDMAParam.updSemId = 0;
				stCfg2NDDMAParam.chkSemId = 0;
				stCfg2NDDMAParam.addr     = 0;
			} else {
				stCfg2NDDMAParam.mtrREn = 0;
			}

			stCfg2NDDMAParam.cfgQ   = cfgQ;
			*cfgQ_len += ISPSS_DhubCfg2NDDMA(&stCfg2NDDMAParam);
			cfgQ = cfgQ_shadow + *cfgQ_len * 2;
		}
	}
	if (isp_scl_obj->ovp_win_updated == 2) {
		ISPSS_SCLDBG("Entering Updated Sclar Setting :%s,%d\n",
				__func__, __LINE__);
		cfgQ_shadow = (int *)pSclBcmBuf->dma_cfgQ->addr;
		cfgQ_len = &(pSclBcmBuf->dma_cfgQ->len);
		cfgQ = cfgQ_shadow + *cfgQ_len * 2;
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x002);
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x000);

		/*Program 2NDDMA For write client WR0*/
		stCfg2NDDMAParam.dhubId = hwObjSt->dmaDhubID;
		stCfg2NDDMAParam.pdhubHandle = (struct HDL_dhub2d *)hwObjSt->dmaDhubID;
		stCfg2NDDMAParam.pBcmBuf = pSclBcmBuf->bcm_buf;
		stCfg2NDDMAParam.dmaId  = hwObjSt->dmaWID[0];
		stCfg2NDDMAParam.addr   = isp_scl_obj->outAddr_Y;
		stCfg2NDDMAParam.width  = ((isp_scl_obj->outputWin.width *
					isp_scl_obj->output_bit_depth) + 7)/8;
		stCfg2NDDMAParam.stride = isp_scl_obj->ovp_win_stride;
		stCfg2NDDMAParam.height = isp_scl_obj->outputWin.height;
		stCfg2NDDMAParam.cfgQ   = cfgQ;
		stCfg2NDDMAParam.mtrREn = 0;
		*cfgQ_len += ISPSS_DhubCfg2NDDMA(&stCfg2NDDMAParam);
		cfgQ = cfgQ_shadow + *cfgQ_len * 2;

		/*Program 2NDDMA for write client WR1*/
		stCfg2NDDMAParam.dhubId = hwObjSt->dmaDhubID;
		stCfg2NDDMAParam.pdhubHandle = (struct HDL_dhub2d *)hwObjSt->dmaDhubID;
		stCfg2NDDMAParam.pBcmBuf = pSclBcmBuf->bcm_buf;
		stCfg2NDDMAParam.dmaId  = hwObjSt->dmaWID[1];
		stCfg2NDDMAParam.addr   = isp_scl_obj->outAddr_UV;
		stCfg2NDDMAParam.width  = ((isp_scl_obj->outputWin.width *
					isp_scl_obj->output_bit_depth) + 7)/8;
		stCfg2NDDMAParam.stride = isp_scl_obj->ovp_win_stride;
		stCfg2NDDMAParam.height = isp_scl_obj->outputWin.height/2;
		stCfg2NDDMAParam.cfgQ   = cfgQ;
		stCfg2NDDMAParam.mtrREn = 0;
		*cfgQ_len += ISPSS_DhubCfg2NDDMA(&stCfg2NDDMAParam);
		cfgQ = cfgQ_shadow + *cfgQ_len * 2;

		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x208);
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x000);


		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x0C0);
		ISPSS_BCMBUF_Write(pSclBcmBuf->clear_bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x000);

		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x030);
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x104);
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_tg_ctrl), 0x001);

		isp_scl_obj->crop_scl_actv_win.width = isp_scl_obj->outputWin.width;
		isp_scl_obj->crop_scl_actv_win.height = isp_scl_obj->outputWin.height;

		z++;
	}
	cfgQ_shadow = (int *)pSclBcmBuf->dma_cfgQ->addr;
	cfgQ_len = &(pSclBcmBuf->dma_cfgQ->len);
	cfgQ = cfgQ_shadow + *cfgQ_len * 2;
	//Set Input TG Params
	ISPSS_BE_SCL_SetINTGParams(isp_scl_obj, isp_scl_obj->inputWin.width,
			isp_scl_obj->inputWin.height, pSclBcmBuf->bcm_buf);


	if (IS_UPSAMPLING_ENABLE(isp_scl_obj->inSrcFrmt, isp_scl_obj->outSrcFrmt)) {
		ISPSS_BE_SCL_SetINTGParams_UV(isp_scl_obj, isp_scl_obj->inputWin.width,
				isp_scl_obj->inputWin.height, pSclBcmBuf->bcm_buf);
		hwObjSt->ovp_cfg9.uCFG9_clken_ctrl0_UV = 0;
		hwObjSt->ovp_cfg9.uCFG9_clken_ctrl1_UV = 0;
		hwObjSt->ovp_cfg9.uCFG9_clken_ctrl2_UV = 1;
		hwObjSt->ovp_cfg9.uCFG9_fifo_full_ctrl1_UV = 1;
		hwObjSt->ovp_cfg9.uCFG9_fifo_full_ctrl2_UV = 1;
		hwObjSt->ovp_cfg9.uCFG9_tg_sel = 1;
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf, (isp_scl_obj->base_addr +
					RA_OVPDNSCLWRAP_CFG9), hwObjSt->ovp_cfg9.u32);
	}

	//If input form ISP ,don't configure Read client.
	if (!psclRqstMsg->dnscl.isInputFrmIsp) {
		//In pix calculation
		hwObjSt->ovp_sclOFIFO_Y.usclOFIFO_YInpix_tot =
			(isp_scl_obj->inputWin.width * isp_scl_obj->inputWin.height);
		ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
				(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_sclOFIFO_YInpix),
				hwObjSt->ovp_sclOFIFO_Y.u32);

		if (((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
			(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) &&
			(psclRqstMsg->dnscl.isEnableSclBypass == DISABLE_BYPASSPATH)) {
			ISPSS_SCLDBG("Enter INPIX FOR UV\n");
			hwObjSt->ovp_sclOFIFO_UV.usclOFIFO_UVInpix_tot =
			(isp_scl_obj->inputWin.width * isp_scl_obj->inputWin.height)/2;//4
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
			(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_sclOFIFO_UVInpix),
			(isp_scl_obj->inputWin.width * isp_scl_obj->inputWin.height)/4);
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
			(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA)) {
			hwObjSt->ovp_sclOFIFO_UV.usclOFIFO_UVInpix_tot =
			(isp_scl_obj->inputWin.width *
			isp_scl_obj->inputWin.height);
			ISPSS_BCMBUF_Write(pSclBcmBuf->bcm_buf,
			(isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_sclOFIFO_UVInpix),
			(isp_scl_obj->inputWin.width * isp_scl_obj->inputWin.height/2));
		} else {
			// Don't configure UVInpix_tot for YUV444P/YUV422P/RGB444
		}

		//Progarm For Read Client Config of Y (RD0)
		ISPSS_FE_RD_CLIENT_ClientConfigSCLInstance(pSclBcmBuf->bcm_buf,
				ISP_OVPDNSCLWRAP_RD0, isp_scl_obj->inSrcFrmt,
				(isp_scl_obj->inputWin.width + isp_scl_obj->tg_crop.leftCrop +
				 isp_scl_obj->tg_crop.rightCrop), isp_scl_obj->inputWin.height,
				isp_scl_obj->in_bit_depth);

		//Progarm For Read Client Config of UV (RD1)
		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
			ISPSS_FE_RD_CLIENT_ClientConfigSCLInstance(pSclBcmBuf->bcm_buf,
				ISP_OVPDNSCLWRAP_RD1, isp_scl_obj->inSrcFrmt,
				(isp_scl_obj->inputWin.width + isp_scl_obj->tg_crop.leftCrop +
				isp_scl_obj->tg_crop.rightCrop),
				(isp_scl_obj->inputWin.height)/2, isp_scl_obj->in_bit_depth);
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA)) {
			ISPSS_FE_RD_CLIENT_ClientConfigSCLInstance(pSclBcmBuf->bcm_buf,
				ISP_OVPDNSCLWRAP_RD1, isp_scl_obj->inSrcFrmt,
				(isp_scl_obj->inputWin.width),
				(isp_scl_obj->inputWin.height),
				isp_scl_obj->in_bit_depth);
		} else {
			// Don't configure RD1 for YUV444P/YUV422P/RGB444
		}

		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
			hwObjSt->ovp_cfg0.uCFG0_rd_initval0 = 1;
			hwObjSt->ovp_cfg0.uCFG0_rd_initval1 = 1;
		}

		/*Stride Calculation */
		if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P) {
			isp_scl_obj->in_win_stride = (((isp_scl_obj->contentWin.width *
					isp_scl_obj->in_bit_depth * 2) + 127)/128) * 16;
		} else if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P ||
				isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888) {
			isp_scl_obj->in_win_stride = (((isp_scl_obj->contentWin.width *
					isp_scl_obj->in_bit_depth * 3) + 127)/128) * 16;
		} else if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
			isp_scl_obj->in_win_stride = ((((isp_scl_obj->contentWin.width *
					isp_scl_obj->in_bit_depth * 32)/30) + 127)/128) * 16;
		} else {
			isp_scl_obj->in_win_stride = (((isp_scl_obj->contentWin.width *
					isp_scl_obj->in_bit_depth) + 127)/128) * 16;
		}

		alignX = ISPSS_BE_SCL_getAlignX(isp_scl_obj->inSrcFrmt, isp_scl_obj->in_bit_depth,
				isp_scl_obj->in_mtrConfigDesc.m_mtrR_mode);
		if (!psclRqstMsg->in_mtrMode) {
			isp_scl_obj->inAddr_y = (isp_scl_obj->inAddr_y) +
				(isp_scl_obj->inputWin.y * isp_scl_obj->in_win_stride)+
				(((int)(isp_scl_obj->inputWin.x/alignX))*alignX);
		} else {
			isp_scl_obj->inAddr_y = (isp_scl_obj->inputWin.y * MMU_STRIDE_LENGTH) +
				(((int)(isp_scl_obj->inputWin.x/alignX))*alignX);
		}

		/*Program Luma scalar for dhub for Read Client*/
		stCfgRd2NDDMAParam.dhubId = hwObjSt->dmaDhubID;
		stCfgRd2NDDMAParam.pdhubHandle = (struct HDL_dhub2d *)hwObjSt->dmaDhubID;
		stCfgRd2NDDMAParam.pBcmBuf = pSclBcmBuf->bcm_buf;
		stCfgRd2NDDMAParam.dmaId  = hwObjSt->dmaRID[0];
		stCfgRd2NDDMAParam.addr   = isp_scl_obj->inAddr_y;

		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA))
			stCfgRd2NDDMAParam.width  = ((isp_scl_obj->inputWin.width) *
					((isp_scl_obj->in_bit_depth) * 32))/(30 * 8);
		else if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422P) {
			stCfgRd2NDDMAParam.width  = ((isp_scl_obj->inputWin.width *
						isp_scl_obj->in_bit_depth * 2) + 7)/8;
		} else if (isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV444P ||
				isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888) {
			stCfgRd2NDDMAParam.width  = ((isp_scl_obj->inputWin.width *
						isp_scl_obj->in_bit_depth * 3) + 7)/8;
		} else {
			stCfgRd2NDDMAParam.width  = (((isp_scl_obj->inputWin.width +
							isp_scl_obj->tg_crop.leftCrop +
							isp_scl_obj->tg_crop.rightCrop) *
						isp_scl_obj->in_bit_depth) + 7)/8;
		}

		if (psclRqstMsg->in_mtrMode) {
			stCfgRd2NDDMAParam.addr     = isp_scl_obj->inAddr_y;
			stCfgRd2NDDMAParam.stride   = isp_scl_obj->in_win_stride;
			stCfgRd2NDDMAParam.mtrREn   = psclRqstMsg->in_mtrMode;
			stCfgRd2NDDMAParam.updSemId = 0;
			stCfgRd2NDDMAParam.chkSemId = 0;
			stCfgRd2NDDMAParam.mtrREn = psclRqstMsg->in_mtrMode;
		} else {
			stCfgRd2NDDMAParam.stride = isp_scl_obj->in_win_stride;
			stCfgRd2NDDMAParam.mtrREn = 0;
		}
		stCfgRd2NDDMAParam.height = isp_scl_obj->inputWin.height;
		stCfgRd2NDDMAParam.cfgQ   = cfgQ;
		*cfgQ_len += ISPSS_DhubCfg2NDDMA(&stCfgRd2NDDMAParam);
		cfgQ = cfgQ_shadow + *cfgQ_len * 2;

		/* Program Chroma scalar for dhub for Read Client
		 * If Input format is YUV444P/YUV422P/RGB444 ,
		 * then no need of Read starting Read Client1
		 */
		if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA) ||
				(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA)) {
			if (!psclRqstMsg->in_mtrMode) {
				isp_scl_obj->inAddr_UV = isp_scl_obj->inAddr_UV +
					(isp_scl_obj->in_win_stride *
					 ((isp_scl_obj->inputWin.y)/2)+
					 (((int)(isp_scl_obj->inputWin.x/alignX)) * alignX));
			} else {
				isp_scl_obj->inAddr_UV = (MMU_STRIDE_LENGTH *
						((isp_scl_obj->inputWin.y)/2)+
						(((int)(isp_scl_obj->inputWin.x/alignX))*alignX));
			}
			ISPSS_SCLDBG("Entering READ CLIENT 1\n");
			stCfgRd2NDDMAParam.dhubId = hwObjSt->dmaDhubID;
			stCfgRd2NDDMAParam.pdhubHandle = (struct HDL_dhub2d *)hwObjSt->dmaDhubID;
			stCfgRd2NDDMAParam.pBcmBuf = pSclBcmBuf->bcm_buf;
			stCfgRd2NDDMAParam.dmaId  = hwObjSt->dmaRID[1];
			stCfgRd2NDDMAParam.addr   = isp_scl_obj->inAddr_UV;

			if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV422SP_DWA) ||
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA)) {
				stCfgRd2NDDMAParam.width  = ((isp_scl_obj->inputWin.width) *
						((isp_scl_obj->in_bit_depth) * 32))/(30*8);
			} else {
				stCfgRd2NDDMAParam.width  = (((isp_scl_obj->inputWin.width +
								isp_scl_obj->tg_crop.leftCrop +
								isp_scl_obj->tg_crop.rightCrop) *
							isp_scl_obj->in_bit_depth)+7)/8;
			}

			if (psclRqstMsg->in_mtrMode) {
				stCfgRd2NDDMAParam.addr = isp_scl_obj->inAddr_UV;
				stCfgRd2NDDMAParam.stride   = isp_scl_obj->in_win_stride;
				stCfgRd2NDDMAParam.mtrREn   = psclRqstMsg->in_mtrMode;
				stCfgRd2NDDMAParam.updSemId = 0;
				stCfgRd2NDDMAParam.chkSemId = 0;
			} else {
				stCfgRd2NDDMAParam.stride = isp_scl_obj->in_win_stride;
				stCfgRd2NDDMAParam.mtrREn = 0;
			}
			if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP) ||
					(isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_YUV420SP_DWA))
				stCfgRd2NDDMAParam.height = isp_scl_obj->inputWin.height/2;
			else
				stCfgRd2NDDMAParam.height = isp_scl_obj->inputWin.height;

			stCfgRd2NDDMAParam.cfgQ   = cfgQ;
			*cfgQ_len += ISPSS_DhubCfg2NDDMA(&stCfgRd2NDDMAParam);
			cfgQ = cfgQ_shadow + *cfgQ_len * 2;
		}
	}

	if (psclRqstMsg->in_mtrMode || psclRqstMsg->out_mtrMode)
		ISPSS_BE_MTR_kickOff(pSclBcmBuf->bcm_buf, 1);

	/* Start Clients of SCL */
	ISPSS_BE_SCL_ClientStart(isp_scl_obj, pSclBcmBuf, hwObjSt, psclRqstMsg);
}

/*********************************************************************
 * FUNCTION: Push a frame for SCL processing
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_BE_SCL_Start(struct ISP_BE_RQST_MSG *psclRqstMsg, struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{

	int ret = ISPSS_OK;
	struct ISP_BE_BCM *pSclBcmBuff =  &(psclRqstMsg->bcmBuf);
	/*Clear Clients */
	ISPSS_BE_SCL_ClientClear(isp_scl_obj, pSclBcmBuff, hwObjSt, psclRqstMsg);
	/*Program SCL pipe */
	ISPSS_BE_SCL_programSclPipe(pSclBcmBuff, isp_scl_obj, hwObjSt, psclRqstMsg);

	return ret;
}

/******************************************************************
 * FUNCTION: Get interrupt status
 * PARAMS: pstdewarpRqstMsg - Request message
 * RETURN: 0 - No interrupt
 *         1 - Interrupt
 *****************************************************************/
INT ISPSS_BE_SCL_FrameInterrupt(struct ISP_BE_RQST_MSG *psclRqstMsg)
{
	int instat;
	struct HDL_semaphore *pSemHandle =
		ispss_dhub_semaphore(&ISPSS_TSB_dhubHandle.dhub);
	/* check dnscl interrupt status to figure out which CPCB interrupt */
	instat = ispss_semaphore_chk_full(pSemHandle, -1);

	if ((bTST(instat, ispDhubSemMap_TSB_Scldn_intr2))) {
		ISPSS_SCLDBG("Interrupt TSB set\n");
		return instat;
	} else {
		return 0;
	}
}
