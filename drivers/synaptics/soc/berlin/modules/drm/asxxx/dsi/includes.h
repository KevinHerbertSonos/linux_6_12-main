/* SPDX-License-Identifier: GPL-2.0 */
/**
 * Copyright (C) 2018 Synopsys, Inc.
 *
 * @file includes.h
 * @brief includes file
 * included as a part of Synopsys MIPI DSI Host controller driver
 *
 * @author Luis Oliveira <luis.oliveira@synopsys.com>
 */

#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/reset.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include "dsih_core.h"
#include "avio_io.h"

#ifdef iowrite32
#undef iowrite32
#endif
#ifdef ioread32
#undef ioread32
#endif

#include "Galois_memmap.h"

#define iowrite32(OFF, VAL) GA_REG_WORD32_WRITE((unsigned int)(long long)OFF, VAL)
#define ioread32(OFF, VAL) GA_REG_WORD32_READ((unsigned int)(long long)OFF, VAL) 

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define mipi_dbg_print(X,...) printk(__VA_ARGS__)
#define MIPI_RET(ret) 0

#define DSI_CMD_DELAY		 0xFF
#define DSI_CMD_BYTE_WRITE	0x5
#define DSI_CMD_SHORT_WRITE   0x15
#define DSI_CMD_LONG_WRITE	0x39
#define GEN_CMD_BYTE_WRITE	0x13
#define GEN_CMD_SHORT_WRITE   0x23
#define GEN_CMD_LONG_WRITE	0x29

#define MAX_DSI_LONG_WRITE_PARAMS   256
/*
 * this macro only works in C99 so you can disable this, by placing
 * #define FUNC_NAME "MIPI_DSI", or __FUNCTION__
 */
#define FUNC_NAME __func__

/**
 * Main structures to instantiate the driver
 */
struct mipi_dsi_dev {

	/** HW version */
	uint32_t hw_version;
	int position;

	/* timeout for FIFO full */
	int timeout;

	/* device node */
	struct device *parent_dev;

	/* interrupts */
	int irq_core;

	/* mutex */
	struct reset_control	*rst_1;
	struct reset_control	*rst_2;
	struct reset_control	*rst_3;

	/* device Tree Information */
	char *device_name;

	/* MIPI DSI Controller */
	uint32_t core_addr;
	uint32_t core_mem_size;

	dphy_t phy;

	/*
	 * number of lanes physically connected to controller
	 */
	uint8_t max_lanes;

	/*
	 * maximum number of byte clock cycles needed by the PHY to perform
	 * the Bus Turn Around operation - REQUIRED
	 */
	uint16_t max_bta_cycles;

	/*
	 * describe the color mode pin (dpicolorm) whether it is active
	 * high or low - REQUIRED
	 */
	int color_mode_polarity;

	/*
	 * describe the shut down pin (dpishutdn) whether it is
	 * active high or low - REQUIRED
	 */
	int shut_down_polarity;

	dsih_dpi_video_t dpi_video;
	dsih_dpi_video_t dpi_video_old;
	dsih_cmd_mode_video_t cmd_mode_video;
	dsih_cmd_mode_video_t cmd_mode_video_old;

	void *pHandle;
};

void dsi_platform_init(struct mipi_dsi_dev *dev,
			int display, int video_mode, int lanes);
#endif /* __INCLUDES_H__ */
