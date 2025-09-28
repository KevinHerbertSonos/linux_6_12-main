/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _BCMBUF_DEBUG_H_
#define _BCMBUF_DEBUG_H_

#include "com_type.h"
//#include "OSAL_api.h"

/*********************************************************************
 * FUNCTION: convert BCMBUF into raw pointers
 * PARAMS: *pbcmbuf - pointer to the BCMBUF
 *         *start - pointer to the start of the BCM buffer
 *         *size - pointer to the size of the BCM buffer in bytes
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_BCMBUF_To_Raw(struct BCMBUF *pbcmbuf, UINT64 **start, INT32 *size);

/*********************************************************************
 * FUNCTION: Do direct register write from raw pointer
 * PARAMS: *pdata - pointer to the (val,addr) register address/value pair
 *         length - size of the register address/value pair buffer in bytes
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_BCMBUF_Raw_DirectWrite(UINT32 *pdata, UINT32 length);

/*********************************************************************
 * FUNCTION: Do direct register write from BCMBUF
 * PARAMS: *pbcmbuf- pointer to the BCMBUF containing (val,addr) register address/value pair
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_BCMBUF_DirectWrite(struct BCMBUF *pbcmbuf);

/*********************************************************************
 * FUNCTION: Do direct register write from CFGQ
 * PARAMS: *pCfgQ - pointer to the pCfgQ containing (val,addr) register address/value pair
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_CFGQ_DirectWrite(struct DHUB_CFGQ *pCfgQ);

/*********************************************************************
 * FUNCTION: log the register address/value pair stored in  raw pointer
 * PARAMS: *pdata - pointer to the (val,addr) register address/value pair
 *         length - size of the register address/value pair buffer in bytes
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_BCMBUF_Raw_LogPrint(UINT64 *pdata, UINT32 length);

/*********************************************************************
 * FUNCTION: log the register address/value pair stored in BCMBUF
 * PARAMS: *pbcmbuf- pointer to the BCMBUF containing (val,addr) register address/value pair
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_BCMBUF_LogPrint(struct BCMBUF *pbcmbuf);

/*********************************************************************
 * FUNCTION: log the register address/value pair stored in CFGQ
 * PARAMS: *pCfgQ - pointer to the pCfgQ containing (val,addr) register address/value pair
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_CFGQ_LogPrint(struct DHUB_CFGQ *pCfgQ);

/*********************************************************************
 * FUNCTION: Allocate BCMBUF and perform BCM test for the given triplet (addr, setVal, GetVal)
 * PARAMS: *ProgramRegVal - pointer to the triplet (addr, setVal, GetVal)
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_BCMBUF_BCMTest(UINT32 ProgramRegVal[][3], UINT32 nRegs, int blockFlag);

/*********************************************************************
 * FUNCTION: wrapper to the BCM unit test function - construct the buffer
 * and invoke the BCM unit test function
 * PARAMS: startAddr - start address of the register to be used for BCM unit test
 * nRegs - Number of register to be unit tested in BCM unit test
 * NOTE: this API is only called for debug purpose
 ********************************************************************/
INT ISPSS_BCMBUF_BCMTest_UnitTest(UINT32 startAddr, UINT32 nRegs, int blockFlag, int loopCount);

void ISPSS_BCMBUF_DhubConfig_LogPrint(void);
#endif
