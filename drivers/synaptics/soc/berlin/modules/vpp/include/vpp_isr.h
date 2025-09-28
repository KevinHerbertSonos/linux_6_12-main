// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2023 Synaptics Incorporated */

#ifndef _VPP_ISR_H_
#define _VPP_ISR_H_

#include "vpp_defines.h"

void VPP_CreateISRTask(void);
void VPP_StopISRTask(void);
void VPP_EnableDhubInterrupt(bool enable);
int VPP_PushFrameToInputQueue(int planeId, int msgID, VBUF_INFO *pFrameInfo);
#endif //_VPP_ISR_H_
