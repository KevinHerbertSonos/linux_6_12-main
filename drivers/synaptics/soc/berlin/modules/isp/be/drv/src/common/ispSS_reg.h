/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPSS_REG_H_
#define _ISPSS_REG_H_

#include <linux/kernel.h>
#include "isp_driver.h"
#include "ispSS_memmap.h"
#include "com_type.h"
#include "ispMISC.h"
#include "ispbe_api.h"
#include "drv_msg.h"

#define MEMMAP_ISPSS_REG_BASE (MEMMAP_ISP_REG_BASE)
#define MEMMAP_ISPSS_MISC_CLKRST_BASE_ADDR\
	(MEMMAP_ISP_REG_BASE + ISPSS_MEMMAP_GLB_REG_BASE + RA_IspMISC_clkrst)
#define MEMMAP_ISPSS_MISC_INTR_BASE_ADDR\
	(MEMMAP_ISP_REG_BASE + ISPSS_MEMMAP_GLB_REG_BASE + RA_IspMISC_interrupt)

/* helper MACROs */

#define ISPSS_REG_WRITE32(reg, val)	ispss_write_phys_reg(reg, val)
#define ISPSS_REG_READ32(reg, val)\
	((*(volatile unsigned int *)(val)) = ispss_read_phys_reg(reg))

#define ISPSS_IO32RD(d, a)	((d) = ispss_read_phys_reg(a))
#define ISPSS_IO32WR(d, a)	ispss_write_phys_reg(a, d)

#define bTST(x, b)	(((x) >> (b)) & 1)
#define ModInc(x, i, mod)\
	do {\
		(x) += (i);\
		while ((x) >= (mod))\
			(x) -= (mod);\
	} while (0)

#define CPCB_1 0

SIGN32 ispss_create_iomap(void);
UNSG32 ispss_destroy_iomap(void);
UNSG64 ispss_devmem_phy_to_virt(UNSG32 uPhyAddr);
UNSG32 ispss_read_phys_reg(UNSG32 uPhyAddr);
HRESULT ispss_enable_dhub_sem_intr(enum ISP_DHUBSEM_TSB dhub_semid, int enable);
HRESULT ispss_wait_dhub_sem_intr(CC_MSG_t *msg);
HRESULT ispss_dhub_init(void);
void ispss_write_phys_reg(UNSG32 uPhyAddr, UNSG32 val);

#endif
