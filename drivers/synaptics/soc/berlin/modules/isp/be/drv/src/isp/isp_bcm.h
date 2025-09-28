/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ISP_BCM_H__
#define __ISP_BCM_H__

/* Whether to block on semaphore full */
#define NOBLOCK	0
#define BLOCK	1

int isp_bcm_open(void);
void isp_bcm_close(void);
int isp_bcm_commit(void *pBcmBuf, bool is_immediate, int path, int block);
void isp_bcm_configure(int path);
void *isp_bcm_get_next_bcmbuf(void);
unsigned int is_isp_bcm_buffer_full(int path);
void isp_bcm_deconfigure(int path);
void isp_bcm_update_interrupt_mux(int path, int isConfigure);
void isp_bcm_enable_clock(void);
void isp_bcm_disable_clock(void);
#endif
