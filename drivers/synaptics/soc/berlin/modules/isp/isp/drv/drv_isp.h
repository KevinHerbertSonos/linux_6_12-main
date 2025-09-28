/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __DRV_ISP_H__
#define __DRV_ISP_H__
#include "reg_if.h"

void isp_drv_core_clear_intr(int intr_num, int *status, void *mod_base);
int isp_core_mod_probe(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_core_mod_close(isp_device *isp_dev, isp_module_ctx *mod_ctx);
long isp_core_mod_ioctl(isp_device *isp_dev, isp_module_ctx *mod_ctx,
			unsigned int cmd, unsigned long arg);

#endif /* __DRV_ISP_H__ */
