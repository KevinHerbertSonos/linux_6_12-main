/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __BCMBUF_QUEUE_H__
#define __BCMBUF_QUEUE_H__

#include "ispSS_common.h"
#include "ispSS_rqstq.h"
#include "ispSS_shm.h"

enum ISPSS_BCMBUF_ITEM_TYPE {
	ISPSS_BCMBUF_ITEM_CLEAR_BCM_NDX,
	ISPSS_BCMBUF_ITEM_PROGRAM_BCM_NDX,
	ISPSS_BCMBUF_ITEM_MAX
};

enum ISPSS_CFGQ_ITEM_TYPE {
	ISPSS_CFGQ_ITEM_DHUB_CFGQ_NDX,
	ISPSS_CFGQ_ITEM_FINAL_CGQ_NDX,
	ISPSS_CFGQ_ITEM_MAX
};

struct ISPSS_SHM_HANDLE {
	SHM_HANDLE  hShm;
	void *addr, *phy_addr;
	int size;
};

struct ISPSS_BCMBUF_QUEUE_CFG {
	// Max numbe of BCMBUF item - 2 for ping-pong implementation
	int maxQueueLen;

	// Configure bcmbuf - number of bcmbuf and its size
	int maxBcmBuf;
	int bcmBufSize;

	// Configure cfgq - number of cfgq and its size
	int maxDhubCfgQ;
	int dmaCmdSize;

	//auto push the pop'ed bcmbuf
	int autoPushOnPopFlag;
};

struct ISPSS_BCMBUF_ITEM {
	struct ISPSS_SHM_HANDLE bcmBufShmHandle;
	struct BCMBUF *pBcmBuf;

	struct ISPSS_SHM_HANDLE cfgQShmHandle;
	struct DHUB_CFGQ *pCfgQ;
};

struct ISPSS_BCMBUF_QUEUE {
	struct ISPSS_BCMBUF_QUEUE_CFG queueCfg;

	struct ISPSS_SHM_HANDLE shmHandle;
	struct ISPSS_BCMBUF_ITEM *pQueueElements;

	struct ISP_REQUEST_QUEUE bcmbufQueue;
};

/***************************************************************
 * FUNCTION: allocate register programming buffer queue
 * PARAMS: *pQueue - pointer to a queue to be created/initialized
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer queue
 ****************************************************************/
int ISPSS_BCMBUF_QUEUE_Create(struct ISPSS_BCMBUF_QUEUE *pQueue);

/***************************************************************
 * FUNCTION: free register programming buffer queue
 * PARAMS: *pQueue - pointer to a queue to be freed
 * RETURN:  1 - succeed
 *          0 - failed to free a BCM buffer queue
 ****************************************************************/
int ISPSS_BCMBUF_QUEUE_Destroy(struct ISPSS_BCMBUF_QUEUE *pQueue);


/*************************************************
 * FUNCTION: reset bcmbuf queue to be empty state
 * PARAMS: *pBcmBufQueue - pionter to a bcmbuf queue
 * RETURN:
 ****************************************************************/
void ISPSS_BCMBUF_QUEUE_Reset(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue);

/*********************************************************
 * FUNCTION: push a bcmbuf into bcmbuf queue
 * PARAMS: *pBcmBufQueue - pointer to bcmbuf queue
 *         *pBcmBufQueueElement - pointer to a bcmbuf or ISPSS_BCMBUF_ITEM
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
int ISPSS_BCMBUF_QUEUE_push(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue,
		struct ISPSS_BCMBUF_ITEM *pBcmBufQueueElement);

/******************************************************************
 * FUNCTION: pop a bcmbuf out of a bcmbuf queue
 *           and also update head pointer.
 * PARAMS: *pBcmBufQueue - pointer to a bcmbuf queue
 *         **pBcmBufQueueElement - pointer to the bcmbuf
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int ISPSS_BCMBUF_QUEUE_pop_and_commit(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue,
		void **pBcmBufQueueElement);

/*********************************************************
 * FUNCTION: selest BCM sub-buffer to use
 * PARAMS: *pBcmBufQueue - pointer to bcmbuf queue
 *         *pBcmBufQueueElement - pointer to a bcmbuf or ISPSS_BCMBUF_ITEM
 *         subID - DV_1, DV_2, DV_3
 ********************************************************/
void ISPSS_BCMBUF_QUEUE_Element_Select(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue,
		struct ISPSS_BCMBUF_ITEM *pBcmBufQueueElement, int subID);

#endif
