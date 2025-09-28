// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include <linux/string.h>

#include "hrx-vip-framequeue.h"

#define WRAPAROUND(a)	(((a) >= MAX_NUM_VIP_FRAMES) ? 0 : (a))

/*************************************************
 * FUNCTION: reset frame queue to be empty state
 * PARAMS: *frmq - pionter to a frame queue
 *
 ************************************************/
void vip_frmq_reset(VIP_FRAMEQUEUE *frmq)
{
	/* reset read/write pointer of frame queue */
	/* the frame queue is empty when read pointer equals write pointer */
	frmq->head = 0;
	frmq->tail = 0;
	frmq->tail_shadow = 0;
	/* reset frame pointers to NULL in frame queue */
	memset(frmq->frame_descrs, 0, sizeof(void *) * MAX_NUM_VIP_FRAMES);

	mutex_init(&frmq->frmq_lock);
}

/*********************************************************
 * FUNCTION: check if frame queue is full
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - full
 *         0 - not full
 ********************************************************/
int vip_frmq_isfull(VIP_FRAMEQUEUE *frmq)
{
	/* first check whether frame queue is full */
	if (WRAPAROUND(frmq->tail_shadow+1) == frmq->head)
		return 1;
	else
		return 0;
}

/*********************************************************
 * FUNCTION: check if frame queue is empty
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - empty
 *         0 - not empty
 ********************************************************/
int vip_frmq_isempty(VIP_FRAMEQUEUE *frmq)
{
	if (frmq->tail == frmq->head)
		return 1;
	else
		return 0;
}

/*********************************************************
 * FUNCTION: check if frame queue is dirty, i.e. commit needs to be called
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - full
 *         0 - not full
 ********************************************************/
int vip_frmq_isdirty(VIP_FRAMEQUEUE *frmq)
{
	return (frmq->tail_shadow != frmq->tail);
}

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *frmq - pointer to frame queue
 *		 *frm_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *		 0 - fail: frame queue is full
 ********************************************************/
int vip_frmq_push(VIP_FRAMEQUEUE *frmq, void *frm_descr)
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
	frmq->tail_shadow = WRAPAROUND(frmq->tail+1);

	mutex_unlock(&frmq->frmq_lock);

	return 1;
}

/*********************************************************
 * FUNCTION: udpate write pointer with shadow write pointer
 * PARAMS: *frmq - pointer to frame queue
 ********************************************************/
void vip_frmq_push_commit(VIP_FRAMEQUEUE *frmq)
{
	/* update write pointer */
	frmq->tail = frmq->tail_shadow;
}

/*********************************************************
 * FUNCTION: udpate shadow write pointer with write pointer
 * PARAMS: *frmq - pointer to frame queue
 ********************************************************/
void vip_frmq_push_uncommit(VIP_FRAMEQUEUE *frmq)
{
	/* update write pointer */
	frmq->tail_shadow = frmq->tail;
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
int vip_frmq_pop(VIP_FRAMEQUEUE *frmq, void **frm_descr)
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
	frmq->head = WRAPAROUND(frmq->head+1);
	mutex_unlock(&frmq->frmq_lock);

	return 1;
}
