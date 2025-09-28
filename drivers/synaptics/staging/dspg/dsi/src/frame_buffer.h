/**
 * @file frame_buffer.h
 * @brief MIPI DSI controller driver
 *
 * Copyright (C) 2010 Synopsys, Inc. All rights reserved.
 *
 * @version 1.00a first release
 */

#ifndef __FRAME_BUFFER_H__
#define __FRAME_BUFFER_H__

#include "includes.h"

int
bus32_write(uint32_t offset, uint32_t data);

int
bus32_read(uint32_t offset, uint32_t *data);

void
init_frame_buffer(struct mipi_dsi_dev *dev);

int
fb_mipi_open(struct fb_info *info, int user);

int
fb_mipi_release(struct fb_info *info, int user);

int
fb_mipi_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);

ssize_t
fb_mipi_read(struct fb_info *info, char __user *buf, size_t count,
	     loff_t *ppos);

ssize_t
fb_mipi_write(struct fb_info *info, const char __user *buf, size_t count,
	      loff_t *ppos);


/***************************************************************
 * Misc functions
 **************************************************************/
/**
 * Checks var and eventually tweaks it to something supported, DO NOT
 * MODIFY PAR
 */
int
fb_mipi_check_var(struct fb_var_screeninfo *var, struct fb_info *info);

/**
 * Set the video mode according to info->var
 */
int
fb_mipi_set_par(struct fb_info *info);

/**
 * Set color register
 */
int
fb_mipi_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue,
		unsigned transp, struct fb_info *info);

/**
 * Set color registers in batch
 */
int
fb_mipi_setcmap(struct fb_cmap *cmap, struct fb_info *info);

/**
 * Blank display
 */
int
fb_mipi_blank(int blank, struct fb_info *info);

/**
 * Pan display
 */
int
fb_mipi_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);

/**
 * Draws a rectangle
 */
void
fb_mipi_fillrect(struct fb_info *info, const struct fb_fillrect *rect);

/**
 * Copy data from area to another
 */
void
fb_mipi_copyarea(struct fb_info *info, const struct fb_copyarea *region);

/**
 * Draws a image to the display
 */
void
fb_mipi_imageblit(struct fb_info *info, const struct fb_image *image);

/**
 * Draws cursor
 */
int
fb_mipi_cursor(struct fb_info *info, struct fb_cursor *cursor);

/**
 * Rotates the display
 */
void
fb_mipi_rotate(struct fb_info *info, int angle);

/**
 * Wait for blit idle, optional
 */
int
fb_mipi_sync(struct fb_info *info);

/**
 * Perform fb specific mmap
 */
int
fb_mipi_mmap(struct fb_info *info, struct vm_area_struct *vma);

/**
 * Get capability given var
 */
void
fb_mipi_get_caps(struct fb_info *info, struct fb_blit_caps *caps,
		struct fb_var_screeninfo *var);

/**
 * Teardown any resources to do with this framebuffer
 */
void
fb_mipi_destroy(struct fb_info *info);

#endif /* __FRAME_BUFFER_H__ */
