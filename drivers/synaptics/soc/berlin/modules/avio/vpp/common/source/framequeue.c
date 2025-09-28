// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */

#include <linux/string.h>

#include "framequeue.h"

#define WRAPAROUND(a)	(((a) >= MAX_NUM_FRAMES) ? 0 : (a))

/*************************************************
 * FUNCTION: reset frame queue to be empty state
 * PARAMS: *frmq - pionter to a frame queue
 *
 ************************************************/
void frmq_reset(FRAMEQUEUE *frmq)
{
	/* reset read/write pointer of frame queue */
	/* the frame queue is empty when read pointer equals write pointer */
	frmq->shadow_head = 0;
	frmq->head = 0;
	frmq->tail = 0;

	/* reset frame pointers to NULL in frame queue */
	memset(frmq->frame_descrs, 0, sizeof(void *) * MAX_NUM_FRAMES);

	mutex_init(&frmq->frmq_lock);
	return;
}


/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *frmq - pointer to frame queue
 *		 *frm_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *		 0 - fail: frame queue is full
 ********************************************************/
int frmq_push(FRAMEQUEUE *frmq, void *frm_descr)
{
	mutex_lock(&frmq->frmq_lock);

	/* first check whether frame queue is full */
	if (WRAPAROUND(frmq->tail+1) == frmq->head) {
		mutex_unlock(&frmq->frmq_lock);
		return 0;
	}

	/* push frame descriptor into frame queue if not full */
	frmq->frame_descrs[frmq->tail] = frm_descr;

	/* update shadow write pointer */
	frmq->tail = WRAPAROUND(frmq->tail + 1);

	mutex_unlock(&frmq->frmq_lock);

	return 1;
}

/******************************************************************
 * FUNCTION: pop a frame descriptor out of a frame queue
 *		   but actually without update head pointer.
 * PARAMS: *frmq - pointer to a frame queue
 *		 **frm_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *		 0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int frmq_pop(FRAMEQUEUE *frmq, void **frm_descr)
{
	mutex_lock(&frmq->frmq_lock);

	/* first check whether frame queue is empty */
	if (frmq->head == frmq->tail) {
		mutex_unlock(&frmq->frmq_lock);
		return 0;
	}

	/* pop a frame descriptor from frame queue if not empty */
	*frm_descr = frmq->frame_descrs[frmq->head];

	/* update shadow read pointer */
	frmq->shadow_head = WRAPAROUND(frmq->head + 1);
	mutex_unlock(&frmq->frmq_lock);

	return 1;
}

/******************************************************************
 * FUNCTION: commit previous pop operation
 *		   by actually update head pointer.
 * PARAMS: *frmq - pointer to a frame queue
 * RETURN: 1 - succeed
 *		 0 - command queue is empty, no command is available
 *****************************************************************/
int frmq_pop_commit(FRAMEQUEUE *frmq)
{
	mutex_lock(&frmq->frmq_lock);
	/* update shadow read pointer */
	frmq->head = frmq->shadow_head;
	mutex_unlock(&frmq->frmq_lock);

	return 1;
}