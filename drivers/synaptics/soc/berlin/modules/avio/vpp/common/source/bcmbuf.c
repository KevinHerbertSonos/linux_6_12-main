// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */
#include "linux/slab.h"
#include "asm/io.h"

#include "bcmbuf.h"

static VPP_MEM_LIST *bcm_mem_list;
/***************************************************************
 * FUNCTION: allocate register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 *	   : size - size of the buffer to allocate
 *	   :	  - (should be a multiple of 4)
 * RETURN:  0		- succeed
 *		  non-zero - failed to initialize a BCM buffer
 ****************************************************************/
int bcmbuf_create(BCMBUF *pbcmbuf, int size, VPP_MEM_LIST *memlist)
{
	if (size <= 0)
		return (BCMBUF_EBADPARAM);

	pbcmbuf->vpp_bcmbuf_mem_handle.size = size;
	VPP_MEM_AllocateMemory(memlist, VPP_MEM_TYPE_DMA,
					&pbcmbuf->vpp_bcmbuf_mem_handle, 0);

	pbcmbuf->head = pbcmbuf->vpp_bcmbuf_mem_handle.k_addr;
	pbcmbuf->phy_addr = pbcmbuf->vpp_bcmbuf_mem_handle.p_addr;
	pbcmbuf->size = size;
	bcm_mem_list = memlist;
	return 0;
}

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  0 - succeed
 *		  non-zero - failed to initialize a BCM buffer
 ****************************************************************/
int bcmbuf_destroy(BCMBUF *pbcmbuf)
{
	/* de-allocate memory for the buffer */
	if (!pbcmbuf->head)
		return (BCMBUF_EBADCALL);

	VPP_MEM_FreeMemory(bcm_mem_list,
			VPP_MEM_TYPE_DMA,
			&pbcmbuf->vpp_bcmbuf_mem_handle);

	pbcmbuf->head = NULL;

	return BCMBUF_OK;
}

/***************************************************************
 * FUNCTION: reset a register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  0 - succeed
 *		  non-0 - failed to initialize a BCM buffer
 ****************************************************************/
int bcmbuf_reset(BCMBUF *pbcmbuf)
{

	pbcmbuf->tail = pbcmbuf->head + pbcmbuf->size;

	/*set pointers to the head*/
	pbcmbuf->writer = pbcmbuf->head;
	pbcmbuf->subID = -1; /* total */

	return BCMBUF_OK;
}

/*********************************************************
 * FUNCTION: Select sub register programming buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *		 subID - CPCB_1, CPCB_2, CPCB_3 or total
 ********************************************************/
void bcmbuf_select(BCMBUF *pbcmbuf, int subID)
{
	/* reset read/write pointer of the buffer */
	pbcmbuf->writer = pbcmbuf->head;
	pbcmbuf->subID = subID;

	return;
}

/*********************************************************
 * FUNCTION: write register address (4 bytes) and value (4 bytes) to the buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *			   address - address of the register to be set
 *			   value - the value to be written into the register
 * RETURN: 0 - succeed
 *		 non-0 - register programming buffer is full
 ********************************************************/
int bcmbuf_write(BCMBUF *pbcmbuf, unsigned int address, unsigned int value)
{
	unsigned int *end;

	/*if not enough space for storing another 8 bytes, wrap around happens*/
	end = pbcmbuf->tail;

	if (pbcmbuf->writer == end){
		/*the buffer is full, no space for wrap around*/
		pr_err("LCDC_BCMBUF_Write failed (0x%x 0x%x)\r\n", address, value);
		return BCMBUF_EBCMBUFFULL;
	}

	/*save the data to the buffer*/
	*pbcmbuf->writer = value;
	pbcmbuf->writer ++;
	*pbcmbuf->writer = address;
	pbcmbuf->writer ++;
	return BCMBUF_OK;
}

/*********************************************************************
 * FUNCTION: do the hardware transaction
 * PARAMS: *buf - pointer to the buffer descriptor
 ********************************************************************/
void bcmbuf_hardwaretrans(HDL_dhub2d *pDhubHandle,
						  int dhubID,
						  unsigned int QID,
						  BCMBUF *pbcmbuf,
						  int block)
{
	phys_addr_t start;
	int size;
	int shm_offset;

	shm_offset = 0;

	size = (long long)pbcmbuf->writer - (long long)pbcmbuf->head;

	if (size <= 0)
		return;

	shm_offset = shm_offset * 4;

	/* get non-cache physical address for DMA */
	start = (pbcmbuf->phy_addr + shm_offset);

	bcmbuf_raw_hardwaretrans(pDhubHandle,
							dhubID,
							QID,
							start,
							size,
							block);

	return;
}

/*********************************************************************
 * FUNCTION: send a BCM BUF info to a BCM cfgQ
 * PARAMS: *pbcmbuf - pointer to the BCMBUF
 *		 *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
int bcmbuf_to_CFGQ(HDL_dhub2d *pDhubHandle,
				   int dhubID,
				   unsigned int QID,
				   BCMBUF *pbcmbuf,
				   DHUB_CFGQ *cfgQ)
{
	phys_addr_t phy_start;
	int size, shm_offset;
	unsigned int bcm_sched_cmd[2];

	shm_offset = 0;
	phy_start = pbcmbuf->phy_addr;

	size = (int)(long long)pbcmbuf->writer-(int)(long long)pbcmbuf->head;
	if (size <= 0)
		return BCMBUF_EBADPARAM;
	shm_offset = shm_offset * 4;

	VPP_MEM_FlushCache(bcm_mem_list, &pbcmbuf->vpp_bcmbuf_mem_handle, 0);

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
							(int)(long long)phy_start,
							(int)size, 0, 0, 0, 1,
							bcm_sched_cmd);

	while (!BCM_SCHED_PushCmd(QID, bcm_sched_cmd, cfgQ->addr + cfgQ->len*2));
	cfgQ->len += 2;

	return BCMBUF_OK;
}

/*********************************************************************
 * FUNCTION: send a BCM cfgQ info to a BCM cfgQ
 * PARAMS: src_cfgQ - pointer to the source BCM cfgQ
 *		 *cfgQ - target BCM cfgQ
 * NOTE: this API is only called from VBI/VDE ISR.
 ********************************************************************/
void bcmbuf_CFGQ_To_CFGQ(HDL_dhub2d *pDhubHandle,
					   int dhubID,
					   unsigned int QID,
					   DHUB_CFGQ *src_cfgQ,
					   DHUB_CFGQ *cfgQ)
{
#ifndef DISABLE_BCM_ENGINE
	unsigned int bcm_sched_cmd[2];

	if (src_cfgQ->len <= 0)
		return ;

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
							(int)(long long)src_cfgQ->phy_addr,
							(int)src_cfgQ->len*8, 0, 0, 0, 1,
							bcm_sched_cmd);

	while (!BCM_SCHED_PushCmd(QID, bcm_sched_cmd, cfgQ->addr + cfgQ->len*2));
	cfgQ->len += 2;
#endif
}

/*******************************************************************************
 * FUNCTION: commit cfgQ which contains BCM DHUB programming info to interrupt service routine
 * PARAMS: *cfgQ - cfgQ
 *		 cpcbID - cpcb ID which this cmdQ belongs to
 *		 intrType - interrupt type which this cmdQ belongs to: 0 - VBI, 1 - VDE
 * NOTE: this API is only called from VBI/VDE ISR.
 *******************************************************************************/
int bcmbuf_DHUB_CFGQ_Commit(HDL_dhub2d *pDhubHandle,
							int dhubID,
							unsigned int sched_qid,
							DHUB_CFGQ *cfgQ,
							int cpcbID,
							int intrType)
{
#ifndef DISABLE_BCM_ENGINE
	unsigned int bcm_sched_cmd[2];

	if (cfgQ->len <= 0)
		return BCMBUF_EBADPARAM;

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
							(int)(long long)cfgQ->phy_addr,
							(int)cfgQ->len*8, 0, 0, 0, 1,
							bcm_sched_cmd);

	while (!BCM_SCHED_PushCmd(sched_qid, bcm_sched_cmd, NULL));
#endif

	return BCMBUF_OK;
}

void bcmbuf_cfgq_hardwaretrans(HDL_dhub2d *pDhubHandle,
								int dhubID,
								unsigned int QID,
								DHUB_CFGQ *cfgQ,
								int block)
{
	if (cfgQ->len <= 0)
		return;

	bcmbuf_raw_hardwaretrans(pDhubHandle,
							dhubID,
							QID,
							cfgQ->phy_addr,
							(cfgQ->len * 8),
							block);
}

int bcmbuf_DHUB_AutoPush(unsigned int sched_qid, int intrType, int enable)
{
	return BCM_SCHED_AutoPushCmd(sched_qid, enable);
}

void bcmbuf_raw_hardwaretrans(HDL_dhub2d *pDhubHandle,
							int dhubID,
							unsigned int QID,
							phys_addr_t start,
							size_t size,
							int block)
{
	HDL_semaphore *pSemHandle;
	int status;
	unsigned int bcm_sched_cmd[2];

	if (size <= 0)
		return;

	/* start BCM engine */
	pSemHandle = dhub_semaphore(&(pDhubHandle->dhub));
	if (block) {
		/* clear possible BCM previous interrupt */
		status = semaphore_chk_full(pSemHandle, dhubID);
		if (status) {
			semaphore_pop(pSemHandle, dhubID, 1);
			semaphore_clr_full(pSemHandle, dhubID);
		}
	}

	/* clear BCM interrupt */
	status = semaphore_chk_full(pSemHandle, dhubID);
	while (status) {
		semaphore_pop(pSemHandle, dhubID, 1);
		semaphore_clr_full(pSemHandle, dhubID);
		status = semaphore_chk_full(pSemHandle, dhubID);
	}

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
							(int)(long long)start,
							(int)size, 0, 0, 0, 1,
							bcm_sched_cmd);

	while( !BCM_SCHED_PushCmd(QID, bcm_sched_cmd, NULL));

	if (block) {
		/* check BCM interrupt */
		pSemHandle = dhub_semaphore(&(pDhubHandle->dhub));
		status = semaphore_chk_full(pSemHandle, dhubID);
		while (!status) {
			status = semaphore_chk_full(pSemHandle, dhubID);
		}

		/* clear BCM interrupt */
		semaphore_pop(pSemHandle, dhubID, 1);
		semaphore_clr_full(pSemHandle, dhubID);
	}
}
