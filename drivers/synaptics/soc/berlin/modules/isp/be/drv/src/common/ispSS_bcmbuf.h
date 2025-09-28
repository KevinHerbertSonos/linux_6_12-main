/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _BCMBUF_H_
#define _BCMBUF_H_

#include "com_type.h"

#define BCM_NON_BLOCKING_WAIT 0
#define BCM_BLOCKING_WAIT 1

struct DHUB_CFGQ {
	UINT64 handle;
	INT32 shm_offset;
	UINT64 *addr;
	INT len;
	UINT64 *phy_addr;
};

/* structure of a buffer descriptor */
/* a buffer descriptor contains the pointers of the entire buffer and sub-buffers */
struct BCMBUF {
	/*Note: Ensure the following order matches the order of hal_dhub.c:VIP_BCMBUF
	 * 'writer' is updated inside hal_dhub.c:clear functions
	 */
	UINT64 *head;          // head of total BCM buffer
	UINT64 *tail;          // tail of the buffer, used for checking wrap around
	UINT64 *writer;        // write pointer of queue, update with shadow_tail with commit
	int size;                    // size of total BCM buffer
	UINT64 handle;
	INT32 shm_offset;
	UINT64 *dv1_head;      // head of BCM sub-buffer used for CPCB0
	UINT64 *dv2_head;      // head of BCM sub-buffer used for CPCB1
	UINT64 *dv3_head;      // head of BCM sub-buffer used for CPCB2
	int subID;                   // sub-buffer ID currently in use
	UINT64 *phy_addr;
};

/******* register programming buffer APIs **************/
/***************************************************************
 * FUNCTION: allocate register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 *       : size - size of the buffer to allocate
 *       :      - (should be a multiple of 4)
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int ISPSS_BCMBUF_Create(struct BCMBUF *pbcmbuf, int size);

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int ISPSS_BCMBUF_Destroy(struct BCMBUF *pbcmbuf);

/***************************************************************
 * FUNCTION: reset a register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
int ISPSS_BCMBUF_Reset(struct BCMBUF *pbcmbuf);

/*********************************************************
 * FUNCTION: selest BCM sub-buffer to use
 * PARAMS: *buf - pointer to the buffer descriptor
 *         subID - DV_1, DV_2, DV_3
 ********************************************************/
void ISPSS_BCMBUF_Select(struct BCMBUF *pbcmbuf, int subID);

/*********************************************************
 * FUNCTION: write register address (4bytes) and value (4bytes) to the buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               address - address of the register to be set
 *               value - the value to be written into the register
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int ISPSS_BCMBUF_Write(struct BCMBUF *pbcmbuf, unsigned int address, unsigned int value);

/*********************************************************
 * FUNCTION: write a block of data to BCM buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               *pdata - pointer to the data
 *               length - the length of the data to be written to BCM buffer
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int ISPSS_BCMBUF_WriteBlock(struct BCMBUF *pbcmbuf, UINT64 *pdata, unsigned int length);

/*********************************************************************
 * FUNCTION: do the hardware transmission
 * PARAMS: block - 0: return without waiting for transaction finishing
 *                 1: return after waiting for transaction finishing
 ********************************************************************/
void ISPSS_BCMBUF_HardwareTrans(struct BCMBUF *pbcmbuf, int block);

/*********************************************************************
 * FUNCTION: do the hardware transaction
 * PARAMS: pdata - pointer to the data block
 *               length - data length for transaction
 ********************************************************************/
void ISPSS_BCMBUF_HardwareTransBlock(const unsigned int *pdata, unsigned int length, int block);

/*********************************************************************
 * FUNCTION: commit raw buffer to BCM
 * PARAMS: start      - start address of raw buffer
 *         size       - size of bcm buffer
 ********************************************************************/
int ISPSS_BCMDHUB_Raw_To_Commit(unsigned int *start, int size);

/*******************************************************************************
 * FUNCTION: commit cfgQ which contains BCM DHUB programming info to interrupt service routine
 * PARAMS: *cfgQ - cfgQ
 *         cpcbID - cpcb ID which this cmdQ belongs to
 *         intrType - interrupt type which this cmdQ belongs to: 0 - VBI, 1 - VDE
 * NOTE: this API is only called from VBI/VDE ISR.
 *******************************************************************************/
int ISPSS_BCMDHUB_CFGQ_Commit(struct DHUB_CFGQ *cfgQ, int cpcbID, int intrType, int block);

/*******************************************************************************
 * FUNCTION: commit cfgQ which contains DMA dhub programming info to VBI interrupt service routine
 * PARAMS: *cfgQ - cfgQ
 *         cpcbID - cpcb ID which this cmdQ belongs to
 * NOTE: this API is only called from VBI/VDE ISR.
 *******************************************************************************/
int ISPSS_DMADHUB_CFGQ_Commit(struct DHUB_CFGQ *cfgQ, int cpcbID);

/*********************************************************************
 * FUNCTION: send a BCM BUF info to a BCM cfgQ
 * PARAMS: *pbcmbuf - pointer to the struct BCMBUF
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int ISPSS_BCMBUF_To_CFGQ(struct BCMBUF *pbcmbuf, struct DHUB_CFGQ *cfgQ);

/*********************************************************************
 * FUNCTION: send a raw BCM BUF info to a BCM cfgQ
 * PARAMS: pdata - pointer to the data block
 *         length - data length for transaction
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int ISPSS_BCMBUF_Raw_To_CFGQ(const UINT32 *pdata, UINT32 length, struct DHUB_CFGQ *cfgQ);

/*********************************************************************
 * FUNCTION: send a BCM cfgQ info to a BCM cfgQ
 * PARAMS: src_cfgQ - pointer to the source BCM cfgQ
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int ISPSS_CFGQ_To_CFGQ(struct DHUB_CFGQ *src_cfgQ, struct DHUB_CFGQ *cfgQ);

int ISPSS_BCMDHUB_AutoPush(int cpcbID, int intrType, int enable);

int ISPSS_CFGQ_Create(struct DHUB_CFGQ *pCfgQ, int size);

int ISPSS_CFGQ_Destroy(struct DHUB_CFGQ *pCfgQ);

int ISPSS_CFGQ_To_BCMBUF(struct DHUB_CFGQ *pCfgQ, struct BCMBUF *pbcmbuf);

void ISPSS_BCMBUF_Init(void);

void ISPSS_BCMBUF_DeInit(void);

#endif

