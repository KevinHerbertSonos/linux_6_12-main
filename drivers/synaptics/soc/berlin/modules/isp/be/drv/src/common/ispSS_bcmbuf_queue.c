// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispSS_bcmbuf_queue.h"
#include "ispbe_err.h"
#include "ispSS_shm.h"
#include "ispSS_bcmbuf.h"
#include "ispSS_shm.h"

static INT ISPSS_BCMBUF_QUEUE_ShmAlloc(struct ISPSS_SHM_HANDLE *pShmHandle, int size)
{
	HRESULT iResult = ISPSS_OK;

	iResult = ispSS_SHM_Allocate(SHM_NONSECURE, size, 1024,
				&pShmHandle->hShm, SHM_NONSECURE_CONTIG);
	if (iResult != ISPSS_OK)
		goto e_ShmAlloc;

	pShmHandle->size = size;
	ispSS_SHM_GetVirtualAddress(pShmHandle->hShm, 0, &pShmHandle->addr);
	ispSS_SHM_GetPhysicalAddress(pShmHandle->hShm, 0, &pShmHandle->phy_addr);

e_ShmAlloc:
	return iResult;
}

static INT ISPSS_BCMBUF_QUEUE_Free(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue)
{
	int i;

	for (i = 0; i < pBcmBufQueue->queueCfg.maxQueueLen; i++) {
		struct ISPSS_BCMBUF_ITEM *pQueueItem = &(pBcmBufQueue->pQueueElements[i]);

		ispSS_SHM_Release(pQueueItem->bcmBufShmHandle.hShm);
		ispSS_SHM_Release(pQueueItem->cfgQShmHandle.hShm);
	}
	ispSS_SHM_Release(pBcmBufQueue->shmHandle.hShm);

	return ISPSS_OK;
}

static INT ISPSS_BCMBUF_QUEUE_Allocate(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue)
{
	HRESULT iResult = ISPSS_OK;
	int i, j;
	int size;

	size = sizeof(struct ISPSS_BCMBUF_ITEM) * pBcmBufQueue->queueCfg.maxQueueLen;
	iResult = ISPSS_BCMBUF_QUEUE_ShmAlloc(&pBcmBufQueue->shmHandle, size);
	if (iResult != ISPSS_OK)
		goto e_AllocateBcmQueue;

	pBcmBufQueue->pQueueElements = pBcmBufQueue->shmHandle.addr;
	memset(pBcmBufQueue->pQueueElements, 0, size);

	for (i = 0; i < pBcmBufQueue->queueCfg.maxQueueLen; i++) {
		struct ISPSS_BCMBUF_ITEM *pQueueItem = &(pBcmBufQueue->pQueueElements[i]);

		size = sizeof(struct BCMBUF) * pBcmBufQueue->queueCfg.maxBcmBuf;
		iResult = ISPSS_BCMBUF_QUEUE_ShmAlloc(&pQueueItem->bcmBufShmHandle, size);
		if (iResult != ISPSS_OK)
			goto e_AllocateBcmQueue1;

		pQueueItem->pBcmBuf = pQueueItem->bcmBufShmHandle.addr;
		memset(pQueueItem->pBcmBuf, 0, size);
	}

	for (i = 0; i < pBcmBufQueue->queueCfg.maxQueueLen; i++) {
		struct ISPSS_BCMBUF_ITEM *pQueueItem = &(pBcmBufQueue->pQueueElements[i]);

		size = sizeof(struct DHUB_CFGQ) * pBcmBufQueue->queueCfg.maxDhubCfgQ;
		iResult = ISPSS_BCMBUF_QUEUE_ShmAlloc(&pQueueItem->cfgQShmHandle, size);
		if (iResult != ISPSS_OK)
			goto e_AllocateBcmQueue2;

		pQueueItem->pCfgQ = pQueueItem->cfgQShmHandle.addr;
		memset(pQueueItem->pCfgQ, 0, size);
	}
	/* Initialize mutex here */
	mutex_init(&pBcmBufQueue->bcmbufQueue.isp_rqstq_lock);
	goto e_AllocateBcmQueue;

e_AllocateBcmQueue2:
	for (j = 0; j < i; j++) {
		struct ISPSS_BCMBUF_ITEM *pQueueItem = &(pBcmBufQueue->pQueueElements[i]);

		ispSS_SHM_Release(pQueueItem->cfgQShmHandle.hShm);
	}
	i = pBcmBufQueue->queueCfg.maxQueueLen;

e_AllocateBcmQueue1:
	for (j = 0; j < i; j++) {
		struct ISPSS_BCMBUF_ITEM *pQueueItem = &(pBcmBufQueue->pQueueElements[i]);

		ispSS_SHM_Release(pQueueItem->bcmBufShmHandle.hShm);
	}
	ispSS_SHM_Release(pBcmBufQueue->shmHandle.hShm);

e_AllocateBcmQueue:
	return iResult;
}

/***************************************************************
 * FUNCTION: allocate register programming buffer queue
 * PARAMS: *pBcmBufQueue - pointer to a queue to be created/initialized
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer queue
 ****************************************************************/
int ISPSS_BCMBUF_QUEUE_Create(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue)
{
	HRESULT iResult = ISPSS_OK;
	int i, j, k, l, maxL;
	struct ISPSS_BCMBUF_ITEM *pQueueItem = NULL;

	if (!pBcmBufQueue || (pBcmBufQueue->queueCfg.maxQueueLen <= 0) ||
			(pBcmBufQueue->queueCfg.bcmBufSize <= 0) ||
			(pBcmBufQueue->queueCfg.dmaCmdSize <= 0))
		return (ISPSS_EBADPARAM);

	//Allocate/create BCM Queue
	iResult = ISPSS_BCMBUF_QUEUE_Allocate(pBcmBufQueue);
	if (iResult != ISPSS_OK)
		goto eCreateBcmQueue;

	//Allocate/create BCM & CFGQ
	for (i = 0; i < pBcmBufQueue->queueCfg.maxQueueLen; i++) {
		pQueueItem = &(pBcmBufQueue->pQueueElements[i]);

		for (j = 0; j < pBcmBufQueue->queueCfg.maxBcmBuf; j++) {
			iResult = ISPSS_BCMBUF_Create(&pQueueItem->pBcmBuf[j],
						pBcmBufQueue->queueCfg.bcmBufSize);
			if (iResult != ISPSS_OK)
				goto eCreateBcmQueue1;

			ISPSS_BCMBUF_Reset(&pQueueItem->pBcmBuf[j]);
			ISPSS_BCMBUF_Select(&pQueueItem->pBcmBuf[j], 0);
		}
	}

	for (i = 0; i < pBcmBufQueue->queueCfg.maxQueueLen; i++) {
		pQueueItem = &(pBcmBufQueue->pQueueElements[i]);
		for (j = 0; j < pBcmBufQueue->queueCfg.maxDhubCfgQ; j++) {
			iResult = ISPSS_CFGQ_Create(&pQueueItem->pCfgQ[j],
						pBcmBufQueue->queueCfg.dmaCmdSize);
			if (iResult != ISPSS_OK)
				goto eCreateBcmQueue2;

			pQueueItem->pCfgQ[j].len = 0;
		}
	}

	//Push all the items to the queue
	for (i = 0; i < pBcmBufQueue->queueCfg.maxQueueLen; i++) {
		pQueueItem = &(pBcmBufQueue->pQueueElements[i]);
		ISPSS_BCMBUF_QUEUE_push(pBcmBufQueue, pQueueItem);
	}

	goto eCreateBcmQueue;


eCreateBcmQueue2:
	for (k = 0; k < i; k++) {
		maxL = (k == (i-1)) ? j : pBcmBufQueue->queueCfg.maxDhubCfgQ;
		pQueueItem = &(pBcmBufQueue->pQueueElements[k]);
		for (l = 0; l < maxL; l++)
			ISPSS_CFGQ_Destroy(&pQueueItem->pCfgQ[l]);
	}

	i = pBcmBufQueue->queueCfg.maxQueueLen;
	j = pBcmBufQueue->queueCfg.maxBcmBuf;
eCreateBcmQueue1:
	for (k = 0; k < i; k++) {
		maxL = (k == (i-1)) ? j : pBcmBufQueue->queueCfg.maxBcmBuf;
		pQueueItem = &(pBcmBufQueue->pQueueElements[k]);
		for (l = 0; l < maxL; l++)
			ISPSS_BCMBUF_Destroy(&pQueueItem->pBcmBuf[l]);
	}
	ISPSS_BCMBUF_QUEUE_Free(pBcmBufQueue);
eCreateBcmQueue:

	return iResult;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_QUEUE_Create);

/***************************************************************
 * FUNCTION: free register programming buffer queue
 * PARAMS: *pBcmBufQueue - pointer to a queue to be freed
 * RETURN:  1 - succeed
 *          0 - failed to free a BCM buffer queue
 ****************************************************************/
int ISPSS_BCMBUF_QUEUE_Destroy(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue)
{
	int i, j;

	for (i = 0; i < pBcmBufQueue->queueCfg.maxQueueLen; i++) {
		struct ISPSS_BCMBUF_ITEM *pQueueItem = &(pBcmBufQueue->pQueueElements[i]);

		for (j = 0; j < pBcmBufQueue->queueCfg.maxDhubCfgQ; j++)
			ISPSS_CFGQ_Destroy(&pQueueItem->pCfgQ[j]);
		for (j = 0; j < pBcmBufQueue->queueCfg.maxBcmBuf; j++)
			ISPSS_BCMBUF_Destroy(&pQueueItem->pBcmBuf[j]);
	}

	ISPSS_BCMBUF_QUEUE_Free(pBcmBufQueue);

	return ISPSS_OK;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_QUEUE_Destroy);

/*************************************************
 * FUNCTION: reset bcmbuf queue to be empty state
 * PARAMS: *pBcmBufQueue - pionter to a bcmbuf queue
 * RETURN:  1 - succeed
 *          0 - failed to initialize a BCM buffer
 ****************************************************************/
void ISPSS_BCMBUF_QUEUE_Reset(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue)
{
	isp_rqstq_reset(&(pBcmBufQueue->bcmbufQueue));
}
EXPORT_SYMBOL(ISPSS_BCMBUF_QUEUE_Reset);

/*********************************************************
 * FUNCTION: push a bcmbuf into bcmbuf queue
 * PARAMS: *pBcmBufQueue - pointer to bcmbuf queue
 *         *pBcmBufQueueElement - pointer to a bcmbuf or ISPSS_BCMBUF_ITEM
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
int ISPSS_BCMBUF_QUEUE_push(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue,
					struct ISPSS_BCMBUF_ITEM *pBcmBufQueueElement)
{
	HRESULT iResult;

	iResult = isp_rqstq_push(&(pBcmBufQueue->bcmbufQueue), pBcmBufQueueElement);

	return iResult;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_QUEUE_push);

/******************************************************************
 * FUNCTION: pop a bcmbuf out of a bcmbuf queue
 *           but actually without update head pointer.
 * PARAMS: *pBcmBufQueue - pointer to a bcmbuf queue
 *         **pBcmBufQueueElement - pointer to the bcmbuf
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
static int ISPSS_BCMBUF_QUEUE_pop(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue,
		void **pBcmBufQueueElement)
{
	HRESULT iResult;

	iResult = isp_rqstq_pop(&(pBcmBufQueue->bcmbufQueue), (void **)pBcmBufQueueElement);

	if ((iResult) && pBcmBufQueue->queueCfg.autoPushOnPopFlag)
		iResult = isp_rqstq_push_shadow(&(pBcmBufQueue->bcmbufQueue), *pBcmBufQueueElement);
	return iResult;
}

/******************************************************************
 * FUNCTION: commit previous pop operation
 *           by actually update head pointer.
 * PARAMS: *pBcmBufQueue - pointer to a bcmbuf queue
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
static int ISPSS_BCMBUF_QUEUE_pop_commit(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue)
{
	HRESULT iResult;

	iResult = isp_rqstq_pop_commit(&(pBcmBufQueue->bcmbufQueue));

	if ((iResult) && pBcmBufQueue->queueCfg.autoPushOnPopFlag)
		iResult = isp_rqstq_push_shadow_commit(&(pBcmBufQueue->bcmbufQueue));

	return iResult;
}

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
							void **pBcmBufQueueElement)
{
	HRESULT iResult;

	iResult = ISPSS_BCMBUF_QUEUE_pop(pBcmBufQueue, pBcmBufQueueElement);
	if (iResult)
		iResult = ISPSS_BCMBUF_QUEUE_pop_commit(pBcmBufQueue);

	return iResult;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_QUEUE_pop_and_commit);

/*********************************************************
 * FUNCTION: selest BCM sub-buffer to use
 * PARAMS: *pBcmBufQueue - pointer to bcmbuf queue
 *         *pBcmBufQueueElement - pointer to a bcmbuf or ISPSS_BCMBUF_ITEM
 *         subID - DV_1, DV_2, DV_3
 ********************************************************/
void ISPSS_BCMBUF_QUEUE_Element_Select(struct ISPSS_BCMBUF_QUEUE *pBcmBufQueue,
		struct ISPSS_BCMBUF_ITEM *pBcmBufQueueElement, int subID)
{
	int i;

	for (i = 0; i < pBcmBufQueue->queueCfg.maxBcmBuf; i++) {
		ISPSS_BCMBUF_Reset(&pBcmBufQueueElement->pBcmBuf[i]);
		ISPSS_BCMBUF_Select(&pBcmBufQueueElement->pBcmBuf[i], subID);
	}

	for (i = 0; i < pBcmBufQueue->queueCfg.maxDhubCfgQ; i++)
		pBcmBufQueueElement->pCfgQ[i].len = 0;
}
EXPORT_SYMBOL(ISPSS_BCMBUF_QUEUE_Element_Select);
