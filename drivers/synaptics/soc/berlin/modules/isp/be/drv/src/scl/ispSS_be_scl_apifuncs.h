/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_BE_SCL_APIFUNCS_H_
#define _ISPSS_BE_SCL_APIFUNCS_H_

#include "ispSS_be_scl_module.h"
#include "ovpdnSclWrap.h"

/*******************************************************************
 * FUNCTION: Initial configuration done during driver create/open
 * PARAMS:
 * RETURN:
 ******************************************************************/
INT ISPSS_BE_SCL_initalConfig(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf);

/*********************************************************************
 * FUNCTION: Push a frame for SCL processing
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_BE_SCL_Start(struct ISP_BE_RQST_MSG *psclRqstMsg,
		struct ISP_BE_SCL_OBJ *isp_scl_obj, struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

/*********************************************************************
 * FUNCTION: Check for interrupt for each frame
 * PARAMS:   psclRqstMsg - Scalar request structure pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_BE_SCL_FrameInterrupt(struct ISP_BE_RQST_MSG *psclRqstMsg);

/*********************************************************************
 * FUNCTION: Process Dump register for SCL
 * PARAMS:   isp_scl_obj -  SCL object pointer
 * RETURN:   ISPSS_OK
 ********************************************************************/
INT ISPSS_BE_SCL_ProcessDumpRegs(struct ISP_BE_SCL_OBJ *isp_scl_obj);

/*********************************************************************
 * FUNCTION: Submit the BCM buffers to Hardware
 * PARAMS: pstsclRqstMsg - Request message
 * RETURN: ISPSS_OK - succeed
 *********************************************************************/
INT ISPSS_BE_SCL_SubmitHW(struct ISP_BE_BCM *pSclBcmBuf, INT commit_QId);

INT ISPSS_FRC_SCL_ReadSclCoeff(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut,
		INT16 **vlut, struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

INT ISPSS_FRC_SCL_PrintSclCoeffShadow(struct ISP_BE_SCL_OBJ *isp_scl_obj,
		INT UnitNum, INT16 **hlut, INT16 **vlut,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt);

INT ISPSS_FRC_SCL_SetSclCoeff(struct ISP_BE_SCL_OBJ *isp_scl_obj, INT UnitNum,
		struct ISP_BE_SCL_RES *pIORes, struct ISP_BE_SCL_CTRL *pSclCtrl,
		struct ISP_BE_SCL_HW_STRUCT *hwObjSt, struct BCMBUF *pbcmbuf);

void ISPSS_BE_SCL_SetINTGParams_UV(struct ISP_BE_SCL_OBJ *isp_scl_obj, UINT32 width,
		UINT32 height, struct BCMBUF *pbcmbuf);

void dump_to_hex(const unsigned char *buffer, size_t length);
#endif
