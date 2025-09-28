// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispSS_be_frc_scl.h"
#include "ispSS_be_scl_common.h"
#include "ispSS_be_frc_scl_prv.h"
#include "ispSS_coeff_adp_scl_soft.h"

#define   bSCL1D_YC_COEFF_CFG1_coeff0          12
#define   MSK32SCL1D_YC_COEFF_CFG1_coeff0      0x00000FFF

/*********************************************************
 * FUNCTION: INTernal function to get the start address
 * of an FRC SCL instance by unit number
 * PARAMS: UnitNum - unit number
 * RETURN:  the start address
 ********************************************************/
UINT32 ISPSS_INTERNAL_FRC_SCL_GetRegStartAddr(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		enum ISP_FRC_SCL_NUM UnitNum)
{
	UINT32 uiRegAddr = 0;

	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC;
		break;
	case ISP_FRC_SCL_OVP_UV:
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC;
		break;
	default:
		return 0;
	}

	return uiRegAddr;
}

/****************************************************************
 * FUNCTION: load the default register values to FRC SCL the unit
 * PARAMS: UnitNum - unit number
 * RETURN:  1 - succeed
 * others - failed to load CPCB EE default values to BCM buffe
 *****************************************************************/
INT ISPSS_FRC_SCL_LoadDefaultVal(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	INT uiRetVal = ISPSS_OK;
	UINT32 uiRegAddr = 0;

	uiRegAddr = ISPSS_INTERNAL_FRC_SCL_GetRegStartAddr(isp_scl_obj, UnitNum);

	hwObjSt->uiSclCfg15[UnitNum] = 0;
	hwObjSt->uiSclCfg0[UnitNum] = 0;

	hwObjSt->hscl_coeff[UnitNum] = -1; // invalid
	hwObjSt->vscl_coeff[UnitNum] = -1; // invalid

	/* For customized SCL coeffs */
	hwObjSt->hscl_coeff_update[UnitNum] = 0; // No customized
	hwObjSt->vscl_coeff_update[UnitNum] = 0; // No customized

	hwObjSt->scl_coeff_update[UnitNum] = 0;
	hwObjSt->scl_force_sel[UnitNum] = MODE_COEFF_SEL_AUTO;

	/*Cropping related initialization*/
	isp_scl_obj->scl_ctrl[UnitNum].leftCrop     = 0;
	isp_scl_obj->scl_ctrl[UnitNum].rightCrop    = 0;
	isp_scl_obj->scl_ctrl[UnitNum].uncrop_IHRes = 0;
	isp_scl_obj->scl_ctrl[UnitNum].uiUVswapEn   = 0;

	/* Crop after scaling related initialization */
	isp_scl_obj->scl_ctrl[UnitNum].cropAfterSclLeft    = 0;
	isp_scl_obj->scl_ctrl[UnitNum].cropAfterSclRight   = 0;
	isp_scl_obj->scl_ctrl[UnitNum].cropAfterSclTop     = 0;
	isp_scl_obj->scl_ctrl[UnitNum].cropAfterSclBottom  = 0;
	return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Read the  Main  scalar coefficients
 * PARAMS:
 *             isp_scl_obj - isp scl object
 *             UnitNum - Unit num to identify the plain
 *             HTapN   - Number of H Taps
 *             VTapN   - Number of V Taps
 *             hlut    - Horizontal coefficients
 *             vlut    - Vertical Coefficients
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer or invalid input
 ***************************************************************************************
 */
INT ISPSS_SCL_MainCoeffRead(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut, INT16 **vlut, struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	UINT32 uiRegAddr = 0;
	INT16 i;
	INT uiRetVal = ISPSS_OK;

	T32SCL1D_YC_COEFF_CFG0 stSclCoeffCfg0;
	T32SCL1D_YC_COEFF_CFG1 stSclCoeffCfg1;
	T32SCL1D_YC_COEFF_CFG2 stSclCoeffCfg2;
	T32SCL1D_YC_COEFF_CFG3 stSclCoeffCfg3;
	T32SCL1D_YC_COEFF_CFG4 stSclCoeffCfg4;

	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
		uiRegAddr = isp_scl_obj->base_addr +
			RA_OVPDNSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_COEFF;
		break;
	case ISP_FRC_SCL_OVP_UV:
		uiRegAddr = isp_scl_obj->base_addr +
			RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_COEFF;
		break;
	default:
		return ISPSS_EBADPARAM;
	}
	GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, &stSclCoeffCfg0.u32);
	stSclCoeffCfg0.uCFG0_coeffload   = 0;

	/*Horizontal Coeff table loading*/
	stSclCoeffCfg0.uCFG0_coeff_hvsel = 1;
	for (i = 0; i <= 16; i++) {
		stSclCoeffCfg0.uCFG0_coeff_index = i;
		GA_REG_WORD32_WRITE(uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, stSclCoeffCfg0.u32);

		stSclCoeffCfg0.uCFG0_coeffread   = 1;
		GA_REG_WORD32_WRITE(uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, stSclCoeffCfg0.u32);

		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG1, &stSclCoeffCfg1.u32);
		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG2, &stSclCoeffCfg2.u32);
		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG3, &stSclCoeffCfg3.u32);
		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG4, &stSclCoeffCfg4.u32);

		hlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
		hlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
		hlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
		hlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
		hlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
		hlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
		hlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
		hlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;
		stSclCoeffCfg0.uCFG0_coeffread   = 0;
		GA_REG_WORD32_WRITE(uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, stSclCoeffCfg0.u32);
	}

	/*Vertical Coeff table loading*/
	stSclCoeffCfg0.uCFG0_coeff_hvsel = 0;
	for (i = 0; i <= 16; i++) {
		stSclCoeffCfg0.uCFG0_coeff_index = i;
		GA_REG_WORD32_WRITE(uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, stSclCoeffCfg0.u32);

		stSclCoeffCfg0.uCFG0_coeffread   = 1;
		GA_REG_WORD32_WRITE(uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, stSclCoeffCfg0.u32);

		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG1, &stSclCoeffCfg1.u32);
		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG2, &stSclCoeffCfg2.u32);
		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG3, &stSclCoeffCfg3.u32);
		GA_REG_WORD32_READ(uiRegAddr + RA_SCL1D_YC_COEFF_CFG4, &stSclCoeffCfg4.u32);

		vlut[i][0] = stSclCoeffCfg1.uCFG1_coeff0;
		vlut[i][1] = stSclCoeffCfg1.uCFG1_coeff1;
		vlut[i][2] = stSclCoeffCfg2.uCFG2_coeff2;
		vlut[i][3] = stSclCoeffCfg2.uCFG2_coeff3;
		vlut[i][4] = stSclCoeffCfg3.uCFG3_coeff4;
		vlut[i][5] = stSclCoeffCfg3.uCFG3_coeff5;
		vlut[i][6] = stSclCoeffCfg4.uCFG4_coeff6;
		vlut[i][7] = stSclCoeffCfg4.uCFG4_coeff7;
		stSclCoeffCfg0.uCFG0_coeffread   = 0;
		GA_REG_WORD32_WRITE(uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, stSclCoeffCfg0.u32);
	}

	return uiRetVal;

}
static INT ISPSS_SCL_MainSclAdp1dType(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_RES *pIORes, INT8 *sel_1D_adp)
{
	*sel_1D_adp = 1;/* 1D Scalar */

	return ISPSS_OK;
}
/*  -calculate htap and vtap numbers- */
static void ISPSS_SCL_MainSclCalTaps(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, struct ISP_BE_SCL_RES *pIORes, INT8 *phTap,
		INT8 *pvTap, struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	INT8 sel_1D_adp, hTaps, vTaps;
	UINT32 max_pixels_in_scl_mem = 1920;

	ISPSS_SCL_MainSclAdp1dType(isp_scl_obj, pIORes, &sel_1D_adp);
	if (sel_1D_adp == 0) {
		hTaps = 4;
		vTaps = 4;
	} else {
		if ((pIORes->OHRes > pIORes->IHRes) && (pIORes->OHRes > 1920))
			hTaps = 5;
		else if ((pIORes->OHRes > pIORes->IHRes) && (pIORes->IHRes > 1920))/* Upscale */
			hTaps = 5;
		else
			hTaps = 8;//8
		max_pixels_in_scl_mem = 960;
		//else if ((pIORes->OHRes <= pIORes->IHRes) || (pIORes->IHRes <= 1920))
		if ((pIORes->IVRes == pIORes->OVRes) && (pIORes->OHRes == pIORes->IHRes))
			vTaps = 1;
		else if (pIORes->IVRes == pIORes->OVRes)
			vTaps = 1;
		else if (pIORes->OHRes > pIORes->IHRes)
			vTaps = 3;
		else if ((pIORes->OHRes > pIORes->IHRes)
			&& (pIORes->IHRes > max_pixels_in_scl_mem))
		/* Upscale */
			vTaps = 3;
		else if ((pIORes->OHRes <= pIORes->IHRes) &&
				(pIORes->IHRes > 1920) && (pIORes->OHRes > 1920))
		/* DownScale 4K display*/
			vTaps = 3;
		else if (pIORes->OHRes <= pIORes->IHRes)/* DownScale */
			/* OVP and VIP SCL has memsize max ((575+1)*10) pixles.
			 * So if OHRes less than 960 only can hold 6 lines
			 */
			if (pIORes->OHRes < 960)
				vTaps = 6;
			else
				vTaps = 3;
			//Avoid Blink issue during tap change in multi stream mode, need to revist
		else if ((pIORes->OVRes > pIORes->IVRes) && (pIORes->OVRes > 1440))
		/* Fixing the faint line issue for up scaling o/p Vres more than 1440 */
			vTaps = 5;
		else
			vTaps = 6;

	}

	if ((isp_scl_obj->inSrcFrmt == ISPSS_SRCFMT_RGB888) && (pIORes->OHRes > 960))
		vTaps = 2;

	*phTap = hTaps;
	*pvTap = vTaps;
	ISPSS_SCLDBG("%s,%d, Htaps:%d Vtaps:%d OHRES:%d\n",
		__func__, __LINE__, *phTap, *pvTap, pIORes->OHRes);
}

  /* -Configure Scl for special cases in Deint- */
static INT ISPSS_SCL_SclConfigForDeint(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, INT8 *enDeintSmallUpScale)
{
	*enDeintSmallUpScale = 0;
	return ISPSS_OK;
}

/***************************************************************************************
 * FUNCTION: Load the Main scalar parameters
 * PARAMS:
 *           isp_scl_obj - ISP SCL object
 *           UnitNum - Unit num to identify the plain
 *           pIORes - Input/Output resolution
 *           pSclCtrl - Scalar Control parameters
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
static INT ISPSS_SCL_SetMainSclparams(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf)
{
	INT8 sel_1D_adp = -1;
	INT8 up_down = -1;
	UINT32 HRatio, VRatio;
	UINT32 minHRes, hRes;
	INT8 hTaps = -1, vTaps = -1;
	INT8 enDeintSmallUpScale = 0;
	INT16 wls = 0, pls = 0, fcp = 0;
	INT32 memsize = 0;
	INT16 maxMemSize_1D = 0;

	union ISP_SCL_CTRL0 stSclCtrl0;
	union ISP_SCL_CTRL1 stSclCtrl1;
	INT uiRetVal = ISPSS_OK;

	T32SCL1D_YC_CFG0 stSclCfg0;
	T32SCL1D_YC_CFG1 stSclCfg1;
	T32SCL1D_YC_CFG2 stSclCfg2;
	T32SCL1D_YC_CFG3 stSclCfg3 = {0};
	T32SCL1D_YC_CFG4 stSclCfg4 = {0};
	T32SCL1D_YC_CFG5 stSclCfg5;
	T32SCL1D_YC_CFG6 stSclCfg6;
	T32SCL1D_YC_CFG7 stSclCfg7 = {0};
	T32SCL1D_YC_CFG8 stSclCfg8;
	T32SCL1D_YC_CFG9 stSclCfg9;
	T32SCL1D_YC_CFG10 stSclCfg10;
	T32SCL1D_YC_CFG11 stSclCfg11;
	T32SCL1D_YC_CFG12 stSclCfg12;
	T32SCL1D_YC_CFG15 stSclCfg15;
	T32SCL1D_YC_CFG16 stSclCfg16;
	T32SCL1D_YC_CFG17 stSclCfg17;
	T32SCL1D_YC_CFG18 stSclCfg18;
	T32SCL1D_YC_CFG19 stSclCfg19;
	T32SCL1D_YC_CFG20 stScl1dYcCfg20;

	UINT32 uiRegAddr = 0;
	UINT32 uncrop_IHRes = pSclCtrl->uncrop_IHRes;

	if (ISP_FRC_SCL_OVP_Y != UnitNum && ISP_FRC_SCL_OVP_UV != UnitNum)
		return ISPSS_EBADPARAM;

	/* select the scalar */
	ISPSS_SCL_SclConfigForDeint(isp_scl_obj, UnitNum, pIORes, &enDeintSmallUpScale);
	ISPSS_SCL_MainSclAdp1dType(isp_scl_obj, pIORes, &sel_1D_adp);

	/*  - select the up/down scale mode - */

	if ((pIORes->OHRes < pIORes->IHRes) || ((pIORes->OHRes == pIORes->IHRes) &&
		(pIORes->OVRes <  pIORes->IVRes)))
		/* Horizontal down scaling or veritcal only down scaling */
		up_down = 1;/* down scale */
	else
		up_down = 0;/* up scale/1:1(bypass) */

	uiRegAddr = ISPSS_INTERNAL_FRC_SCL_GetRegStartAddr(isp_scl_obj, UnitNum);
	/* Following this are SCL programming */

	/*  -calculate htap and vtap numbers */
	ISPSS_SCL_MainSclCalTaps(isp_scl_obj, UnitNum, pIORes, &hTaps, &vTaps, hwObjSt);
	/* set initial phase and ctl0 - */
	if (sel_1D_adp == 0) {
		stSclCfg0.uCFG0_even = 0x80;
		stSclCfg0.uCFG0_odd = 0x80;
		stSclCfg0.uCFG0_hinitph = 0x80;
	} else {
		stSclCfg0.uCFG0_even = (vTaps % 2)<<7;
		stSclCfg0.uCFG0_odd = (vTaps % 2)<<7;
		stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
	}

	stSclCtrl0.uchAvg4FiltSel = 0;
	stSclCtrl0.uchCscEn = 0;
	if (pSclCtrl->NLEn == 1)
		stSclCtrl0.uchHnlEn = 1;
	else
		stSclCtrl0.uchHnlEn = 0;

	if (sel_1D_adp == 1 && up_down == 1) {
		/* 1D down scaling */
		stSclCtrl0.uchUpsEn = 1;
		stSclCtrl0.uchDnsEn = 1;
	} else {
		stSclCtrl0.uchUpsEn = 0;
		switch (UnitNum) {
		case ISP_FRC_SCL_OVP_Y:
		case ISP_FRC_SCL_OVP_UV:
			stSclCtrl0.uchDnsEn = 0;
			break;
		default:
			return ISPSS_EBADPARAM;
		}
	}
	stSclCtrl0.uchDgnlEn = 1;
	stSclCtrl0.uchDemoModeEn = 0;

	//This is not required if clock gating is done after SetSclParams()
	stSclCfg15.u32 = hwObjSt->uiSclCfg15[UnitNum];

	if (stSclCfg15.uCFG15_bypass)
		//Enable UPS
		stSclCtrl0.uchUpsEn = 1;

	stSclCfg0.uCFG0_ctrl0 = stSclCtrl0.uch8;

	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG0, stSclCfg0.u32));
	hwObjSt->uiSclCfg0[UnitNum] = stSclCfg0.u32;
	/*	  - set input and output resolutions - */

	stSclCfg1.uCFG1_ivres = pIORes->IVRes;
	stSclCfg1.uCFG1_ovres = pIORes->OVRes;
	//stSclCfg2.uCFG2_ihres = pIORes->IHRes;
	stSclCfg2.uCFG2_ihres = uncrop_IHRes;          //Read the complete uncropped resolution
	stSclCfg2.uCFG2_ohres = pIORes->OHRes;

	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG1, stSclCfg1.u32));
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG2, stSclCfg2.u32));
	/*  -calculate horizontal and vertical scaling ratio - */

	HRatio = ((UINT32) pIORes->IHRes << 20) / pIORes->OHRes;
	VRatio = ((UINT32) pIORes->IVRes << 20) / pIORes->OVRes;

	stSclCfg3.u32 = 0;
	stSclCfg4.u32 = 0;
	stSclCfg3.uCFG3_vratio = VRatio;
	stSclCfg4.uCFG4_hratio = HRatio;
	if (pIORes->OVRes < pIORes->IVRes)
		/* 1: the most bottom pixel of input image is used to
		 * interpolate the most bottom pixel of output image
		 */
		stSclCfg3.uCFG3_mbp_align = 1;
	else
		/* Must be 0 if Vertically '1:1 or upscale' */
		stSclCfg3.uCFG3_mbp_align = 0;

	if (pIORes->OHRes < pIORes->IHRes)
		/* 1: the most right pixel of input image is used to
		 * interpolate the most right pixel of output image
		 */
		stSclCfg4.uCFG4_mrp_align = 1;
	else
		/* Must be 0 if Horizontally '1:1 or upscale' */
		stSclCfg4.uCFG4_mrp_align = 0;

	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG3, stSclCfg3.u32));
	hwObjSt->uiSclCfg3[UnitNum] = stSclCfg3.u32;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG4, stSclCfg4.u32));

	stSclCfg7.uCFG7_ups_cblank = 0x200;

	stSclCfg7.uCFG7_htap_offset = 0;
	stSclCfg7.uCFG7_vtap_offset = 0;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG7, stSclCfg7.u32));
	hwObjSt->uiSclCfg7[UnitNum] = stSclCfg7.u32;

	/*  -set SCL control register- */
	stSclCfg8.uCFG8_init_ratio = 0x100000;
	stSclCfg9.uCFG9_inc_ratio  = 0;
	stSclCfg10.uCFG10_nlcres   = 0;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG8, stSclCfg8.u32));
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG9, stSclCfg9.u32));
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG10, stSclCfg10.u32));

	stSclCfg11.uCFG11_avg4_coeff0 = 0x840;
	stSclCfg11.uCFG11_avg4_coeff1 = 0x240;

	stSclCfg11.uCFG11_dign_dr1 = 0x50;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG11, stSclCfg11.u32));

	stSclCfg12.uCFG12_avg4_coeff2 = 0x240;
	stSclCfg12.uCFG12_avg4_coeff3 = 0x840;
	stSclCfg12.uCFG12_dign_dr2 = 0x14;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG12, stSclCfg12.u32));
	stSclCtrl1.uch8 = 0;
	stSclCtrl1.uchSel_1D_adp = sel_1D_adp;
	stSclCtrl1.uchUp_down = up_down;
	if (sel_1D_adp == 0) {
		stSclCtrl1.uchVSclInUpsEn    = 1;
		stSclCtrl1.uchVSclOutUpsEn   = 0;
		stSclCtrl1.uchVSclInUpsCoeff = 0;
	} else {
		stSclCtrl1.uchVSclInUpsEn    = 0;
		stSclCtrl1.uchVSclInUpsCoeff = 0;

		switch (UnitNum) {
		case ISP_FRC_SCL_OVP_Y:
		case ISP_FRC_SCL_OVP_UV:
			stSclCtrl1.uchVSclOutUpsEn   = 0;
			break;
		default:
			return ISPSS_EBADPARAM;
		}

	}

	stSclCfg15.u32 = hwObjSt->uiSclCfg15[UnitNum];
	stSclCfg15.uCFG15_ctrl1 = stSclCtrl1.uch8;
	stSclCfg15.uCFG15_frndsel = 1;
	stSclCfg15.uCFG15_hctrl = 0x1A;
	stSclCfg15.uCFG15_vscl11 = 1;
	stSclCfg15.uCFG15_hscl11 = 1;
	if ((vTaps == 1 || vTaps == 2)
			&& (stSclCfg15.uCFG15_vscl11 == 0)
	  )
		stSclCfg15.uCFG15_vctrl = 0x0B;
	else
		stSclCfg15.uCFG15_vctrl = 0x1A;

	//LSB 2 bit for to crop at beginning of line, MSB 2 bit to crop at end of line
	stSclCfg15.uCFG15_crop = ((pSclCtrl->leftCrop&0x3)) | ((pSclCtrl->rightCrop&0x03) << 2);

	//VDB_print("Scalar crop control : 0x%x\n", stSclCfg15.uCFG15_crop);

	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG15, stSclCfg15.u32));
	hwObjSt->uiSclCfg15[UnitNum] = stSclCfg15.u32;

	//lsize calculation should be based on complete/uncropped resolution
	hRes = uncrop_IHRes;
	if (sel_1D_adp == 0) {
		stSclCfg17.uCFG17_lsize   = (hRes / 10) + ((hRes % 10) ? 1:0);
		stSclCfg17.uCFG17_lsize_A = (hRes / 10) + ((hRes % 10) ? 1:0);
	} else {
		if (hRes < pIORes->OHRes)
			minHRes = hRes;
		else
			minHRes = pIORes->OHRes;
		/* If Scalar chop is enabled, then choose hres as follows
		 * Upscale, -> hres = uncropped ihres
		 * downscale, -> hres = ohres
		 * 1:1, -> hres = uncropped ihres
		 */
		if ((pSclCtrl->leftCrop+pSclCtrl->rightCrop) != 0) {
			//Use cropped Hres for comparision
			if (pIORes->IHRes <= pIORes->OHRes)
				minHRes = hRes;
			else
				minHRes = pIORes->OHRes;
		}
		stSclCfg17.uCFG17_lsize   = (minHRes / 10) + ((minHRes % 10) ? 1:0);
		stSclCfg17.uCFG17_lsize_A = (minHRes / 10) + ((minHRes % 10) ? 1:0);
	}
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG17, stSclCfg17.u32));

	stSclCfg18.u32 = 0;
	if (UnitNum == ISP_FRC_SCL_OVP_UV)
		maxMemSize_1D = 287;
	else
		maxMemSize_1D = 575;

	/* Prefetch only enable in 4K small downscaling with 4K canvas.
	 * If enabled for bigger PIP style cases needs specific tuning
	 * of udf/ovf margins, which is not required. Making the min resolution
	 * to ISP_RES_MIN_4Kx2K instead of 4K30 to avoid line issue when small
	 * down scaling while playing Dolby L5 cert streams
	 */
	if ((pIORes->IVRes > pIORes->OVRes) &&
		((isp_scl_obj->inputWin.height*(100-14)/100) <= pIORes->OVRes))
		/* Tune the limit if required */
		stSclCfg18.uCFG18_pftch = 1;
	else
		stSclCfg18.uCFG18_pftch = 0;
	if (stSclCfg18.uCFG18_pftch == 1) {
		memsize = (stSclCfg17.uCFG17_lsize * vTaps) - 1;
		if ((sel_1D_adp == 1) && (memsize > maxMemSize_1D))
			stSclCfg5.uCFG5_memsize = maxMemSize_1D;
		else if ((sel_1D_adp == 0) && (memsize > 1727))
			stSclCfg5.uCFG5_memsize = 1727;
		else
			stSclCfg5.uCFG5_memsize = memsize;
	} else {
		if (sel_1D_adp == 0)
			stSclCfg5.uCFG5_memsize = 1727;
		else
			stSclCfg5.uCFG5_memsize = maxMemSize_1D;
	}
	stSclCfg5.uCFG5_vwrap   = 0;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG5, stSclCfg5.u32));

	stSclCfg6.uCFG6_ups_cswap = pSclCtrl->uiUVswapEn;
	stSclCfg6.uCFG6_ups_cshift = 0;
	stSclCfg6.uCFG6_ups_yshift = 0;
	stSclCfg6.uCFG6_ups_yblank = 0;

	if ((pIORes->IHRes > 1920 && (((pIORes->IHRes == pIORes->OHRes)
		&& (pIORes->IVRes == pIORes->OVRes))/* 1:1 case */ ||
		(pIORes->IVRes > pIORes->OVRes && ((((pIORes->IVRes - pIORes->OVRes) * 100) /
		(pIORes->IVRes)) <= 13))))
		/* vertical only or both vertical and horizontal down scaling till 13% */
			|| (stSclCfg18.uCFG18_pftch == 1)
	  )
		stSclCfg6.uCFG6_fstall = 0x0;
	else
		stSclCfg6.uCFG6_fstall = 0x80;
	stSclCfg6.uCFG6_bstall = 0x80;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG6, stSclCfg6.u32));
	hwObjSt->uiSclCfg6[UnitNum] = stSclCfg6.u32;

	if (((pIORes->IVRes == pIORes->OVRes) && (vTaps == 1))
			&& (stSclCfg15.uCFG15_vscl11 == 0)
	  )
		stSclCfg18.uCFG18_fifo_mode  = 1;
	else
		stSclCfg18.uCFG18_fifo_mode  = 0; //Lindo

	if ((stSclCfg18.uCFG18_fifo_mode == 1)
			|| (stSclCfg18.uCFG18_pftch == 1)
	  ) {

		if (pIORes->IHRes < pIORes->OHRes)
			minHRes = pIORes->IHRes;
		else
			minHRes = pIORes->OHRes;


		wls = (stSclCfg5.uCFG5_memsize + 1) / stSclCfg17.uCFG17_lsize;
		pls = ((stSclCfg5.uCFG5_memsize + 1) - (stSclCfg17.uCFG17_lsize * wls)) * 10;
		fcp = wls * minHRes + pls;

		stSclCfg18.uCFG18_fifo_depth = (stSclCfg18.uCFG18_pftch) ? fcp : (fcp - 100);
		if (stSclCfg18.uCFG18_fifo_mode == 0)
			stSclCfg18.uCFG18_fifo_dfst  = 0;
		else
			stSclCfg18.uCFG18_fifo_dfst  = minHRes;
	} else {
		stSclCfg18.uCFG18_fifo_mode  = 0;
		stSclCfg18.uCFG18_fifo_depth = 0;
		stSclCfg18.uCFG18_fifo_dfst  = 0;
	}
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG18, stSclCfg18.u32));
	stSclCfg16.u32 = 0;

	if ((up_down == 0)
			|| (stSclCfg18.uCFG18_pftch == 1)
	  )
		stSclCfg16.uCFG16_en_n          = 1;
	else
		stSclCfg16.uCFG16_en_n          = 0;

	if ((pIORes->OHRes > pIORes->IHRes &&  pIORes->OVRes < pIORes->IVRes) ||
			(pIORes->OHRes < pIORes->IHRes &&  pIORes->OVRes > pIORes->IVRes))
		stSclCfg16.uCFG16_xbstall_en    = 0;
	else if (enDeintSmallUpScale == 1)
		stSclCfg16.uCFG16_xbstall_en    = 0;
	else if ((stSclCfg16.uCFG16_en_n == 1 && up_down == 0) ||
			(stSclCfg18.uCFG18_fifo_mode == 1))
		stSclCfg16.uCFG16_xbstall_en    = 1;
	else
		stSclCfg16.uCFG16_xbstall_en    = 0;

	if ((pIORes->OHRes - pIORes->IHRes) >= 30)
		stSclCfg16.uCFG16_xbstall_dly   = 30;
	else
		stSclCfg16.uCFG16_xbstall_dly   = 0;

	if (stSclCfg18.uCFG18_pftch == 1) {
		stSclCfg16.uCFG16_ovf_margin = 0xA;
		stSclCfg16.uCFG16_udf_margin = 0x14;
	} else {
		stSclCfg16.uCFG16_ovf_margin = 0;
		stSclCfg16.uCFG16_udf_margin = 0;
	}

		CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
			uiRegAddr + RA_SCL1D_YC_CFG16, stSclCfg16.u32));
	stSclCfg19.u32 = 0;
	stSclCfg19.uCFG19_scl_after_crop_h =
		((pSclCtrl->cropAfterSclLeft << 4) | (pSclCtrl->cropAfterSclRight));
	stSclCfg19.uCFG19_scl_after_crop_v =
		((pSclCtrl->cropAfterSclTop << 4) | (pSclCtrl->cropAfterSclBottom));

	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG19, stSclCfg19.u32));
	stScl1dYcCfg20.u32 = 0;
	stScl1dYcCfg20.uCFG20_dpwr_regs = 3;
	stScl1dYcCfg20.uCFG20_syp_yc420_regs = 0;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG20, stScl1dYcCfg20.u32));

	return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_ReadSclCoeff(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut, INT16 **vlut,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	INT uiRetVal = ISPSS_OK;

	if ((UnitNum < ISP_FRC_SCL_OVP_Y) || (UnitNum >= ISP_FRC_SCL_MAX))
		return ISPSS_EBADPARAM;

	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
	case ISP_FRC_SCL_OVP_UV:
		ISPSS_SCL_MainCoeffRead(isp_scl_obj, UnitNum, hlut, vlut, hwObjSt);
		break;
	default:
		return ISPSS_EBADPARAM;
	}

	return uiRetVal;
}
/***************************************************************************************
 * FUNCTION: Print scl coeff shadow used for writing
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_PrintSclCoeffShadow(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut, INT16 **vlut,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	INT uiRetVal = ISPSS_OK;
	UINT64 *ptr = NULL;
	int sclLayer;
	int coeffMsk = 0;
	int coeffBitWidth = 0;
	int hLutFailCount = 0;
	int vLutFailCount = 0;
	static int hLutFailIndex[ISP_FRC_SCL_NUM_OF_PHASES][ISP_FRC_SCL_NUM_OF_COEFF];
	static int vLutFailIndex[ISP_FRC_SCL_NUM_OF_PHASES][ISP_FRC_SCL_NUM_OF_COEFF];
	INT i = 0, j = 0;

	if ((UnitNum < ISP_FRC_SCL_OVP_Y) || (UnitNum >= ISP_FRC_SCL_MAX))
		return ISPSS_EBADPARAM;

	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
		sclLayer = 0;//3
		coeffMsk = MSK32SCL1D_YC_COEFF_CFG1_coeff0;
		coeffBitWidth = bSCL1D_YC_COEFF_CFG1_coeff0;
		break;
	case ISP_FRC_SCL_OVP_UV:
		sclLayer = 1;//4
		coeffMsk = MSK32SCL1D_YC_COEFF_CFG1_coeff0;
		coeffBitWidth = bSCL1D_YC_COEFF_CFG1_coeff0;
		break;
	default:
		return ISPSS_EBADPARAM;
	}

	ptr = hwObjSt->scl_coeffs[hwObjSt->hscl_coeff[UnitNum]];
	ptr += ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

	pr_debug("Shadow BCM:Default:Read HLUT\n");
	pr_debug("ISP_FRC_SCL_NUM_OF_PHASES = %d ISP_FRC_SCL_NUM_OF_COEFF =  %d\n",
			ISP_FRC_SCL_NUM_OF_PHASES, ISP_FRC_SCL_NUM_OF_COEFF);
	for (i = 0; i < ISP_FRC_SCL_NUM_OF_PHASES; i++) {
		for (j = 0; j < ISP_FRC_SCL_NUM_OF_COEFF; j += 2) {
			if ((hlut[i][j] != (*ptr & coeffMsk)) || (hlut[i][j] !=
		isp_scl_obj->def_idx_coeff_tbl[hwObjSt->hscl_coeff[UnitNum]][i][j])) {
				hLutFailIndex[hLutFailCount][0] = i;
				hLutFailIndex[hLutFailCount][1] = j;
				hLutFailCount++;
			}
			if ((hlut[i][j+1] != ((*ptr >> coeffBitWidth) & coeffMsk))
				|| (hlut[i][j+1] !=
		isp_scl_obj->def_idx_coeff_tbl[hwObjSt->hscl_coeff[UnitNum]][i][j+1])) {
				hLutFailIndex[hLutFailCount][0] = i;
				hLutFailIndex[hLutFailCount][1] = j+1;
				hLutFailCount++;
			}
			ptr = ptr + 2;
		}
		ptr = ptr + 6;
		pr_debug("\n");
	}

	ptr = hwObjSt->scl_coeffs[hwObjSt->vscl_coeff[UnitNum]];
	ptr += ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

	pr_debug("Shadow BCM:Default:Read VLUT\n");
	pr_debug("ISP_FRC_SCL_NUM_OF_PHASES = %d ISP_FRC_SCL_NUM_OF_COEFF =  %d\n",
			ISP_FRC_SCL_NUM_OF_PHASES, ISP_FRC_SCL_NUM_OF_COEFF);
	for (i = 0; i < ISP_FRC_SCL_NUM_OF_PHASES; i++) {
		for (j = 0; j < ISP_FRC_SCL_NUM_OF_COEFF; j += 2) {
			if ((vlut[i][j] != (*ptr & coeffMsk)) || (vlut[i][j] !=
		isp_scl_obj->def_idx_coeff_tbl[hwObjSt->vscl_coeff[UnitNum]][i][j])) {
				vLutFailIndex[vLutFailCount][0] = i;
				vLutFailIndex[vLutFailCount][1] = j;
				vLutFailCount++;
			}
			if ((vlut[i][j+1] != ((*ptr >> coeffBitWidth) & coeffMsk))
				|| (vlut[i][j+1] !=
		isp_scl_obj->def_idx_coeff_tbl[hwObjSt->vscl_coeff[UnitNum]][i][j+1])) {
				vLutFailIndex[vLutFailCount][0] = i;
				vLutFailIndex[vLutFailCount][1] = j+1;
				vLutFailCount++;
			}
			ptr = ptr + 2;
		}
		ptr = ptr + 6;
		pr_debug("\n");
	}

	pr_debug("\n");
	if ((hLutFailCount != 0) || (vLutFailCount != 0)) {
		pr_debug("hLutFailCount = %d\n", hLutFailCount);
		for (j = 0; j < hLutFailCount; j++) {
			pr_debug("(%d, %d) ", hLutFailIndex[j][0], hLutFailIndex[j][1]);
			if (j%20 == 0)
				pr_debug("\n");
		}
		pr_debug("\n");
		pr_debug("vLutFailCount = %d\n", vLutFailCount);
		for (j = 0; j < vLutFailCount; j++) {
			pr_debug("(%d, %d) ", vLutFailIndex[j][0], vLutFailIndex[j][1]);
			if (j%20 == 0)
				pr_debug("\n");
		}
		pr_debug("\n");
		pr_debug("LUT programming FAILED\n");
	} else
		pr_debug("LUT programming PASS\n");

	return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Load the required Coeff from pre calculated table
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetSclCoeff(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf)
{
	int tempHorSclCoeffMode, tempVerSclCoeffMode;
	//const UINT32  *TablePrt;
	//INT Index;
	//UINT32 Ratio;
	UINT32 uiRegAddr;
	INT uiRetVal = ISPSS_OK;
	INT8 hTaps = -1, vTaps = -1;
	//INT8 sel_1D_adp = -1;
	INT8 sclLayer;
	T32SCL1D_YC_COEFF_CFG0 stScl1DCoeffcf0;
	//T32COEFF_CFG0 stSclCoeffCfg0;
	UINT64 *ptr = NULL;

	if ((UnitNum < ISP_FRC_SCL_OVP_Y) || (UnitNum >= ISP_FRC_SCL_MAX))
		return ISPSS_EBADPARAM;

	//All scalar calculation should be based on cropped resolution
	pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

	if (UnitNum == ISP_FRC_SCL_OVP_Y)
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_Y +
			RA_OVPSCL_SCL1D_YC_COEFF;
	else if (UnitNum == ISP_FRC_SCL_OVP_UV)
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_UV +
			RA_OVPSCL_SCL1D_YC_COEFF;

	//Revert back the modification done to 'pIORes->IHRes'
	pIORes->IHRes = pSclCtrl->uncrop_IHRes;

	if (pIORes != NULL) {
		switch (UnitNum) {
		case ISP_FRC_SCL_OVP_Y:
		case ISP_FRC_SCL_OVP_UV:
			ISPSS_SCL_MainSclCalTaps(isp_scl_obj, UnitNum,
					pIORes, &hTaps, &vTaps, hwObjSt);
			break;
		}
	}
	pr_debug("%d ,%s ,Htaps=%d Vtaps=%d\n", __LINE__, __func__, hTaps, vTaps);

	if (!(pSclCtrl->ForceSel & MODE_COEFF_SEL_FORCE_H)) {
		if (pIORes != NULL) {
			if (ISP_FRC_SCL_OVP_Y == UnitNum || ISP_FRC_SCL_OVP_UV == UnitNum) {
				switch (hTaps) {
				case 4:
					tempHorSclCoeffMode = COEFF_H_MAIN_ADP_HLUT_CS;
					break;
				case 5:
					tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_BLK5;
					break;
				case 8:
					if ((pIORes->IHRes/pIORes->OHRes) >= 5)
						tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_PH8_SOFT;
					else
						tempHorSclCoeffMode = COEFF_H_MAIN_1D_HLUT_PH8;
					break;
				default:
					tempHorSclCoeffMode = -1;
					pr_debug("%d %s Unsupported HLut table\n",
						__LINE__, __func__);
					return ISPSS_EUNSUPPORT;
				}
			}
		} else {
			tempHorSclCoeffMode = -1;
		}
	} else {
		// force the user selection.
		tempHorSclCoeffMode = pSclCtrl->HSclMode;
		hTaps = 0;//FIXME
		pr_debug("%d %s Unsupported now. Needs prg updates\n", __LINE__, __func__);
		return ISPSS_EUNSUPPORT;
	}

	if (!(pSclCtrl->ForceSel & MODE_COEFF_SEL_FORCE_V)) {
		if (pIORes != NULL) {
			if (ISP_FRC_SCL_OVP_Y == UnitNum || ISP_FRC_SCL_OVP_UV == UnitNum) {
				switch (vTaps) {
				case 1:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_DBG1;
					break;
				case 2:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_2TAP;
					break;
				case 3:
					if ((pIORes->IHRes / pIORes->OHRes) >= 5)
						tempVerSclCoeffMode =
							COEFF_V_MAIN_1D_VLUT_3TAP_SOFT;
					else
						tempVerSclCoeffMode =
							COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL;
					break;
				case 4:
					tempVerSclCoeffMode = COEFF_V_MAIN_ADP_HLUT_BS;
					break;
				case 5:
					tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_BLK5;
					break;
				case 6:
					if ((pIORes->IHRes / pIORes->OHRes) >= 5)
						/* Using 6tap soft coefficients for larger down
						 * scaling to reduce sawtooth effect
						 */
						tempVerSclCoeffMode =
							COEFF_V_MAIN_1D_VLUT_6TAP_SOFT;
					else
						tempVerSclCoeffMode = COEFF_V_MAIN_1D_VLUT_6TAP;
					break;
				default:
					tempVerSclCoeffMode = -1;
					pr_debug("%d %s Unsupported VLut table\n",
						__LINE__, __func__);
					return ISPSS_EUNSUPPORT;
				}
			}
		} else
			tempVerSclCoeffMode = -1;
	} else {
		// force the user selection.
		tempVerSclCoeffMode = pSclCtrl->VSclMode;
		vTaps = 0;//FIXME
		pr_debug("%d %s Unsupported now. Needs prg updates\n", __LINE__, __func__);
		return ISPSS_EUNSUPPORT;
	}

	/* Calculating sclLayer of scl_coeffs table */
	if (UnitNum == ISP_FRC_SCL_OVP_Y)
		sclLayer = 0;//3
	else if (UnitNum == ISP_FRC_SCL_OVP_UV)
		sclLayer = 1;//4

	if ((tempHorSclCoeffMode != -1) && (hwObjSt->hscl_coeff[UnitNum]
				!= tempHorSclCoeffMode ||
				hwObjSt->hscl_coeff_update[UnitNum] == 1)) {

		hwObjSt->hscl_coeff[UnitNum] = tempHorSclCoeffMode;
		hwObjSt->hscl_coeff_update[UnitNum] = 0;
		ptr = hwObjSt->scl_coeffs[tempHorSclCoeffMode];
		ptr += ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
		ISPSS_BCMBUF_WriteBlock(pbcmbuf, ptr, ISP_FRC_COEFFTAB_BCMBUF_SIZE);
	}

	if ((tempVerSclCoeffMode != -1) && (hwObjSt->vscl_coeff[UnitNum]
				!= tempVerSclCoeffMode ||
				hwObjSt->vscl_coeff_update[UnitNum] == 1)) {

		hwObjSt->vscl_coeff[UnitNum] = tempVerSclCoeffMode;
		hwObjSt->vscl_coeff_update[UnitNum] = 0;
		ptr = hwObjSt->scl_coeffs[tempVerSclCoeffMode];
		ptr += ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;
		ISPSS_BCMBUF_WriteBlock(pbcmbuf, ptr, ISP_FRC_COEFFTAB_BCMBUF_SIZE);
	}
	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
	case ISP_FRC_SCL_OVP_UV:
		stScl1DCoeffcf0.u32               = 0;
		stScl1DCoeffcf0.uCFG0_htap        = hTaps;
		stScl1DCoeffcf0.uCFG0_vtap        = vTaps;
		stScl1DCoeffcf0.uCFG0_coeff_index = 0;
		stScl1DCoeffcf0.uCFG0_coeff_hvsel = 0;
		stScl1DCoeffcf0.uCFG0_coeffread   = 0;
		stScl1DCoeffcf0.uCFG0_coeffload   = 0;
		CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
			uiRegAddr + RA_SCL1D_YC_COEFF_CFG0, stScl1DCoeffcf0.u32));
		break;
	}

	return ISPSS_OK;
}

/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4, 5)
 *               pIORes -poINTer to I&O resolution
 *               pSclCtrl - poINTer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetSclCtrlParams(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf)
{
	INT uiRetVal = ISPSS_OK;
	/*Take backup of the uncropped resolution - so that IHRes
	 * can be reverted at later function calls also
	 */
	pSclCtrl->uncrop_IHRes = pIORes->IHRes;
	//All scalar calculation should be based on cropped resolution
	pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);

	if ((UnitNum < ISP_FRC_SCL_OVP_Y) || (UnitNum >= ISP_FRC_SCL_MAX))
		return ISPSS_EBADPARAM;

	ISPSS_SCLDBG("%s %d ::: pIORes =  %d %d %d %d UnitNum = %d\n", __func__, __LINE__,
			pIORes->IHRes, pIORes->IVRes, pIORes->OHRes, pIORes->OVRes, UnitNum);

	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
	case ISP_FRC_SCL_OVP_UV:
		CHECK_ISP_SCL_RETVAL(ISPSS_SCL_SetMainSclparams(isp_scl_obj,
			UnitNum, pIORes, pSclCtrl, hwObjSt, pbcmbuf));
		break;
	default:
		return ISPSS_EBADPARAM;
	}

	//Revert back the modification done to 'pIORes->IHRes'
	pIORes->IHRes = pSclCtrl->uncrop_IHRes;

	/*load HV filter coefficiets*/
	if (pSclCtrl->DynamicLoad)
		ISPSS_FRC_SCL_SetSclCoeff(isp_scl_obj, UnitNum, pIORes, pSclCtrl, hwObjSt, pbcmbuf);
	return uiRetVal;
}

INT ISPSS_SCL_SetMainSclScanParams(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf)
{
	UINT32 uiRegAddr;
	INT8 sel_1D_adp;
	INT8 vTaps, hTaps;
	T32SCL1D_YC_CFG0 stSclCfg0;
	T32SCL1D_YC_CFG3 stSclCfg3;
	T32SCL1D_YC_CFG7 stSclCfg7;
	INT uiRetVal = ISPSS_OK;

	pr_debug("%s:%d -- %d %d %d\n", __func__, __LINE__, pIORes->IHRes,
			pIORes->OHRes, pSclCtrl->leftCrop + pSclCtrl->rightCrop);
	//All scalar calculation should be based on cropped resolution
	pIORes->IHRes -= (pSclCtrl->leftCrop + pSclCtrl->rightCrop);
	stSclCfg0.u32 = hwObjSt->uiSclCfg0[UnitNum];
	stSclCfg3.u32 = hwObjSt->uiSclCfg3[UnitNum];
	stSclCfg7.u32 = hwObjSt->uiSclCfg7[UnitNum];

	uiRegAddr = ISPSS_INTERNAL_FRC_SCL_GetRegStartAddr(isp_scl_obj, UnitNum);

	ISPSS_SCL_MainSclAdp1dType(isp_scl_obj, pIORes, &sel_1D_adp);
	ISPSS_SCL_MainSclCalTaps(isp_scl_obj, UnitNum, pIORes, &hTaps, &vTaps, hwObjSt);

	if (sel_1D_adp == 0) {
		stSclCfg0.uCFG0_even = 0x80;
		stSclCfg0.uCFG0_odd = 0x80;
		stSclCfg0.uCFG0_hinitph = 0x80;
	} else {
		stSclCfg0.uCFG0_even = (vTaps % 2)<<7;
		stSclCfg0.uCFG0_odd = (vTaps % 2)<<7;
		stSclCfg0.uCFG0_hinitph = (hTaps % 2)<<7;
	}
	stSclCfg7.uCFG7_vtap_offset = 0;
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG0, stSclCfg0.u32));
	hwObjSt->uiSclCfg0[UnitNum] = stSclCfg0.u32;

	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
		uiRegAddr + RA_SCL1D_YC_CFG7, stSclCfg7.u32));
	hwObjSt->uiSclCfg7[UnitNum] = stSclCfg7.u32;

	//Revert back the modification done to 'pIORes->IHRes'
	pIORes->IHRes = pSclCtrl->uncrop_IHRes;

	return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: Load the scalar FRC parameters
 * PARAMS:
 *           isp_scl_obj - vpp object
 *           UnitNum - Unit num to identify the plain
 *           pFrcRes - FRC input and output resolutions
 * RETURN:  1 - succeed
 *                others - failed to load values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetFrcParams(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, struct ISP_BE_SCL_RES *pFrcRes,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf)
{
	UINT32 uiRegAddr;
	union ISP_FRC_CTRL0_0 stFrcCtrl0_0;
	union ISP_FRC_CTRL0_1 stFrcCtrl0_1;
	union ISP_FRC_CTRL1_2 stFrcCtrl1_2;
	T32SCL1D_YC_FRC_CFG0 stSclFrcCfg0;
	T32SCL1D_YC_FRC_CFG1 stSclFrcCfg1;
	T32SCL1D_YC_FRC_CFG2 stSclFrcCfg2;
	T32SCL1D_YC_FRC_CFG3 stSclFrcCfg3;
	INT uiRetVal = ISPSS_OK;
	UNSG16 temp_uCFG1_dly_de_lrst = 0;
	INT8 enDeintSmallUpScale = 0;

	ISPSS_SCL_SclConfigForDeint(isp_scl_obj, UnitNum, pFrcRes, &enDeintSmallUpScale);
	if ((UnitNum < ISP_FRC_SCL_OVP_Y) || (UnitNum >= ISP_FRC_SCL_MAX))
		return ISPSS_EBADPARAM;

	stSclFrcCfg1.u32 = 0;
	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
		uiRegAddr = isp_scl_obj->base_addr +
			RA_OVPDNSCLWRAP_OVPSCL_Y + RA_OVPSCL_SCL1D_YC_FRC;
		break;
	case ISP_FRC_SCL_OVP_UV:
		uiRegAddr = isp_scl_obj->base_addr +
			RA_OVPDNSCLWRAP_OVPSCL_UV + RA_OVPSCL_SCL1D_YC_FRC;
		break;
	default:
		return ISPSS_EBADPARAM;
	}

	stFrcCtrl0_0.uchFrcMode = 3;
	stFrcCtrl0_0.uchMosiacMode = 0;
	stFrcCtrl0_0.uchBitMode_16_24 = 1;
	stFrcCtrl0_0.uchSpMode = 0;

	stFrcCtrl0_1.uchLoadRead = 0;
	stFrcCtrl0_1.uchLoadWrite = 0;
	stFrcCtrl0_1.uchReadCtrl = 0;
	stFrcCtrl0_1.uchWriteCtrl = 0;
	stFrcCtrl0_1.uchFreeze = 0;
	stFrcCtrl0_1.uchInvFeFld = 0;
	stFrcCtrl0_1.uchInvBeFld = 0;
	stFrcCtrl0_1.uchBeFrstResetEn = 0;

	stSclFrcCfg0.uCFG0_sclclk_ctrl = 0x80;
	stSclFrcCfg0.uCFG0_ctrl0 = stFrcCtrl0_0.uch8;
	stSclFrcCfg0.uCFG0_ctrl1 = stFrcCtrl0_1.uch8;
	if ((pFrcRes->IHRes <= pFrcRes->OHRes) && (pFrcRes->IHRes > 1280)
			&& (enDeintSmallUpScale == 0))
		stSclFrcCfg0.uCFG0_sclclk_ctrl1 = 0x60;
	else
		stSclFrcCfg0.uCFG0_sclclk_ctrl1 = 0x80;

	stFrcCtrl1_2.uchTreaLessReadCtrl = 0;
	stFrcCtrl1_2.uchTreaLessWriteCtrl = 0;
	stSclFrcCfg1.uCFG1_ctrl2 = stFrcCtrl1_2.uch8;
	stSclFrcCfg1.uCFG1_dly_frst_de = 0x14;

	if (ISP_FRC_SCL_OVP_Y == UnitNum || ISP_FRC_SCL_OVP_UV == UnitNum) {
		/* Fix: DataEnable and LineReset delay is increasing
		 * to 0x50 for scaling greater than 10
		 */
		if ((pFrcRes->IHRes/pFrcRes->OHRes) <= 10)
			temp_uCFG1_dly_de_lrst = 0x14;
		else
			temp_uCFG1_dly_de_lrst = 0x50;

	} else {
		if ((pFrcRes->IHRes*100)/(pFrcRes->OHRes) > (24*100))
			/* When down scale increases de_lrst should be increased  */
			temp_uCFG1_dly_de_lrst = 0x300;
		else
			temp_uCFG1_dly_de_lrst = 0x100;
	}

	if (pFrcRes->OHRes > pFrcRes->IHRes)
		stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;
	else if (isp_scl_obj->scl_ctrl[UnitNum].NLEn == 1)
		stSclFrcCfg1.uCFG1_dly_de_lrst = 0x200;
	else if ((pFrcRes->OHRes < pFrcRes->IHRes) && (temp_uCFG1_dly_de_lrst == 0x100)
			&& (UnitNum != ISP_FRC_SCL_OVP_Y)  && (UnitNum != ISP_FRC_SCL_OVP_UV))
		/* change only if temp_uCFG1_dly_de_lrst = 0x100(default) */
		stSclFrcCfg1.uCFG1_dly_de_lrst = 0x2C;
	else
		stSclFrcCfg1.uCFG1_dly_de_lrst = temp_uCFG1_dly_de_lrst;
	stSclFrcCfg2.uCFG2_bevres = pFrcRes->IVRes;
	stSclFrcCfg2.uCFG2_fevres = pFrcRes->OVRes;
	stSclFrcCfg2.uCFG2_dly_lrst_de = 0x14;//250;

	stSclFrcCfg3.uCFG3_behres = pFrcRes->IHRes;
	stSclFrcCfg3.uCFG3_clnt_ctrl = 3;
	stSclFrcCfg3.uCFG3_ctrl = 0;
	stSclFrcCfg3.uCFG3_rff_ctrl = 0;

	if (ISP_FRC_SCL_OVP_Y == UnitNum || ISP_FRC_SCL_OVP_UV == UnitNum) {
		CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
			uiRegAddr + RA_SCL1D_YC_FRC_CFG0, stSclFrcCfg0.u32));
		CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
			uiRegAddr + RA_SCL1D_YC_FRC_CFG1, stSclFrcCfg1.u32));
		CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
			uiRegAddr + RA_SCL1D_YC_FRC_CFG2, stSclFrcCfg2.u32));
		CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf,
			uiRegAddr + RA_SCL1D_YC_FRC_CFG3, stSclFrcCfg3.u32));
	}
	hwObjSt->uiSclFrcCfg1[UnitNum] = stSclFrcCfg1.u32;
	hwObjSt->uiSclFrcCfg0[UnitNum] = stSclFrcCfg0.u32;
	return uiRetVal;
}

/***************************************************************************************
 * FUNCTION: set delay between DE  to line reset.
 * PARAMS: UnitNum - unit number
 *               DeLrstDelay -delay value
 * RETURN:  1 - succeed
 *                others - failed to load  values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetDeLrstDelay(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		UINT32 DeLrstDelay, struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf)
{
	UINT32 uiRegAddr;
	UINT32 uiRetVal = ISPSS_OK;
	T32SCL1D_YC_FRC_CFG1 stSclFrcCfg1;

	if ((UnitNum < ISP_FRC_SCL_OVP_Y) || (UnitNum >= ISP_FRC_SCL_MAX))
		return ISPSS_EBADPARAM;
	/*get register start address */
	uiRegAddr = isp_scl_obj->base_addr;
	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
		uiRegAddr += RA_OVPDNSCLWRAP_OVPSCL_Y  +
			RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG1;
		break;
	case ISP_FRC_SCL_OVP_UV:
		uiRegAddr += RA_OVPDNSCLWRAP_OVPSCL_UV +
			RA_OVPSCL_SCL1D_YC_FRC + RA_SCL1D_YC_FRC_CFG1;
		break;
	default:
		return ISPSS_EBADPARAM;
	}
	stSclFrcCfg1.u32 = hwObjSt->uiSclFrcCfg1[UnitNum];
	stSclFrcCfg1.uCFG1_dly_de_lrst = DeLrstDelay;
	/*set it to BCM buffer*/
	CHECK_ISP_SCL_RETVAL(ISP_BCMBUF_Write_DBG(pbcmbuf, uiRegAddr, stSclFrcCfg1.u32));
	hwObjSt->uiSclFrcCfg1[UnitNum] = stSclFrcCfg1.u32;
	return  uiRetVal;
}

static INT ISPSS_Generate_Scl_MainCoefficiena(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		int UnitNum, UINT64 **pptr, int hv,
		UINT16 (*coeffs)[ISP_FRC_SCL_NUM_OF_COEFF],
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	UINT32 uiRegAddr = 0;
	INT16 i;
	INT uiRetVal = ISPSS_OK;
	UINT64 *ptr = *pptr;
	UINT64 val;
	UINT32 temp;

	T32SCL1D_YC_COEFF_CFG0 stSclCoeffCfg0;
	T32SCL1D_YC_COEFF_CFG1 stSclCoeffCfg1;
	T32SCL1D_YC_COEFF_CFG2 stSclCoeffCfg2;
	T32SCL1D_YC_COEFF_CFG3 stSclCoeffCfg3;
	T32SCL1D_YC_COEFF_CFG4 stSclCoeffCfg4;

	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_Y +
			RA_OVPSCL_SCL1D_YC_COEFF;
		break;
	case ISP_FRC_SCL_OVP_UV:
		uiRegAddr = isp_scl_obj->base_addr + RA_OVPDNSCLWRAP_OVPSCL_UV +
			RA_OVPSCL_SCL1D_YC_COEFF;
		break;
	default:
		return ISPSS_EBADPARAM;
	}

	/*Coeff table loading*/
	for (i = 0; i < ISP_FRC_SCL_NUM_OF_PHASES; i++) {
		stSclCoeffCfg1.u32 =  0;
		temp = (coeffs[i][1]) & 0x0FFF;
		stSclCoeffCfg1.u32 = ((coeffs[i][0]) & 0x0FFF) | (temp<<12);
		//stSclCoeffCfg1.uCFG1_coeff0 = coeffs[i][0];
		//stSclCoeffCfg1.uCFG1_coeff1 = coeffs[i][1];
		stSclCoeffCfg2.u32 = 0;
		temp = (coeffs[i][3]) & 0x0FFF;
		stSclCoeffCfg2.u32 = ((coeffs[i][2]) & 0x0FFF) | (temp<<12);
		//stSclCoeffCfg2.uCFG2_coeff2 = coeffs[i][2];
		//stSclCoeffCfg2.uCFG2_coeff3 = coeffs[i][3];

		stSclCoeffCfg3.u32 = 0;
		temp = (coeffs[i][5]) & 0x0FFF;
		stSclCoeffCfg3.u32 = ((coeffs[i][4]) & 0x0FFF) | (temp<<12);
		//stSclCoeffCfg3.uCFG3_coeff4 = coeffs[i][4];
		//stSclCoeffCfg3.uCFG3_coeff5 = coeffs[i][5];

		stSclCoeffCfg4.u32 = 0;
		temp = (coeffs[i][7]) & 0x0FFF;
		/* FIXME 27th bit is getting set in the user space code,
		 * replicating it here. should be done correctly
		 */
		stSclCoeffCfg4.u32 = ((coeffs[i][6]) & 0x0FFF) | (temp<<12) | (1<<27);
		//stSclCoeffCfg4.uCFG4_coeff6 = coeffs[i][6];
		//stSclCoeffCfg4.uCFG4_coeff7 = coeffs[i][7];

		val = uiRegAddr + RA_SCL1D_YC_COEFF_CFG1;
		*ptr++ = (val << 32) | stSclCoeffCfg1.u32;
		val = uiRegAddr + RA_SCL1D_YC_COEFF_CFG2;
		*ptr++ = (val << 32) | stSclCoeffCfg2.u32;
		val = uiRegAddr + RA_SCL1D_YC_COEFF_CFG3;
		*ptr++ = (val << 32) | stSclCoeffCfg3.u32;
		val = uiRegAddr + RA_SCL1D_YC_COEFF_CFG4;
		*ptr++ = (val << 32) | stSclCoeffCfg4.u32;

		stSclCoeffCfg0.u32 = 0;
		temp = (((hv ^ 1) << 13) & 0x2000);
		stSclCoeffCfg0.u32 = ((i<<8) & 0x1F00) | (temp);

		val = uiRegAddr + RA_SCL1D_YC_COEFF_CFG0;
		*ptr++ = (val << 32) | stSclCoeffCfg0.u32;
		stSclCoeffCfg0.uCFG0_coeffload   = 1;
		val = uiRegAddr + RA_SCL1D_YC_COEFF_CFG0;
		*ptr++ = (val << 32) | stSclCoeffCfg0.u32;
		stSclCoeffCfg0.uCFG0_coeffload   = 0;
		val = uiRegAddr + RA_SCL1D_YC_COEFF_CFG0;
		*ptr++ = (val << 32) | stSclCoeffCfg0.u32;
	}

	*pptr = ptr;

	return uiRetVal;
}

static INT ISPSS_Generate_Scl_Coefficiena(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		int UnitNum, UINT64 **pptr, int hv,
		UINT16 (*coeffs)[ISP_FRC_SCL_NUM_OF_COEFF],
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	switch (UnitNum) {
	case ISP_FRC_SCL_OVP_Y:
	case ISP_FRC_SCL_OVP_UV:
		ISPSS_Generate_Scl_MainCoefficiena(isp_scl_obj, UnitNum, pptr, hv, coeffs, hwObjSt);
		break;
	default:
		return (ISPSS_EBADPARAM);
	}

	return (ISPSS_OK);
}

/***************************************************************************************
 * FUNCTION: program SCL scaler coeffs with customized coeffs table
 * PARAMS:
 *            coeffID - scalar coeffs mode
 *              hv     - indicating it is for horizontal(0) or vertical(1) scaler
 *              UnitNum    - indicating which plane the coeff is for
 *        coeffs    - customized coeffs talbe's pointer;
 *                      Note: if it is NULL, it means recover default coeffs table
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
static INT ISPSS_Customize_Coefficients(struct ISP_BE_SCL_OBJ *isp_scl_obj, int coeffID, int hv,
		INT UnitNum, BOOL bCust, struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	UINT16 (*coeffs)[8];
	UINT64 *ptr;
	int sclLayer;

	/* Calculating sclLayer of scl_coeffs table */
	if (UnitNum == ISP_FRC_SCL_OVP_Y)
		sclLayer = 0;//3
	else if (UnitNum == ISP_FRC_SCL_OVP_UV)
		sclLayer = 1;//4
	else
		ASSERT(0);

	if (coeffID < FIRST_SCL_COEFF || coeffID >= COEFF_MAX)
		return ISPSS_EBADPARAM;

	ptr = hwObjSt->scl_coeffs[coeffID];
	ptr += ISP_FRC_COEFFTAB_BCMBUF_NUM_OF_PAIRS * sclLayer;

	if (bCust == true) {
		if (hwObjSt->cust_sc_bPrepared[sclLayer][coeffID]) {
			coeffs = (UINT16 (*)[ISP_FRC_SCL_NUM_OF_COEFF])
				hwObjSt->cust_scl_coeffs[sclLayer][coeffID];
			hwObjSt->cust_sc_bLoaded[sclLayer][coeffID] = true;
		} else
			return ISPSS_EUNCONFIG;
	} else {
		coeffs = (UINT16 (*)[ISP_FRC_SCL_NUM_OF_COEFF])
			isp_scl_obj->def_idx_coeff_tbl[coeffID];
		hwObjSt->cust_sc_bLoaded[sclLayer][coeffID] = false;
	}
	ISPSS_Generate_Scl_Coefficiena(isp_scl_obj, UnitNum, &ptr, hv, coeffs, hwObjSt);
	ispSS_SHM_CleanCache(hwObjSt->scl_coeffs_handle[coeffID],
			(ISP_FRC_COEFFTAB_BCMBUF_SIZE * sclLayer), ISP_FRC_COEFFTAB_BCMBUF_SIZE);


	return ISPSS_OK;
}

static INT FRC_SCL_GenerateMain1DCoeffTabH(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT16 coeffID, INT16 hvSel, INT format,
		INT16 (*hlut)[8], struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	INT16 hcf1[32][8], hcf2;
	INT16 i, j;
	INT uiRetVal = ISPSS_OK;

	/* Convert INTo sign magnitude */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 8; j++) {
			hcf2 = (hlut[i][7-j])&0x800;
			hcf1[i][j] = (hlut[i][7-j])&0x7ff;
			hcf1[i][j] = (hcf2 ? -hcf1[i][j] : hcf1[i][j])&0x7ff;
			hcf1[i][j] = ((hcf2 | hcf1[i][j])&0xfff);
		}
	}

	/*Horizontal Coeff table loading*/
	for (i = 0; i < ISP_FRC_SCL_NUM_OF_PHASES; i++) {
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][0] = hcf1[i][7];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][1] = hcf1[i][6];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][2] = hcf1[i][5];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][3] = hcf1[i][4];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][4] = hcf1[i][3];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][5] = hcf1[i][2];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][6] = hcf1[i][1];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][7] = hcf1[i][0];
	}

	return uiRetVal;

}

static INT FRC_SCL_GenerateMain1DCoeffTabV(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT16 coeffID, INT16 hvSel, INT format,
		INT16 (*vlut)[6], struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	INT16 vcf1[32][8], vcf2;
	INT16 i, j;
	INT uiRetVal = ISPSS_OK;
	INT vc[6];
	INT16 vTaps;
	/* Convert INTo sign magnitude */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 6; j++) {
			vcf2 = (vlut[i][5-j])&0x800;
			vcf1[i][j] = (vlut[i][5-j])&0x7ff;
			vcf1[i][j] = (vcf2 ? -vcf1[i][j] : vcf1[i][j])&0x7ff;
			vcf1[i][j] = ((vcf2 | vcf1[i][j])&0xfff);
		}
	}

	switch (coeffID) {
	case COEFF_V_MAIN_1D_VLUT_DBG1:
		vTaps = 1;
		break;
	case COEFF_V_MAIN_1D_VLUT_2TAP:
		vTaps = 2;
		break;
	case COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL:
	case COEFF_V_MAIN_1D_VLUT_3TAP_MED:
	case COEFF_V_MAIN_1D_VLUT_3TAP_SOFT:
		vTaps = 3;
		break;
	case COEFF_V_MAIN_1D_VLUT_BLK5:
		vTaps = 5;
		break;
	case COEFF_V_MAIN_1D_VLUT_6TAP:
	case COEFF_V_MAIN_1D_VLUT_6TAP_SOFT:
	case COEFF_V_MAIN_1D_VLUT_6TAP_SSOFT:
		vTaps = 6;
		break;
	default:
		vTaps = -1;
		break;
	}

	/* Fitting tap position based on number vtaps */
	for (i = 0; i < 6; i++) {
		vc[i] = 6 + i - vTaps;
		if (vc[i] >= 6)
			vc[i] = 0;
	}

	/*Vertical Coeff table loading*/
	for (i = 0; i < ISP_FRC_SCL_NUM_OF_PHASES; i++) {
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][0] = vcf1[i][vc[5]];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][1] = vcf1[i][vc[4]];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][2] = vcf1[i][vc[3]];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][3] = vcf1[i][vc[2]];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][4] = vcf1[i][vc[1]];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][5] = vcf1[i][vc[0]];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][6] = 0;
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][7] = 0;
	}

	return uiRetVal;

}

static INT ISPSS_FRC_SCL_GenerateMainAdpCoeffTabHV(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT16 coeffID, INT16 hvSel,
		INT format, INT16 (*lut)[4])
{
	INT16 cf1[32][4], cf2;
	INT16 i, j;
	INT uiRetVal = ISPSS_OK;


	/* Convert INTo sign magnitude */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < 4; j++) {
			cf2 = (lut[i][3-j])&0x800;
			cf1[i][j] = (lut[i][3-j])&0x7ff;
			cf1[i][j] = (cf2 ? -cf1[i][j] : cf1[i][j])&0x7ff;
			cf1[i][j] = ((cf2 | cf1[i][j])&0xfff);
		}
	}
	/*Coeff table loading*/
	for (i = 0; i < ISP_FRC_SCL_NUM_OF_PHASES; i++) {
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][0] = cf1[i][3];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][1] = cf1[i][2];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][2] = cf1[i][1];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][3] = cf1[i][0];
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][4] = 0;
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][5] = 0;
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][6] = 0;
		isp_scl_obj->def_idx_coeff_tbl[coeffID][i][7] = 0;
	}
	return uiRetVal;
}

void ISPSS_FRC_SCL_GenerateCoeffTables(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt)
{
	ISPSS_FRC_SCL_GenerateMainAdpCoeffTabHV(isp_scl_obj,
			COEFF_H_MAIN_ADP_HLUT_CS, 1, 0x1A, hlut_cs);
	ISPSS_FRC_SCL_GenerateMainAdpCoeffTabHV(isp_scl_obj,
			COEFF_V_MAIN_ADP_HLUT_BS, 0, 0x1A, hlut_bs);

	FRC_SCL_GenerateMain1DCoeffTabH(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_BLK5,
			1, 0x1A, hlut_blk5, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabH(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_PH8,
			1, 0x1A, hlut_ph8, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabH(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_PH8_SOFT,
			1, 0x1A, hlut_ph8_soft, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_DBG1,
			0, 0x0B, vlut_dbg1, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_2TAP,
			0, 0x0B, vlut_2tap, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL,
			0, 0x1A, vlut_3tap_cubic_1DScl, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_MED,
			0, 0x1A, vlut_3tap_med, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_SOFT,
			0, 0x1A, vlut_3tap_soft, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_BLK5,
			0, 0x1A, vlut_blk5, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP,
			0, 0x1A, vlut_6tap, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP_SOFT,
			0, 0x1A, vlut_6tap_soft, hwObjSt);
	FRC_SCL_GenerateMain1DCoeffTabV(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP_SSOFT,
			0, 0x1A, vlut_6tap_ssoft, hwObjSt);

	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_ADP_HLUT_CS,
			0, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_ADP_HLUT_BS,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);

	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_BLK5,
			0, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_PH8,
			0, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_PH8_SOFT,
			0, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_DBG1,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_2TAP,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_MED,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_SOFT,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_BLK5,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP_SOFT,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP_SSOFT,
			1, ISP_FRC_SCL_OVP_Y, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_ADP_HLUT_CS,
			0, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_ADP_HLUT_BS,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);

	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_BLK5,
			0, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_PH8,
			0, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_H_MAIN_1D_HLUT_PH8_SOFT,
			0, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_DBG1,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_2TAP,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_MED,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_3TAP_SOFT,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_BLK5,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP_SOFT,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
	ISPSS_Customize_Coefficients(isp_scl_obj, COEFF_V_MAIN_1D_VLUT_6TAP_SSOFT,
			1, ISP_FRC_SCL_OVP_UV, false, hwObjSt);
}
