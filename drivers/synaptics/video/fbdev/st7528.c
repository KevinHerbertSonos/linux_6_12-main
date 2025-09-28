/*
 * drivers/video/st7528.c - a framebuffer driver for ST7528 based displays
 *
 * Copyright (C) 2014 DSP Group.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/timer.h>
#include <linux/file.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/of_gpio.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/st7528.h>

#define ST7528_FB_NAME		"st7528fb"
#define LCD_WIDTH		128
#define LCD_HEIGHT		96
#define LCD_FRAMEBUFFER_SIZE	((LCD_WIDTH / 2) * LCD_HEIGHT)

struct st7528fb {
	struct spi_device *dev;
	struct spi_message spi_msg;
	struct spi_transfer spi_xfer;

	unsigned int cs_gpio, reset_gpio;
	unsigned int fps;
	unsigned int bruteforce;

	/* fake framebuffer */
	unsigned long alloc_size;
	struct fb_info *info;
	unsigned char *fb;
	unsigned char *transfer_fb;

	/* refresh vars */
	struct task_struct *thread;
	wait_queue_head_t wq;
	wait_queue_head_t vblank_wq;
	struct mutex transfer_lock;
	struct hrtimer refresh_timer;
	ktime_t refresh_timeout;

	atomic_t need_update;
	atomic_t vblanks;
};

static struct fb_fix_screeninfo st7528fb_fix = {
	.id = ST7528_FB_NAME,
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_STATIC_PSEUDOCOLOR,
	.accel = FB_ACCEL_NONE,
	.line_length = LCD_WIDTH / 2,
};

static struct fb_var_screeninfo st7528fb_var = {
	.xres = LCD_WIDTH,
	.yres = LCD_HEIGHT,
	.xres_virtual = LCD_WIDTH,
	.yres_virtual = LCD_HEIGHT,
	.xoffset = 0,
	.yoffset = 0,
	.bits_per_pixel = 4,
	.grayscale = 1,
	.red = { .length = 4 },
	.green = { .length = 4 },
	.blue = { .length = 4 },
	.vmode = FB_VMODE_NONINTERLACED,
	.width = 60,
	.height = 44,
};

static void
st7528fb_do_seq(struct st7528fb *st7528fb, unsigned char *cmd, int num_cmds)
{
	unsigned int cs_gpio = st7528fb->cs_gpio;

	if (num_cmds <= 0)
		return;

	gpio_set_value(cs_gpio, 0);
	ndelay(100);

	st7528fb->spi_xfer.tx_buf = (void *)cmd;
	st7528fb->spi_xfer.len = num_cmds;
	st7528fb->spi_xfer.bits_per_word = 8;
	st7528fb->spi_xfer.cs_change = 0;

	spi_sync(st7528fb->dev, &st7528fb->spi_msg);

	ndelay(100);
	gpio_set_value(cs_gpio, 1);
}

static void
st7528fb_convert(struct st7528fb *st7528fb)
{
	struct fb_info *info = st7528fb->info;
	unsigned char *fb = st7528fb->fb;
	unsigned char *tfb = st7528fb->transfer_fb;
	int xres = info->var.xres, yres = info->var.yres;
	int x, y, page, col, row, pos, val;

	for (x = 0; x < (xres * yres) / 2; x++)
		tfb[x] = 0xff;

	for (y = 0; y < yres; y++) {
		page = (y/8) * xres * 4;
		row = y & 0x7;
		col = 0;
		for (x = 0; x < xres; x++) {
			pos = ((y * xres) + x) / 2;
			val = fb[pos];
			if (x & 1)
				val >>= 4;
			if (val & 8)
				tfb[page + col]     &= ~(1 << row);
			if (val & 4)
				tfb[page + col + 1] &= ~(1 << row);
			if (val & 2)
				tfb[page + col + 2] &= ~(1 << row);
			if (val & 1)
				tfb[page + col + 3] &= ~(1 << row);
			col += 4;
		}
	}
}

static void
st7528fb_send_data(struct st7528fb *st7528fb, int page, int column, int len,
		   unsigned char *buf)
{
	unsigned char cmds[6];
	unsigned int cs_gpio = st7528fb->cs_gpio;

	if (len <= 0)
		return;

	gpio_set_value(cs_gpio, 0);
	ndelay(100);

	st7528fb->spi_xfer.len = 5;
	st7528fb->spi_xfer.bits_per_word = 8;
	st7528fb->spi_xfer.cs_change = 0;
	st7528fb->spi_xfer.tx_buf = (void *)&cmds[0];
	cmds[0] = CMD_PAGE   | (page & 0xf);
	cmds[1] = CMD_COLUMN | ((column >> 4) & 0xf);
	cmds[2] =                column & 0xf;
	cmds[3] = CMD_SET_DATA_DIR;
	cmds[4] = len - 1;

	spi_sync(st7528fb->dev, &st7528fb->spi_msg);

	st7528fb->spi_xfer.len = len;
	st7528fb->spi_xfer.bits_per_word = 8;
	st7528fb->spi_xfer.cs_change = 0;
	st7528fb->spi_xfer.tx_buf = (void *)buf;

	spi_sync(st7528fb->dev, &st7528fb->spi_msg);

	ndelay(100);
	gpio_set_value(cs_gpio, 1);
}

static void
st7528fb_do_transfer_rows(struct st7528fb *st7528fb, unsigned int row, int rows)
{
	struct fb_info *info = st7528fb->info;
	int i, off, page;
	int pages = (info->var.yres + 7) / 8;
	int columns = info->var.xres * 4;

	for (page = 0; page < pages; page++) {
		off = 0;
		for (i = 0; i < columns; i += 256) {
			st7528fb_send_data(st7528fb, page, off, 256,
					   st7528fb->transfer_fb +
					   (page * columns) + i);
			off += 64;
		}
	}
}

static void
st7528fb_do_transfer(struct st7528fb *st7528fb)
{
	struct fb_info *info = st7528fb->info;

	st7528fb_convert(st7528fb);
	st7528fb_do_transfer_rows(st7528fb, 0, info->var.yres);

	/* display is up to date */
	atomic_set(&st7528fb->need_update, 0);
}

static inline void
st7528fb_run_timer(struct st7528fb *st7528fb)
{
	hrtimer_start(&st7528fb->refresh_timer,
		      st7528fb->refresh_timeout,
		      HRTIMER_MODE_REL);
}

static enum hrtimer_restart
st7528fb_timer(struct hrtimer *timer)
{
	struct st7528fb *st7528fb = container_of(timer, struct st7528fb,
						 refresh_timer);

	atomic_set(&st7528fb->need_update, 1);
	wake_up_interruptible(&st7528fb->wq);
	hrtimer_forward_now(timer, st7528fb->refresh_timeout);

	return HRTIMER_RESTART;
}

static int
st7528fb_refresh_thread(void *data)
{
	struct st7528fb *st7528fb = data;

	/* will only run in bruteforce mode */
	while (!kthread_should_stop()) {
		if (atomic_read(&st7528fb->need_update) == 1) {
			flush_kernel_vmap_range(st7528fb->fb,
						st7528fb->alloc_size);

			mutex_lock(&st7528fb->transfer_lock);
			st7528fb_do_transfer(st7528fb);
			mutex_unlock(&st7528fb->transfer_lock);

			/* signal that the next vblank happens */
			atomic_inc(&st7528fb->vblanks);
			wake_up_interruptible(&st7528fb->vblank_wq);
		}

		wait_event_interruptible(st7528fb->wq,
					 kthread_should_stop() ||
					 atomic_read(&st7528fb->need_update)
					 == 1);
	}

	return 0;
}

static ssize_t
st7528fb_write(struct fb_info *info, const char __user *buf, size_t count,
	       loff_t *ppos)
{
	struct st7528fb *st7528fb;
	unsigned int off;

	/* skip if senseless :) */
	if (!count)
		return 0;

	off = *ppos;
	st7528fb = info->par;

	if (off > info->screen_size)
		return -ENOSPC;

	if ((count + off) > info->screen_size)
		return -ENOSPC;

	/* copy from userspace to the fb memory */
	count -= copy_from_user(info->screen_base + off, buf, count);
	*ppos += count;

	flush_kernel_vmap_range(st7528fb->fb, st7528fb->alloc_size);

	/*
	 * XXX could be optimized by just transferring
	 * the display pages that have changed
	 */
	mutex_lock(&st7528fb->transfer_lock);
	st7528fb_do_transfer(st7528fb);
	mutex_unlock(&st7528fb->transfer_lock);

	/* return nicely */
	if (count)
		return count;

	/* we should always be able to write atleast one byte */
	return -EFAULT;
}

static int
st7528fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return remap_vmalloc_range(vma, (void *)info->fix.smem_start,
				   vma->vm_pgoff);
}

static int
set_fps(struct st7528fb *st7528fb, unsigned int fps)
{
	if (fps == 0 || fps > 250)
		return -EBADMSG;

	st7528fb->fps = fps;
	st7528fb->refresh_timeout = ktime_set(0, ((1000 + st7528fb->fps - 1) /
					      st7528fb->fps) * 1000000L);

	return 0;
}

static int
st7528fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct st7528fb *st7528fb = info->par;
	int ret = 0;
	void *buffer;
	union user_args {
		struct st7528_cmd_array cmd_array;
		struct st7528_bruteforce_params bruteforce_params;
	} p;
	unsigned int vblanks;

	switch (cmd) {
	case ST7528_MANUAL_SYNC:
		atomic_set(&st7528fb->need_update, 1);
		wake_up_interruptible(&st7528fb->wq);
		break;
	case ST7528_SEND_CMD_ARRAY:
		if (copy_from_user(&p.cmd_array,
				   (void __user *)arg,
				   sizeof(p.cmd_array))) {
			ret = -EINVAL;
			break;
		}
		if (p.cmd_array.num_cmds == 0) {
			ret = -EINVAL;
			break;
		}
		if (!access_ok(p.cmd_array.cmds, p.cmd_array.num_cmds)) {
			ret = -EINVAL;
			break;
		}
		buffer = vmalloc(p.cmd_array.num_cmds);
		if (!buffer) {
			ret = -ENOMEM;
			break;
		}
		if (copy_from_user(buffer, p.cmd_array.cmds,
				   p.cmd_array.num_cmds)) {
			ret = -EINVAL;
			vfree(buffer);
			break;
		}
		mutex_lock(&st7528fb->transfer_lock);
		st7528fb_do_seq(st7528fb, (unsigned char *)buffer,
				p.cmd_array.num_cmds);
		mutex_unlock(&st7528fb->transfer_lock);
		vfree(buffer);
		break;
	case ST7528_SET_BRUTEFORCE_PARAMS:
		if (copy_from_user(&p.bruteforce_params,
				   (void __user *)arg,
				   sizeof(p.bruteforce_params))) {
			ret = -EINVAL;
			break;
		}

		if (p.bruteforce_params.fps < 1 ||
		    p.bruteforce_params.fps > 250) {
			ret = -EINVAL;
			break;
		}

		set_fps(st7528fb, p.bruteforce_params.fps);

		if (st7528fb->thread) {
			if (st7528fb->bruteforce == 1)
				hrtimer_cancel(&st7528fb->refresh_timer);
			wake_up_interruptible(&st7528fb->wq);
			kthread_stop(st7528fb->thread);
		}

		/* spawn refresh thread */
		st7528fb->thread = kthread_run(st7528fb_refresh_thread,
					       st7528fb, "kst7528fbd");
		if (IS_ERR(st7528fb->thread)) {
			ret = PTR_ERR(st7528fb->thread);
			break;
		}

		if (st7528fb->bruteforce == 1)
			st7528fb_run_timer(st7528fb);

		break;
	case FBIO_WAITFORVSYNC:
		vblanks = atomic_read(&st7528fb->vblanks);
		unlock_fb_info(info);
		/* XXX */
		do {
			if (!st7528fb->bruteforce)
				break;

			ret = wait_event_interruptible(st7528fb->vblank_wq,
				atomic_read(&st7528fb->vblanks) != vblanks);
		} while (ret < 0);
		lock_fb_info(info);
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int
st7528fb_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
		   unsigned int blue, unsigned int transp,
		   struct fb_info *fb_info)
{
	return 0;
}

static struct fb_ops st7528fb_ops = {
	.owner = THIS_MODULE,
	.fb_write = st7528fb_write,
	.fb_setcolreg = st7528fb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_imageblit = cfb_imageblit,
	.fb_copyarea = cfb_copyarea,
	.fb_mmap = st7528fb_mmap,
	.fb_ioctl = st7528fb_ioctl,
};

static ssize_t
sysfs_set_bruteforce(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t count)
{
	unsigned long value;
	struct st7528fb *st7528fb = dev_get_drvdata(dev);
	int ret;

	ret = kstrtoul(buf, 0, &value);
	if (ret < 0)
		return ret;

	if (st7528fb->bruteforce == 0 && value) {
		/* was off and gets enabled */
		st7528fb->bruteforce = 1;
		st7528fb_run_timer(st7528fb);
	} else if (st7528fb->bruteforce == 1 && value == 0) {
		/* was on and gets disabled */
		st7528fb->bruteforce = 0;
		hrtimer_cancel(&st7528fb->refresh_timer);
	}

	return count;
}

static ssize_t
sysfs_get_bruteforce(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct st7528fb *st7528fb = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", st7528fb->bruteforce ? "yes" : "no");
}

static ssize_t
sysfs_set_fps(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	ssize_t ret;
	unsigned long value;
	struct st7528fb *st7528fb = dev_get_drvdata(dev);

	ret = kstrtoul(buf, 0, &value);
	if (ret < 0)
		return ret;

	ret = set_fps(st7528fb, value);
	if (ret == 0)
		ret = count;

	return ret;
}

static ssize_t
sysfs_get_fps(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct st7528fb *st7528fb = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", st7528fb->fps);
}

static struct device_attribute st7528fb_attrs[] = {
	__ATTR(bruteforce, 0600, sysfs_get_bruteforce, sysfs_set_bruteforce),
	__ATTR(fps, 0600, sysfs_get_fps, sysfs_set_fps),
};

static int
st7528fb_probe(struct spi_device *spi)
{
	int i, ret, reset_gpio, cs_gpio;
	unsigned int alloc_size;
	struct st7528fb *st7528fb;
	struct fb_info *info;
	struct device_node *np;

	np = spi->dev.of_node;
	if (np == NULL)
		dev_info(&spi->dev, "of node is null\n");

	reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	if (reset_gpio < 0) {
		dev_err(&spi->dev, "Reset value not specified in devtree\n");
		return -ENODEV;
	}

	cs_gpio = of_get_named_gpio(np, "cs-gpio", 0);
	if (cs_gpio < 0)
		cs_gpio = -1;

	spi->mode = SPI_MODE_3;
	spi->bits_per_word = 8;
	spi->controller_data = (void *)-1; /* we will handle the CS */

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "cannot setup SPI slave device\n");
		goto out;
	}

	/* allocate driver data */
	ret = -ENOMEM;
	st7528fb = kmalloc(sizeof(*st7528fb), GFP_KERNEL);
	if (!st7528fb)
		goto out;

	memset(st7528fb, 0, sizeof(*st7528fb));

	atomic_set(&st7528fb->vblanks, 0);

	st7528fb->cs_gpio = cs_gpio;
	st7528fb->reset_gpio = reset_gpio;
	st7528fb->fps = CONFIG_FB_ST7528_FPS;
	if (st7528fb->fps == 0)
		st7528fb->fps = 1;

	st7528fb->bruteforce = 1;

	mutex_init(&st7528fb->transfer_lock);
	init_waitqueue_head(&st7528fb->wq);
	init_waitqueue_head(&st7528fb->vblank_wq);
	spi_message_init(&st7528fb->spi_msg);
	spi_message_add_tail(&st7528fb->spi_xfer, &st7528fb->spi_msg);

	st7528fb->dev = spi;

	hrtimer_init(&st7528fb->refresh_timer, CLOCK_REALTIME,
		     HRTIMER_MODE_REL);
	st7528fb->refresh_timer.function = st7528fb_timer;
	st7528fb->refresh_timeout = ktime_set(0, ((1000 + st7528fb->fps - 1) /
					      st7528fb->fps) * 1000000L);

	/* setup framebuffer */
	ret = -ENOMEM;
	info = framebuffer_alloc(sizeof(u32) * 256, &spi->dev);
	if (!info)
		goto err_st7528fb;

	info->pseudo_palette = info->par;
	info->par = st7528fb;
	info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;

	info->var = st7528fb_var;
	info->fix = st7528fb_fix;

	info->fbops = &st7528fb_ops;

	ret = fb_alloc_cmap(&info->cmap, 16, 0);
	if (ret < 0)
		goto err_free_info;

	/* attach info to driver data */
	st7528fb->info = info;

	info->screen_size = info->fix.line_length * info->var.yres_virtual;
	alloc_size = PAGE_ALIGN(info->screen_size);

	/* setup fake framebuffer */
	ret = -ENOMEM;
	st7528fb->alloc_size = alloc_size;
	st7528fb->fb = vmalloc_32_user(alloc_size);
	if (!st7528fb->fb)
		goto err_free_cmap;

	memset(st7528fb->fb, 0xff, alloc_size);

	info->screen_base = (char __iomem *)st7528fb->fb;
	info->fix.smem_start = (unsigned long)info->screen_base;
	info->fix.smem_len = alloc_size;

	/* allocate buffer for pixel conversion */
	st7528fb->transfer_fb = vmalloc(LCD_FRAMEBUFFER_SIZE);
	if (!st7528fb->transfer_fb)
		goto err_free_fb;

	memset(st7528fb->transfer_fb, 0xff, LCD_FRAMEBUFFER_SIZE);

	/* attach driver data to the device */
	dev_set_drvdata(&spi->dev, st7528fb);

	/* spawn refresh thread */
	st7528fb->thread = kthread_run(st7528fb_refresh_thread, st7528fb,
				       "kst7528fbd");
	if (IS_ERR(st7528fb->thread)) {
		ret = PTR_ERR(st7528fb->thread);
		goto err_free_transfer_fb;
	}

	/* if bruteforce got enable trigger the update mechanism */
	if (st7528fb->bruteforce)
		st7528fb_run_timer(st7528fb);

	/* register framebuffer userspace device */
	ret = register_framebuffer(info);
	if (ret < 0)
		goto err_free_kthread;

	for (i = 0; i < ARRAY_SIZE(st7528fb_attrs); i++) {
		ret = device_create_file(&spi->dev, &st7528fb_attrs[i]);
		if (ret) {
			while (--i >= 0)
				device_remove_file(&spi->dev,
						   &st7528fb_attrs[i]);
			dev_err(&spi->dev,
				"failed to create st7528fb sysfs file\n");
			goto err_unregister_framebuffer;
		}
	}

	dev_info(&spi->dev,
		 "fb%d: %s frame buffer device, %dx%d, %d bpp, %luk, %u fps\n",
		info->node, info->fix.id, info->var.xres, info->var.yres,
		info->var.bits_per_pixel, info->screen_size >> 10,
		st7528fb->fps);

	return 0;

err_unregister_framebuffer:
	unregister_framebuffer(st7528fb->info);
err_free_kthread:
	kthread_stop(st7528fb->thread);
err_free_transfer_fb:
	vfree(st7528fb->transfer_fb);
err_free_fb:
	vfree(st7528fb->fb);
err_free_cmap:
	fb_dealloc_cmap(&info->cmap);
err_free_info:
	framebuffer_release(st7528fb->info);
err_st7528fb:
	kfree(st7528fb);
out:
	return ret;
}

static int
st7528fb_remove(struct spi_device *spi)
{
	int i;
	struct st7528fb *st7528fb = dev_get_drvdata(&spi->dev);

	for (i = 0; i < ARRAY_SIZE(st7528fb_attrs); i++)
		device_remove_file(&spi->dev, &st7528fb_attrs[i]);

	kthread_stop(st7528fb->thread);
	fb_dealloc_cmap(&st7528fb->info->cmap);
	framebuffer_release(st7528fb->info);
	vfree(st7528fb->fb);
	vfree(st7528fb->transfer_fb);
	kfree(st7528fb);

	return 0;
}

static const struct of_device_id st7528_dt_match[] = {
	{ .compatible = "dspg,st7528_spi", },
	{}
};

static struct spi_driver st7528fb_driver = {
	.driver = {
		.name	= "st7528_spi",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
		.of_match_table = st7528_dt_match,
	},
	.probe	= st7528fb_probe,
	.remove	= st7528fb_remove,
};

static int __init
st7528fb_init(void)
{
	return spi_register_driver(&st7528fb_driver);
}
module_init(st7528fb_init);

static void __exit
st7528fb_exit(void)
{
	return spi_unregister_driver(&st7528fb_driver);
}
module_exit(st7528fb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("Framebuffer driver for ST7528 based displays");
