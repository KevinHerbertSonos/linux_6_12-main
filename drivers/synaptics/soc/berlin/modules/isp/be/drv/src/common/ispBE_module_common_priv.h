/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPBE_MODULE_COMMON_PRIV_H_
#define _ISPBE_MODULE_COMMON_PRIV_H_

struct ISPBE_CLIENT_QUEUE {
	/* rqst frames received from a client are queued here */
	struct ISP_REQUEST_QUEUE  InRqstQ;
	struct ISP_REQUEST_QUEUE  *OutRqstQ;
	struct ISP_REQUEST_QUEUE  *ProcessRqstQ;

	UINT   InputQCnt; //Number frame waiting in m_InFrameQ.
	UINT   ProcessQCnt; //Number frame waiting in m_ProcessFrameQ.
	UINT   OutputQCnt; //Number frame waiting in m_OutFrameQ.
};

#endif //_ISPBE_MODULE_COMMON_PRIV_H_
