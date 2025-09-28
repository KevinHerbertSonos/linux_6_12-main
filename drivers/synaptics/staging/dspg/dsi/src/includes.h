/**
 * @file includes.h
 * @brief MIPI DSI controller driver
 *
 * Copyright (C) 2010 Synopsys, Inc. All rights reserved.
 *
 * @version 1.00a first release
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
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <asm/io.h>

#include "../include/mipi_dsi.h"

#define MIPI_DSIH_MAX_PARAMS		200

/**
 * This macro only works in C99 so you can disable this, by placing
 * #define FUNC_NAME "MIPI_DSI", or __FUNCTION__
 */
#define FUNC_NAME __func__

/**
 * @short Frame buffer structure
 */
struct frame_buffer {
	/**
	 * Defines the current state of a particular video card. Inside
	 * fb_info, there exists a fb_ops structure which is a collection of
	 * needed functions to make fbdev and fbcon work. fb_info is only
	 * visible to the kernel.
	 */
	struct fb_info info;

	/**
	 * The operation file
	 */
	struct fb_ops ops;
};

/**
 * @short Main structures to instantiate the driver
 */
struct mipi_dsi_dev {
	/** HW version */
	uint32_t hw_version;
	int position;
	/** timeout for FIFO full */
	int timeout;
	/** Device node */
	struct device		*parent_dev;
	/** Device list */
	struct list_head	devlist;
	/** Interrupts */
	uint32_t			irq[2];
	/** Spinlock */
	spinlock_t			slock;
	/** Mutex */
	struct mutex		mutex;

	/** Device Tree Information */
	char				*device_name;
	/** MIPI DSI Controller */
	void __iomem		*core_addr;
	uint32_t			core_mem_size;
#if 0
	/** Clock Manager */
	void __iomem		*clk_addr;
	uint32_t			clk_mem_size;
		/** Video Bridge */
	void __iomem		*vid_bridge_addr;
	uint32_t			vid_bridge_mem_size;
#endif
	/** Frame buffer */
	struct frame_buffer fb;

	/** For sending signals to user space */
	pid_t				app_pid;
	dphy_t phy;
	/** Number of lanes physically connected to controller - REQUIRED */
	uint8_t				max_lanes;
	/** Maximum number of byte clock cycles needed by the PHY to perform
	 * the Bus Turn Around operation - REQUIRED */
	uint16_t				max_bta_cycles;
	/** Describe the color mode pin (dpicolorm) whether it is active high or low - REQUIRED */
	int					color_mode_polarity;
	/** Describe the shut down pin (dpishutdn) whether it is active high or low - REQUIRED */
	int					shut_down_polarity;
	//**/
	dsih_dpi_video_t		dpi_video;
	dsih_dpi_video_t		dpi_video_old;
	/**/
	dsih_cmd_mode_video_t   cmd_mode_video;
	dsih_cmd_mode_video_t   cmd_mode_video_old;
	/* parameter count is included in first two bytes for long writes */
	u8		cmd_buf[MIPI_DSIH_MAX_PARAMS + 2];
	int		reset_gpio_panel;
	bool		read_display_status;
	/* LDO DSI regulator */
	struct regulator *ldo_dsi;
};

/**
 * @brief Dynamic memory allocation
 * Instance 0 will have the total size allocated so far and also the number
 * of calls to this function (number of allocated instances)
 */
struct mem_alloc{
	int				instance;
	const char			*info;
	size_t				size;
	void				*pointer; // the pointer allocated by this instance
	struct mem_alloc	*last;
	struct mem_alloc	*prev;
};

/**
 * @short Allocate memory for the driver
 * @param[in] into allocation name
 * @param[in] size allocation size
 * @param[in,out] allocated return structure for the allocation, may be NULL
 * @return if successful, the pointer to the new created memory
 *	   if not, NULL
 */
void *
alloc_mem(char *info, size_t size, struct mem_alloc *allocated);

void dsi_platform_init(struct mipi_dsi_dev *dev, int display, int video_mode, int lanes);

#endif /* __INCLUDES_H__ */
