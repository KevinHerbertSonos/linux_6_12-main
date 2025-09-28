// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/mutex.h>
#include <linux/delay.h>

#include "ispSS_bcmbuf.h"
#include "ispSS_reg.h"
#include "ispSS_api_dhub.h"
#include "ispSS_shm.h"
#include "ispbe_err.h"

extern struct HDL_dhub2d ISPSS_TSB_dhubHandle;
static struct mutex gBCM_SUBMIT_lock;

void ISPSS_BCMBUF_Init(void)
{
	mutex_init(&gBCM_SUBMIT_lock);
}

void ISPSS_BCMBUF_DeInit(void)
{
	mutex_destroy(&gBCM_SUBMIT_lock);
}

/***************************************************************
 * FUNCTION: allocate register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 *       : size - size of the buffer to allocate
 *       :      - (should be a multiple of 4)
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
INT ISPSS_BCMBUF_Create(struct BCMBUF *pbcmbuf, int size)
{
	if (size <= 0)
		return (ISPSS_EBADPARAM);

	/* allocate memory for the buffer */
	if (ispSS_SHM_Allocate(SHM_NONSECURE, size, 32, &pbcmbuf->handle,
				SHM_NONSECURE_CONTIG) != SUCCESS)
		return ISPSS_ENOMEM;

	pbcmbuf->size = size;
	ispSS_SHM_GetVirtualAddress(pbcmbuf->handle, 0, (void *)&pbcmbuf->head);
	ispSS_SHM_GetPhysicalAddress(pbcmbuf->handle, 0, (void *)&pbcmbuf->phy_addr);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_Create);

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
INT ISPSS_BCMBUF_Destroy(struct BCMBUF *pbcmbuf)
{
	if (!pbcmbuf)
		return ISPSS_EBADPARAM;
	/* allocate memory for the buffer */
	if (!pbcmbuf->head)
		return (ISPSS_EBADCALL);

	ispSS_SHM_Release(pbcmbuf->handle);

	pbcmbuf->head = NULL;

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_Destroy);

/***************************************************************
 * FUNCTION: reset a register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
INT ISPSS_BCMBUF_Reset(struct BCMBUF *pbcmbuf)
{
	if (!pbcmbuf)
		return ISPSS_OK;

	pbcmbuf->tail = pbcmbuf->head + (pbcmbuf->size/2);

	/*set pointers to the head*/
	pbcmbuf->writer = pbcmbuf->head;
	pbcmbuf->dv1_head = pbcmbuf->head;
	pbcmbuf->dv2_head = pbcmbuf->dv1_head + (pbcmbuf->size/4);
	pbcmbuf->dv3_head = pbcmbuf->dv2_head + (pbcmbuf->size/4);
	pbcmbuf->subID = -1; /* total */

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_Reset);

/*********************************************************
 * FUNCTION: Select sub register programming buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *         subID - CPCB_1, CPCB_2, CPCB_3 or total
 ********************************************************/
void ISPSS_BCMBUF_Select(struct BCMBUF *pbcmbuf, INT subID)
{
	if (!pbcmbuf)
		return;
	/* reset read/write pointer of the buffer */
	if (subID == CPCB_1)
		pbcmbuf->writer = pbcmbuf->dv1_head;
	else
		pbcmbuf->writer = pbcmbuf->head;

	pbcmbuf->subID = subID;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_Select);

/*********************************************************
 * FUNCTION: write register address (4 bytes) and value (4 bytes) to the buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               address - address of the register to be set
 *               value - the value to be written into the register
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
INT ISPSS_BCMBUF_Write(struct BCMBUF *pbcmbuf, UINT32 address, UINT32 value)
{
	UINT64 *end;
	UINT64 *start;

	if (!pbcmbuf) {
		ISPSS_REG_WRITE32(address, value);
		return ISPSS_OK;
	}
	start = pbcmbuf->dv1_head;
	/*if not enough space for storing another 8 bytes, wrap around happens*/
	if (pbcmbuf->subID == CPCB_1)
		end = pbcmbuf->dv2_head;
	else
		end = pbcmbuf->tail;

	if (end == pbcmbuf->writer)
		/*the buffer is full, no space for wrap around*/
		return ISPSS_EBCMBUFFULL;

	/*save the data to the buffer*/
	bcm_buffer_write(pbcmbuf, address, value);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_Write);

/*********************************************************
 * FUNCTION: write a block of data to BCM buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               *pdata - pointer to the data
 *               length - the length of the data to be written to BCM buffer
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
INT ISPSS_BCMBUF_WriteBlock(struct BCMBUF *pbcmbuf, UINT64 *pdata, UINT32 length)
{
	UINT64 *end;

	if (!pbcmbuf)
		return ISPSS_EBADPARAM;

	if (pbcmbuf->subID == CPCB_1)
		end = pbcmbuf->dv2_head;
	else
		end = pbcmbuf->tail;

	if (pbcmbuf->writer > end - (length >> 3))
		/*the buffer is full*/
		return ISPSS_EBCMBUFFULL;

	/*save the data to BCM buffer*/
	GaloisMemcpy(pbcmbuf->writer, pdata, length);

	pbcmbuf->writer += (length >> 3);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_WriteBlock);

/*********************************************************************
 * FUNCTION: do the hardware transaction
 * PARAMS: *buf - pointer to the buffer descriptor
 ********************************************************************/
void ISPSS_BCMBUF_HardwareTrans(struct BCMBUF *pbcmbuf, INT block)
{
	struct HDL_semaphore *pSemHandle;
	struct HDL_dhub2d *pDhubHandle;
	UINT64 *start;
	int status;
	int dhubID, size;
	UNSG32 FullSts;
	INT32 shm_offset;
	unsigned int bcm_sched_cmd[2];

	if (!pbcmbuf)
		return;

	if (pbcmbuf->subID == CPCB_1) {
		start = pbcmbuf->dv1_head;
		shm_offset = pbcmbuf->dv1_head - pbcmbuf->head;
	} else {
		start = pbcmbuf->head;
		shm_offset = 0;
	}

	size = pbcmbuf->writer-start;

	if (size <= 0)
		return;

	if (bcm_buffer_dummy_write(pbcmbuf, size/4))
		//If dummy entry added then size inc by 8 (8 bytes per dummy entry)
		size += 8;

	shm_offset = shm_offset * 4;
	/* flush data in D$ */
	ispSS_SHM_CleanCache(pbcmbuf->handle, shm_offset, size);
	/* get non-cache physical address for DMA */
	start = pbcmbuf->phy_addr;
	start += shm_offset;

	mutex_lock(&gBCM_SUBMIT_lock);

	do {
		BCM_SCHED_GetFullSts(BCM_SCHED_Q12, &FullSts);
	} while (FullSts);

	/* start BCM engine */
	dhubID = ispDhubChMap_TSB_BCM_R1;
	pDhubHandle = &ISPSS_TSB_dhubHandle;

	if (block) {
		pSemHandle = ispss_dhub_semaphore(&(pDhubHandle->dhub));
		/* clear possible BCM previous interrupt */
		status = ispss_semaphore_chk_full(pSemHandle, dhubID);
		if (status) {
			semaphore_pop(pSemHandle, dhubID, 1);
			semaphore_clr_full(pSemHandle, dhubID);
		}
	}

	/* clear BCM interrupt */
	pSemHandle = ispss_dhub_semaphore(&(pDhubHandle->dhub));
	status = ispss_semaphore_chk_full(pSemHandle, dhubID);
	while (status) {
		semaphore_pop(pSemHandle, dhubID, 1);
		semaphore_clr_full(pSemHandle, dhubID);
		status = ispss_semaphore_chk_full(pSemHandle, dhubID);
	}

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
			(UINT64)start, (INT)size, 0, 0, 0, 1, bcm_sched_cmd);
	while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL))
		;

	if (block) {
		/* check BCM interrupt */
		pSemHandle = ispss_dhub_semaphore(&(pDhubHandle->dhub));
		status = ispss_semaphore_chk_full(pSemHandle, dhubID);
		while (!status)
			status = ispss_semaphore_chk_full(pSemHandle, dhubID);

		/* clear BCM interrupt */
		semaphore_pop(pSemHandle, dhubID, 1);
		semaphore_clr_full(pSemHandle, dhubID);
	}

	mutex_unlock(&gBCM_SUBMIT_lock);
}
EXPORT_SYMBOL(ISPSS_BCMBUF_HardwareTrans);

static void ISPSS_BCMBUF_Commit_To_CFGQ(int intrType,
		UINT64 phy_start, UINT32 size, struct DHUB_CFGQ *cfgQ)
{
	int index;
	unsigned int bcm_sched_cmd[2];
	/* Acq lock as there could be concurrent users for BCM,
	 * ISP & BE are dual users
	 */
	mutex_lock(&gBCM_SUBMIT_lock);

	dhub_channel_generate_cmd(&(ISPSS_TSB_dhubHandle.dhub),
			ispDhubChMap_TSB_BCM_R1, (INT)phy_start,
			(INT)size, 0, 0, 0, 1, bcm_sched_cmd);

	while (!(index = BCM_SCHED_PushCmd(BCM_SCHED_Q13,
					bcm_sched_cmd, cfgQ->addr + cfgQ->len)))
		usleep_range(2, 5);

	cfgQ->len += index;

	mutex_unlock(&gBCM_SUBMIT_lock);
}

/*********************************************************************
 * FUNCTION: send a BCM BUF info to a BCM cfgQ
 * PARAMS: *pbcmbuf - pointer to the BCMBUF
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int ISPSS_BCMBUF_To_CFGQ(struct BCMBUF *pbcmbuf, struct DHUB_CFGQ *cfgQ)
{
	UINT64 *start, *phy_start;
	INT32 size, shm_offset;

	if (!pbcmbuf) {
		pr_err("%s: pbcmbuf is NULL\n", __func__);
		return ISPSS_EBADPARAM;
	}

	if (pbcmbuf->subID == CPCB_1) {
		start = pbcmbuf->dv1_head;
		shm_offset = pbcmbuf->dv1_head - pbcmbuf->head;

		phy_start = pbcmbuf->phy_addr;
	} else {
		start = pbcmbuf->head;
		shm_offset = 0;
		phy_start = pbcmbuf->phy_addr;
	}

	size = (UINT64)pbcmbuf->writer-(UINT64)start;

	if (size <= 0) {
		pr_err("%s: size is %d\n", __func__, size);
		return ISPSS_EBADPARAM;
	}

	if (bcm_buffer_dummy_write(pbcmbuf, size/4)) {
		// If dummy entry added then size inc by 8 (8 bytes per dummy entry)
		size += 8;
	}

	shm_offset = shm_offset*4;
	ispSS_SHM_CleanCache(pbcmbuf->handle, shm_offset,  size);

	ISPSS_BCMBUF_Commit_To_CFGQ(BCM_SCHED_Q13, (UINT64)phy_start, size, cfgQ);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_To_CFGQ);

/*********************************************************************
 * FUNCTION: send a raw BCM BUF info to a BCM cfgQ
 * PARAMS: pdata - pointer to the data block
 *         length - data length for transaction
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int ISPSS_BCMBUF_Raw_To_CFGQ(const UINT32 *pdata, UINT32 length, struct DHUB_CFGQ *cfgQ)
{
	if (!cfgQ)
		return ISPSS_EBADPARAM;

	if (raw_bcm_buffer_dummy_write((UINT64 *)pdata, length/8)) {
		//If dummy entry added then size inc by 8 (8 bytess per dummy entry)
		length += 8;
	}

	ISPSS_BCMBUF_Commit_To_CFGQ(BCM_SCHED_Q13, (UINT64)pdata, length, cfgQ);

	return ISPSS_OK;
}

/*********************************************************************
 * FUNCTION: send a BCM cfgQ info to a BCM cfgQ
 * PARAMS: src_cfgQ - pointer to the source BCM cfgQ
 *         *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int ISPSS_CFGQ_To_CFGQ(struct DHUB_CFGQ *src_cfgQ, struct DHUB_CFGQ *cfgQ)
{
	if ((!cfgQ) || (!src_cfgQ))
		return ISPSS_EBADPARAM;

	if (src_cfgQ->len <= 0)
		return ISPSS_EBADPARAM;

	if (raw_bcm_buffer_dummy_write((UINT64 *)src_cfgQ->addr, src_cfgQ->len)) {
		//If dummy entry added then size inc by 1 pair(2 words per dummy entry)
		src_cfgQ->len += 1;
	}

	ispSS_SHM_CleanCache(src_cfgQ->handle, 0, src_cfgQ->len*8);

	ISPSS_BCMBUF_Commit_To_CFGQ(BCM_SCHED_Q13,
			(UINT64)src_cfgQ->phy_addr, (INT)src_cfgQ->len*8, cfgQ);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_CFGQ_To_CFGQ);

/*********************************************************************
 * FUNCTION: commit raw BCMBUF to interrupt service routine
 * PARAMS: start      - start address of bcm buffer
 *         size       - size of bcm buffer
 ********************************************************************/
int ISPSS_BCMDHUB_Raw_To_Commit(uint32_t *start, int size)
{
	int dhubID;
	struct HDL_dhub2d *pDhubHandle;
	unsigned int bcm_sched_cmd[2];
	unsigned int FullSts;

	/* Acq lock as there could be concurrent users for
	 * BCM, ISP & BE are dual users
	 */
	mutex_lock(&gBCM_SUBMIT_lock);

	do {
		BCM_SCHED_GetFullSts(BCM_SCHED_Q12, &FullSts);
	} while (FullSts);

	dhubID = ispDhubChMap_TSB_BCM_R1;
	pDhubHandle = &ISPSS_TSB_dhubHandle;

	dhub_channel_generate_cmd(&(pDhubHandle->dhub),
			ispDhubChMap_TSB_BCM_R1, (UINT64)start, (INT)size,
			0, 0, 0, 1, bcm_sched_cmd);
	while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL))
		;

	mutex_unlock(&gBCM_SUBMIT_lock);

	return ISPSS_OK;

}
EXPORT_SYMBOL(ISPSS_BCMDHUB_Raw_To_Commit);

/*******************************************************************************
 * FUNCTION: commit cfgQ which contains BCM DHUB programming info to interrupt service routine
 * PARAMS: *cfgQ - cfgQ
 *         cpcbID - cpcb ID which this cmdQ belongs to
 *         intrType - interrupt type which this cmdQ belongs to
 *******************************************************************************/
int ISPSS_BCMDHUB_CFGQ_Commit(struct DHUB_CFGQ *cfgQ, int cpcbID, int intrType, int block)
{
	/*trig_event*/
	unsigned int sched_qid = intrType;
	UNSG32 FullSts;
	unsigned int bcm_sched_cmd[2];
	int ret  = ISPSS_OK;
	struct HDL_semaphore *pSemHandle;
	struct HDL_dhub2d *pDhubHandle;
	int dhubID, status;
	int counter;

	if (!cfgQ)
		return ISPSS_EBADPARAM;

	if (cfgQ->len <= 0)
		return ISPSS_EBADPARAM;

	mutex_lock(&gBCM_SUBMIT_lock);

	counter = 0;
	do {
		BCM_SCHED_GetFullSts(sched_qid, &FullSts);
		usleep_range(2, 5);

		counter++;
		if (counter % 30000 == 0) {
			pr_err("%s %d: get full: %X qnum %d\n", __func__, __LINE__,
					FullSts, intrType);
			ret = ISPSS_EBCMBUFFULL;
			goto exit;
		}
	} while (FullSts);

	if (block) {
		dhubID = ispDhubChMap_TSB_BCM_R1;
		pDhubHandle = &ISPSS_TSB_dhubHandle;

		pSemHandle = ispss_dhub_semaphore(&(pDhubHandle->dhub));
		status = ispss_semaphore_chk_full(pSemHandle, dhubID);
		if (status) {
			semaphore_pop(pSemHandle, dhubID, 1);
			semaphore_clr_full(pSemHandle, dhubID);
		}

		pSemHandle = ispss_dhub_semaphore(&(pDhubHandle->dhub));
		status = ispss_semaphore_chk_full(pSemHandle, dhubID);
		counter = 0;
		while (status) {
			semaphore_pop(pSemHandle, dhubID, 1);
			semaphore_clr_full(pSemHandle, dhubID);

			status = ispss_semaphore_chk_full(pSemHandle, dhubID);
			usleep_range(5, 10);
			counter++;
			if (counter % 20000l == 0) {
				ret = ISPSS_EBCMBUFFULL;
				pr_err("%s %d: sem full: %X\n", __func__, __LINE__, status);
				goto exit;
			}
		}
	}

	ispSS_SHM_CleanCache(cfgQ->handle, 0, cfgQ->len*16);
	dhub_channel_generate_cmd(&(ISPSS_TSB_dhubHandle.dhub), ispDhubChMap_TSB_BCM_R1,
			(UINT64)cfgQ->phy_addr, (INT)cfgQ->len*8, 0, 0, 0, 1, bcm_sched_cmd);

	counter = 0;
	while (!BCM_SCHED_PushCmd(sched_qid, bcm_sched_cmd, NULL)) {
		usleep_range(2, 10);
		counter++;
		if (counter % 30000 == 0) {
			pr_err("%s %d: push cmd\n", __func__, __LINE__);
			BCM_SCHED_Flush(1 << intrType);
			ret = ISPSS_EBCMBUFFULL;
			goto exit;
		}
	}
	counter = 0;
	if (block) {
		pSemHandle = ispss_dhub_semaphore(&(pDhubHandle->dhub));
		status = ispss_semaphore_chk_full(pSemHandle, dhubID);
		counter = 0;

		while (!status) {
			status = ispss_semaphore_chk_full(pSemHandle, dhubID);
			usleep_range(5, 10);
			counter++;
			if (counter % 30000 == 0) {
				pr_err("%s %d: sem full: %X qnum %d\n", __func__, __LINE__,
						status, intrType);
				ret = ISPSS_EBCMBUFFULL;
				break;
			}
		}
		semaphore_pop(pSemHandle, dhubID, 1);
		semaphore_clr_full(pSemHandle, dhubID);
	}
exit:
	mutex_unlock(&gBCM_SUBMIT_lock);

	return ret;
}
EXPORT_SYMBOL(ISPSS_BCMDHUB_CFGQ_Commit);

int ISPSS_BCMDHUB_AutoPush(int cpcbID, int intrType, int enable)
{
	/*trig_event*/
	unsigned int sched_qid = intrType;

	BCM_SCHED_AutoPushCmd(sched_qid, enable);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMDHUB_AutoPush);

INT ISPSS_CFGQ_Create(struct DHUB_CFGQ *pCfgQ, int size)
{
	//Allocate CfgQ
	if (ispSS_SHM_Allocate(SHM_NONSECURE, size, 32, &pCfgQ->handle,
				SHM_NONSECURE_CONTIG) != ISPSS_OK)
		return (ISPSS_ENOMEM);

	pCfgQ->len = 0;
	ispSS_SHM_GetVirtualAddress(pCfgQ->handle, 0, (void *)&pCfgQ->addr);
	ispSS_SHM_GetPhysicalAddress(pCfgQ->handle, 0, (void *)&pCfgQ->phy_addr);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_CFGQ_Create);

INT ISPSS_CFGQ_Destroy(struct DHUB_CFGQ *pCfgQ)
{
	if (pCfgQ->addr) {
		ispSS_SHM_Release(pCfgQ->handle);
		return ISPSS_OK;
	}

	return ISPSS_EBADCALL;
}
EXPORT_SYMBOL(ISPSS_CFGQ_Destroy);
