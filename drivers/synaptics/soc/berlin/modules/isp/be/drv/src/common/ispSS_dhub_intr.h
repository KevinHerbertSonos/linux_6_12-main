/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ISPSS_DHUB_INTR_H__
#define __ISPSS_DHUB_INTR_H__

//Enable debug log in the DHUB ISR and other modules
#define ISPSS_DEBUG_LOG_MODULE_DHUB_ENABLE
#define ISPSS_DEBUG_LOG_MODULE_DWP_ENABLE
#define ISPSS_DEBUG_LOG_MODULE_ROT_ENABLE
#define ISPSS_DEBUG_LOG_MODULE_SCL_ENABLE
#define ISPSS_DEBUG_LOG_MODULE_TILE_ENABLE
#define ISPSS_DEBUG_LOG_MODULE_CORE_ENABLE
#define ISPSS_DEBUG_LOG_MODULE_CSI_ENABLE

enum ISPSS_DEBUG_LOG_MODULE {
	ISPSS_DEBUG_LOG_MODULE_DHUB = 1,
	ISPSS_DEBUG_LOG_MODULE_DWP,
	ISPSS_DEBUG_LOG_MODULE_ROT,
	ISPSS_DEBUG_LOG_MODULE_SCL,
	ISPSS_DEBUG_LOG_MODULE_TILE,
	ISPSS_DEBUG_LOG_MODULE_CORE,
	ISPSS_DEBUG_LOG_MODULE_CSI,
};

#define IS_ISPSS_DEBUG_LATE_INTR_CLEAR(flag)        (flag & 0x01)
#define IS_ISPSS_DEBUG_LOG_ENABLE(flag, module)     (flag & (0x01 << module))
#define IS_ISPSS_DEBUG_LOG_REG_ENABLE(flag, module) (flag & (0x10000 << module))
#define _IS_ISPSS_DEBUG_LATE_INTR_CLEAR() \
	IS_ISPSS_DEBUG_LATE_INTR_CLEAR(g_ispssDhubContext.clearAfterIntHandlerFlag)
#define _IS_ISPSS_DEBUG_LOG_ENABLE(module) \
	IS_ISPSS_DEBUG_LOG_ENABLE(g_ispssDhubContext.clearAfterIntHandlerFlag, module)
#define _IS_ISPSS_DEBUG_LOG_REG_ENABLE(module) \
	IS_ISPSS_DEBUG_LOG_REG_ENABLE(g_ispssDhubContext.clearAfterIntHandlerFlag, module)
#define _IS_ISPSS_DEBUG_LOG_REG_PROCESS(module)
#define _IS_ISPSS_DEBUG_LOG_ENABLE_DHUB() \
	_IS_ISPSS_DEBUG_LOG_ENABLE(ISPSS_DEBUG_LOG_MODULE_DHUB)
#define _IS_ISPSS_DEBUG_LOG_ENABLE_DWP() \
	_IS_ISPSS_DEBUG_LOG_ENABLE(ISPSS_DEBUG_LOG_MODULE_DHUB)
#define _IS_ISPSS_DEBUG_LOG_ENABLE_ROT() \
	_IS_ISPSS_DEBUG_LOG_ENABLE(ISPSS_DEBUG_LOG_MODULE_DWP)
#define _IS_ISPSS_DEBUG_LOG_ENABLE_SCL() \
	_IS_ISPSS_DEBUG_LOG_ENABLE(ISPSS_DEBUG_LOG_MODULE_SCL)
#define _IS_ISPSS_DEBUG_LOG_ENABLE_TILE() \
	_IS_ISPSS_DEBUG_LOG_ENABLE(ISPSS_DEBUG_LOG_MODULE_TILE)
#define _IS_ISPSS_DEBUG_LOG_ENABLE_CORE() \
	_IS_ISPSS_DEBUG_LOG_ENABLE(ISPSS_DEBUG_LOG_MODULE_CORE)
#define _IS_ISPSS_DEBUG_LOG_ENABLE_CSI() \
	_IS_ISPSS_DEBUG_LOG_ENABLE(ISPSS_DEBUG_LOG_MODULE_CSI)

#ifdef ISPSS_DEBUG_LOG_MODULE_DHUB_ENABLE
	#define ISPSS_DEBUG_LOG_DHUB(...) \
		do { \
			if (_IS_ISPSS_DEBUG_LOG_ENABLE_DHUB()) { \
				pr_debug("%s:%d: ", __func__, __LINE__); \
				pr_debug(__VA_ARGS__); \
			} \
			_IS_ISPSS_DEBUG_LOG_REG_PROCESS(ISPSS_DEBUG_LOG_MODULE_DHUB); \
		} while (0)
#else //ISPSS_DEBUG_LOG_MODULE_DHUB_ENABLE
	#define ISPSS_DEBUG_LOG_DHUB(...)
#endif //ISPSS_DEBUG_LOG_MODULE_DHUB_ENABLE

#ifdef ISPSS_DEBUG_LOG_MODULE_DWP_ENABLE
	#define ISPSS_DEBUG_LOG_DWP(...) \
		do { \
			if (_IS_ISPSS_DEBUG_LOG_ENABLE_DWP()) { \
				pr_debug("%s:%d: ", __func__, __LINE__); \
				pr_debug(__VA_ARGS__); \
			} \
			_IS_ISPSS_DEBUG_LOG_REG_PROCESS(ISPSS_DEBUG_LOG_MODULE_DWP); \
		} while (0)
#else //ISPSS_DEBUG_LOG_MODULE_DWP_ENABLE
	#define ISPSS_DEBUG_LOG_DWP(...)
#endif //ISPSS_DEBUG_LOG_MODULE_DWP_ENABLE

#ifdef ISPSS_DEBUG_LOG_MODULE_ROT_ENABLE
	#define ISPSS_DEBUG_LOG_ROT(...) \
		do { \
			if (_IS_ISPSS_DEBUG_LOG_ENABLE_ROT()) { \
				pr_debug("%s:%d: ", __func__, __LINE__); \
				pr_debug(__VA_ARGS__); \
			} \
			_IS_ISPSS_DEBUG_LOG_REG_PROCESS(ISPSS_DEBUG_LOG_MODULE_ROT); \
		} while (0)
#else //ISPSS_DEBUG_LOG_MODULE_ROT_ENABLE
	#define ISPSS_DEBUG_LOG_ROT(...)
#endif //ISPSS_DEBUG_LOG_MODULE_ROT_ENABLE

#ifdef ISPSS_DEBUG_LOG_MODULE_SCL_ENABLE
	#define ISPSS_DEBUG_LOG_SCL(...) \
		do { \
			if (_IS_ISPSS_DEBUG_LOG_ENABLE_SCL()) { \
				pr_debug("%s:%d: ", __func__, __LINE__); \
				pr_debug(__VA_ARGS__); \
			} \
			_IS_ISPSS_DEBUG_LOG_REG_PROCESS(ISPSS_DEBUG_LOG_MODULE_SCL); \
		} while (0)
#else //ISPSS_DEBUG_LOG_MODULE_SCL_ENABLE
	#define ISPSS_DEBUG_LOG_SCL(...)
#endif //ISPSS_DEBUG_LOG_MODULE_SCL_ENABLE

#ifdef ISPSS_DEBUG_LOG_MODULE_TILE_ENABLE
	#define ISPSS_DEBUG_LOG_TILE(...) \
		do { \
			if (_IS_ISPSS_DEBUG_LOG_ENABLE_TILE()) { \
				pr_debug("%s:%d: ", __func__, __LINE__); \
				pr_debug(__VA_ARGS__); \
			} \
			_IS_ISPSS_DEBUG_LOG_REG_PROCESS(ISPSS_DEBUG_LOG_MODULE_TILE); \
		} while (0)
#else //ISPSS_DEBUG_LOG_MODULE_TILE_ENABLE
	#define ISPSS_DEBUG_LOG_TILE(...)
#endif //ISPSS_DEBUG_LOG_MODULE_TILE_ENABLE

#ifdef ISPSS_DEBUG_LOG_MODULE_CORE_ENABLE
#define ISPSS_DEBUG_LOG_CORE(...) \
	do { \
		if (_IS_ISPSS_DEBUG_LOG_ENABLE_CORE()) { \
			pr_debug("%s:%d: ", __func__, __LINE__); \
			pr_debug(__VA_ARGS__); \
		} \
		_IS_ISPSS_DEBUG_LOG_REG_PROCESS(ISPSS_DEBUG_LOG_MODULE_CORE); \
	} while (0)
#else //ISPSS_DEBUG_LOG_MODULE_CORE_ENABLE
#define ISPSS_DEBUG_LOG_CORE(...)
#endif //ISPSS_DEBUG_LOG_MODULE_CORE_ENABLE

//DHUB Channel interrupt types
#define ISPSS_DHUB_CHAN_INTR_MASK_EMPTY_NDX         0
#define ISPSS_DHUB_CHAN_INTR_MASK_FULL_NDX          1
#define ISPSS_DHUB_CHAN_INTR_MASK_ALMOSTEMPTY_NDX   2
#define ISPSS_DHUB_CHAN_INTR_MASK_ALMOSTFULL_NDX    3
//Get mask DHUB Channel interrupt type
#define ISPSS_DHUB_CHAN_INTR_MASK_GETMASK(NDX)      (0x1 << NDX)
//Check DHUB Channel interrupt type mask
#define ISPSS_DHUB_CHAN_INTR_MASK_EMPTY_FLAG(mask) \
	((mask & \
	  (ISPSS_DHUB_CHAN_INTR_MASK_GETMASK(ISPSS_DHUB_CHAN_INTR_MASK_EMPTY_NDX))) ? 1 : 0)
#define ISPSS_DHUB_CHAN_INTR_MASK_FULL_FLAG(mask) \
	((mask & \
	  (ISPSS_DHUB_CHAN_INTR_MASK_GETMASK(ISPSS_DHUB_CHAN_INTR_MASK_FULL_NDX))) ? 1 : 0)
#define ISPSS_DHUB_CHAN_INTR_MASK_ALMOSTEMPTY_FLAG(mask) \
	((mask & \
	  (ISPSS_DHUB_CHAN_INTR_MASK_GETMASK(ISPSS_DHUB_CHAN_INTR_MASK_ALMOSTEMPTY_NDX))) ? 1 : 0)
#define ISPSS_DHUB_CHAN_INTR_MASK_ALMOSTFULL_FLAG(mask) \
	((mask & \
	  (ISPSS_DHUB_CHAN_INTR_MASK_GETMASK(ISPSS_DHUB_CHAN_INTR_MASK_ALMOSTFULL_NDX))) ? 1 : 0)

typedef void (*ISPSS_DhubIntrAllFunc)(struct HDL_dhub2d *pdhubHandle);

void ISPSS_DhubIntrAllEnable(struct HDL_dhub2d *pdhubHandle, int intr, int enable);
int ISPSS_DhubIntrAllCheckStatus(struct HDL_dhub2d *pdhubHandle, int intr);
void ISPSS_DhubIntrAllClearStatus(void);
void ISPSS_DhubIntrAllUpdateStatus(void);
void ISPSS_DhubIntrAllDumpStatus(void);

void ISPSS_DhubIntrEnable(struct HDL_dhub2d *pdhubHandle, int intr, int enable);
int ISPSS_DhubIntrCheckStatus(struct HDL_dhub2d *pdhubHandle, int intr);

void ISPSS_DhubIntrRegisterHandler(struct HDL_dhub2d *pdhubHandle,
		int intr, void *pArgs, ISPSS_DHUB_INTR_HANDLER intrHandler);
void ISPSS_DhubIntrHandler(struct HDL_dhub2d *pdhubHandle,
		UINT32 instat, struct HDL_dhub2d *pdhubHandle1, UINT32 instat1);

void ISPSS_DhubIntrLogToReg(int module);

#endif // __ISPSS_DHUB_INTR_H__
