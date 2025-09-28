// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 VeriSilicon Holdings Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 * The GPL License (GPL)
 *
 * Copyright (c) 2023 VeriSilicon Holdings Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 *****************************************************************************
 *
 * Note: This software is released under dual MIT and GPL licenses. A
 * recipient may use this file under the terms of either the MIT license or
 * GPL License. If you wish to use only one license not the other, you can
 * indicate your decision by deleting one of the above license notices in your
 * version of this file.
 *
 *****************************************************************************/
#include <linux/module.h>
#include <linux/version.h>
#include <linux/platform_device.h>

#include "vvcam_dw_platform.h"

static void vvcam_dw_pdev_release(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return;
}

static struct resource vvcam_dw_resource[] = {
	[0] = {
		.start = VVCAM_DWE_REG_BASE,
		.end   = VVCAM_DWE_REG_BASE + VVCAM_DWE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = VVCAM_VSE_REG_BASE,
		.end   = VVCAM_VSE_REG_BASE + VVCAM_VSE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = VVCAM_DW_RST_REG_BASE,
		.end   = VVCAM_DW_RST_REG_BASE + VVCAM_DW_RST_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[3] = {
		.start = VVCAM_DWE_IRQ,
		.end   = VVCAM_DWE_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	[4] = {
		.start = VVCAM_VSE_IRQ,
		.end   = VVCAM_VSE_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	[5] = {
		.start = VVCAM_DW_FE_IRQ,
		.end   = VVCAM_DW_FE_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device vvcam_dw_pdev = {
	.name			= "vvcam-dw200",
	.id				= 0,
	.resource		= vvcam_dw_resource,
	.num_resources	= ARRAY_SIZE(vvcam_dw_resource),
	.dev.release	= vvcam_dw_pdev_release,
};

int vvcam_dw_platform_device_register(void)
{
	int ret = 0;

	ret = platform_device_register(&vvcam_dw_pdev);
	if (ret) {
		goto err_register;
	}

	return ret;

err_register:
	platform_device_unregister(&vvcam_dw_pdev);
	return ret;
}

void vvcam_dw_platform_device_unregister(void)
{
	platform_device_unregister(&vvcam_dw_pdev);

	return;
}
