/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_RQSTQ_H_
#define _ISPSS_RQSTQ_H_

#include "com_type.h"

#include <linux/mutex.h>

#define ISP_RQSTQ_LOCK_ENABLE

/* maximum number of frames in frame buffer queue */
#ifdef MAX_NUM_FRAMES
#error "duplicated MACRO definition of MAX_NUM_FRAMES"
#else
#define MAX_NUM_FRAMES    20
#endif

/* BCM buffer size */
#ifdef BCM_BUFFER_SIZE
#error "duplicated MACRO definition of BCM_BUFFER_SIZE"
#else
/* DV1_BCM_BUFFER_SIZE + DV2_BCM_BUFFER_SIZE + DV3_BCM_BUFFER_SIZE = BCM_BUFFER_SIZE */
#define BCM_BUFFER_SIZE   0x17700//96000
#define DV1_BCM_BUFFER_SIZE   0x7d00//32000
#define DV2_BCM_BUFFER_SIZE   0x7d00//32000
#define DV3_BCM_BUFFER_SIZE   0x7d00//32000
#define VDE_BCM_BUFFER_SIZE   0x800
#endif

/* DMA command buffer size */
#ifdef DMA_CMD_BUFFER_SIZE
#error "duplicated MACRO definition of DMA_CMD_BUFFER_SIZE"
#else
#define DMA_CMD_BUFFER_SIZE   (100 * 8)
#endif

/* structure for frame descriptor queue */
struct ISP_REQUEST_QUEUE {
	void *request_descrs[MAX_NUM_FRAMES];
	int shadow_head;    // shadow read pointer of queue, update with pop
	int shadow_tail;    // shadow read pointer of queue, update with push_shadow
	int head;           // read pointer of queue, update with pop_commit
	int tail;           // write pointer of queue, update with push & push_commit
	int count;
	struct mutex isp_rqstq_lock;
};

/* frame queue APIs */
/*************************************************
 * FUNCTION: reset frame queue to be empty state
 * PARAMS: *request_queue - pionter to a frame queue
 *
 ************************************************/
void isp_rqstq_reset(struct ISP_REQUEST_QUEUE *request_queue);

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *request_queue - pointer to frame queue
 *         *request_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
int isp_rqstq_push(struct ISP_REQUEST_QUEUE *request_queue, void *request_descr);

/******************************************************************
 * FUNCTION: pop a frame descriptor out of a frame queue
 *           but actually without update head pointer.
 * PARAMS: *request_queue - pointer to a frame queue
 *         **request_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int isp_rqstq_pop(struct ISP_REQUEST_QUEUE *request_queue, void **request_descr);

/******************************************************************
 * FUNCTION: commit previous pop operation
 *           by actually update head pointer.
 * PARAMS: *request_queue - pointer to a frame queue
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
int isp_rqstq_pop_commit(struct ISP_REQUEST_QUEUE *request_queue);

/******************************************************************
 * FUNCTION: Get the number of request pending in the queue
 * PARAMS: *request_queue - pointer to a frame queue
 * RETURN: number of request pending in the queue
 *****************************************************************/
int isp_rqstq_get_count(struct ISP_REQUEST_QUEUE *request_queue);

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *request_queue - pointer to frame queue
 *         *request_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
int isp_rqstq_push_shadow(struct ISP_REQUEST_QUEUE *request_queue, void *request_descr);

/******************************************************************
 * FUNCTION: commit previous push_shadow operation
 *           by actually update tail pointer.
 * PARAMS: *request_queue - pointer to a frame queue
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 *****************************************************************/
int isp_rqstq_push_shadow_commit(struct ISP_REQUEST_QUEUE *request_queue);

/******************************************************************
 * FUNCTION: retrieve uncommitted frame descriptor pushed into frame queue
 *           i.e. read from the tail instead of head
 * PARAMS: *request_queue - pointer to a frame queue
 *         **request_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *         0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int isp_rqstq_pop_shadow(struct ISP_REQUEST_QUEUE *request_queue, void **request_descr);

#endif
