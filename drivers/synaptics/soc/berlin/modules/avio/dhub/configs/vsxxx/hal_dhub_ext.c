// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated
 *
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *      http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/export.h>
#include <linux/kernel.h>

#include "ctypes.h"
#include "avio_io.h"

#include "avio_dhub_cfg.h"
#include "avio_dhub_cfg_prv.h"
#include "hal_dhub.h"
#include "hal_dhub_wrap.h"
#include "avio_dhub_drv.h"
#include "drv_dhub.h"


/******************************************************************************************************************
 *   Function: dhub_channel_axqos
 *   Description: Configurate ax QoS channel.
 *   Return:         UNSG32                      -   Number of (adr,pair) added to cfgQ, or (when cfgQ==NULL)
 *                                                   0 if either cmdQ or dataQ in HBO is still busy
 ******************************************************************************************************************/
UNSG32  dhub_channel_axqos(
		void        *hdl,               /*! Handle to HDL_dhub !*/
		SIGN32      id,                 /*! Channel ID in $dHubReg !*/
		UNSG32      awQosLO,            /*! AWQOS value when low priority !*/
		UNSG32      awQosHI,            /*! AWQOS value when high priority !*/
		UNSG32      arQosLO,            /*! ARQOS value when low priority !*/
		UNSG32      arQosHI,            /*! ARQOS value when high priority !*/
		T64b        cfgQ[]              /*! Pass NULL to directly init dHub, or
						Pass non-zero to receive programming sequence
						in (adr,data) pairs
						!*/
		)
{
	HDL_dhub *dhub = (HDL_dhub*)hdl;
	T32dHubChannel_AWQOS awQos;
	T32dHubChannel_ARQOS arQos;
	UNSG32 i = 0, a;

	xdbg ("hal_dhub::  value of id is %0d \n" , id);

	a = dhub->ra + RA_dHubReg_ARR + id*sizeof(SIE_dHubChannel);
	xdbg ("hal_dhub::  value of Channel Addr    is %0x \n" , a);

	awQos.u32 = 0; awQos.uAWQOS_LO = awQosLO; awQos.uAWQOS_HI = awQosHI;
	xdbg ("hal_dhub::  addr of ChannelCFG is %0x data is %0x \n" , a + RA_dHubChannel_AWQOS , awQos.u32);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_AWQOS, awQos.u32);

	arQos.u32 = 0; arQos.uARQOS_LO = arQosLO; arQos.uARQOS_HI = arQosHI;
	xdbg ("hal_dhub::  addr of ChannelCFG is %0x data is %0x \n" , a + RA_dHubChannel_ARQOS , arQos.u32);
	IO32CFG(cfgQ, i, a + RA_dHubChannel_ARQOS, arQos.u32);

	return i;
	/** ENDOFFUNCTION: dhub_channel_axqos **/
}
