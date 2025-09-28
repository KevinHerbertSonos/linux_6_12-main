/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISP_BE_FRC_SCL_PRV_H_
#define _ISP_BE_FRC_SCL_PRV_H_

#include "com_type.h"
#include "ispSS_bcmbuf.h"
#include "ispbe_err.h"
#include "ispSS_be_scl_module.h"
/**********************************************************************
 *macro definitions for FRC scaler                                    *
 ***********************************************************************/

/*register number in a FRC_SCL unit, neglect those useless*/
#define ISP_FRC_SCL_REG_NUM 73//register number in local buffer

/*maximum address in the line buffer*/
#define ISP_SCL_LINE_BUF_ADDR_MAX 1279

/*scaler tap number restrictions*/
#define ISP_SCL_HTAP_NUM_MIN 1
#define ISP_SCL_HTAP_NUM_MAX 8
#define ISP_SCL_VTAP_NUM_MIN 1
#define ISP_SCL_VTAP_NUM_MAX 6

/*max allowable expension ratio in non linear*/
#define ISP_SCL_MAX_EXP 4
/*max allowable reduction ratio in non linear*/
#define ISP_SCL_MAX_RED 3

/*CSC coefficient and offset number*/
#define ISP_SCL_MAX_COL_COEFF_INDEX   9
#define ISP_SCL_MAX_COL_OFFSET_INDEX  3

#define CHECK_ISP_SCL_RETVAL(cmd)\
	do {\
		uiRetVal = cmd;\
		if (uiRetVal != ISPSS_OK)\
			return uiRetVal;\
	} while (0)

#define ISP_BCMBUF_Write_DBG(XX, YY, ZZ) ISPSS_BCMBUF_Write(XX, YY, ZZ)

#endif
