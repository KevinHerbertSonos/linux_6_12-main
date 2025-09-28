// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ispSS_rqstq.h"

#define WRAPAROUND(a)    (((a) >= MAX_NUM_FRAMES)?0:(a))


/*************************************************
 * FUNCTION: reset frame queue to be empty state
 * PARAMS: *request_queue - pionter to a frame queue
 *
 ************************************************/
void isp_rqstq_reset(struct ISP_REQUEST_QUEUE *request_queue)
{
	/* reset read/write pointer of frame queue */
	/* the frame queue is empty when read pointer equals write pointer */
	request_queue->shadow_head = 0;
	request_queue->shadow_tail = 0;
	request_queue->head = 0;
	request_queue->tail = 0;
	request_queue->count = 0;

	/* reset frame pointers to NULL in frame queue */
	GaloisMemSet(request_queue->request_descrs, 0, sizeof(void *)*MAX_NUM_FRAMES);
}
EXPORT_SYMBOL(isp_rqstq_reset);

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *request_queue - pointer to frame queue
 *         *request_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
int isp_rqstq_push(struct ISP_REQUEST_QUEUE *request_queue, void *request_descr)
{
	/* first check whether frame queue is full */
	if (WRAPAROUND(request_queue->tail+1) == request_queue->head)
		return 0;

	/* push frame descriptor into frame queue if not full */
	request_queue->request_descrs[request_queue->tail] = request_descr;

#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_lock(&request_queue->isp_rqstq_lock);
#endif

	/* update shadow write pointer */
	request_queue->tail = WRAPAROUND(request_queue->tail+1);
	request_queue->count++;

#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_unlock(&request_queue->isp_rqstq_lock);
#endif

	return 1;
}
EXPORT_SYMBOL(isp_rqstq_push);

/******************************************************************
 * FUNCTION: pop a frame descriptor out of a frame queue
 *           but actually without update head pointer.
 * PARAMS: *request_queue - pointer to a frame queue
 *         **request_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int isp_rqstq_pop(struct ISP_REQUEST_QUEUE *request_queue, void **request_descr)
{
	/* first check whether frame queue is empty */
	if (request_queue->head == request_queue->tail)
		return 0;

	/* pop a frame descriptor from frame queue if not empty */
	*request_descr = request_queue->request_descrs[request_queue->head];

	/* update shadow read pointer */
	request_queue->shadow_head = WRAPAROUND(request_queue->head+1);

	return 1;
}
EXPORT_SYMBOL(isp_rqstq_pop);

/******************************************************************
 * FUNCTION: commit previous pop operation
 *           by actually update head pointer.
 * PARAMS: *request_queue - pointer to a frame queue
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
int isp_rqstq_pop_commit(struct ISP_REQUEST_QUEUE *request_queue)
{
#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_lock(&request_queue->isp_rqstq_lock);
#endif

	/* update shadow read pointer */
	if (request_queue->head != request_queue->shadow_head) {
		request_queue->head = request_queue->shadow_head;
		request_queue->count--;
	}

#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_unlock(&request_queue->isp_rqstq_lock);
#endif

	return 1;
}
EXPORT_SYMBOL(isp_rqstq_pop_commit);

/******************************************************************
 * FUNCTION: Get the number of request pending in the queue
 * PARAMS: *request_queue - pointer to a frame queue
 * RETURN: number of request pending in the queue
 *****************************************************************/
int isp_rqstq_get_count(struct ISP_REQUEST_QUEUE *request_queue)
{
	return request_queue->count;
}
EXPORT_SYMBOL(isp_rqstq_get_count);

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *request_queue - pointer to frame queue
 *         *request_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
int isp_rqstq_push_shadow(struct ISP_REQUEST_QUEUE *request_queue, void *request_descr)
{
	/* first check whether frame queue is full */
	if (WRAPAROUND(request_queue->tail+1) == request_queue->head)
		return 0;

	/* push frame descriptor into frame queue if not full */
	request_queue->request_descrs[request_queue->tail] = request_descr;

#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_lock(&request_queue->isp_rqstq_lock);
#endif

	/* update shadow write pointer */
	request_queue->shadow_tail = WRAPAROUND(request_queue->tail+1);

#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_unlock(&request_queue->isp_rqstq_lock);
#endif

	return 1;
}
EXPORT_SYMBOL(isp_rqstq_push_shadow);

/******************************************************************
 * FUNCTION: commit previous push shadow operation
 *           by actually update tail pointer.
 * PARAMS: *request_queue - pointer to a frame queue
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
int isp_rqstq_push_shadow_commit(struct ISP_REQUEST_QUEUE *request_queue)
{
#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_lock(&request_queue->isp_rqstq_lock);
#endif

	/* update shadow read pointer */
	if (request_queue->tail != request_queue->shadow_tail) {
		request_queue->tail = request_queue->shadow_tail;
		request_queue->count++;
	}

#ifdef ISP_RQSTQ_LOCK_ENABLE
	mutex_unlock(&request_queue->isp_rqstq_lock);
#endif

	return 1;
}
EXPORT_SYMBOL(isp_rqstq_push_shadow_commit);

/******************************************************************
 * FUNCTION: retrieve uncommitted frame descriptor pushed into frame queue
 *           i.e. read from the tail instead of head
 * PARAMS: *request_queue - pointer to a frame queue
 *         **request_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int isp_rqstq_pop_shadow(struct ISP_REQUEST_QUEUE *request_queue, void **request_descr)
{
	/* first check whether frame is temporariy pushed*/
	if (request_queue->tail == request_queue->shadow_tail)
		return 0;

	/* pop a frame descriptor from frame queue if some uncommitted frame is pushed*/
	*request_descr = request_queue->request_descrs[request_queue->tail];

	return 1;
}
EXPORT_SYMBOL(isp_rqstq_pop_shadow);
