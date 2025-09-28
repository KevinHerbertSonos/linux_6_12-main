// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_VIP_FRAMEQUEUE_H
#define HRX_VIP_FRAMEQUEUE_H


#ifdef __cplusplus
extern "C" {
#endif

#include <linux/sched.h>
#include <linux/mutex.h>

#ifndef MAX_NUM_VIP_FRAMES
#define MAX_NUM_VIP_FRAMES	8
#endif

/* structure for frame descriptor queue */
typedef struct VIP_FRAMEQUEUE_T {
	void *frame_descrs[MAX_NUM_VIP_FRAMES];
	int head; // Read index of queue, update with pop_commit
	int tail_shadow; // Temporary write pointer of queue
	int tail; // write index of queue, update with push
	struct mutex frmq_lock;
} VIP_FRAMEQUEUE;

/*************************************************
 * FUNCTION: reset frame queue to be empty state
 * PARAMS: *frmq - pionter to a frame queue
 *
 ************************************************/
void vip_frmq_reset(VIP_FRAMEQUEUE *frmq);

/*********************************************************
 * FUNCTION: check if frame queue is full
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - full
 *         0 - not full
 ********************************************************/
int vip_frmq_isfull(VIP_FRAMEQUEUE *frmq);

/*********************************************************
 * FUNCTION: check if frame queue is empty
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - empty
 *         0 - not empty
 ********************************************************/
int vip_frmq_isempty(VIP_FRAMEQUEUE *frmq);

/*********************************************************
 * FUNCTION: check if frame queue is dirty, i.e. commit needs to be called
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - full
 *         0 - not full
 ********************************************************/
int vip_frmq_isdirty(VIP_FRAMEQUEUE *frmq);

/*********************************************************
 * FUNCTION: push a frame descriptor into frame queue
 * PARAMS: *frmq - pointer to frame queue
 *		 *frm_descr - pointer to a frame descriptor
 * RETURN: 1 - succeed
 *		 0 - fail: frame queue is full
 ********************************************************/
int vip_frmq_push(VIP_FRAMEQUEUE *frmq, void *frm_descr);

/*********************************************************
 * FUNCTION: udpate write pointer with shadow write pointer
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
void vip_frmq_push_commit(VIP_FRAMEQUEUE *frmq);

/*********************************************************
 * FUNCTION: udpate shadow write pointer with write pointer
 * PARAMS: *frmq - pointer to frame queue
 * RETURN: 1 - succeed
 *         0 - fail: frame queue is full
 ********************************************************/
void vip_frmq_push_uncommit(VIP_FRAMEQUEUE *frmq);

/******************************************************************
 * FUNCTION: pop a frame descriptor out of a frame queue
 *		   but actually without update head pointer.
 * PARAMS: *frmq - pointer to a frame queue
 *		 **frm_descr - pointer to the frame descriptor
 * RETURN: 1 - succeed
 *		 0 - command queue is empty, no command is available
 * NOTE: use pop_commit to actually update head pointer.
 *****************************************************************/
int vip_frmq_pop(VIP_FRAMEQUEUE *frmq, void **frm_descr);

#ifdef __cplusplus
}
#endif


#endif //HRX_VIP_FRAMEQUEUE_H
