// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_VIP_BCMBUF_H
#define HRX_VIP_BCMBUF_H

// error code definitions
typedef enum {
	BCMBUF_OK           = 0x0000,
	BCMBUF_EBADPARAM    = 0x0001,
	BCMBUF_ENOMEM       = 0x0002,
	BCMBUF_EBADCALL     = 0x0003,
	BCMBUF_EBCMBUFFULL  = 0x0004,
} BCMBUF_ERROR;

typedef struct DHUB_CFGQ_T {
	void *handle;
	int shm_offset;
	int *addr;
	int len;
	unsigned int *phy_addr;
} DHUB_CFGQ;

/* Buffer descriptor contains pointers of:
 * - Entire buffer
 * - Sub-buffers
 */
typedef struct BCMBUF_T {
	/* Note: Ensure the following order matches the order of hal_dhub.c:VIP_BCMBUF
	 * 'writer' is updated inside hal_dhub.c:clear functions
	 */
	unsigned int *head;	// head of total BCM buffer
	unsigned int *tail;	// tail of the buffer, used for checking wrap around
	unsigned int *writer;	// write pointer of queue, update with shadow_tail with commit
	int size;	// size of total BCM buffer
	int subID;	// sub-buffer ID currently in use
	int *phy_addr;
	VPP_MEM vpp_bcmbuf_mem_handle;
} BCMBUF;

/******* register programming buffer APIs **************/
/***************************************************************
 * FUNCTION: allocate register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 *	   : size - size of the buffer to allocate
 *	   :	  - (should be a multiple of 4)
 * RETURN:  1 - succeed
 *		  0 - failed to initialize a BCM buffer
 ****************************************************************/
int bcmbuf_create(BCMBUF *pbcmbuf, int size, VPP_MEM_LIST *memlist);

/***************************************************************
 * FUNCTION: free register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *		  0 - failed to initialize a BCM buffer
 ****************************************************************/
int bcmbuf_destroy(BCMBUF *pbcmbuf, VPP_MEM_LIST *memlist);

/***************************************************************
 * FUNCTION: reset a register programming buffer
 * PARAMS: *buf - pointer to a register programming buffer
 * RETURN:  1 - succeed
 *		  0 - failed to initialize a BCM buffer
 ****************************************************************/
int bcmbuf_reset(BCMBUF *pbcmbuf);

/*********************************************************
 * FUNCTION: selest BCM sub-buffer to use
 * PARAMS: *buf - pointer to the buffer descriptor
 *		 subID - DV_1, DV_2, DV_3
 ********************************************************/
void bcmbuf_select(BCMBUF *pbcmbuf, int subID);

/*********************************************************
 * FUNCTION: write register address (4bytes) and value (4bytes) to the buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *			   address - address of the register to be set
 *			   value - the value to be written into the register
 * RETURN: 1 - succeed
 *			   0 - register programming buffer is full
 ********************************************************/
int bcmbuf_write(BCMBUF *pbcmbuf, unsigned int address, unsigned int value);

/*********************************************************
 * FUNCTION: write a block of data to BCM buffer
 * PARAMS: *buf - pointer to the buffer descriptor
 *               *pdata - pointer to the data
 *               length - the length of the data to be written to BCM buffer
 * RETURN: 1 - succeed
 *               0 - register programming buffer is full
 ********************************************************/
int bcmbuf_write_block(BCMBUF *pbcmbuf, u32 *pdata, u32 length);

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
	bool *first_intr);

int bcmbuf_Commit(HDL_dhub2d *pDhubHandle,
	int dhubID,
	unsigned int sched_qid,
	BCMBUF *pbcmbuf,
	int block,
	VPP_MEM_LIST *memlist);

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
	VPP_MEM_LIST *memlist);

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
	DHUB_CFGQ *cfgQ);

int create_bcmbuf_cfgq(DHUB_CFGQ *cfgQ, VPP_MEM *pvpp_mem,
							VPP_MEM_LIST *memlist);

void bcmbuf_DHUB_CFGQ_Reset(DHUB_CFGQ *cfgQ);
u32  dhub2nd_channel_cfg_vip_bcm(HDL_dhub2d *hdl, /*! Handle to HDL_dhub2d !*/
	s32 id, /*! Channel ID in $dHubReg2D !*/
	u32 addr,/*! CMD: 2ND-buffer address !*/
	s32 burst, /*! CMD: line stride size in bytes !*/
	s32 step1, /*! CMD: buffer width in bytes !*/
	s32 size1, /*! CMD: buffer height in lines !*/
	s32 step2, /*! CMD: loop size (1~4) of semaphore operations !*/
	s32 size2, /*! CMD: semaphore operation at CMD/MTU (0/1) !*/
	s32 chkSemId, /*! CMD: semaphore loop pattern - non-zero to check !*/
	s32 updSemId, /*! CMD: semaphore loop pattern - non-zero to update !*/
	s32 interrupt, /*! CMD: raise interrupt at CMD finish !*/
	s32 enable, /*! 0 to disable, 1 to enable !*/

	BCMBUF *pbcmbuf
	/*! Pass NULL to directly init dHub2ND, or
	 *Pass non-zero to receive programming sequence
	 *in (adr,data) pairs.
	 */
	);
#endif //HRX_VIP_BCMBUF_H
