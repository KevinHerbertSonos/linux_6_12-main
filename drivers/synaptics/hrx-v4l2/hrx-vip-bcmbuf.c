// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-drv.h"
#include "hrx-vip-bcmbuf.h"
#include "hrx-reg.h"

#define BCM_SCHED_PushCmd wrap_BCM_SCHED_PushCmd
#define BCM_SCHED_SetMux wrap_BCM_SCHED_SetMux
#define BCM_SCHED_GetEmptySts wrap_BCM_SCHED_GetEmptySts

#define USE_BCM
//#define DEBUG_BCM

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
	pbcmbuf->phy_addr = (void *) pbcmbuf->vpp_bcmbuf_mem_handle.p_addr;
	pbcmbuf->size = size;
	return 0;
}

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  0 - succeed
 *		  non-zero - failed to initialize a BCM buffer
 ****************************************************************/
int bcmbuf_destroy(BCMBUF *pbcmbuf, VPP_MEM_LIST *memlist)
{
	/* de-allocate memory for the buffer */
	if (!pbcmbuf->head)
		return (BCMBUF_EBADCALL);

	VPP_MEM_FreeMemory(memlist,
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

	if (pbcmbuf->writer == end) {
		/* the buffer is full, no space for wrap around */
		HRX_LOG(VIP_ERROR, "LCDC_BCMBUF_Write failed (0x%x 0x%x)\r\n", address, value);
		return BCMBUF_EBCMBUFFULL;
	}

#ifdef USE_BCM
	*pbcmbuf->writer = value;
	pbcmbuf->writer++;
	*pbcmbuf->writer = address;
	pbcmbuf->writer++;
#else
	glb_reg_write(address, value);
#endif

#ifdef DEBUG_BCM
	HRX_LOG(VIP_INFO, "%s: addr[%x] = %x\n", __func__, address, value);
#endif
	return BCMBUF_OK;
}

/*********************************************************
 * FUNCTION: write a block of data to BCM buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               *pdata - pointer to the data
 *               length - the length of the data to be written to BCM buffer
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int bcmbuf_write_block(BCMBUF *pbcmbuf, u32 *pdata, u32 length)
{
#ifdef USE_BCM
	u32 *end;

	end = pbcmbuf->tail;

	if (pbcmbuf->writer > end-(length >> 3)) {
		/*the buffer is full*/
		HRX_LOG(VIP_INFO, "Buffer is full\n");
		return BCMBUF_EBCMBUFFULL;
	}

	/*save the data to BCM buffer*/
	memcpy(pbcmbuf->writer, pdata, length);

	pbcmbuf->writer += (length >> 2);
#else
	u32 *ptr = pdata;
	int i = 0;
	int len = length/8;
	u32 address, value;

	for (i = 0; i < len; i++) {
		value = *ptr;
		ptr++;
		address = *ptr;
		ptr++;
		if (address != 0)
			glb_reg_write(address, value);
	}
#endif

#ifdef DEBUG_BCM
	HRX_LOG(VIP_INFO, "%s: length: %d\n", __func__, length);
	{
		int i = 0;
		unsigned int *ptr = pdata;

		for (i = 0; i < length; i += 8) {
			HRX_LOG(VIP_INFO, "%s: addr[%x] = %x\n", __func__, ptr[1], ptr[0]);
			ptr++;
			ptr++;
		}
	}
#endif

	return BCMBUF_OK;
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
	DHUB_CFGQ *cfgQ,
	VPP_MEM_LIST *memlist)
{
	unsigned int *start, *phy_start;
	int size, shm_offset;
	unsigned int bcm_sched_cmd[2];

	start = pbcmbuf->head;
	shm_offset = 0;
	phy_start = pbcmbuf->phy_addr;

	size = (int)((long long)pbcmbuf->writer-(long long)start);

	if (size <= 0)
		return BCMBUF_EBADPARAM;
	shm_offset = shm_offset * 4;

	VPP_MEM_FlushCache(memlist, &pbcmbuf->vpp_bcmbuf_mem_handle, 0);

#ifdef DEBUG_BCM
	{
		int i = 0;
		unsigned int *ptr = start;

		HRX_LOG(VIP_INFO, "%s: addrl[%llx] addrl[%x] size[%d]\n", __func__, (long long)phy_start, (int)(long long)phy_start, (int)size);
		for (i = 0; i < size; i += 8) {
			HRX_LOG(VIP_INFO, "%s: addr[%x] = %x\n", __func__, ptr[1], ptr[0]);
			ptr++;
			ptr++;
		}
	}
#endif
	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
							(int)(long long)phy_start,
							(int)size, 0, 0, 0, 1,
							bcm_sched_cmd);

	while (!BCM_SCHED_PushCmd(QID, bcm_sched_cmd, cfgQ->addr + cfgQ->len*2))
		;
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
	unsigned int bcm_sched_cmd[2];

	if (src_cfgQ->len <= 0)
		return;

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
							(int)(long long)src_cfgQ->phy_addr,
							(int)src_cfgQ->len*8, 0, 0, 0, 1,
							bcm_sched_cmd);

	while (!BCM_SCHED_PushCmd(QID, bcm_sched_cmd, cfgQ->addr + cfgQ->len*2))
		;
	cfgQ->len += 2;
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
	int intrType,
	bool *first_intr)
{
	unsigned int bcm_sched_cmd[2];
	int timeout = 20;
	u32 status;

#ifndef USE_BCM
	return BCMBUF_OK;
#endif

	if (cfgQ->len <= 0)
		return BCMBUF_EBADPARAM;

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID,
							(int)(long long)cfgQ->phy_addr,
							(int)cfgQ->len*8, 0, 0, 0, 1,
							bcm_sched_cmd);


	BCM_SCHED_GetEmptySts(BCM_SCHED_Q5, &status);
#ifdef DEBUG_VIP
	HRX_LOG(VIP_INFO, "BCM_SCHED_Q5 status: %d\n", status);
#endif
	if (status == 1) {
		while (!BCM_SCHED_PushCmd(sched_qid, bcm_sched_cmd, NULL) && (timeout-- > 0))
			msleep_interruptible(1);

		if (timeout == 0)
			HRX_LOG(VIP_INFO, "BCM_SCHED_Q5 BCM_SCHED_PushCmd timed out\n");
	} else {
		u32 bcmFlush;

		HRX_LOG(VIP_DEBUG, "hrx: BCM_SCHED_Q5 status: %d, flushing\n", status);
		bcmFlush = 1 << 5;
		glb_reg_write(BCM_REG_BASE + RA_AVIO_BCM_FLUSH, bcmFlush);
		msleep_interruptible(2);
		timeout = 20;
		while (!BCM_SCHED_PushCmd(sched_qid, bcm_sched_cmd, NULL) && (timeout-- > 0))
			msleep_interruptible(1);

		if (timeout == 0)
			HRX_LOG(VIP_INFO, "BCM_SCHED_Q5 BCM_SCHED_PushCmd timed out after flushing\n");
	}

	if (*first_intr == 1) {
		msleep_interruptible(10);
		BCM_SCHED_GetEmptySts(BCM_SCHED_Q12, &status);
		while (status != 1) {
			HRX_LOG(VIP_INFO, "BCM_SCHED_Q12 is not empty\n");
			msleep_interruptible(20);
			BCM_SCHED_GetEmptySts(BCM_SCHED_Q12, &status);
		}

		while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL))
			;
		*first_intr = 0;
	}

	return BCMBUF_OK;
}

int bcmbuf_Commit(HDL_dhub2d *pDhubHandle,
				int dhubID,
				unsigned int sched_qid,
				BCMBUF *pbcmbuf,
				int block,
				VPP_MEM_LIST *memlist)
{
	HDL_semaphore *pSemHandle;
	u32 *start;
	int status;
	int size;
	unsigned int bcm_sched_cmd[2];

	start = pbcmbuf->head;
	size = (int)(long long)pbcmbuf->writer-(int)(long long)start;

	if (size <= 0)
		return BCMBUF_EBADPARAM;

	VPP_MEM_FlushCache(memlist, &pbcmbuf->vpp_bcmbuf_mem_handle, 0);

	if (block) {
		pSemHandle = dhub_semaphore(&(pDhubHandle->dhub));
		/* clear possible BCM previous interrupt */
		semaphore_pop(pSemHandle, dhubID, 1);
		semaphore_clr_full(pSemHandle, dhubID);
	}

	dhub_channel_generate_cmd(&(pDhubHandle->dhub), dhubID, (int)(long long)start, (int)size, 0, 0, 0, 1,
		(int *) bcm_sched_cmd);
	while (!BCM_SCHED_PushCmd(BCM_SCHED_Q12, bcm_sched_cmd, NULL))
		;

	if (block) {
		/* check BCM interrupt */
		pSemHandle = dhub_semaphore(&(pDhubHandle->dhub));
		status = semaphore_chk_full(pSemHandle, dhubID);
		while (!status)
			status = semaphore_chk_full(pSemHandle, dhubID);
	}

	return BCMBUF_OK;
}

int create_bcmbuf_cfgq(DHUB_CFGQ *cfgQ, VPP_MEM *pvpp_mem,
							VPP_MEM_LIST *memlist)
{
	VPP_MEM_AllocateMemory(memlist, VPP_MEM_TYPE_DMA, pvpp_mem, 0);

	cfgQ->handle = (void *) pvpp_mem;
	cfgQ->addr = pvpp_mem->k_addr;
	cfgQ->phy_addr = (void *)pvpp_mem->p_addr;
	return 0;
}

void bcmbuf_DHUB_CFGQ_Reset(DHUB_CFGQ *cfgQ)
{
	if (cfgQ)
		cfgQ->len = 0;
}

/******************************************************************************************************************
 *   Function: dhub2nd_channel_cfg_vipBcm
 *   Description: Configurate a dHub2ND channel.
 *   Return:         u32                      -   Number of (adr,pair) added to cfgQ
 *****************************************************************************************************************
 */
u32  dhub2nd_channel_cfg_vip_bcm(
	HDL_dhub2d *hdl,               /*! Handle to HDL_dhub2d !*/
	s32 id,                 /*! Channel ID in $dHubReg2D !*/
	u32 addr,               /*! CMD: 2ND-buffer address !*/
	s32 burst,              /*! CMD: line stride size in bytes !*/
	s32 step1,              /*! CMD: buffer width in bytes !*/
	s32 size1,              /*! CMD: buffer height in lines !*/
	s32 step2,              /*! CMD: loop size (1~4) of semaphore operations !*/
	s32 size2,              /*! CMD: semaphore operation at CMD/MTU (0/1) !*/
	s32 chkSemId,           /*! CMD: semaphore loop pattern - non-zero to check !*/
	s32 updSemId,           /*! CMD: semaphore loop pattern - non-zero to update !*/
	s32 interrupt,          /*! CMD: raise interrupt at CMD finish !*/
	s32 enable,             /*! 0 to disable, 1 to enable !*/

	BCMBUF *pbcmbuf
		/*! Pass NULL to directly init dHub2ND, or
		 * Pass non-zero to receive programming sequence
		 * in (adr,data) pairs
		 */
	)
{
	HDL_dhub2d *dhub2d = (HDL_dhub2d *)hdl;
	SIE_dHubCmd2ND cmd;
	s32 semId_enable = 0;
	u32 a, j = 0;
	T32dHubChannel_ROB_MAP stdHubChannelRob_Map;

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);
	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_START, 0);

	stdHubChannelRob_Map.u32 = 0;
	if (updSemId != 0) {
		if (chkSemId & 0x1) /* Assuming all Luma channel will be odd and chroma will be even */
			stdHubChannelRob_Map.uROB_MAP_ID = 2;
		else
			stdHubChannelRob_Map.uROB_MAP_ID = 1;
	} else
		stdHubChannelRob_Map.uROB_MAP_ID = 0;

	a = dhub2d->ra + RA_dHubReg2D_dHub + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel) + RA_dHubChannel_ROB_MAP;
	bcmbuf_write(pbcmbuf, a, stdHubChannelRob_Map.u32);

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);
	cmd.uMEM_addr = addr;
	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_MEM, cmd.u32dHubCmd2ND_MEM);

	if (updSemId == 0)
		semId_enable = 0;
	else
		semId_enable = 1;

	cmd.u32dHubCmd2ND_DESC = 0;
	cmd.uDESC_burst = burst;
	cmd.uDESC_interrupt = interrupt;
	cmd.uDESC_chkSemId = chkSemId;
	cmd.uDESC_updSemId = updSemId;
	if (updSemId != 0) {
		cmd.uDESC_ovrdQos = 1;
		cmd.uDESC_disSem  = 1;
		cmd.uDESC_qosSel  = 1;
	} else {
		cmd.uDESC_ovrdQos = 0;
		cmd.uDESC_disSem  = 0;
		cmd.uDESC_qosSel  = 0;
	}
	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_DESC, cmd.u32dHubCmd2ND_DESC);

	cmd.uDESC_1D_ST_step = step1;
	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_DESC_1D_ST, cmd.u32dHubCmd2ND_DESC_1D_ST);
	cmd.uDESC_1D_SZ_size = size1;
	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_DESC_1D_SZ, cmd.u32dHubCmd2ND_DESC_1D_SZ);

	cmd.uDESC_2D_ST_step = step2;
	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_DESC_2D_ST, cmd.u32dHubCmd2ND_DESC_2D_ST);
	cmd.uDESC_2D_SZ_size = size2;
	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_DESC_2D_SZ, cmd.u32dHubCmd2ND_DESC_2D_SZ);

	bcmbuf_write(pbcmbuf, a + RA_dHubCmd2ND_START, enable);

	return j;
}
