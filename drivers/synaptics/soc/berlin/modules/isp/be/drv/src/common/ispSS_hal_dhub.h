/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ISPSS_HAL_DHUB_H__
#define __ISPSS_HAL_DHUB_H__

void dhub2nd_channel_clear_seq(void *hdl, SIGN32 id);
void dhub2nd_channel_start_seq(void *hdl, SIGN32 id);
void dhub_channel_clear_bcmbuf(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf);
void hbo_queue_clear_bcmbuf(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf);
void dhub2d_channel_clear_bcmbuf(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf);
void dhub2d_channel_clear_seq_bcm(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf);
void dhub_channel_clear_seq(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf);
void dhub2nd_channel_clear_seq_bcm(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf);
int dhub_wait_complete(int ch, struct HDL_semaphore *pSemHandle, int timeout);
void dhub2nd_channel_clear(void *hdl, SIGN32 id);
UNSG32  dhub_channel_axqos_isp(void *hdl, SIGN32 id, UNSG32 awQosLO,
		UNSG32 awQosHI, UNSG32 arQosLO, UNSG32 arQosHI, T64b cfgQ[]);
UNSG32 dhub2nd_channel_enable(void *hdl, SIGN32 id,
		SIGN32 enable, T64b cfgQ[]);
UNSG32 dhub_channel_enable_InverseScan_vppBcm(void *hdl, SIGN32 id,
		SIGN32 iMode, UNSG32 pbcmbuf);
UNSG32 dhub_channel_enable_bcmbuf(void *hdl, SIGN32 id,
		SIGN32 enable, struct BCMBUF *pbcmbuf);
UNSG32 hbo_queue_enable_bcmbuf(void *hdl, SIGN32 id,
		SIGN32 enable, struct BCMBUF *pbcmbuf);
void dhub2d_channel_enable_bcmbuf(void *hdl, SIGN32 id,
		SIGN32 enable, struct BCMBUF *pbcmbuf);
void dhub2nd_channel_clear_bcmbuf(void  *hdl, SIGN32 id,
		struct BCMBUF *pbcmbuf);

#endif //__ISPSS_HAL_DHUB_H__

