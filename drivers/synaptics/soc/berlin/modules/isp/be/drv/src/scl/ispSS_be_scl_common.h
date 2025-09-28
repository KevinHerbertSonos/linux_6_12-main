/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_BE_SCL_COMMON_H_
#define _ISPSS_BE_SCL_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/kernel.h>
#include "ispSS_bcmbuf.h"
#include "ispSS_reg.h"

/* You can place the DEBUG macros here, which are common to all file */
#define ISPSS_SCL_DBG_ERROR(msg) (printf(msg))

#define ISP_DEBUG_CALLTRACE "ISPDBG:DNSCL: "
#define ISPSS_SCLDBG(...) \
	do { \
		if (0) \
			pr_err(ISP_DEBUG_CALLTRACE  __VA_ARGS__); \
	} while (0)
#define ISPSS_SCLDBG_CT(...) \
	do { \
		if (1) \
			pr_debug(ISP_DEBUG_CALLTRACE  __VA_ARGS__); \
	} while (0)
#define ISPSS_SCLDBG_BCMQ(...) \
	do { \
		if (1) \
			pr_debug(ISP_DEBUG_CALLTRACE  __VA_ARGS__); \
	} while (0)

#define GA_REG_WORD32_READ(offset, holder) \
	(*(holder) = (*((volatile unsigned int *)ispss_devmem_phy_to_virt(offset))))
#define GA_REG_WORD32_WRITE(addr, data) \
	((*((volatile unsigned int *)ispss_devmem_phy_to_virt(addr))) = ((unsigned int)(data)))

#ifdef __cplusplus
}
#endif
#endif
