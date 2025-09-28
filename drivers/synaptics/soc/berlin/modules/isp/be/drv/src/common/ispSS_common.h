/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_COMMON_H_
#define _ISPSS_COMMON_H_

#ifdef DISABLE_BCM_ENGINE
#define START_1DDMA(dhubID, dmaID, start_addr, size, cfgQ, ptr) \
	dhub_channel_big_write_cmd(&(((struct HDL_dhub2d *)dhubID)->dhub), dmaID, \
			(INT)start_addr, size, 0, 0, 0, 1, NULL, NULL)
#define CLEAR_1DDMA(dhubID, dmaID, bcmbuf) \
	do { \
	} while (0)
#else
#define START_1DDMA(dhubID, dmaID, start_addr, size, cfgQ, ptr) \
	do { \
		dhub_channel_big_write_cmd(&(((struct HDL_dhub2d *)dhubID)->dhub), dmaID, \
			(INT)start_addr, size, 0, 0, 0, 1, cfgQ, ptr) \
	} while (0)
#define CLEAR_1DDMA(dhubID, dmaID, bcmbuf) (\
		dhub_channel_clear_seq(dhubID, dmaID, bcmbuf);)
#endif

#ifdef DISABLE_BCM_ENGINE
#define START_2DDMA(dhubID, dmaID, start_addr, stride, width, height, cfgQ) \
	do { \
		dhub2d_channel_cfg((struct HDL_dhub2d *)dhubID, dmaID, start_addr, \
			stride, width, height, 1, 0, 0, 0, 0, 1, NULL) \
	} while (0)
#define CLEAR_2DDMA(dhubID, dmaID, bcmbuf) \
	do { \
		dhub2d_channel_clear_seq(dhubID, dmaID); \
		dhub2d_channel_start_seq(dhubID, dmaID); \
	} while (0)
#else
#define START_2DDMA(dhubID, dmaID, start_addr, stride, width, height, cfgQ) \
	do { \
		dhub2d_channel_cfg((struct HDL_dhub2d *)dhubID, dmaID, start_addr, \
			stride, width, height, 1, 0, 0, 0, 0, 1, cfgQ) \
	} while (0)
#define CLEAR_2DDMA(dhubID, dmaID, bcmbuf) \
	do { \
		dhub2d_channel_clear_seq(dhubID, dmaID); \
		dhub2d_channel_start_seq(dhubID, dmaID); \
		dhub2d_channel_clear_seq_bcm(dhubID, dmaID, bcmbuf); \
	} while (0)
#endif

#ifdef DISABLE_BCM_ENGINE
#define START_2NDDMA(dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, cfgQ) \
		ispss_dhub2nd_channel_cfg((struct HDL_dhub2d *)dhubID, dmaID, start_addr, burst, \
		step1, size1, step2, size2, 0, 0, 0, 1, NULL)
#define START_2NDDMA_MTRR(dhubID, dmaID, start_addr, burst, step1, size1, step2, \
		size2, chkSemId, updSemId, cfgQ) \
		ispss_dhub2nd_channel_cfg((struct HDL_dhub2d *)dhubID, dmaID, start_addr, burst, \
		step1, size1, step2, size2, chkSemId, updSemId, 0, 1, NULL)
#define CLEAR_2NDDMA(dhubID, dmaID, bcmbuf) \
	do { \
		dhub2nd_channel_clear_seq(dhubID, dmaID); \
		dhub2nd_channel_start_seq(dhubID, dmaID); \
	} while (0)
#else
#define START_2NDDMA(dhubID, dmaID, start_addr, burst, step1, size1, step2, size2, cfgQ) \
		ispss_dhub2nd_channel_cfg((struct HDL_dhub2d *)dhubID, dmaID, start_addr, burst,\
		step1, size1, step2, size2, 0, 0, 0, 1, cfgQ)
#define START_2NDDMA_MTRR(dhubID, dmaID, start_addr, burst, step1, size1, step2, \
		size2, chkSemId, updSemId, cfgQ) \
		ispss_dhub2nd_channel_cfg((struct HDL_dhub2d *)dhubID, dmaID, start_addr, burst,\
		step1, size1, step2, size2, chkSemId, updSemId, 0, 1, cfgQ)
#define CLEAR_2NDDMA(dhubID, dmaID, bcmbuf) \
	do { \
		dhub2nd_channel_clear_seq(dhubID, dmaID); \
		dhub2nd_channel_start_seq(dhubID, dmaID); \
		dhub2nd_channel_clear_seq_bcm(dhubID, dmaID, bcmbuf); \
	} while (0)
#endif

#endif
