/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_ISP_DHUB_H_
#define _DRV_ISP_DHUB_H_

int isp_dhub_mod_probe(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_exit(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_open(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_close(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_suspend(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_mod_resume(isp_device *isp_dev, isp_module_ctx *mod_ctx);
int isp_dhub_write_reg(unsigned int reg, unsigned int val);
int isp_dhub_read_reg(unsigned int reg);
void *isp_dhub_get_mode_ctx(void);
long isp_dhub_mod_func(isp_module_ctx *mod_ctx, unsigned int cmd, void *arg);
long isp_dhub_mod_ioctl(isp_device *isp_dev, isp_module_ctx *mod_ctx,
			unsigned int cmd, unsigned long arg);

#endif //_DRV_ISP_DHUB_H_

