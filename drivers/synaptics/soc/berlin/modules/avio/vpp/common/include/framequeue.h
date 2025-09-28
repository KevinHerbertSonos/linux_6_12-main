// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */

#ifndef _FRAMEQUEUE_H_
#define _FRAMEQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/sched.h>
#include <linux/mutex.h>

#ifndef MAX_NUM_FRAMES
#define MAX_NUM_FRAMES	4
#endif

/* structure for frame descriptor queue */
typedef struct FRAMEQUEUE_T{
	void *frame_descrs[MAX_NUM_FRAMES];
	int shadow_head;   // shadow read index of queue, update with pop
	int head;          // read index of queue, update with pop_commit
	int tail;          // write index of queue, update with push
	struct mutex frmq_lock;
} FRAMEQUEUE;

/* frame queue APIs */
/*************************************************
 * FUNCTION: reset frame queue to be empty state
 * PARAMS: *frmq - pionter to a frame queue
 *
 ************************************************/
void frmq_reset(FRAMEQUEUE *frmq);

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *frmq - pointer to frame queue
 *		 *frm_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *		 0 - fail: frame queue is full
 ********************************************************/
int frmq_push(FRAMEQUEUE *frmq, void *frm_descr);

/******************************************************************
 * FUNCTION: pop a frame descriptor out of a frame queue
 *		   but actually without update head pointer.
 * PARAMS: *frmq - pointer to a frame queue
 *		 **frm_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *		 0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int frmq_pop(FRAMEQUEUE *frmq, void **frm_descr);

/******************************************************************
 * FUNCTION: commit previous pop operation
 *		   by actually update head pointer.
 * PARAMS: *frmq - pointer to a frame queue
 * RETURN: 1 - succeed
 *		 0 - command queue is empty, no command is available
 *****************************************************************/
int frmq_pop_commit(FRAMEQUEUE *frmq);

#ifdef __cplusplus
}
#endif

#endif

