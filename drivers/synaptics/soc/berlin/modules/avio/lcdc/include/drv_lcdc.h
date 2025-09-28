// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */

#ifndef _DRV_LCDC_H_
#define _DRV_LCDC_H_

#include "avio_sub_module.h"
#include "avio_common.h"
#include "vpp_mem.h"

#define LCDC_MODULE_NAME     "avio_module_lcdc"

typedef struct LCDC_CTX_T {
	struct device   *dev;
	VPP_MEM_LIST    *vpp_mem_lcdc_list;
} LCDC_CTX;

int avio_module_drv_lcdc_probe(struct platform_device *dev);
int AVIO_MEM_IsReady(void);
#endif //_DRV_LCDC_H_