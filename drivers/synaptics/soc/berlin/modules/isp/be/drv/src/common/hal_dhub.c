// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/mutex.h>
#include <linux/delay.h>
#include "com_type.h"
#include "ispSS_dhub_drv.h"
#include "ispSS_reg.h"
#include "ispSS_hal_dhub.h"

#undef IO32CFG
#define IO32CFG(cfgQ, i, a, d)  do { \
	if (cfgQ) {\
		if (getBcmDhubType()) {\
			(cfgQ)[i][0] = 0x0;\
			(cfgQ)[i][1] = getBcmDummyRegAddr();\
			(i)++;\
		} \
		(cfgQ)[i][0] = (d); (cfgQ)[i][1] = (a);\
	} else\
		ISPSS_IO32WR(d, a); \
	(i)++;\
} while (0)

#undef IO32WR
#define IO32WR(d, a) ISPSS_IO32WR(d, a)

#undef IO32RD
#define IO32RD(d, a) ISPSS_IO32RD(d, a)

#include "ispSS_api_dhub.h"
#include "ispSS_memmap.h"
#include "ispBCM.h"
#include "ispSS_bcmbuf.h"
/**	SECTION - handle of local contexts **/

/**	ENDOFSECTION **/

/**	SECTION - API definitions for $SemaHub **/
/***********************************************************************
 *	Function: semaphore_hdl
 *	Description: Initialize HDL_semaphore with a $SemaHub BIU instance
 *  UNSG32 ra - Base address of BIU instance of $SemaHub
 *  void *hdl - Handle to HDL_semaphore
 ***********************************************************************/
void semaphore_hdl(UNSG32 ra, void *hdl)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;

	sem->ra = ra;
}

/**************************************************************************************
 * Function: semaphore_cfg
 * Description: Configurate a semaphore's depth & reset pointers.
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub
 * SIGN32 depth - Semaphore virtual FIFO depth
 * T64b cfgQ[] - Pass NULL to directly init SemaHub, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 ***************************************************************************************/
UNSG32 semaphore_cfg(void *hdl, SIGN32 id, SIGN32 depth, T64b cfgQ[])
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	T32Semaphore_CFG	cfg;
	UNSG32 i = 0, a = sem->ra + RA_SemaHub_ARR + id*sizeof(SIE_Semaphore);

	cfg.u32 = 0; cfg.uCFG_DEPTH = sem->depth[id] = depth;
	IO32CFG(cfgQ, i, a + RA_Semaphore_CFG, cfg.u32);

	return i;
}

/****************************************************************************
 *	Function: semaphore_intr_enable
 *	Description: Configurate interrupt enable bits of a semaphore.
 *  void *hdl - Handle to HDL_semaphore
 *  SIGN32 id - Semaphore ID in $SemaHub
 *  SIGN32 empty - Interrupt enable for CPU at condition 'empty'
 *  SIGN32 full - Interrupt enable for CPU at condition 'full'
 *  SIGN32 almostEmpty -Interrupt enable for CPU at condition 'almostEmpty'
 *  SIGN32 almostFull -Interrupt enable for CPU at condition 'almostFull'
 *  SIGN32 cpu - CPU ID (0/1/2)
 *****************************************************************************/
void semaphore_intr_enable(void *hdl, SIGN32 id, SIGN32 empty,
		SIGN32 full, SIGN32 almostEmpty, SIGN32 almostFull, SIGN32 cpu)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	T32SemaINTR_mask mask;
	UNSG32 a = sem->ra + RA_SemaHub_ARR + id * sizeof(SIE_Semaphore);

	mask.u32 = 0;
	mask.umask_empty = empty;
	mask.umask_full = full;
	mask.umask_almostEmpty = almostEmpty;
	mask.umask_almostFull = almostFull;

	IO32WR(mask.u32, a + RA_Semaphore_INTR + cpu * sizeof(SIE_SemaINTR));
}

/*************************************************************************
 * Function: semaphore_query
 * Description: Query current status (counter & pointer) of a semaphore.
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub
 * SIGN32 master - 0/1 as procuder/consumer query
 * UNSG32 *ptr - Non-zero to receive semaphore r/w pointer
 * Return: UNSG32 Current available unit level
 ********************************************************************************/
UNSG32 semaphore_query(void *hdl, SIGN32 id, SIGN32 master, UNSG32 *ptr)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	T32SemaQuery_RESP resp;
	T32SemaQueryMap_ADDR map;

	map.u32 = 0; map.uADDR_ID = id; map.uADDR_master = master;
	IO32RD(resp.u32, sem->ra + RA_SemaHub_Query + map.u32);

	if (ptr)
		*ptr = resp.uRESP_PTR;
	return resp.uRESP_CNT;
}

/**********************************************************************
 *	Function: semaphore_push
 *	Description: Producer semaphore push.
 *  void *hdl - Handle to HDL_semaphore
 *  SIGN32 id - Semaphore ID in $SemaHub
 *  SIGN32 delta - Delta to push as a producer
 ***********************************************************************/
void semaphore_push(void *hdl, SIGN32 id, SIGN32 delta)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	T32SemaHub_PUSH push;

	push.u32 = 0; push.uPUSH_ID = id; push.uPUSH_delta = delta;
	IO32WR(push.u32, sem->ra + RA_SemaHub_PUSH);
}

/*******************************************************************************
 *	Function: semaphore_push
 *	Description: Consumer semaphore pop.
 *  void *hdl - Handle to HDL_semaphore
 *  SIGN32 id - Semaphore ID in $SemaHub
 *  SIGN32 delta - Delta to pop as a consumer
 *******************************************************************************/
void semaphore_pop(void *hdl, SIGN32 id, SIGN32 delta)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	T32SemaHub_POP pop;

	pop.u32 = 0; pop.uPOP_ID = id; pop.uPOP_delta = delta;
	IO32WR(pop.u32, sem->ra + RA_SemaHub_POP);
}

/************************************************************************
 *	Function: semaphore_chk_empty
 *	Description: Check 'empty' status of a semaphore (or all semaphores).
 *  void *hdl - Handle to HDL_semaphore
 *  SIGN32 id - Semaphore ID in $SemaHub -1 to return all 32b of interrupt status
 *	Return:			UNSG32 status bit of given semaphore, or
 *	status bits of all semaphores if id==-1
 ************************************************************************/
UNSG32	semaphore_chk_empty(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	UNSG32 d;

	IO32RD(d, sem->ra + RA_SemaHub_empty);
	return (id < 0) ? d : bTST(d, id);
}

/*********************************************************************************
 * Function: ispss_semaphore_chk_full
 * Description: Check 'full' status of a semaphore (or all semaphores).
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub -1 to return all 32b of interrupt status
 * Return: UNSG32 status bit of given semaphore, or
 * status bits of all semaphores if id==-1
 *********************************************************************************/
UNSG32	ispss_semaphore_chk_full(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	UNSG32 d;

	IO32RD(d, sem->ra + RA_SemaHub_full);
	return (id < 0) ? d : bTST(d, id);
	/**	ENDOFFUNCTION: semaphore_chk_full **/
}
EXPORT_SYMBOL(ispss_semaphore_chk_full);

int dhub_wait_complete(int ch, struct HDL_semaphore *pSemHandle, int timeout)
{
	int dHubStatus = 0;

	do {
		dHubStatus = ispss_semaphore_chk_full(pSemHandle, -1);
		if (dHubStatus & (1<<ch))
			break;

		timeout--;
		msleep(1);

		if (timeout <= 0) {
			// Print err msg only dectect timeout 0
			if (timeout == 0)
				pr_debug("DMA timeout\n");
			return -1;
		}
	} while ((dHubStatus & (1<<ch)) == 0);
	//pop sema
	semaphore_pop(pSemHandle, ch, 1);
	semaphore_clr_full(pSemHandle, ch);
	if (ispss_semaphore_chk_full(pSemHandle, ch)) {
		pr_debug("Error: dHub channel %d interrupt is NOT cleared\n", ch);
		return -1;
	}

	return 0;
}
/*********************************************************************************
 * Function: semaphore_chk_almostEmpty
 * Description: Check 'almostEmpty' status of a semaphore (or all semaphores).
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub
 * Return: UNSG32 status bit of given semaphore, or
 * status bits of all semaphores if id==-1
 *********************************************************************************/
UNSG32 semaphore_chk_almostEmpty(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	UNSG32 d;

	IO32RD(d, sem->ra + RA_SemaHub_almostEmpty);
	return (id < 0) ? d : bTST(d, id);
}

/*********************************************************************************
 * Function: semaphore_chk_almostFull
 * Description: Check 'almostFull' status of a semaphore (or all semaphores).
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub -1 to return all 32b of the interrupt status
 * Return: UNSG32 status bit of given semaphore, or
 * status bits of all semaphores if id==-1
 **********************************************************************************/
UNSG32 semaphore_chk_almostFull(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;
	UNSG32 d;

	IO32RD(d, sem->ra + RA_SemaHub_almostFull);
	return (id < 0) ? d : bTST(d, id);
}

/***************************************************
 * Function: semaphore_clr_empty
 * Description: Clear 'empty' status of a semaphore.
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub
 **************************************************/
void semaphore_clr_empty(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;

	IO32WR(1<<id, sem->ra + RA_SemaHub_empty);
}

/**************************************************
 * Function: semaphore_clr_full
 * Description: Clear 'full' status of a semaphore.
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub
 **************************************************/
void semaphore_clr_full(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;

	IO32WR(1<<id, sem->ra + RA_SemaHub_full);
}

/**********************************************************
 * Function: semaphore_clr_almostEmpty
 * Description: Clear 'almostEmpty' status of a semaphore.
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub
 **********************************************************/
void semaphore_clr_almostEmpty(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;

	IO32WR(1<<id, sem->ra + RA_SemaHub_almostEmpty);
}

/*********************************************************
 * Function: semaphore_clr_almostFull
 * Description: Clear 'almostFull' status of a semaphore.
 * void *hdl - Handle to HDL_semaphore
 * SIGN32 id - Semaphore ID in $SemaHub
 *********************************************************/
void semaphore_clr_almostFull(void *hdl, SIGN32 id)
{
	struct HDL_semaphore *sem = (struct HDL_semaphore *)hdl;

	IO32WR(1<<id, sem->ra + RA_SemaHub_almostFull);
}

/** ENDOFSECTION **/

/** SECTION - API definitions for $HBO **/
/***********************************************************
 * Function: hbo_hdl
 * Description: Initialize HDL_hbo with a $HBO BIU instance.
 * UNSG32 mem - Base address of dHub.HBO SRAM
 * UNSG32 ra - Base address of BIU instance of $dHub
 * void *hdl - Handle to HDL_dhub2d
 ***********************************************************/
void hbo_hdl(UNSG32	mem, UNSG32 ra, void *hdl)
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	struct HDL_semaphore *fifoCtl = &(hbo->fifoCtl);

	hbo->mem = mem; hbo->ra = ra;
	semaphore_hdl(ra + RA_HBO_FiFoCtl, fifoCtl);
}

/************************************************************************
 * Function: hbo_fifoCtl
 * Description: Get HDL_semaphore pointer from a HBO instance.
 * Return: void* Handle for HBO.FiFoCtl
 *************************************************************************/
void *hbo_fifoCtl(void *hdl)
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	struct HDL_semaphore *fifoCtl = &(hbo->fifoCtl);

	return fifoCtl;
}

/****************************************************************************
 * Function: hbo_queue_cfg
 * Description: Configurate a FIFO's base, depth & reset pointers.
 * void *hdl - Handle to HDL_hbo
 * SIGN32 id - Queue ID in $HBO
 * UNSG32 base - Channel FIFO base address (byte address)
 * SIGN32 depth - Channel FIFO depth, in 64b word
 * SIGN32 enable - 0 to disable, 1 to enable
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 *****************************************************************************/
UNSG32	hbo_queue_cfg(void *hdl, SIGN32 id, UNSG32 base,
		SIGN32 depth, SIGN32 enable, T64b cfgQ[])
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	struct HDL_semaphore *fifoCtl = &(hbo->fifoCtl);
	T32FiFo_CFG			cfg;
	UNSG32 i = 0, a;

	a = hbo->ra + RA_HBO_ARR + id*sizeof(SIE_FiFo);
	IO32CFG(cfgQ, i, a + RA_FiFo_START, 0);

	cfg.u32 = 0; cfg.uCFG_BASE = hbo->base[id] = base;
	IO32CFG(cfgQ, i, a + RA_FiFo_CFG, cfg.u32);

	i += semaphore_cfg(fifoCtl, id, depth, cfgQ ? (cfgQ + i) : NULL);
	IO32CFG(cfgQ, i, a + RA_FiFo_START, enable);

	return i;
}

/***********************************************************************
 * Function: hbo_queue_enable
 * Description: HBO FIFO enable/disable.
 * void *hdl - Handle to HDL_hbo
 * SIGN32 id - Queue ID in $HBO
 * SIGN32 enable - 0 to disable, 1 to enable
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 ************************************************************************/
UNSG32	hbo_queue_enable(void *hdl, SIGN32 id,
		SIGN32 enable, T64b cfgQ[])
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	UNSG32 i = 0, a;

	a = hbo->ra + RA_HBO_ARR + id*sizeof(SIE_FiFo);

	IO32CFG(cfgQ, i, a + RA_FiFo_START, enable);
	return i;
}

/***************************************************************
 * Function: hbo_queue_clear
 * Description: Issue HBO FIFO clear (will NOT wait for finish).
 * void *hdl - Handle to HDL_hbo
 * SIGN32 id - Queue ID in $HBO
 ***************************************************************/
void hbo_queue_clear(void *hdl, SIGN32 id)
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	UNSG32 a = hbo->ra + RA_HBO_ARR + id*sizeof(SIE_FiFo);

	IO32WR(1, a + RA_FiFo_CLEAR);
}

/*************************************************************
 * Function: hbo_queue_busy
 * Description: Read HBO 'BUSY' status for all channel FIFOs.
 * void *hdl - Handle to HDL_hbo
 * Return: UNSG32 'BUSY' status bits of all channels
 *************************************************************/
UNSG32 hbo_queue_busy(void *hdl)
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	UNSG32 d;

	IO32RD(d, hbo->ra + RA_HBO_BUSY);
	return d;
}

/***********************************************************************
 * Function: hbo_queue_clear_done
 * Description: Wait for a given channel or all channels to be cleared.
 * void *hdl - Handle to HDL_hbo
 * SIGN32 id - Queue ID in $HBO
 **********************************************************************/
void hbo_queue_clear_done(void *hdl, SIGN32 id)
{
	UNSG32 d;

	do {
		d = hbo_queue_busy(hdl);
		if (id >= 0)
			d = bTST(d, id);
	} while (d);

}

/********************************************************************
 * Function: hbo_queue_read
 * Description: Read a number of 64b data & pop FIFO from HBO SRAM.
 * void *hdl - Handle to HDL_hbo
 * SIGN32 id - Queue ID in $HBO
 * SIGN32 n - Number of 64b entries to write
 * T64b data[] - To receive read data
 * UNSG32 *ptr - Pass in current FIFO pointer (in 64b word) & receive
 *                updated new pointer, Pass NULL to read from HW
 * Return: UNSG32 Number of 64b data being read (=n), or (when cfgQ==NULL)
 * 0 if there're not sufficient data in FIFO
 ************************************************************************/
UNSG32 hbo_queue_read(void *hdl, SIGN32 id, SIGN32 n,
		T64b data[], UNSG32 *ptr)
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	SIGN32 i;
	UNSG32 p, base, depth;

	base = hbo->mem + hbo->base[id]; depth = hbo->fifoCtl.depth[id];
	i = hbo_queue_query(hdl, id, SemaQueryMap_ADDR_master_consumer, &p);
	if (i < n)
		return 0;

	if (ptr)
		p = *ptr;
	for (i = 0; i < n; i++) {
		IO32RD(data[i][0], base + p*8);
		IO32RD(data[i][1], base + p*8 + 4);
		ModInc(p, 1, depth);
	}
	hbo_queue_pop(hdl, id, n);
	if (ptr)
		*ptr = p;
	return n;
}

/*****************************************************************
 * Function: hbo_queue_write
 * Description: Write a number of 64b data & push FIFO to HBO SRAM.
 * void *hdl - Handle to HDL_hbo
 * SIGN32 id - Queue ID in $HBO
 * SIGN32 n - Number of 64b entries to write
 * T64b data[] - Write data
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * UNSG32 *ptr - Pass in current FIFO pointer (in 64b word) & receive
 *                updated new pointer, Pass NULL to read from HW
 * Return: UNSG32 Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
 * 0 if there're not sufficient space in FIFO
 ************************************************************************/
UNSG32 hbo_queue_write(void *hdl, SIGN32 id, SIGN32 n,
		T64b data[], T64b cfgQ[], UNSG32 *ptr)
{
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;
	SIGN32 i, p, depth;
	UNSG32 base, j = 0;

	base = hbo->mem + hbo->base[id]; depth = hbo->fifoCtl.depth[id];
	if (!ptr) {
		i = hbo_queue_query(hdl, id, SemaQueryMap_ADDR_master_producer, &p);
		if (i > depth - n)
			return 0;
	} else
		p = *ptr;

	for (i = 0; i < n; i++) {
		IO32CFG(cfgQ, j, base + p*8, data[i][0]);
		IO32CFG(cfgQ, j, base + p*8 + 4, data[i][1]);
		ModInc(p, 1, depth);
	}
	if (!cfgQ)
		hbo_queue_push(hdl, id, n);
	else {
		T32SemaHub_PUSH	push;

		push.u32 = 0; push.uPUSH_ID = id; push.uPUSH_delta = n;
		IO32CFG(cfgQ, j, hbo->fifoCtl.ra + RA_SemaHub_PUSH, push.u32);
	}
	if (ptr)
		*ptr = p;
	return j;
	/**	ENDOFFUNCTION: hbo_queue_write **/
}
/**	ENDOFSECTION **/

/** SECTION - API definitions for $dHubReg **/
/*****************************************************************
 * Function: dhub_hdl
 * Description: Initialize HDL_dhub with a $dHub BIU instance.
 * UNSG32 mem - Base address of dHub.HBO SRAM
 * UNSG32 ra - Base address of BIU instance of $dHub
 * void *hdl - Handle to HDL_dhub2d
 *******************************************************************/
void dhub_hdl(UNSG32 mem, UNSG32 ra, void *hdl)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	struct HDL_hbo *hbo = &(dhub->hbo);
	struct HDL_semaphore *semaHub = &(dhub->semaHub);

	dhub->ra = ra;
	semaphore_hdl(ra + RA_dHubReg_SemaHub, semaHub);
	hbo_hdl(mem, ra + RA_dHubReg_HBO, hbo);
}

/**************************************************************
 * Function: ispss_dhub_semaphore
 * Description: Get HDL_semaphore pointer from a dHub instance.
 * Return: void* Handle for dHub.SemaHub
 *************************************************************/
void *ispss_dhub_semaphore(void *hdl)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	struct HDL_semaphore *semaHub = &(dhub->semaHub);

	return semaHub;
}
EXPORT_SYMBOL(ispss_dhub_semaphore);

/*********************************************************
 * Function: dhub_hbo
 * Description: Get HDL_hbo pointer from a dHub instance.
 * Return: void* Handle for dHub.HBO
 ***********************************************************/
void *dhub_hbo(void *hdl)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	struct HDL_hbo *hbo = &(dhub->hbo);

	return hbo;
}

/***********************************************************************
 * Function: dhub_channel_axqos_isp
 * Description: Config QOS
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
 * 0 if either cmdQ or dataQ in HBO is still busy
 *************************************************************************/
UNSG32  dhub_channel_axqos_isp(
		void        *hdl,               /*! Handle to HDL_dhub !*/
		SIGN32      id,                 /*! Channel ID in $dHubReg !*/
		UNSG32      awQosLO,            /*! AWQOS value when low priority !*/
		UNSG32      awQosHI,            /*! AWQOS value when high priority !*/
		UNSG32      arQosLO,            /*! ARQOS value when low priority !*/
		UNSG32      arQosHI,            /*! ARQOS value when high priority !*/
		T64b        cfgQ[])
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	T32dHubChannel_AWQOS awQos;
	T32dHubChannel_ARQOS arQos;
	UNSG32 i = 0, a;

	pr_debug("hal_dhub::  value of id is %0d\n", id);

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);
	pr_debug("hal_dhub::  value of Channel Addr    is %0x\n", a);

	awQos.u32 = 0; awQos.uAWQOS_LO = awQosLO; awQos.uAWQOS_HI = awQosHI;
	pr_debug("hal_dhub::  addr of ChannelCFG is %0x data is %0x\n",
			a + RA_dHubChannel_AWQOS, awQos.u32);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_AWQOS, awQos.u32);

	arQos.u32 = 0; arQos.uARQOS_LO = arQosLO; arQos.uARQOS_HI = arQosHI;
	pr_debug("hal_dhub::  addr of ChannelCFG is %0x data is %0x\n",
			a + RA_dHubChannel_ARQOS, arQos.u32);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_ARQOS, arQos.u32);

	return i;
}

/**********************************************************************
 * Function: dhub_channel_cfg
 * Description: Configurate a dHub channel.
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
 * 0 if either cmdQ or dataQ in HBO is still busy
 ************************************************************************/
UNSG32	dhub_channel_cfg(
		void		*hdl, /*! Handle to HDL_dhub !*/
		SIGN32		id, /*! Channel ID in $dHubReg !*/
		UNSG32		baseCmd, /*! Channel FIFO base address (byte address) for cmdQ !*/
		UNSG32		baseData, /*! Channel FIFO base address (byte address) for dataQ !*/
		SIGN32		depthCmd, /*! Channel FIFO depth for cmdQ, in 64b word !*/
		SIGN32		depthData, /*! Channel FIFO depth for dataQ, in 64b word !*/
		SIGN32		MTU, /*! See 'dHubChannel.CFG.MTU' !*/
		SIGN32		QoS, /*! See 'dHubChannel.CFG.QoS' !*/
		SIGN32		selfLoop, /*! See 'dHubChannel.CFG.selfLoop' !*/
		SIGN32		enable, /*! 0 to disable, 1 to enable !*/
		T64b		cfgQ[])
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	struct HDL_hbo *hbo = &(dhub->hbo);
	T32dHubChannel_CFG	cfg;
	UNSG32 i = 0, a, busyStatus, cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	pr_debug("hal_dhub::  value of id is %0d\n", id);
	pr_debug("hal_dhub::  value of baseCmd   is %0d\n", baseCmd);
	pr_debug("hal_dhub::  value of baseData  is %0d\n", baseData);
	pr_debug("hal_dhub::  value of depthCmd  is %0d\n", depthCmd);
	pr_debug("hal_dhub::  value of depthData is %0d\n", depthData);
	pr_debug("hal_dhub::  value of MTU       is %0d\n", MTU);
	pr_debug("hal_dhub::  value of QOS       is %0d\n", QoS);
	pr_debug("hal_dhub::  value of SelfLoop  is %0d\n", selfLoop);
	pr_debug("hal_dhub::  value of Enable    is %0d\n", enable);

	if (!cfgQ) {
		hbo_queue_enable(hbo,  cmdID, 0, NULL);
		hbo_queue_clear(hbo,  cmdID);
		hbo_queue_enable(hbo, dataID, 0, NULL);
		hbo_queue_clear(hbo, dataID);
		busyStatus = hbo_queue_busy(hbo);
		//if (bTST(busyStatus, cmdID) || bTST(busyStatus, dataID))
		//	return 0;
	}
	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);
	pr_debug("hal_dhub::  value of Channel Addr    is %0x\n", a);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_START, 0);

	cfg.u32 = 0; cfg.uCFG_MTU = MTU; cfg.uCFG_QoS = QoS; cfg.uCFG_selfLoop = selfLoop;
	switch (MTU) {
	case dHubChannel_CFG_MTU_8byte:
		dhub->MTUb[id] = 3; break;
	case dHubChannel_CFG_MTU_32byte:
		dhub->MTUb[id] = 5; break;
	case dHubChannel_CFG_MTU_128byte:
		dhub->MTUb[id] = 7; break;
	case dHubChannel_CFG_MTU_4096byte:
		dhub->MTUb[id] = 12; break;
	}
	pr_debug("hal_dhub::  addr of ChannelCFG is %0x data is %0x\n",
			a + RA_dHubChannel_CFG, cfg.u32);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_CFG, cfg.u32);

	i += hbo_queue_cfg(hbo,  cmdID, baseCmd, depthCmd, enable, cfgQ ? (cfgQ + i) : NULL);
	i += hbo_queue_cfg(hbo, dataID, baseData, depthData, enable, cfgQ ? (cfgQ + i) : NULL);
	pr_debug("hal_dhub::  addr of ChannelEN is %0x data is %0x\n",
			a + RA_dHubChannel_START, enable);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_START, enable);

	return i;
}

/**********************************************************************
 * Function: dhub_channel_enable
 * Description: dHub channel enable/disable.
 * void *hdl - Handle to HDL_dhub2d
 * SIGN32 id - Channel ID in $dHubReg2D
 * SIGN32 enable - 0 to disable, 1 to enable
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 *****************************************************************************/
UNSG32 dhub_channel_enable(void *hdl, SIGN32 id,
		SIGN32 enable, T64b	cfgQ[])
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	UNSG32 i = 0, a;

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);

	IO32CFG(cfgQ, i, a + RA_dHubChannel_START, enable);
	return i;
}

/*******************************************************************
 * Function: dhub_channel_clear
 * Description: Issue dHub channel clear (will NOT wait for finish).
 * void *hdl Handle to HDL_dhub
 * SIGN32 id Channel ID in $dHubReg
 *****************************************************************/
void dhub_channel_clear(void *hdl, SIGN32 id)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	UNSG32 a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);

	IO32WR(1, a + RA_dHubChannel_CLEAR);
	/**	ENDOFFUNCTION: dhub_channel_clear **/
}

/***************************************************************************
 * Function: dhub_channel_flush
 * Description: Issue dHub channel (H2M only) flush (will NOT wait for finish).
 * void *hdl Handle to HDL_dhub
 * SIGN32 id Channel ID in $dHubReg
 ***************************************************************************/
void dhub_channel_flush(void *hdl, SIGN32 id)
{
	UNSG32 a;
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);

	IO32WR(1, a + RA_dHubChannel_FLUSH);
	/**	ENDOFFUNCTION: dhub_channel_flush **/
}

/*******************************************************************************
 * Function: dhub_channel_busy
 * Description: Read dHub 'BUSY' status for all channel FIFOs.
 * void	*hdl Handle to HDL_dhub
 * Return: UNSG32 'BUSY' status bits of all channels
 ****************************************************************************/
UNSG32	dhub_channel_busy(void *hdl)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	UNSG32 d;

	IO32RD(d, dhub->ra + RA_dHubReg_BUSY);
	return d;
	/**	ENDOFFUNCTION: dhub_channel_busy **/
}

/*****************************************************************
 * Function: dhub_channel_pending
 * Description: Read dHub 'PENDING' status for all channel FIFOs.
 * void	*hdl Handle to HDL_dhub
 * Return: UNSG32 'PENDING' status bits of all channels
 *****************************************************************/
UNSG32 dhub_channel_pending(void *hdl)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	UNSG32 d;

	IO32RD(d, dhub->ra + RA_dHubReg_PENDING);
	return d;
}

/**********************************************************************
 * Function: dhub_channel_clear_done
 * Description: Wait for a given channel or all channels to be cleared.
 * void	*hdl Handle to HDL_dhub
 * SIGN32 id Channel ID in $dHubReg -1 to wait for all channel clear done
 **********************************************************************/
void dhub_channel_clear_done(void *hdl, SIGN32 id)
{
	UNSG32 d;

	do {
		d = dhub_channel_busy(hdl);
		d |= dhub_channel_pending(hdl);
		if (id >= 0)
			d = bTST(d, id);
	} while (d);

	/**	ENDOFFUNCTION: dhub_channel_clear_done **/
}

/*****************************************************************
 * Function: dhub_channel_write_cmd
 * Description: Write a 64b command for a dHub channel.
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * UNSG32 *ptr - Pass in current cmdQ pointer (in 64b word) & receive
 *                updated new pointer, Pass NULL to read from HW
 * Return: UNSG32 Number of (adr,pair) added to cfgQ if success, or
 * 0 if there're not sufficient space in FIFO
 ******************************************************************/
UNSG32	dhub_channel_write_cmd(
		void *hdl,			/*! Handle to HDL_dhub !*/
		SIGN32 id,			/*! Channel ID in $dHubReg !*/
		UNSG32 addr,		/*! CMD: buffer address !*/
		SIGN32 size,		/*! CMD: number of bytes to transfer !*/
		SIGN32 semOnMTU,	/*! CMD: semaphore operation at CMD/MTU (0/1) !*/
		SIGN32 chkSemId,	/*! CMD: non-zero to check semaphore !*/
		SIGN32 updSemId,	/*! CMD: non-zero to update semaphore !*/
		SIGN32 interrupt,	/*! CMD: raise interrupt at CMD finish !*/
		T64b cfgQ[],
		UNSG32 *ptr
		)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	struct HDL_hbo *hbo = &(dhub->hbo);
	SIE_dHubCmd			cmd;
	SIGN32 i;

	cmd.ie_HDR.u32dHubCmdHDR_DESC = 0;
	i = size >> dhub->MTUb[id];
	if ((i << dhub->MTUb[id]) < size)
		cmd.ie_HDR.uDESC_size = size;
	else {
		cmd.ie_HDR.uDESC_sizeMTU = 1;
		cmd.ie_HDR.uDESC_size = i;
	}
	cmd.ie_HDR.uDESC_chkSemId = chkSemId; cmd.ie_HDR.uDESC_updSemId = updSemId;
	cmd.ie_HDR.uDESC_semOpMTU = semOnMTU; cmd.ie_HDR.uDESC_interrupt = interrupt;
	cmd.uMEM_addr = addr;

	return hbo_queue_write(hbo, dhub_id2hbo_cmdQ(id), 1, (T64b *)&cmd, cfgQ, ptr);
}

/*****************************************************************
 * Function: dhub_channel_generate_cmd
 * Description:
 * void *hdl - Handle to HDL_dhub
 * SIGN32 id - Channel ID in $dHubReg
 * UINT64 addr - buffer address
 * SIGN32 size - number of bytes to transfer
 * SIGN32 semOnMTU - semaphore operation at CMD/MTU (0/1)
 * SIGN32 chkSemId - non-zero to check semaphore
 * SIGN32 updSemId - non-zero to update semaphore
 * SIGN32 interrupt - raise interrupt at CMD finish
 ******************************************************************/
void dhub_channel_generate_cmd(void *hdl, SIGN32 id, UINT64 addr,
		SIGN32 size, SIGN32 semOnMTU, SIGN32 chkSemId,
		SIGN32 updSemId, SIGN32 interrupt, SIGN32 *pData)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	SIE_dHubCmd			cmd;
	SIGN32 i, *pcmd;

	cmd.ie_HDR.u32dHubCmdHDR_DESC = 0;
	i = size >> dhub->MTUb[id];
	if ((i << dhub->MTUb[id]) < size)
		cmd.ie_HDR.uDESC_size = size;
	else {
		cmd.ie_HDR.uDESC_sizeMTU = 1;
		cmd.ie_HDR.uDESC_size = i;
	}
	cmd.ie_HDR.uDESC_chkSemId = chkSemId; cmd.ie_HDR.uDESC_updSemId = updSemId;
	cmd.ie_HDR.uDESC_semOpMTU = semOnMTU; cmd.ie_HDR.uDESC_interrupt = interrupt;
	cmd.uMEM_addr = addr;

	pcmd = (SIGN32 *)(&cmd);
	pData[0] = pcmd[0];
	pData[1] = pcmd[1];
	/**	ENDOFFUNCTION: dhub_channel_write_cmd **/
}

/*********************************************************************
 * Function: dhub_channel_big_write_cmd
 * Description: Write a sequence of 64b command for a dHub channel.
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * UNSG32 *ptr - Pass in current cmdQ pointer (in 64b word) & receive
 *                updated new pointer, Pass NULL to read from HW
 * Return: UNSG32 Number of (adr,pair) added to cfgQ if success, or
 * 0 if there're not sufficient space in FIFO
 ********************************************************************/
UNSG32 dhub_channel_big_write_cmd(
		void	*hdl,		/*!	Handle to HDL_dhub !*/
		SIGN32	id,			/*!	Channel ID in $dHubReg !*/
		UNSG32	addr,		/*!	CMD: buffer address !*/
		SIGN32	size,		/*!	CMD: number of bytes to transfer !*/
		SIGN32	semOnMTU,	/*!	CMD: semaphore operation at CMD/MTU (0/1) !*/
		SIGN32	chkSemId,	/*!	CMD: non-zero to check semaphore !*/
		SIGN32	updSemId,	/*!	CMD: non-zero to update semaphore !*/
		SIGN32	interrupt,	/*!	CMD: raise interrupt at CMD finish !*/
		T64b	cfgQ[],
		UNSG32	*ptr)
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	SIGN32 i;
	SIGN32 j, jj;

	i = size >> dhub->MTUb[id];
	//size < 64K
	if (size < (1<<16)) {
		j = dhub_channel_write_cmd(hdl, id,
				addr, size, semOnMTU,
				chkSemId, updSemId, interrupt, cfgQ, ptr);
	} else {
		SIGN32 size0, size1;

		size0 = 0xffff << dhub->MTUb[id];
		j = 0;

		//size > 128x64k
		while (i > 0xffff) {
			jj = dhub_channel_write_cmd(hdl,
					id, addr, size0, semOnMTU,
					chkSemId, updSemId, 0, cfgQ, ptr);

			if (cfgQ)
				cfgQ += jj;
			j += jj;

			i -= 0xffff;
			size -= size0;
			addr += size0;
		}

		if ((i << dhub->MTUb[id]) == size)
			j += dhub_channel_write_cmd(hdl,
					id, addr, size, semOnMTU,
					chkSemId, updSemId, interrupt, cfgQ, ptr);
		else {
			size0 = i << dhub->MTUb[id];
			j += dhub_channel_write_cmd(hdl, id,
					addr, size0, semOnMTU,
					chkSemId, updSemId, 0, cfgQ, ptr);
			if (cfgQ)
				cfgQ += j;
			addr += size0;
			size1 = size - size0;
			j += dhub_channel_write_cmd(hdl, id,
					addr, size1, semOnMTU,
					chkSemId, updSemId, interrupt, cfgQ, ptr);
		}
	}

	return j;
}

/**	ENDOFSECTION **/

/**	SECTION - API definitions for $dHubReg2D **/
/******************************************************************
 * Function: dhub2d_hdl
 * Description: Initialize HDL_dhub2d with a $dHub2D BIU instance.
 * UNSG32 mem - Base address of dHub2D.dHub.HBO SRAM
 * UNSG32 ra - Base address of BIU instance of $dHub2D
 * void *hdl - Handle to HDL_dhub2d
 ******************************************************************/
void dhub2d_hdl(UNSG32 mem, UNSG32 ra, void *hdl)
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	struct HDL_dhub *dhub = &(dhub2d->dhub);

	dhub2d->ra = ra;
	dhub_hdl(mem, ra + RA_dHubReg2D_dHub, dhub);
}

/*****************************************************************
 * Function: dhub2d_channel_cfg
 * Description: Configurate a dHub2D channel.
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 ****************************************************************/
UNSG32 dhub2d_channel_cfg(
		void	*hdl,		/*!	Handle to HDL_dhub2d !*/
		SIGN32	id,			/*!	Channel ID in $dHubReg2D !*/
		UNSG32	addr,		/*!	CMD: 2D-buffer address !*/
		SIGN32	stride,		/*!	CMD: line stride size in bytes !*/
		SIGN32	width,		/*!	CMD: buffer width in bytes !*/
		SIGN32	height,		/*!	CMD: buffer height in lines !*/
		SIGN32	semLoop,	/*!	CMD: loop size (1~4) of semaphore operations !*/
		SIGN32	semOnMTU,	/*!	CMD: semaphore operation at CMD/MTU (0/1) !*/
		SIGN32	chkSemId[],	/*!	CMD: semaphore loop pattern - non-zero to check !*/
		SIGN32	updSemId[],	/*! CMD: semaphore loop pattern - non-zero to update !*/
		SIGN32	interrupt,	/*! CMD: raise interrupt at CMD finish !*/
		SIGN32	enable,		/*! 0 to disable, 1 to enable !*/
		T64b	cfgQ[])
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	struct HDL_dhub *dhub = &(dhub2d->dhub);
	SIE_dHubCmd2D		cmd;
	SIE_dHubCmdHDR		hdr;
	SIGN32 i, size = width;
	UNSG32 a, j = 0;

	a = dhub2d->ra + RA_dHubReg2D_ARR + id*sizeof(SIE_dHubCmd2D);
	IO32CFG(cfgQ, j, a + RA_dHubCmd2D_START, 0);

	cmd.uMEM_addr = addr;
	cmd.uDESC_stride = stride; cmd.uDESC_numLine = height;
	cmd.uDESC_hdrLoop = semLoop; cmd.uDESC_interrupt = interrupt;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2D_MEM, cmd.u32dHubCmd2D_MEM);
	IO32CFG(cfgQ, j, a + RA_dHubCmd2D_DESC, cmd.u32dHubCmd2D_DESC);

	hdr.u32dHubCmdHDR_DESC = 0;
	i = size >> dhub->MTUb[id];
	if ((i << dhub->MTUb[id]) < size)
		hdr.uDESC_size = size;
	else {
		hdr.uDESC_sizeMTU = 1;
		hdr.uDESC_size = i;
	}
	hdr.uDESC_semOpMTU = semOnMTU;
	for (i = 0; i < semLoop; i++) {
		if (chkSemId)
			hdr.uDESC_chkSemId = chkSemId[i];
		if (updSemId)
			hdr.uDESC_updSemId = updSemId[i];
		IO32CFG(cfgQ, j, a + RA_dHubCmd2D_HDR
				+ i * sizeof(SIE_dHubCmdHDR),
				hdr.u32dHubCmdHDR_DESC);
	}
	IO32CFG(cfgQ, j, a + RA_dHubCmd2D_START, enable);

	return j;
}

/************************************************************************
 * Function: dhub2d_channel_enable
 * Description: dHub2D channel enable/disable.
 * void *hdl - Handle to HDL_dhub2d
 * SIGN32 id - Channel ID in $dHubReg2D
 * SIGN32 enable - 0 to disable, 1 to enable
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 ************************************************************************/
UNSG32 dhub2d_channel_enable(void *hdl, SIGN32 id,
		SIGN32 enable, T64b cfgQ[])
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 i = 0, a;

	a = dhub2d->ra + RA_dHubReg2D_ARR + id*sizeof(SIE_dHubCmd2D);

	IO32CFG(cfgQ, i, a + RA_dHubCmd2D_START, enable);
	return i;
}

/******************************************************************
 * Function: dhub2d_channel_clear
 * Description: Issue dHub2D channel clear (will NOT wait for finish).
 * void *hdl - Handle to HDL_dhub2d
 * SIGN32 id - Channel ID in $dHubReg2D
 ******************************************************************/
void dhub2d_channel_clear(void *hdl, SIGN32 id)
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 a = dhub2d->ra + RA_dHubReg2D_ARR + id*sizeof(SIE_dHubCmd2D);

	IO32WR(1, a + RA_dHubCmd2D_CLEAR);
}

/******************************************************************
 * Function: ispss_dhub2nd_channel_cfg
 * Description: Configurate a dHub2ND channel.
 * void *hdl - Handle to HDL_dhub2d
 * SIGN32 id - Channel ID in $dHubReg2D
 * SIGN32 enable - 0 to disable, 1 to enable
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 *******************************************************************/
UNSG32 ispss_dhub2nd_channel_cfg(
		void	*hdl,		/*! Handle to HDL_dhub2d !*/
		SIGN32	id,			/*! Channel ID in $dHubReg2D !*/
		UNSG32	addr,		/*! CMD: 2ND-buffer address !*/
		SIGN32	burst,		/*! CMD: line stride size in bytes !*/
		SIGN32	step1,		/*! CMD: buffer width in bytes !*/
		SIGN32	size1,		/*! CMD: buffer height in lines !*/
		SIGN32	step2,	    /*! CMD: loop size (1~4) of semaphore operations !*/
		SIGN32	size2,		/*! CMD: semaphore operation at CMD/MTU (0/1) !*/
		SIGN32	chkSemId,	/*! CMD: semaphore loop pattern - non-zero to check !*/
		SIGN32	updSemId,	/*! CMD: semaphore loop pattern - non-zero to update !*/
		SIGN32	interrupt,	/*! CMD: raise interrupt at CMD finish !*/
		SIGN32	enable,		/*! 0 to disable, 1 to enable !*/
		T64b	cfgQ[])
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	SIE_dHubCmd2ND		cmd;
	SIGN32 semId_enable = 0;
	UNSG32 a, j = 0;
	T32dHubChannel_ROB_MAP stdHubChannelRob_Map;

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);

	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_START, 0);

	stdHubChannelRob_Map.u32 = 0;
	if (updSemId != 0)
		/* Assuming all Luma channel will be odd and chroma will be even */
		if (chkSemId & 0x1)
			stdHubChannelRob_Map.uROB_MAP_ID = 2;
		else
			stdHubChannelRob_Map.uROB_MAP_ID = 1;
	else
		stdHubChannelRob_Map.uROB_MAP_ID = 0;
	a = dhub2d->ra + RA_dHubReg2D_dHub + RA_dHubReg_ARR +
		id * sizeof(SIE_dHubChannel) + RA_dHubChannel_ROB_MAP;
	IO32CFG(cfgQ, j, a, stdHubChannelRob_Map.u32);

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);

	cmd.uMEM_addr = addr;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_MEM, cmd.u32dHubCmd2ND_MEM);

	if (updSemId == 0)
		semId_enable = 0;
	else
		semId_enable = 1;

	memset(&cmd, 0, sizeof(SIE_dHubCmd2ND));
	cmd.u32dHubCmd2ND_DESC = 0;
	cmd.uDESC_burst = burst;
	cmd.uDESC_interrupt = interrupt;
	cmd.uDESC_chkSemId = chkSemId;
	cmd.uDESC_updSemId = updSemId;
	if (updSemId != 0) {
		cmd.uDESC_ovrdQos = 1;
		cmd.uDESC_disSem  = 1;
		cmd.uDESC_qosSel  = 1;
	} else {
		cmd.uDESC_ovrdQos = 0;
		cmd.uDESC_disSem  = 0;
		cmd.uDESC_qosSel  = 0;
	}
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC, cmd.u32dHubCmd2ND_DESC);

	memset(&cmd, 0, sizeof(SIE_dHubCmd2ND));
	cmd.uDESC_1D_ST_step = step1;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_1D_ST, cmd.u32dHubCmd2ND_DESC_1D_ST);

	memset(&cmd, 0, sizeof(SIE_dHubCmd2ND));
	cmd.uDESC_1D_SZ_size = size1;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_1D_SZ, cmd.u32dHubCmd2ND_DESC_1D_SZ);

	memset(&cmd, 0, sizeof(SIE_dHubCmd2ND));
	cmd.uDESC_2D_ST_step = step2;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_2D_ST, cmd.u32dHubCmd2ND_DESC_2D_ST);

	memset(&cmd, 0, sizeof(SIE_dHubCmd2ND));
	cmd.uDESC_2D_SZ_size = size2;
	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_DESC_2D_SZ, cmd.u32dHubCmd2ND_DESC_2D_SZ);

	IO32CFG(cfgQ, j, a + RA_dHubCmd2ND_START, enable);

	return j;
	/**	ENDOFFUNCTION: ispss_dhub2nd_channel_cfg **/
}
EXPORT_SYMBOL(ispss_dhub2nd_channel_cfg);

/********************************************************************************
 * Function: dhub2nd_channel_enable
 * Description: dHub2ND channel enable/disable.
 * void *hdl - Handle to HDL_dhub2d
 * SIGN32 id - Channel ID in $dHubReg2D
 * SIGN32 enable - 0 to disable, 1 to enable
 * T64b cfgQ[] - Pass NULL to directly init dHub2D, or Pass non-zero to
 *                receive programming sequence in (adr,data) pairs
 * Return: UNSG32 Number of (adr,pair) added to cfgQ
 *******************************************************************************/
UNSG32 dhub2nd_channel_enable(void *hdl, SIGN32 id,
		SIGN32 enable, T64b cfgQ[])
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 i = 0, a;

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);

	IO32CFG(cfgQ, i, a + RA_dHubCmd2ND_START, enable);
	return i;
	/** ENDOFFUNCTION: dhub2nd_channel_enable **/
}

/***********************************************************************
 *	Function: dhub2nd_channel_clear
 *	Description: Issue dHub2ND channel clear (will NOT wait for finish).
 *  void *hdl - Handle to HDL_dhub2d
 *  SIGN32 id - Channel ID in $dHubReg2D
 *********************************************************************/
void dhub2nd_channel_clear(void *hdl, SIGN32 id)
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);

	IO32WR(1, a + RA_dHubCmd2ND_CLEAR);
}

/****************************************************************
 *	Function: dhub2d_channel_busy
 *	Description: Read dHub3D 'BUSY' status for all channel FIFOs.
 *  void *hdl - Handle to HDL_dhub2d
 *	Return: UNSG32 'BUSY' status bits of all channels
 ****************************************************************/
UNSG32 dhub2d_channel_busy(void *hdl)
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 d;

	IO32RD(d, dhub2d->ra + RA_dHubReg2D_BUSY);
	return d;
}

/************************************************************************
 *	Function: dhub2d_channel_clear_done
 *	Description: Wait for a given channel or all channels to be cleared.
 *  void *hdl - Handle to HDL_dhub2d
 *  SIGN32 id - Channel ID in $dHubReg2D
 ***********************************************************************/
void dhub2d_channel_clear_done(void	*hdl, SIGN32 id)
{
	UNSG32 d;

	do {
		d = dhub2d_channel_busy(hdl);
		if (id >= 0)
			d = bTST(d, id);
	} while (d);

	/**	ENDOFFUNCTION: dhub2d_channel_clear_done **/
}
UNSG32 dhub_channel_enable_InverseScan_vppBcm(void *hdl, SIGN32 id,
		SIGN32 iMode, UNSG32 pbcmbuf)
{
	return 0;
}



struct mutex gBCM_SCHED_lock;

void BCM_SCHED_Open(void)
{
	mutex_init(&gBCM_SCHED_lock);
}

void BCM_SCHED_Close(void)
{
	mutex_destroy(&gBCM_SCHED_lock);
}

void BCM_SchedSetMux(UNSG32 QID, UNSG32 TrigEvent)
{
	UNSG32 addr;
	UNSG32 bcmBaseAddr = getBcmBaseAddr();

	if (TrigEvent > BCM_SCHED_TRIG_NONE)
		return;

	if (QID <= BCM_SCHED_Q11) {
		addr = RA_ISP_BCM_BCM_Q0 + QID * 4;
		ISPSS_REG_WRITE32(bcmBaseAddr + addr, TrigEvent);
	} else if ((QID >= BCM_SCHED_Q14) && (QID <= BCM_SCHED_Q18)) {
		addr = RA_ISP_BCM_BCM_Q14 + (QID-BCM_SCHED_Q14) * 4;
		ISPSS_REG_WRITE32(bcmBaseAddr + addr, TrigEvent);
	}
}
EXPORT_SYMBOL(BCM_SchedSetMux);

int BCM_SCHED_AutoPushCmd(UNSG32 QID, UNSG8 uchEnable)
{
	volatile UNSG32 uiBcmQ_AutoPushSts = 0;
	UNSG32 bcmBaseAddr = getBcmBaseAddr();

	/* parameter error */
	if ((QID > BCM_SCHED_Q18) || (uchEnable > 1))
		return -1;

	//Read the BCM Auto Push register
	ISPSS_REG_READ32(bcmBaseAddr + RA_ISP_BCM_BCM_AUTOPUSH, &uiBcmQ_AutoPushSts);

	//Enable AutoPush for requested Queue.
	if (uchEnable)
		uiBcmQ_AutoPushSts |= (1 << QID);
	else
		uiBcmQ_AutoPushSts &= ~(1 << QID);

	mutex_lock(&gBCM_SCHED_lock);
	//Write to Register
	ISPSS_REG_WRITE32(bcmBaseAddr + RA_ISP_BCM_BCM_AUTOPUSH, uiBcmQ_AutoPushSts);

	mutex_unlock(&gBCM_SCHED_lock);
	return 0;
}


/**********************************************************************************
 *       Function: dhub_channel_enable_InverseScan
 *       Description: Inverse scan for dHub channel enable/disable.
 *		 SIGN32          id - Channel Number
 *		 SIGN32          iMode - Mode of scanninf
 *		 T64b            cfgQ[]	- Prepared command is update dto cfgQ,
 *                       Pass NULL to write directly to Dhub
 *       Return:                 UNSG32  -       Number of (adr,pair) added to cfgQ
 **********************************************************************************/
UNSG32 dhub_channel_enable_InverseScan(void *hdl, SIGN32 id,
		SIGN32 iMode, T64b cfgQ[])
{
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;
	T32dHubChannel_CFG      cfg;
	UNSG32 i = 0, a;

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);

	//Get the configuration of channel
	getDhubChannelInfo(hdl, id, &cfg);

	switch (iMode) {
	//Normal Scanning
	case 0:
		cfg.uCFG_hScan = 0; cfg.uCFG_vScan = 0;
		break;
		//Only H inverse scan
	case 1:
		cfg.uCFG_hScan = 1;
		break;
		//Only V inverse Scan
	case 2:
		cfg.uCFG_vScan = 1;
		break;
	case 3:
		//Both HV inverse
		cfg.uCFG_hScan = 1; cfg.uCFG_vScan = 1;
		break;
	}

	IO32CFG(cfgQ, i, a + RA_dHubChannel_CFG, cfg.u32);
	return i;
}

int BCM_SCHED_PushCmd(UNSG32 QID, UNSG32 *pCmd, UINT64 *cfgQ)
{
	UNSG32 value, addr, j;
	UNSG32 bcmBaseAddr = getBcmBaseAddr();
	UINT64 val = 0;

	if ((QID > BCM_SCHED_Q18) || !pCmd)
		return -1; /* parameter error */

	if (!cfgQ) {
		ISPSS_REG_READ32(bcmBaseAddr + RA_ISP_BCM_BCM_FULL_STS, &value);
		if (value & (1 << QID))
			return 0; /* Q FIFO is full */
	}

	if (pCmd) {
		j = 0;
		addr = QID * 4 * 2;
		if (cfgQ) {
			if (getBcmDhubType()) {
				cfgQ[j*2+0] = 0x0;
				cfgQ[j*2+1] = getBcmDummyRegAddr();
				j++;
			}
			val = bcmBaseAddr + addr;
			cfgQ[j] = val << 32 | pCmd[0];
			j++;
			val = 0;
			if (getBcmDhubType()) {
				cfgQ[j*2+0] = 0x0;
				cfgQ[j*2+1] = getBcmDummyRegAddr();
				j++;
			}
			val = bcmBaseAddr + addr + 4;
			cfgQ[j] = val << 32 | pCmd[1];
			j++;
		} else {
			mutex_lock(&gBCM_SCHED_lock);
			ISPSS_REG_WRITE32(bcmBaseAddr + addr, pCmd[0]);
			ISPSS_REG_WRITE32(bcmBaseAddr + addr + 4, pCmd[1]);
			mutex_unlock(&gBCM_SCHED_lock);
			j = 2;
		}
	}

	return j;
}

void BCM_SCHED_GetEmptySts(UNSG32 QID, UNSG32 *EmptySts)
{
	UNSG32 bcmQ_sts;
	UNSG32 bcmBaseAddr = getBcmBaseAddr();

	ISPSS_REG_READ32(bcmBaseAddr + RA_ISP_BCM_BCM_EMP_STS, &bcmQ_sts);
	if (bcmQ_sts & (1 << QID))
		*EmptySts = 1;
	else
		*EmptySts = 0;
}

void BCM_SCHED_GetFullSts(UNSG32 QID, UNSG32 *FullSts)
{
	UNSG32 bcmQ_sts;
	UNSG32 bcmBaseAddr = getBcmBaseAddr();

	ISPSS_REG_READ32(bcmBaseAddr + RA_ISP_BCM_BCM_FULL_STS, &bcmQ_sts);
	if (bcmQ_sts & (1 << QID))
		*FullSts = 1;
	else
		*FullSts = 0;
}
EXPORT_SYMBOL(BCM_SCHED_GetFullSts);

void BCM_SCHED_Flush(UNSG32 mask)
{
	UNSG32 bcmBaseAddr = getBcmBaseAddr();

	ISPSS_REG_WRITE32(bcmBaseAddr + RA_ISP_BCM_BCM_FLUSH, mask);
}
EXPORT_SYMBOL(BCM_SCHED_Flush);

/****************************************************
 *  dhub2d_channel_clear_seq()
 *
 *  dHub2D channel clear sequence
 ****************************************************/
void dhub2d_channel_clear_seq(void *hdl, SIGN32 id)
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	dhub2d_channel_clear((struct HDL_dhub2d *)hdl, id);
	dhub2d_channel_clear_done((struct HDL_dhub2d *)hdl, id);

	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 0, NULL);

	dhub_channel_enable(&((struct HDL_dhub2d *)hdl)->dhub, id, 0, NULL);
	dhub_channel_clear(&((struct HDL_dhub2d *)hdl)->dhub, id);
	dhub_channel_clear_done(&((struct HDL_dhub2d *)hdl)->dhub, id);

	hbo_queue_clear(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID);
	hbo_queue_clear_done(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID);

	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 0, NULL);
	hbo_queue_clear(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID);
	hbo_queue_clear_done(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID);
}

/****************************************************
 *  dhub2d_channel_start_seq()
 *
 *  dHub2D channel start sequence
 ****************************************************/
void dhub2d_channel_start_seq(void *hdl, SIGN32 id)
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 1, NULL);
	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 1, NULL);

	dhub_channel_enable(&((struct HDL_dhub2d *)hdl)->dhub, id, 1, NULL);
}

/****************************************************
 *  dhub2d_channel_clear_seq()
 *
 *  dHub2D channel clear sequence
 ****************************************************/
void dhub2nd_channel_clear_seq(void *hdl, SIGN32 id)
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	dhub2nd_channel_clear((struct HDL_dhub2d *)hdl, id);
	dhub2d_channel_clear_done((struct HDL_dhub2d *)hdl, id);

	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 0, NULL);

	dhub_channel_enable(&((struct HDL_dhub2d *)hdl)->dhub, id, 0, NULL);
	dhub_channel_clear(&((struct HDL_dhub2d *)hdl)->dhub, id);
	dhub_channel_clear_done(&((struct HDL_dhub2d *)hdl)->dhub, id);

	hbo_queue_clear(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID);
	hbo_queue_clear_done(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID);

	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 0, NULL);
	hbo_queue_clear(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID);
	hbo_queue_clear_done(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID);
}

/****************************************************
 *  dhub2d_channel_start_seq()
 *
 *  dHub2D channel start sequence
 ****************************************************/
void dhub2nd_channel_start_seq(void *hdl, SIGN32 id)
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 1, NULL);
	hbo_queue_enable(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 1, NULL);

	dhub_channel_enable(&((struct HDL_dhub2d *)hdl)->dhub, id, 1, NULL);
}


/**************************************************************************
 *  DHub/HBO clear/disable/enable functions
 *  Same as original ones, but use BCM buffer for VIP VBI clear sequence.
 **************************************************************************/
void bcm_buffer_write(void *pBcmBuf, UNSG32 regAddr, UNSG32 regVal)
{
	struct BCMBUF *pbcmbuf = pBcmBuf;
	UINT64 val = 0;

	if (getBcmDhubType()) {
		//FIXME handle u64 handling just like below
		//128b BCM with MSB 64b ignored
		UNSG32 regAddr_temp = getBcmDummyRegAddr();
		UNSG32 regVal_temp = 0x0;
		*pbcmbuf->writer = regVal_temp;
		pbcmbuf->writer++;
		*pbcmbuf->writer = regAddr_temp;
		pbcmbuf->writer++;
	}

	val = regAddr;
	val = (val << 32) | (regVal);
	*pbcmbuf->writer = val;
	pbcmbuf->writer++;
}

int bcm_buffer_dummy_write(void *pBcmBuf, UNSG32 len)
{
	int retVal = 0;
	struct BCMBUF *pbcmbuf = pBcmBuf;
	UINT64 val = 0;

	if (len && len%4) {
		UNSG32 regAddr = getBcmDummyRegAddr();
		UNSG32 regVal = 0x0;

		//Add a dummy register write into BCMBUF
		/*save the data to the buffer*/
		val = regAddr;
		val = (val << 32) | (regVal);
		*pbcmbuf->writer = val;
		pbcmbuf->writer++;

		retVal = 1;
	}

	return retVal;
}

int raw_bcm_buffer_dummy_write(UINT64 *rawBcmBufferBase, UNSG32 len)
{
	int retVal = 0;
	UINT64 val = 0;

	if (len && len%2) {
		//Check if the buffer contains multiple of 2 reg,val pair (i.e 4*32b = 128b)
		UNSG32 regAddr = getBcmDummyRegAddr();
		UNSG32 regVal = 0x0;

		val = regAddr;
		val = (val << 32) | (regVal);
		rawBcmBufferBase += len;
		*rawBcmBufferBase = val;
		rawBcmBufferBase++;

		retVal = 1;
	}

	return retVal;
}

/*
 * void	*hdl Handle to struct HDL_dhub
 * SIGN32 id Channel ID in $dHubReg
 * BCMBUF *pbcmbuf
 */
void dhub_channel_clear_bcmbuf(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf)
{
	UNSG32 a;
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);

	if (pbcmbuf == NULL)
		return;

	/*save the data to the buffer*/
	bcm_buffer_write(pbcmbuf, a + RA_dHubChannel_CLEAR, 1);
}

/*
 * void	*hdl Handle to HDL_dhub
 * SIGN32 id Channel ID in $dHubReg
 * SIGN32 enable  0 to disable, 1 to enable
 * BCMBUF *pbcmbuf
 */
UNSG32 dhub_channel_enable_bcmbuf(void *hdl, SIGN32 id,
		SIGN32 enable, struct BCMBUF *pbcmbuf)
{
	UNSG32 i = 0, a;
	struct HDL_dhub *dhub = (struct HDL_dhub *)hdl;

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);

	bcm_buffer_write(pbcmbuf, a + RA_dHubChannel_START, enable);

	return i;
}

/*
 * void	*hdl Handle to HDL_hbo
 * SIGN32 Queue ID in $HBO
 * BCMBUF *pbcmbuf
 */
void hbo_queue_clear_bcmbuf(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf)
{
	UNSG32 a;
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;

	a = hbo->ra + RA_HBO_ARR + id*sizeof(SIE_FiFo);

	if (pbcmbuf == NULL)
		return;

	/*save the data to the buffer*/
	bcm_buffer_write(pbcmbuf, a + RA_FiFo_CLEAR, 1);
}

/*
 * void	*hdl Handle to HDL_hbo
 * SIGN32 Queue ID in $HBO
 * SIGN32 enable  0 to disable, 1 to enable
 * BCMBUF *pbcmbuf
 */
UNSG32 hbo_queue_enable_bcmbuf(void *hdl, SIGN32 id,
		SIGN32 enable, struct BCMBUF *pbcmbuf)
{
	UNSG32 i = 0, a;
	struct HDL_hbo *hbo = (struct HDL_hbo *)hdl;

	a = hbo->ra + RA_HBO_ARR + id*sizeof(SIE_FiFo);

	bcm_buffer_write(pbcmbuf, a + RA_FiFo_START, enable);
	return i;
}

/*
 * void	*hdl Handle to HDL_dhub2d
 * SIGN32 id Channel ID in $dHubReg2D
 * SIGN32 enable  0 to disable, 1 to enable
 * BCMBUF *pbcmbuf
 */
void dhub2d_channel_enable_bcmbuf(void *hdl, SIGN32 id,
		SIGN32 enable, struct BCMBUF *pbcmbuf)
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 a =  dhub2d->ra + RA_dHubReg2D_ARR + id*sizeof(SIE_dHubCmd2D);

	/*save the data to the buffer*/
	bcm_buffer_write(pbcmbuf, a + RA_dHubCmd2D_START, enable);
}

/*
 * void	*hdl Handle to HDL_dhub2d
 * SIGN32 id Channel ID in $dHubReg2D
 * BCMBUF *pbcmbuf
 */
void dhub2d_channel_clear_bcmbuf(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf)
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 a;

	a = dhub2d->ra + RA_dHubReg2D_ARR + id*sizeof(SIE_dHubCmd2D);

	/*save the data to the buffer*/
	bcm_buffer_write(pbcmbuf, a + RA_dHubCmd2D_CLEAR, 1);
}

/****************************************************************************
 * void		*hdl,				Handle to HDL_dhub2d !
 * SIGN32		id,
 * BCMBUF *pbcmbuf
 */
void dhub2nd_channel_clear_bcmbuf(void	*hdl, SIGN32 id,
		struct BCMBUF *pbcmbuf)
{
	struct HDL_dhub2d *dhub2d = (struct HDL_dhub2d *)hdl;
	UNSG32 a;

	a = dhub2d->ra + RA_dHubReg2D_ARR_2ND + id*sizeof(SIE_dHubCmd2ND);

	/*save the data to the buffer*/
	bcm_buffer_write(pbcmbuf, a + RA_dHubCmd2ND_CLEAR, 1);
}

/****************************************************
 *  dhub2d_channel_clear_seq()
 *
 *  dHub 2D channel clear sequence
 *  No wait, return immediately. Use BCM buffer.
 ****************************************************/
void dhub2d_channel_clear_seq_bcm(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf)
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	dhub2d_channel_clear_bcmbuf((struct HDL_dhub2d *)hdl, id, pbcmbuf);

	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 0, pbcmbuf);

	dhub_channel_enable_bcmbuf(&((struct HDL_dhub2d *)hdl)->dhub, id, 0, pbcmbuf);
	dhub_channel_clear_bcmbuf(&((struct HDL_dhub2d *)hdl)->dhub, id, pbcmbuf);

	hbo_queue_clear_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, pbcmbuf);

	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 0, pbcmbuf);
	hbo_queue_clear_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, pbcmbuf);

	//restart!
	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 1, pbcmbuf);
	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 1, pbcmbuf);
	dhub_channel_enable_bcmbuf(&((struct HDL_dhub2d *)hdl)->dhub, id, 1, pbcmbuf);
}

/****************************************************
 *  dhub_channel_clear_seq()
 *
 *  dHub channel clear sequence
 *  No wait, return immediately. Use BCM buffer.
 ****************************************************/
void dhub_channel_clear_seq(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf)
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	hbo_queue_enable_bcmbuf(dhub_hbo(hdl), cmdID, 0, pbcmbuf);

	dhub_channel_enable_bcmbuf(hdl, id, 0, pbcmbuf);

	dhub_channel_clear_bcmbuf(hdl, id, pbcmbuf);

	hbo_queue_clear_bcmbuf(dhub_hbo(hdl), cmdID, pbcmbuf);

	hbo_queue_enable_bcmbuf(dhub_hbo(hdl), dataID, 0, pbcmbuf);

	hbo_queue_clear_bcmbuf(dhub_hbo(hdl), dataID, pbcmbuf);

	//restart!
	hbo_queue_enable_bcmbuf(dhub_hbo(hdl), cmdID, 1, pbcmbuf);
	hbo_queue_enable_bcmbuf(dhub_hbo(hdl), dataID, 1, pbcmbuf);
	dhub_channel_enable_bcmbuf(hdl, id, 1, pbcmbuf);
}

/****************************************************
 *  dhub2d_channel_clear_seq()
 *
 *  dHub 2D channel clear sequence
 *  No wait, return immediately. Use BCM buffer.
 ****************************************************/
void dhub2nd_channel_clear_seq_bcm(void *hdl, SIGN32 id, struct BCMBUF *pbcmbuf)
{
	UNSG32 cmdID = dhub_id2hbo_cmdQ(id), dataID = dhub_id2hbo_data(id);

	dhub2nd_channel_clear_bcmbuf((struct HDL_dhub2d *)hdl, id, pbcmbuf);

	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 0, pbcmbuf);

	dhub_channel_enable_bcmbuf(&((struct HDL_dhub2d *)hdl)->dhub, id, 0, pbcmbuf);
	dhub_channel_clear_bcmbuf(&((struct HDL_dhub2d *)hdl)->dhub, id, pbcmbuf);

	hbo_queue_clear_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, pbcmbuf);

	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 0, pbcmbuf);
	hbo_queue_clear_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, pbcmbuf);

	//restart!
	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), cmdID, 1, pbcmbuf);
	hbo_queue_enable_bcmbuf(dhub_hbo(&((struct HDL_dhub2d *)hdl)->dhub), dataID, 1, pbcmbuf);
	dhub_channel_enable_bcmbuf(&((struct HDL_dhub2d *)hdl)->dhub, id, 1, pbcmbuf);
}

/**	ENDOFSECTION **/
/**	ENDOFFILE: hal_dhub.c **/
