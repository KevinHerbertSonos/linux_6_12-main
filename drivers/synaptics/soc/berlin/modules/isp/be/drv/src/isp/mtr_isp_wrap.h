/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __MTR_ISP_WRAP_H__
#define __MTR_ISP_WRAP_H__

#include "mtr_isp.h"

#define QOS_DISABLE   0
#define QOS_ENABLE    1


INT32 ISPSS_MTR_ConfigureMtr(struct v4l2_format *f, uint32_t YBaseAddr,
		uint32_t UVBaseAddr, uint32_t path, void *pBCMBuf);
INT32 ISPSS_MTR_StartThread(struct ISPSS_MTR_CONFIG_PARAM *mtrConfigParam,
		struct v4l2_format *f, uint32_t  YBaseAddr, uint32_t UVBaseAddr);
void ISPSS_MTR_PrintInfo(struct ISPSS_MTR_CONFIG_PARAM *mtrConfigParam);
void ISPSS_MTR_Exit(uint32_t path);
uint32_t ISPSS_MTR_QOS_Config(int config);
#endif //MTR_ISP_WRAP_H__
