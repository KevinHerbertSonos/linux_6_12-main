/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ISP_BE_FRC_SCL_H
#define ISP_BE_FRC_SCL_H

#include "ispSS_shm.h"
#include "ispSS_be_scl_module.h"

/**********************************************************************
 *data structure definition for FRC SCL block driver
 ***********************************************************************/

/*Enum for FRC SCL phase */
enum ISP_FRC_SCL_PHASE_HV {
	ISP_FRC_SCL_PHASE_HORIZONTAL = 0,
	ISP_FRC_SCL_PHASE_VERTICAL
};

/*sacler coefficient mode*/
enum ISP_FRC_SCL_COEFF_MODE {
	FIRST_SCL_COEFF =  0,
	COEFF_MAIN_MIN = 0,
	COEFF_H_MAIN_ADP_HLUT_CS = COEFF_MAIN_MIN,
	COEFF_H_MAIN_1D_HLUT_BLK5,
	COEFF_H_MAIN_1D_HLUT_PH8,
	COEFF_H_MAIN_1D_HLUT_PH8_SOFT,
	COEFF_V_MAIN_ADP_HLUT_BS,
	COEFF_V_MAIN_1D_VLUT_DBG1,
	COEFF_V_MAIN_1D_VLUT_2TAP,
	COEFF_V_MAIN_1D_VLUT_3TAP_CUBIC_1DSCL,
	COEFF_V_MAIN_1D_VLUT_3TAP_MED,
	COEFF_V_MAIN_1D_VLUT_3TAP_SOFT,
	COEFF_V_MAIN_1D_VLUT_BLK5,
	COEFF_V_MAIN_1D_VLUT_6TAP_SOFT,
	COEFF_V_MAIN_1D_VLUT_6TAP_SSOFT,
	COEFF_V_MAIN_1D_VLUT_6TAP,
	COEFF_MAIN_MAX = COEFF_V_MAIN_1D_VLUT_6TAP,
	COEFF_GFX_MIN,
	COEFF_H_GFX_HLUT_BLK5_GFX = COEFF_GFX_MIN,
	COEFF_H_GFX_HLUT_PV8,
	COEFF_V_GFX_VLUT_CS_4TAP,
	COEFF_V_GFX_VLUT_3TAP_CUBIC,
	COEFF_V_GFX_VLUT_1TAP,
	COEFF_GFX_MAX = COEFF_V_GFX_VLUT_1TAP,
	COEFF_MAX = T_COEFF_MAX// Update T_COEFF_MAX if changes made to this enum
};

union ISP_SCL_CTRL0 {
	UINT8 uch8;
	struct {
		UINT8 uchAvg4FiltSel:2;
		UINT8 uchCscEn:1;
		UINT8 uchHnlEn:1;
		UINT8 uchUpsEn:1;
		UINT8 uchDnsEn:1;
		UINT8 uchDgnlEn:1;
		UINT8 uchDemoModeEn:1;
	};
};

union ISP_SCL_CTRL1 {
	UINT8 uch8;
	struct {
		UINT8 uchSel_1D_adp:1;
		UINT8 uchUp_down:1;
		UINT8 uchVSclInUpsEn:1;
		UINT8 uchVSclOutUpsEn:1;
		UINT8 uchVSclInUpsCoeff:1;
	};
};

union ISP_FRC_CTRL0_0 {
	UINT8 uch8;
	struct {
		UINT8 uchFrcMode:2;
		UINT8 uchMosiacMode:1;
		UINT8 uchBitMode_16_24:1;
		UINT8 uchSpMode:4;
	};
};

union ISP_FRC_CTRL0_1 {
	UINT8 uch8;
	struct {
		UINT8 uchLoadRead:1;
		UINT8 uchLoadWrite:1;
		UINT8 uchReadCtrl:1;
		UINT8 uchWriteCtrl:1;
		UINT8 uchFreeze:1;
		UINT8 uchInvFeFld:1;
		UINT8 uchInvBeFld:1;
		UINT8 uchBeFrstResetEn:1;
	};
};

union ISP_FRC_CTRL1_2 {
	UINT8 uch8;
	struct {
		UINT8 uchTreaLessReadCtrl:4;
		UINT8 uchTreaLessWriteCtrl:4;
	};
};

/*FRC input resolution*/
struct ISP_FRC_RES {
	UINT32  HRes;
	UINT32  VRes;
};

struct ISP_SCL_CTRL_PARAM {
	struct ISP_BE_SCL_RES IORes;
	struct ISP_BE_SCL_CTRL SclCtrl;
};

/**********************************************************************
 *Function definition for CPCB FRC SCL block driver
 ***********************************************************************/

/***********************************************************************
 * FUNCTION: load the default register values to FRC SCL the unit
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 * RETURN:  1 - succeed
 *		others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_LoadDefaultVal(struct ISP_BE_SCL_OBJ *isp_scl_obj,
					INT UnitNum, struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

/***************************************************************************************
 * FUNCTION: set HV resolution to FRC unit.
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               pFrcRes - pointer to the resolution data structure
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetFrcParams(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pFrcRes,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt,
		struct BCMBUF *pbcmbuf);

/***************************************************************************************
 * FUNCTION: set delay between DE  to line reset.
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               DeLrstDelay -delay value
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetDeLrstDelay(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		UINT32 DeLrstDelay, struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf);

/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               pIORes -pointer to I&O resolution
 *               pSclCtrl - pointer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetSclCtrlParams(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf);

/***************************************************************************************
 * FUNCTION: set control information and I&O resolution to scaler (for PQTOOL)
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               pIORes -pointer to I&O resolution
 *               pSclCtrl - pointer to scaler control information
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetSclCtrlParams_new(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, struct ISP_BE_SCL_RES *pIORes,
		struct ISP_BE_SCL_CTRL *pSclCtrl, struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

/***************************************************************************************
 * FUNCTION: set OSD formatter data format and CSC mode
 * PARAMS: UnitNum - unit number(0, 1, 2, 3, 4)
 *               CscMode -CSC mode (passthrough, 601 or 709)
 *               OsdFmt - OSD input data format
 * RETURN:  1 - succeed
 *                others - failed to load CPCB EE default values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetOsdFmt(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		INT CscMode, struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

/***************************************************************************************
 * FUNCTION: Get the Clock puncture value to be programmed based on the input and output
 *            resolutions.
 * PARAMS: pIORes - Input and Output resolutions.
 *         puchClkPuncture - Clock puntcure value
 * RETURN:  0 - succeed
 *
 ***************************************************************************************/
INT ISPSS_FRC_SCL_GetClkPuncture(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, UCHAR *puchClkPuncture,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

/***************************************************************************************
 * FUNCTION: set BE FIFO for loopback
 * PARAMS: pCpcb0Tot - CPCB0 output total resolution
 *               pCpcb0Res - CPCB0 active resolution
 * RETURN:  1 - succeed
 *                others - failed to load CPCB BE FIFO values to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetBeFifo(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_FRC_RES *pCpcb0Tot,
		struct ISP_FRC_RES *pCpcb0Res,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

/***************************************************************************************
 * FUNCTION: enable/disable alpha divider of GFX0 or PG scaler
 * PARAMS: UnitNum - unit number(2, 3)
 *         Enable - 1: enable, 0: disable
 * RETURN:  1 - succeed
 *                others - failed to load register to BCM buffer
 ***************************************************************************************/
INT ISPSS_FRC_SCL_SetAlphaDivider(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		INT Enable, struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

void ISPSS_FRC_SCL_GenerateCoeffTables(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt);
INT ISPSS_SCL_SetMainSclScanParams(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf);
INT ISPSS_SCL_SetGfxSclScanParams(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt);
UINT32 ISPSS_INTERNAL_FRC_SCL_GetRegStartAddr(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		enum ISP_FRC_SCL_NUM UnitNum);
INT ISPSS_SCL_MainCoeffRead(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut, INT16 **vlut, struct ISP_BE_SCL_HW_STRUCT *hwObjSt);
INT ISPSS_FRC_SCL_ReadSclCoeff(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut, INT16 **vlut,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt);
INT ISPSS_FRC_SCL_PrintSclCoeffShadow(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut, INT16 **vlut,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt);
INT ISPSS_FRC_SCL_SetSclCoeff(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf);
#endif
