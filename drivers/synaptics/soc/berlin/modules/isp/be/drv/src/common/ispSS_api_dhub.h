/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ISPSS_API_DHUB_H__
#define __ISPSS_API_DHUB_H__

#include "com_type.h"
#include "ispDhub.h"


void BCM_SCHED_GetFullSts(UNSG32 QID, UNSG32 *FullSts);
void BCM_SCHED_Flush(UNSG32 mask);

struct HDL_semaphore {
	UNSG32			ra;			/*!	Base address of $SemaHub !*/
	SIGN32			depth[32];	/*!	Array of semaphore (virtual FIFO) depth !*/
};

struct HDL_hbo {
	UNSG32			mem;		/*!	Base address of HBO SRAM !*/
	UNSG32			ra;			/*!	Base address of $HBO !*/
	struct HDL_semaphore	fifoCtl;	/*!	Handle of HBO.FiFoCtl !*/
	UNSG32			base[32];	/*!	Array of HBO queue base address !*/
};

struct HDL_dhub {
	UNSG32			ra;			/*!	Base address of $dHub !*/
	struct HDL_semaphore	semaHub;	/*!	Handle of dHub.SemaHub !*/
	struct HDL_hbo hbo; /*! Handle of dHub.HBO !*/
	SIGN32			MTUb[16];	/*!	Array of dHub channel MTU size bits !*/
};

struct HDL_dhub2d {
	UNSG32			ra;			/*!	Base address of $dHub2D !*/
	struct HDL_dhub dhub; /*! Handle of dHub2D.dHub !*/
};

/****************************************************************************
 * Function: semaphore_hdl
 * Description: Initialize HDL_semaphore with a $SemaHub BIU instance.
 * UNSG32		ra,	*! Base address of a BIU instance of $SemaHub !*
 * void		*hdl	! Handle to HDL_semaphore !*
 * ***************************************************************************
 */
void semaphore_hdl(
				UNSG32		ra,
				void		*hdl
			);

/*******************************************************************************
 * Function: semaphore_cfg
 * Description: Configurate a semaphore's depth & reset pointers.
 * void		*hdl,		*!	Handle to HDL_semaphore !*
 * SIGN32		id,		!	Semaphore ID in $SemaHub !*
 * SIGN32		depth,	*!	Semaphore (virtual FIFO) depth !*
 * T64b		cfgQ[]		*!	Pass NULL to directly init SemaHub, or
 * Pass non-zero to receive programming sequence in (adr,data) pairs
 * Return:			UNSG32	-	Number of (adr,pair) added to cfgQ
 * ******************************************************************************
 */
UNSG32	semaphore_cfg(
		void		*hdl,
		SIGN32		id,
		SIGN32		depth,
		T64b		cfgQ[]);

/***********************************************************************************
 * Function: semaphore_intr_enable
 * Description: Configurate interrupt enable bits of a semaphore.
 * void		*hdl,			*!	Handle to HDL_semaphore !*
 * SIGN32		id,			*!	Semaphore ID in $SemaHub !*
 * SIGN32		empty,		*!	Interrupt enable for CPU at condition 'empty' !
 * SIGN32		full,		*!	Interrupt enable for CPU at condition 'full' !*
 * SIGN32		almostEmpty,*!	Interrupt enable for CPU at condition 'almostEmpty' !
 * SIGN32		almostFull,*!Interrupt enable for CPU at condition 'almostFull' !*
 * SIGN32		cpu			*!	CPU ID (0/1/2) !*
 * **********************************************************************************
 */
void	semaphore_intr_enable(
		void		*hdl,
		SIGN32		id,
		SIGN32		empty,
		SIGN32		full,
		SIGN32		almostEmpty,
		SIGN32		almostFull,
		SIGN32		cpu
		);

/************************************************************************************
 * Function: semaphore_query
 * Description: Query current status (counter & pointer) of a semaphore.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * SIGN32		master,		*!	0/1 as procuder/consumer query !*
 * UNSG32		*ptr		*!	Non-zero to receive semaphore r/w pointer !*
 * Return:			UNSG32		Current available unit level
 * ***********************************************************************************
 */
UNSG32	semaphore_query(
		void		*hdl,
		SIGN32		id,
		SIGN32		master,
		UNSG32		*ptr
		);

/*************************************************************************************
 * Function: semaphore_push
 * Description: Producer semaphore push.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * SIGN32		delta				*!	Delta to pop as a producer !*
 * ************************************************************************************
 */
void	semaphore_push(
		void		*hdl,
		SIGN32		id,
		SIGN32		delta
		);

/*************************************************************************************
 * Function: semaphore_push
 * Description: Consumer semaphore pop.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * SIGN32		delta				*!	Delta to pop as a consumer !*
 * ************************************************************************************
 */
void	semaphore_pop(
		void		*hdl,
		SIGN32		id,
		SIGN32		delta
		);

/**************************************************************************************
 * Function: semaphore_chk_empty
 * Description: Check 'empty' status of a semaphore (or all semaphores).
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * Return:			UNSG32	-	status bit of given semaphore,
 * status bits of all semaphores if id==-1
 * *************************************************************************************
 */
UNSG32	semaphore_chk_empty(
		void		*hdl,
		SIGN32		id
		);

/****************************************************************************************
 * Function: ispss_semaphore_chk_full
 * Description: Check 'full' status of a semaphore (or all semaphores).
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * Return:			UNSG32		status bit of given semaphore, or
 *			status bits of all semaphores if id==-1
 *
 ******************************************************************************************
 */
UNSG32	ispss_semaphore_chk_full(
		void		*hdl,
		SIGN32		id
		);

/******************************************************************************************
 * Function: semaphore_chk_almostEmpty
 * Description: Check 'almostEmpty' status of a semaphore (or all semaphores).
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * Return:			UNSG32	-	status bit of given semaphore, or
 *			status bits of all semaphores if id==-1
 ******************************************************************************************
 */
UNSG32	semaphore_chk_almostEmpty(
		void		*hdl,
		SIGN32		id
		);

/*******************************************************************************************
 * Function: semaphore_chk_almostFull
 * Description: Check 'almostFull' status of a semaphore (or all semaphores).
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * Return:			UNSG32		status bit of given semaphore, or
 *			status bits of all semaphores if id==-1
 *******************************************************************************************
 */
UNSG32	semaphore_chk_almostFull(
		void		*hdl,
		SIGN32		id
		);

/******************************************************************************************
 * Function: semaphore_clr_empty
 * Description: Clear 'empty' status of a semaphore.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * *****************************************************************************************
 */
void	semaphore_clr_empty(
		void		*hdl,
		SIGN32		id
		);

/******************************************************************************************
 * Function: semaphore_clr_full
 * Description: Clear 'full' status of a semaphore.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * *****************************************************************************************
 */
void	semaphore_clr_full(
		void		*hdl,
		SIGN32		id
		);

/*****************************************************************************************
 * Function: semaphore_clr_almostEmpty
 * Description: Clear 'almostEmpty' status of a semaphore
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * ****************************************************************************************
 */
void	semaphore_clr_almostEmpty(
		void		*hdl,
		SIGN32		id
		);

/*****************************************************************************************
 * Function: semaphore_clr_almostFull
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * Description: Clear 'almostFull' status of a semaphore.
 * ****************************************************************************************
 */
void	semaphore_clr_almostFull(
		void		*hdl,
		SIGN32		id
		);

/****************************************************************************************
 * Function: hbo_hdl
 * Description: Initialize HDL_hbo with a $HBO BIU instance.
 * UNSG32		mem,	*!	Base address of HBO SRAM !
 * UNSG32		ra,		*!	Base address of a BIU instance of $HBO !*
 * void		*hdl	*!	Handle to HDL_hbo !*
 * ***************************************************************************************
 */
void	hbo_hdl(
		UNSG32		mem,
		UNSG32		ra,
		void		*hdl
		);

/*****************************************************************************************
 * Function: hbo_fifoCtl
 * Description: Get HDL_semaphore pointer from a HBO instance.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * Return:			void*	Handle for HBO.FiFoCtl
 * *****************************************************************************************
 */
void	*hbo_fifoCtl(
		void		*hdl
		);

/***************************************************************************************
 * DEFINITION - convert HBO FIFO control to semaphore control
 * *************************************************************************************
 */
#define		hbo_queue_intr_enable(hdl, id, empty, full, almostEmpty, almostFull, cpu)\
	semaphore_intr_enable(hbo_fifoCtl(hdl), id, empty, full, almostEmpty, almostFull, cpu)

#define		hbo_queue_query(hdl, id, master, ptr)\
	semaphore_query(hbo_fifoCtl(hdl), id, master, ptr)

#define		hbo_queue_push(hdl, id, delta)\
	semaphore_push(hbo_fifoCtl(hdl), id, delta)

#define		hbo_queue_pop(hdl, id, delta)\
	semaphore_pop(hbo_fifoCtl(hdl), id, delta)

#define		hbo_queue_chk_empty(hdl, id)\
	semaphore_chk_empty(hbo_fifoCtl(hdl), id)

#define		hbo_queue_chk_full(hdl, id)\
	ispss_semaphore_chk_full(hbo_fifoCtl(hdl), id)

#define		hbo_queue_chk_almostEmpty(hdl, id)\
	semaphore_chk_almostEmpty(hbo_fifoCtl(hdl), id)

#define		hbo_queue_chk_almostFull(hdl, id)\
	semaphore_chk_almostFull(hbo_fifoCtl(hdl),	id)

#define		hbo_queue_clr_empty(hdl, id)\
	semaphore_clr_empty(hbo_fifoCtl(hdl), id)

#define		hbo_queue_clr_full(hdl, id)\
	semaphore_clr_full(hbo_fifoCtl(hdl), id)

#define		hbo_queue_clr_almostEmpty(hdl, id)\
	semaphore_clr_almostEmpty(hbo_fifoCtl(hdl), id)

#define		hbo_queue_clr_almostFull(hdl, id)\
	semaphore_clr_almostFull(hbo_fifoCtl(hdl), id)

/******************************************************************************************
 * Function: hbo_queue_cfg
 * Description: Configurate a FIFO's base, depth & reset pointers.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32		id,			*!	Queue ID in $HBO !*
 * UNSG32		base,		*!	Channel FIFO base address (byte address) !*
 * SIGN32		depth,		*!	Channel FIFO depth, in 64b word !*
 * SIGN32		enable,		*!	0 to disable, 1 to enable !*
 * T64b		cfgQ[]		*!	Pass NULL to directly init HBO, or
 * Pass non-zero to receive programming sequence in (adr,data) pairs
 * Return:			UNSG32	-	Number of (adr,pair) added to cfgQ
 * ****************************************************************************************
 */
UNSG32	hbo_queue_cfg(
		void		*hdl,
		SIGN32		id,
		UNSG32		base,
		SIGN32		depth,
		SIGN32		enable,
		T64b		cfgQ[]);

/*****************************************************************************************
 * Function: hbo_queue_enable
 * Description: HBO FIFO enable/disable
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * SIGN32		enable,		*!	0 to disable, 1 to enable !*
 * T64b		cfgQ[]		*!	Pass NULL to directly init HBO, or
 * Pass non-zero to receive programming sequence in (adr,data) pairs
 * Return:			UNSG32		-	Number of (adr,pair) added to cfgQ
 * ***************************************************************************************
 */
UNSG32	hbo_queue_enable(
		void		*hdl,
		SIGN32		id,
		SIGN32		enable,
		T64b		cfgQ[]);

/*****************************************************************************************
 * Function: hbo_queue_clear
 * Description: Issue HBO FIFO clear (will NOT wait for finish).
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * **************************************************************************************
 */
void	hbo_queue_clear(
		void		*hdl,
		SIGN32		id
		);

/*****************************************************************************************
 * Function: hbo_queue_busy
 * Description: Read HBO 'BUSY' status for all channel FIFOs.
 * Return:			UNSG32	-	'BUSY' status bits of all channels
 * ***************************************************************************************
 */
UNSG32	hbo_queue_busy(
		void		*hdl		/*!	Handle to HDL_hbo !*/
		);

/*****************************************************************************************
 * Function: hbo_queue_clear_done
 * Description: Wait for a given channel or all channels to be cleared.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * ***************************************************************************************
 */
void	hbo_queue_clear_done(
		void		*hdl,
		SIGN32		id
		);

/****************************************************************************************
 * Function: hbo_queue_read
 * Description: Read a number of 64b data & pop FIFO from HBO SRAM.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32	id,			*!	Queue ID in $HBO !*
 * SIGN32		n,			*!	Number 64b entries to read !
 * T64b		data[],		*!	To receive read data !*
 * UNSG32		*ptr		*!	Pass in current FIFO pointer (in 64b word),
 * & receive updated new pointer, Pass NULL to read from HW
 * Return:		UNSG32	-	Number of 64b data being read (=n), or (when cfgQ==NULL)
 *			0 if there're not sufficient data in FIFO
 * **************************************************************************************
 */
UNSG32	hbo_queue_read(
		void		*hdl,
		SIGN32		id,
		SIGN32		n,
		T64b		data[],
		UNSG32		*ptr
		);

/******************************************************************************************
 * Function: hbo_queue_write
 * Description: Write a number of 64b data & push FIFO to HBO SRAM.
 * void		*hdl,		*!	Handle to HDL_hbo !*
 * SIGN32		id,		*!	Queue ID in $HBO !*
 * SIGN32		n,		*!	Number 64b entries to write !*
 * T64b		data[],		*!	Write data !*
 * T64b		cfgQ[],		*!	Pass NULL to directly update HBO, or
 * Pass non-zero to receive programming sequence in (adr,data) pairs
 * UNSG32		*ptr		*!	Pass in current FIFO pointer (in 64b word),
 * & receive updated new pointer, Pass NULL to read from HW
 * Return:		UNSG32		Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
 *				0 if there're not sufficient space in FIFO
 * *****************************************************************************************
 */
UNSG32	hbo_queue_write(
		void		*hdl,
		SIGN32		id,
		SIGN32		n,
		T64b		data[],
		T64b		cfgQ[],
		UNSG32		*ptr
		);

/*******************************************************************************************
 * Function: dhub_hdl
 * Description: Initialize HDL_dhub with a $dHub BIU instance.
 * UNSG32		mem,	!	Base address of dHub.HBO SRAM !
 * UNSG32		ra,		!	Base address of a BIU instance of $dHub !
 * void		*hdl	!	Handle to HDL_dhub !*
 * ****************************************************************************************
 */
void	dhub_hdl(
		UNSG32		mem,
		UNSG32		ra,
		void		*hdl
		);

/****************************************************************************************
 * Function: ispss_dhub_semaphore
 * Description: Get HDL_semaphore pointer from a dHub instance.
 * void		*hdl,		*!	Handle to HDL_dhub !*
 * Return:			void*		-	Handle for dHub.SemaHub
 * ***************************************************************************************
 */
void	*ispss_dhub_semaphore(
		void		*hdl
		);

/****************************************************************************************
 * Function: dhub_hbo
 * Description: Get HDL_hbo pointer from a dHub instance.
 * void		*hdl,		*!	Handle to HDL_dhub !*
 * Return:			void*		-	Handle for dHub.HBO
 * **************************************************************************************
 */
void	*dhub_hbo(
		void		*hdl
		);

/*****************************************************************************************
 * Function: dhub_hbo_fifoCtl
 * Description: Get HDL_semaphore pointer from the HBO of a dHub instance.
 * Return:			void*		Handle for dHub.HBO.FiFoCtl
 * ***************************************************************************************
 */
#define	dhub_hbo_fifoCtl(hdl)		(hbo_fifoCtl(dhub_hbo(hdl)))

/****************************************************************************************
 * DEFINITION - convert from dHub channel ID to HBO FIFO ID & semaphore (interrupt) ID
 * **************************************************************************************
 */
/*removed connection from HBO interrupt to semaphore
 * #define	dhub_id2intr(id)			((id)+1)
 */
#define	dhub_id2intr(id)			((id))
#define	dhub_id2hbo_cmdQ(id)		((id)*2)
#define	dhub_id2hbo_data(id)		((id)*2+1)

/*******************************************************************************************
 * DEFINITION - convert dHub cmdQ/dataQ/channel-done interrupt
 *								control to HBO/semaphore control
 * ******************************************************************************************
 */
/*removed connection from HBO interrupt to semaphore
 * #define	dhub_hbo_intr_enable		(hdl,id,empty,full,almostEmpty,almostFull,cpu
 * semaphore_intr_enable	(dhub_semaphore(hdl),0,empty,full,almostEmpty,almostFull,cpu)
 */

#define		dhub_channel_intr_enable(hdl, id, full, cpu)\
	semaphore_intr_enable(dhub_semaphore(hdl), dhub_id2intr(id), 0, full, 0, 0, cpu)

#define		dhub_hbo_cmdQ_intr_enable(hdl, id, empty, almostEmpty, cpu)\
	hbo_queue_intr_enable(dhub_hbo(hdl), dhub_id2hbo_cmdQ(id), empty, 0, almostEmpty, 0, cpu)

#define		dhub_hbo_data_intr_enable(hdl, id, empty, full, almostEmpty, almostFull, cpu)\
	hbo_queue_intr_enable(dhub_hbo(hdl), dhub_id2hbo_data(id), empty, full,\
					almostEmpty, almostFull, cpu)

/************************************************************************************
 * DEFINITION - convert dHub cmdQ opehdltions to HBO FIFO opehdltions
 * ***********************************************************************************
 */
#define		dhub_cmdQ_query(hdl, id, ptr)\
	hbo_queue_query(dhub_hbo(hdl), dhub_id2hbo_cmdQ(id), 0, ptr)

#define		dhub_cmdQ_push(hdl, id, delta)\
	hbo_queue_push(dhub_hbo(hdl), dhub_id2hbo_cmdQ(id), delta)

/*************************************************************************************
 * DEFINITION - convert dHub dataQ opehdltions to HBO FIFO opehdltions
 * ***********************************************************************************
 */
#define		dhub_data_query(hdl, id, master, ptr)\
	hbo_queue_query(dhub_hbo(hdl), dhub_id2hbo_data(id), master, ptr)

#define		dhub_data_push(hdl, id, delta)\
	hbo_queue_push(dhub_hbo(hdl), dhub_id2hbo_data(id), delta)

#define		dhub_data_pop(hdl, id, delta)\
	hbo_queue_pop(dhub_hbo(hdl), dhub_id2hbo_data(id), delta)

/**************************************************************************************
 * Function: dhub_channel_cfg
 * Description: Configurate a dHub channel.
 * void		*hdl,		*!	Handle to HDL_dhub !*
 * SIGN32		id,			*!	Channel ID in $dHubReg !
 * UNSG32		baseCmd,	*!	Channel FIFO base address (byte address) for cmdQ !*
 * UNSG32		baseData,*!	Channel FIFO base address (byte address) for dataQ !*
 * SIGN32		depthCmd,	*!	Channel FIFO depth for cmdQ, in 64b word !*
 * SIGN32		depthData,	*!	Channel FIFO depth for dataQ, in 64b word !*
 * SIGN32		mtu,	*!	See 'dHubChannel.CFG.MTU', 0/1/2 for 8/32/128 bytes !*
 * SIGN32		QoS,		*!	See 'dHubChannel.CFG.QoS' !*
 * SIGN32		selfLoop,	*!	See 'dHubChannel.CFG.selfLoop' !*
 * SIGN32		enable,		*!	0 to disable, 1 to enable !
 * T64b		cfgQ[]		*!	Pass NULL to directly init dHub, or
 * Pass non-zero to receive programming sequence in (adr,data) pairs
 * Return:			UNSG32-	Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
 * 0 if either cmdQ or dataQ in HBO is still busy
 * ***************************************************************************************
 */
UNSG32	dhub_channel_cfg(
		void		*hdl,
		SIGN32		id,
		UNSG32		baseCmd,
		UNSG32		baseData,
		SIGN32		depthCmd,
		SIGN32		depthData,
		SIGN32		mtu,
		SIGN32		QoS,
		SIGN32		selfLoop,
		SIGN32		enable,
		T64b		cfgQ[]);

/*************************************************************************************
 * Function: dhub_channel_enable
 * Description: dHub channel enable/disable.
 * void		*hdl,	!	Handle to HDL_dhub !
 * SIGN32		id		!	Channel ID in $dHubReg !
 * SIGN32		enable,	!	0 to disable, 1 to enable
 * T64b		cfgQ[]		Pass NULL to directly init dHub, or
 * Pass non-zero to receive programming sequenc in (adr,data) pairs
 * Return:			UNSG32		-	Number of (adr,pair) added to cfgQ
 * ************************************************************************************
 */
UNSG32	dhub_channel_enable(
		void		*hdl,
		SIGN32		id,
		SIGN32		enable,
		T64b		cfgQ[]);

/**************************************************************************************
 * Function: dhub_channel_clear
 * Description: Issue dHub channel clear (will NOT wait for finish).
 * void		*hdl,	!	Handle to HDL_dhub !
 * SIGN32		id		!	Channel ID in $dHubReg !
 * ************************************************************************************
 */
void	dhub_channel_clear(
		void		*hdl,
		SIGN32		id
		);

/*************************************************************************************
 * Function: dhub_channel_flush
 * Description: Issue dHub channel (H2M only) flush (will NOT wait for finish).
 * void		*hdl,	!	Handle to HDL_dhub !
 * SIGN32		id		!	Channel ID in $dHubReg !
 * **********************************************************************************
 */
void	dhub_channel_flush(
		void		*hdl,
		SIGN32		id
		);

/***************************************************************************************
 * Function: dhub_channel_busy
 * Description: Read dHub 'BUSY' status for all channel FIFOs
 * void		*hdl	!	Handle to HDL_dhub !
 * Return:			UNSG32	-	'BUSY' status bits of all channels
 * *************************************************************************************
 */
UNSG32	dhub_channel_busy(
		void		*hdl
		);

/*************************************************************************************
 * Function: dhub_channel_pending
 * Description: Read dHub 'PENDING' status for all channel FIFOs.
 * Return:			UNSG32		-	'PENDING' status bits of all channels
 * ***********************************************************************************
 */
UNSG32	dhub_channel_pending(
		void		*hdl	/*!	Handle to HDL_dhub !*/
		);

/************************************************************************************
 * Function: dhub_channel_clear_done
 * Description: Wait for a given channel or all channels to be cleared or flushed.
 * void		*hdl,	!	Handle to HDL_dhub !
 * SIGN32		id		!	Channel ID in $dHubReg
 *	-1 to wait for all channel clear done
 * *********************************************************************************
 */
void	dhub_channel_clear_done(
		void		*hdl,
		SIGN32		id
		);

/************************************************************************************
 * Function: dhub_channel_write_cmd
 * Description: Write a 64b command for a dHub channel.
 * void		*hdl,	!	Handle to HDL_dhub !
 * SIGN32		id,		!	Channel ID in $dHubReg !
 * UNSG32		addr,	!	CMD: buffer address !
 * SIGN32		size,	!	CMD: number of bytes to transfer !
 * SIGN32		semOnMTU,!	CMD: semaphore operation at CMD/MTU (0/1) !
 * SIGN32		chkSemId,!CMD: non-zero to check semaphore !
 * SIGN32		updSemId,	!	CMD: non-zero to update semaphore !
 * SIGN32		interrupt,	!	CMD: raise interrupt at CMD finish !
 * T64b		cfgQ[],	!	Pass NULL to directly update dHub, or
 *				Pass non-zero to receive programming sequence in (adr,data) pairs
 * UNSG32		*ptr	!	Pass in current cmdQ pointer (in 64b word),
 * receive updated new pointer, Pass NULL to read from HW
 * Return:	UNSG32		-	Number of (adr,pair) added to cfgQ if success, or
 * 0 if there're not sufficient space in FIFO
 * **********************************************************************************
 */
UNSG32	dhub_channel_write_cmd(
		void		*hdl,
		SIGN32		id,
		UNSG32		addr,
		SIGN32		size,
		SIGN32		semOnMTU,
		SIGN32		chkSemId,
		SIGN32		updSemId,
		SIGN32		interrupt,
		T64b		cfgQ[],
		UNSG32		*ptr
		);

/************************************************************************************
 * Function: dhub_channel_generate_cmd
 * Description: Write a 64b command for a dHub channel.
 * void		*hdl,		Handle to HDL_dhub !
 * SIGN32		id,			Channel ID in $dHubReg !
 * UINT64      addr,		CMD: buffer address !
 * SIGN32		size,	CMD: number of bytes to transfer !
 * SIGN32		semOnMTU,CMD: semaphore operation at CMD/MTU (0/1) !
 * SIGN32		chkSemId,	!	CMD: non-zero to check semaphore !
 * SIGN32		updSemId,	!	CMD: non-zero to update semaphore !
 * SIGN32		interrupt,	!	CMD: raise interrupt at CMD finish !
 * SIGN32		*pData
 * **********************************************************************************
 */
void	dhub_channel_generate_cmd(
		void		*hdl,
		SIGN32		id,
		UINT64      addr,
		SIGN32		size,
		SIGN32		semOnMTU,
		SIGN32		chkSemId,
		SIGN32		updSemId,
		SIGN32		interrupt,
		SIGN32		*pData
		);

/*************************************************************************************
 * Function: dhub_channel_big_write_cmd
 * Description: Write a sequence of 64b command for a dHub channel
 * void		*hdl,	!Handle to HDL_dhub !
 * SIGN32		id,		!Channel ID in $dHubReg !
 * UNSG32		addr,	!CMD: buffer address !
 * SIGN32		size,	!CMD: number of bytes to transfer !
 * SIGN32		semOnMTU,	!CMD: semaphore operation at CMD/MTU (0/1) !
 * SIGN32		chkSemId,	!CMD: non-zero to check semaphore !
 * SIGN32		updSemId,	!CMD: non-zero to update semaphore !
 * SIGN32		interrupt,	!CMD: raise interrupt at CMD finish !
 * T64b		cfgQ[],	!     Pass NULL to directly update dHub, or
 *					Pass non-zero to receive programming sequence
 *					in (adr,data) pairs
 *	UNSG32		*ptr	!Pass in current cmdQ pointer (in 64b word),
 *					& receive updated new pointer,
 *					Pass NULL to read from HW
 *Return: UNSG32     Number of (adr,pair) added to cfgQ if success, or
 *              0 if there're not sufficient space in FIFO
 * **********************************************************************************
 */
UNSG32  dhub_channel_big_write_cmd(
		void		*hdl,
		SIGN32		id,
		UNSG32		addr,
		SIGN32		size,
		SIGN32		semOnMTU,
		SIGN32		chkSemId,
		SIGN32		updSemId,
		SIGN32		interrupt,
		T64b		cfgQ[],
		UNSG32		*ptr
		);

/*************************************************************************************
 * Function: dhub2d_hdl
 * Description: Initialize HDL_dhub2d with a $dHub2D BIU instance.
 * UNSG32		mem,!	Base address of dHub2D.dHub.HBO SRAM !
 * UNSG32		ra,		Base address of a BIU instance of $dHub2
 * void		*hdl		Handle to HDL_dhub2d
 * ************************************************************************************
 */
void	dhub2d_hdl(
		UNSG32		mem,
		UNSG32		ra,
		void		*hdl
		);

/*************************************************************************************
 * Function: dhub2d_channel_cfg
 * Description: Configurate a dHub2D channel.
 * void		*hdl,		Handle to HDL_dhub2d !
 * SIGN32		id,		Channel ID in $dHubReg2D !
 * UNSG32		addr,	!	CMD: 2D-buffer address !
 * SIGN32		stride,	!	CMD: line stride size in bytes !
 * SIGN32		width,	!	CMD: buffer width in bytes !
 * SIGN32		height,		!	CMD: buffer height in lines !
 * SIGN32		semLoop,	!	CMD: loop size (1~4) of semaphore operations
 * SIGN32		semOnMTU,	!	CMD: semaphore operation at CMD/MTU (0/1)
 * SIGN32		chkSemId[],	!	CMD: semaphore loop pattern - non-zero to check !
 * SIGN32		updSemId[],	!	CMD: semaphore loop pattern - non-zero to update !
 * SIGN32		interrupt,	!	CMD: raise interrupt at CMD finish !
 * SIGN32	enable,		!	0 to disable, 1 to enable
 * T64b		cfgQ[]		!	Pass NULL to directly init dHub2D, or
 * Pass non-zero to receive programming sequence in (adr,data) pairs
 * Return:			UNSG32		-	Number of (adr,pair) added to cfgQ
 * ************************************************************************************
 */
UNSG32	dhub2d_channel_cfg(
		void		*hdl,
		SIGN32		id,
		UNSG32		addr,
		SIGN32		stride,
		SIGN32		width,
		SIGN32		height,
		SIGN32		semLoop,
		SIGN32		semOnMTU,
		SIGN32		chkSemId[],
		SIGN32		updSemId[],
		SIGN32		interrupt,
		SIGN32		enable,
		T64b		cfgQ[]);

/****************************************************************************************
 * Function: ispss_dhub2nd_channel_cfg
 * Description: Configurate a dHub2ND channel.
 * void	*hdl,	 Handle to HDL_dhub2d
 * SIGN32	id,	Channel ID in $dHubReg2D
 * UNSG32	addr,	CMD: 2ND-buffer address
 * SIGN32	burst,	CMD: line stride size in bytes
 * SIGN32	step1,	CMD: buffer width in bytes
 * SIGN32	size1,	CMD: buffer height in lines
 * SIGN32	step2,	 CMD: loop size (1~4) of semaphore operations
 * SIGN32	size2,	CMD: semaphore operation at CMD/MTU (0/1
 * SIGN32	chkSemId,	CMD: semaphore loop pattern - non-zero to check
 * SIGN32	updSemId,	CMD: semaphore loop pattern - non-zero to update
 * SIGN32	interrupt,	CMD: raise interrupt at CMD finish
 * SIGN32	enable,		 0 to disable, 1 to enable
 * T64b	cfgQ[]		 Pass NULL to directly init dHub2ND, or
 *				Pass non-zero to receive programming sequence
 *				in (adr,data) pairs
 * Return: UNSG32 -   Number of (adr,pair) added to cfgQ
 * *************************************************************************************
 */
UNSG32  ispss_dhub2nd_channel_cfg(
		void	*hdl,
		SIGN32	id,
		UNSG32	addr,
		SIGN32	burst,
		SIGN32	step1,
		SIGN32	size1,
		SIGN32	step2,
		SIGN32	size2,
		SIGN32	chkSemId,
		SIGN32	updSemId,
		SIGN32	interrupt,
		SIGN32	enable,
		T64b	cfgQ[]);

/*************************************************************************************
 * Function: dhub2d_channel_enable
 * Description: dHub2D channel enable/disable.
 * void		*hdl,	Handle to HDL_dhub2d
 * SIGN32		id,		Channel ID in $dHubReg2D
 * SIGN32		enable,	to disable, 1 to enable
 * T64b		cfgQ[]	Pass NULL to directly init dHub2D, or
 *			Pass non-zero to receive programming sequence
 *			in (adr,data) pairs
 * Return:			UNSG32		-	Number of (adr,pair) added to cfgQ
 * ***********************************************************************************
 */
UNSG32	dhub2d_channel_enable(
		void		*hdl,
		SIGN32		id,
		SIGN32		enable,
		T64b		cfgQ[]);

/************************************************************************************
 * Function: dhub2d_channel_clear
 * Description: Issue dHub2D channel clear (will NOT wait for finish).
 * void		*hdl,	Handle to HDL_dhub2d
 * SIGN32 id		Channel ID in $dHubReg2D
 * **********************************************************************************
 */
void	dhub2d_channel_clear(
		void		*hdl,
		SIGN32		id
		);

/*************************************************************************************
 * Function: dhub2d_channel_busy
 * Description: Read dHub2D 'BUSY' status for all channel FIFOs
 * void		*hdl		Handle to HDL_dhub2d
 * Return:			UNSG32		-	'BUSY' status bits of all channels
 * ***********************************************************************************
 */
UNSG32	dhub2d_channel_busy(
		void		*hdl
		);

/*************************************************************************************
 * Function: dhub2d_channel_clear_done
 * Description: Wait for a given channel or all channels to be cleared.
 * void		*hdl		Handle to HDL_dhub2d
 * SIGN32	id			Channel ID in $dHubReg2D -
 *					-1 to wait for all channel clear done
 * ***********************************************************************************
 */
void	dhub2d_channel_clear_done(
		void		*hdl,
		SIGN32		id
		);

enum ENUM_BCM_SCHED_QID {
	BCM_SCHED_Q0 = 0,
	BCM_SCHED_Q1,
	BCM_SCHED_Q2,
	BCM_SCHED_Q3,
	BCM_SCHED_Q4,
	BCM_SCHED_Q5,
	BCM_SCHED_Q6,
	BCM_SCHED_Q7,
	BCM_SCHED_Q8,
	BCM_SCHED_Q9,
	BCM_SCHED_Q10,
	BCM_SCHED_Q11,
	BCM_SCHED_Q12,
	BCM_SCHED_Q13,
	BCM_SCHED_Q14,
	BCM_SCHED_Q15,
	BCM_SCHED_Q16,
	BCM_SCHED_Q17,
	BCM_SCHED_Q18,
};

enum ENUM_BCM_SCHED_TRIG_EVENT {
	BCM_SCHED_TRIG_CPCB0_VBI = 0,
	BCM_SCHED_TRIG_CPCB1_VBI,
	BCM_SCHED_TRIG_CPCB2_VBI,
	BCM_SCHED_TRIG_CPCB0_VDE,
	BCM_SCHED_TRIG_CPCB1_VDE,
	BCM_SCHED_TRIG_CPCB2_VDE,
	BCM_SCHED_TRIG_AUD_PRIM,
	BCM_SCHED_TRIG_AUD_SEC,
	BCM_SCHED_TRIG_AUD_HDMI,
	BCM_SCHED_TRIG_AUD_SPDIF,
	BCM_SCHED_TRIG_AUD_MIC, //0xA
	BCM_SCHED_TRIG_VBI_VDE,
	BCM_SCHED_TRIG_DVI_VDE,
	BCM_SCHED_TRIG_SD_WRE,
	BCM_SCHED_TRIG_SD_RDE,
	BCM_SCHED_TRIG_SD_ERR,  //0xF
	BCM_SCHED_TRIG_NONE = 0x1f,
};

void BCM_SCHED_Open(void);
void BCM_SCHED_Close(void);
void BCM_SchedSetMux(UNSG32 QID, UNSG32 TrigEvent);
int BCM_SCHED_PushCmd(UNSG32 QID, UNSG32 *pCmd, UINT64 *cfgQ);
void BCM_SCHED_GetEmptySts(UNSG32 QID, UNSG32 *EmptySts);

int BCM_SCHED_AutoPushCmd(UNSG32 QID, UNSG8 uchEnable);

/*******************************************************************************
 * Function: dhub_channel_enable_InverseScan
 * Description: Enable the inverse scan at loader.
 * *****************************************************************************
 */
UNSG32 dhub_channel_enable_InverseScan(void *hdl, SIGN32 id, SIGN32 iMode, T64b cfgQ[]);
UNSG32 getBcmDhubType(void);
UNSG32 getBcmBaseAddr(void);
UNSG32 getBcmDummyRegAddr(void);
void bcm_buffer_write(void *pbcmbuf, UNSG32 regAddr, UNSG32 regVal);
int bcm_buffer_dummy_write(void *pbcmbuf, UNSG32 len);
int raw_bcm_buffer_dummy_write(UINT64 *rawBcmBufferBase, UNSG32 len);

void dhub2d_channel_clear_seq(void *hdl, SIGN32 id);
void dhub2d_channel_start_seq(void *hdl, SIGN32 id);


#endif //ISPSS_API_DHUB_H__
