/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_BE_SCL_API_H_
#define _ISPSS_BE_SCL_API_H_

INT ISPSS_BE_SCL_Reset(struct ISP_BE_RQST_MSG *psclRqstMsg);
INT ISPSS_BE_DNSCL_GetNoOfFramesWaiting(INT iClientId, UINT32 *puiFramesWaiting);
INT ISPSS_BE_DNSCL_GetNoOfFramesWaiting_ClientQ(struct ISPBE_GET_FRAMEQ_WAIT_CNT *frameWaitCnt);
void ISPSS_BE_DNSCL3_Probe(struct ISPBE_CA_DRV_CTX *drv_ctx);


#endif
