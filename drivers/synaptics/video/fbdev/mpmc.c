/*
 * drivers/video/mpmc.c - a framebuffer driver for CPU-type displays connected
 * to the MPMC controller of DSPG DVF chips
 *
 * Copyright (C) 2020 DSP Group.
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
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/file.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <dt-bindings/dspg/mpmc.h>

#define FBOWNER_SIZE	32
#define DEFAULT_FBOWNER	"kernel"

#define MPMC_FB_NAME	"mpmc"
#define LCD_WIDTH	240
#define LCD_HEIGHT	320

#define MAX_CMD_PARAMS	32

#define CONFIG_FB_MPMC_DEFERRED_IO_DELAY 5

#define MPMCControl		0x000
#define MPMCStatus		0x004
#define MPMCConfig		0x008
#define MPMCStaticConfig1	0x220
#define MPMCStaticWaitWen1	0x224
#define MPMCStaticWaitOen1	0x228
#define MPMCStaticWaitWr1	0x234
#define MPMCStaticWaitTurn1	0x238

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group");
MODULE_DESCRIPTION("Framebuffer driver for CPU-type displays at MPMC");

struct mpmc_transfer {
	struct list_head link;
	unsigned int offset;
	unsigned int toffset;
	uint16_t row;
	uint16_t nr_rows;
};

struct mpmc {
	struct device *dev;
	void __iomem *base;
	void __iomem *regs;
	struct clk *clk;
	struct reset_control *reset;

	unsigned int width, height;
	unsigned int wait_wr;
	unsigned int partial;
	unsigned int lower_limit, upper_limit;

	/* fake framebuffer */
	unsigned long alloc_size;
	struct fb_info *info;
	char *fb;
	char *shadow_fb;
	int nr_pages;
	struct mpmc_transfer *page_transfer;
	int *pages, *tpages, *rows;

	/* refresh vars */
	wait_queue_head_t vblank_wq;
	struct mutex transfer_lock;

	atomic_t vblanks;
	int cd_gpio;
	int cs_gpio;
	int reset_gpio;

	DECLARE_BITMAP(rows_changed, LCD_HEIGHT);
};

static struct fb_fix_screeninfo mpmc_fix = {
	.id = MPMC_FB_NAME,
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.accel = FB_ACCEL_NONE,
};

static struct fb_var_screeninfo mpmc_var = {
	.xoffset = 0,
	.yoffset = 0,
	.bits_per_pixel = 24,
	.red.offset = 0,
	.red.length = 8,
	.red.msb_right = 0,
	.green.offset = 8,
	.green.length = 8,
	.green.msb_right = 0,
	.blue.offset = 0,
	.blue.length = 16,
	.blue.msb_right = 0,
	.transp.offset = 0,
	.transp.length = 0,
	.transp.msb_right = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.width = 43,
	.height = 60,
};

static void
mpmc_writeb(struct mpmc *mpmc, unsigned char dat)
{
	writeb(dat, mpmc->base);
}

static void
mpmc_command(struct mpmc *mpmc, unsigned char *buf, int nr_params)
{
	int i;

	/* set DCX to 0 (control) */
	dev_dbg(mpmc->dev, "command 0x%x\n", buf[0]);
	gpio_set_value(mpmc->cs_gpio, 0);
	ndelay(100);
	gpio_set_value(mpmc->cd_gpio, 0);
	ndelay(100);

	mpmc_writeb(mpmc, buf[0]);

	if (nr_params > 1) {
		/* set DCX to 1 (data) */
		dev_dbg(mpmc->dev, "data mode\n");
		ndelay(100);
		gpio_set_value(mpmc->cd_gpio, 1);
		ndelay(100);

		for (i = 0; i < nr_params - 1; i++)
			mpmc_writeb(mpmc, buf[i + 1]);
	}

	ndelay(100);
	gpio_set_value(mpmc->cs_gpio, 1);
}

static void
mpmc_do_transfer_rect(struct mpmc *mpmc, unsigned int row_start,
			  unsigned int row_end, unsigned int col_start,
			  unsigned int col_end)
{
	unsigned char *buf;
	int i, len, width = mpmc->info->var.bits_per_pixel / 8;

	gpio_set_value(mpmc->cs_gpio, 0);

	/* set DCX to 0 (control) */
	gpio_set_value(mpmc->cd_gpio, 0);

	ndelay(100);

	/* set column address */
	dev_dbg(mpmc->dev, "command 0x%x\n", 0x2a);
	mpmc_writeb(mpmc, 0x2a);

	/* set DCX to 1 (data) */
	ndelay(100);
	gpio_set_value(mpmc->cd_gpio, 1);
	ndelay(100);

	dev_dbg(mpmc->dev, "data mode\n");
	mpmc_writeb(mpmc, col_start >> 8);
	mpmc_writeb(mpmc, col_start & 0xff);
	mpmc_writeb(mpmc, col_end >> 8);
	mpmc_writeb(mpmc, col_end & 0xff);

	ndelay(100);
	gpio_set_value(mpmc->cd_gpio, 0);
	ndelay(100);

	/* set row address */
	dev_dbg(mpmc->dev, "command 0x%x\n", 0x2b);
	mpmc_writeb(mpmc, 0x2b);

	ndelay(100);
	gpio_set_value(mpmc->cd_gpio, 1);
	ndelay(100);

	dev_dbg(mpmc->dev, "data mode\n");
	mpmc_writeb(mpmc, row_start >> 8);
	mpmc_writeb(mpmc, row_start & 0xff);
	mpmc_writeb(mpmc, row_end >> 8);
	mpmc_writeb(mpmc, row_end & 0xff);

	ndelay(100);
	gpio_set_value(mpmc->cd_gpio, 0);
	ndelay(100);

	/* memory write */
	dev_dbg(mpmc->dev, "command 0x%x\n", 0x2c);
	mpmc_writeb(mpmc, 0x2c);

	dev_dbg(mpmc->dev, "data mode\n");
	ndelay(100);
	gpio_set_value(mpmc->cd_gpio, 1);
	ndelay(100);

	if ((col_start == 0) && (col_end == mpmc->width - 1)) {
		buf = &mpmc->fb[mpmc->width * row_start * width];
		len = mpmc->width * (row_end - row_start + 1) * width;

		len -= 3;

		/* send data from now on */
		for (i = 0; i < len; i++)
			writeb(buf[i], mpmc->base);
	} else {
		int j;

		for (j = row_start; j <= row_end; j++) {
			buf = &mpmc->fb[(mpmc->width * i +
							col_start) * width];
			len = (col_end - col_start) * width;

			/* send data from now on */
			for (i = 0; i < len; i++)
				writeb(buf[i], mpmc->base);
		}
	}

	ndelay(100);
	gpio_set_value(mpmc->cs_gpio, 1);
}

static void
mpmc_do_transfer_only_updated_rows(struct mpmc *mpmc)
{
	int i, chunk;
	int width = mpmc->width * mpmc->info->var.bits_per_pixel / 8;

	for (i = 0; i < mpmc->height; i++) {
		if (memcmp(&mpmc->fb[width * i], &mpmc->shadow_fb[width * i],
			   width))
			set_bit(i, mpmc->rows_changed);
		else
			clear_bit(i, mpmc->rows_changed);
	}

	i = 0;
	while (i < mpmc->height) {
		if (!test_bit(i, mpmc->rows_changed)) {
			i++;
			continue;
		}

		chunk = find_next_zero_bit(mpmc->rows_changed,
					   mpmc->height, i);
		chunk -= i;
		if (chunk > mpmc->height - i)
			chunk = mpmc->height - i;

		mpmc_do_transfer_rect(mpmc, i, i + chunk - 1, 0,
				      mpmc->width - 1);

		memcpy(&mpmc->shadow_fb[width * i], &mpmc->fb[width * i],
		       chunk * width);

		i += chunk;
	}
}

static void
mpmc_do_transfer(struct mpmc *mpmc)
{
	if (mpmc->partial)
		mpmc_do_transfer_only_updated_rows(mpmc);
	else
		mpmc_do_transfer_rect(mpmc, 0, mpmc->height - 1, 0,
				      mpmc->width - 1);
}

static void
mpmc_do_transfer_page(struct mpmc *mpmc, unsigned int page,
			  int rows)
{
	struct mpmc_transfer *tfp = &mpmc->page_transfer[page];
	unsigned int trows = rows;

	if (rows <= 0)
		trows = tfp->nr_rows;

	mpmc_do_transfer_rect(mpmc, tfp->row, tfp->row + trows, 0,
			      mpmc->width - 1);
}

static void
mpmc_deferred_io(struct fb_info *info, struct list_head *pagelist)
{
	struct mpmc_transfer *tfp;
	struct page *cur;
	struct mpmc *mpmc = info->par;
	int i2 = -1, i;
	int gap = 0;

	if (list_empty(pagelist))
		return;

	mutex_lock(&mpmc->transfer_lock);
	flush_kernel_vmap_range(mpmc->fb, mpmc->alloc_size);

	memset(mpmc->pages, 0, mpmc->nr_pages * sizeof(int));
	memset(mpmc->tpages, 0, mpmc->nr_pages * sizeof(int));

	list_for_each_entry(cur, pagelist, lru)
		mpmc->pages[cur->index] = 1;

	for (i = 0; i < mpmc->nr_pages; i++) {
		if (mpmc->pages[i] != 0) {
			tfp = &mpmc->page_transfer[i];
			if (i2 == -1)
				i2 = i;

			if (!gap) {
				mpmc->tpages[i2]++;
				mpmc->rows[i2] = tfp->row + tfp->nr_rows;
			}
			gap = 0;
		} else {
			gap = 1;
			i2++;
		}
	}

	/* send pages to display */
	for (i = 0; i < mpmc->nr_pages; i++) {
		tfp = &mpmc->page_transfer[i];
		if (mpmc->tpages[i] == 0)
			continue;
		mpmc_do_transfer_page(mpmc, i, mpmc->rows[i] - tfp->row);
	}

	/* signal the next vblank */
	atomic_inc(&mpmc->vblanks);
	wake_up_interruptible(&mpmc->vblank_wq);
	mutex_unlock(&mpmc->transfer_lock);
}

static struct fb_deferred_io mpmc_defio = {
	.delay		= HZ / 20,
	.deferred_io	= mpmc_deferred_io,
};

static ssize_t
mpmc_write(struct fb_info *info, const char __user *buf, size_t count,
	   loff_t *ppos)
{
	struct mpmc *mpmc;
	unsigned int off;

	/* skip if senseless :) */
	if (!count)
		return 0;

	off = *ppos;
	mpmc = info->par;

	if (off > info->screen_size)
		return -ENOSPC;

	if ((count + off) > info->screen_size)
		return -ENOSPC;

	/* copy from userspace to the fb memory */
	count -= copy_from_user(info->screen_base + off, buf, count);
	*ppos += count;

	/*
	 * XXX could be optimized by just transferring
	 * the display pages that have changed
	 */
	mutex_lock(&mpmc->transfer_lock);
	mpmc_do_transfer(mpmc);
	mutex_unlock(&mpmc->transfer_lock);

	/* return nicely */
	if (count)
		return count;

	/* we should always be able to write at least one byte */
	return -EFAULT;
}

static int
mpmc_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return remap_vmalloc_range(vma, (void *)info->fix.smem_start,
				   vma->vm_pgoff);
}

static int
mpmc_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct mpmc *mpmc = info->par;
	int ret = 0;
	unsigned int vblanks;

	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		vblanks = atomic_read(&mpmc->vblanks);
		unlock_fb_info(info);
		/* XXX */
		do {
			if (!list_empty(&mpmc->info->fbdefio->pagelist))
				ret = wait_event_interruptible(mpmc->vblank_wq,
					atomic_read(&mpmc->vblanks) != vblanks);
			else
				break;
		} while (ret < 0);
		lock_fb_info(info);
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/* setcolreg implementation from 'simplefb', upstream */
static int
mpmc_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
	       unsigned int blue, unsigned int transp, struct fb_info *info)
{
	u32 *pal = info->pseudo_palette;
	u32 cr = red >> (16 - info->var.red.length);
	u32 cg = green >> (16 - info->var.green.length);
	u32 cb = blue >> (16 - info->var.blue.length);
	u32 value;

	if (regno >= 16)
		return -EINVAL;

	value = (cr << info->var.red.offset) |
		(cg << info->var.green.offset) |
		(cb << info->var.blue.offset);

	if (info->var.transp.length > 0) {
		u32 mask = (1 << info->var.transp.length) - 1;

		mask <<= info->var.transp.offset;
		value |= mask;
	}

	pal[regno] = value;

	return 0;
}

static struct fb_ops mpmc_ops = {
	.owner = THIS_MODULE,
	.fb_write = mpmc_write,
	.fb_setcolreg = mpmc_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_imageblit = cfb_imageblit,
	.fb_copyarea = cfb_copyarea,
	.fb_mmap = mpmc_mmap,
	.fb_ioctl = mpmc_ioctl,
};

static ssize_t
partial_store(struct device *dev, struct device_attribute *attr,
	      const char *buf, size_t count)
{
	struct mpmc *mpmc = dev_get_drvdata(dev);
	unsigned long value;
	int ret;

	ret = kstrtoul(buf, 0, &value);
	if (ret < 0)
		return ret;

	mpmc->partial = !!value;

	return count;
}

static ssize_t
partial_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mpmc *mpmc = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", mpmc->partial ? "yes" : "no");
}
static DEVICE_ATTR_RW(partial);

static struct attribute *mpmc_attrs[] = {
	&dev_attr_partial.attr,
	NULL,
};

static const struct attribute_group mpmc_attribute_group = {
	.attrs = mpmc_attrs,
};

enum init_state {
	INIT_CMD,
	INIT_NUM_PARAMS,
	INIT_PARAMS,
	INIT_MDELAY,
	INIT_UDELAY,
	INIT_RST,
};


static int
mpmc_parse_display_sequence(struct mpmc *mpmc, const char *sequence)
{
	struct device_node *pnode =
		of_parse_phandle(mpmc->dev->of_node, "init-sequence", 0);
	struct property *prop;
	u32 key;
	const __be32 *p;
	enum init_state state = INIT_CMD;
	unsigned char buf[255];
	unsigned int cur_param = 0;
	int nr_params;

	if (!pnode)
		return -ENXIO;

	of_property_for_each_u32(pnode, sequence, prop, p, key) {
		switch (state) {
		case INIT_CMD:
			switch (key) {
			case MPMC_INIT_MDELAY:
				state = INIT_MDELAY;
				break;
			case MPMC_INIT_UDELAY:
				state = INIT_UDELAY;
				break;
			case MPMC_INIT_RST:
				state = INIT_RST;
				break;
			default:
				cur_param = 0;
				buf[cur_param++] = key;
				state = INIT_NUM_PARAMS;
				break;
			}
			break;
		case INIT_NUM_PARAMS:
			if (key > MAX_CMD_PARAMS)
				return -EINVAL;
			nr_params = key + 1;
			if (nr_params > 1) {
				state = INIT_PARAMS;
			} else {
				mpmc_command(mpmc, buf, nr_params);
				state = INIT_CMD;
			}
			break;
		case INIT_PARAMS:
			if (cur_param >= nr_params)
				return -EIO;
			buf[cur_param++] = key;
			if (cur_param == nr_params) {
				mpmc_command(mpmc, buf, nr_params);
				state = INIT_CMD;
			}
			break;
		case INIT_MDELAY:
			mdelay(key);
			state = INIT_CMD;
			break;
		case INIT_UDELAY:
			udelay(key);
			state = INIT_CMD;
			break;
		case INIT_RST:
			if (mpmc->reset_gpio >= 0)
				gpio_set_value(mpmc->reset_gpio, !!key);
			state = INIT_CMD;
			break;
		}
	}

	of_node_put(pnode);

	return 0;
}

static int
mpmc_probe(struct platform_device *pdev)
{
	int ret, page;
	unsigned int row, nr_rows, alloc_size;
	struct mpmc *mpmc;
	struct fb_info *info;
	struct device_node *np;
	struct resource *regs, *base;
	u32 val;

	np = pdev->dev.of_node;
	if (np == NULL)
		dev_info(&pdev->dev, "of node is null\n");

	/* allocate driver data */
	mpmc = devm_kzalloc(&pdev->dev, sizeof(*mpmc), GFP_KERNEL);
	if (!mpmc)
		return -ENOMEM;

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs)
		return -EINVAL;

	base = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!base)
		return -EINVAL;

	mpmc->regs = devm_ioremap_resource(&pdev->dev, regs);
	if (IS_ERR(mpmc->regs))
		return PTR_ERR(mpmc->regs);

	mpmc->base = devm_ioremap_resource(&pdev->dev, base);
	if (IS_ERR(mpmc->base))
		return PTR_ERR(mpmc->base);

	mpmc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(mpmc->clk)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		return PTR_ERR(mpmc->clk);
	}
	clk_prepare_enable(mpmc->clk);

	mpmc->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(mpmc->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		return PTR_ERR(mpmc->reset);
	}

	ret = reset_control_deassert(mpmc->reset);
	if (ret)
		return ret;

	mpmc->wait_wr = 3;
	ret = of_property_read_u32(np, "wait-wr", &mpmc->wait_wr);
	if (ret < 0 && ret != -EINVAL) {
		dev_err(&pdev->dev, "error reading 'wait_wr' property\n");
		return ret;
	}

	atomic_set(&mpmc->vblanks, 0);

	mpmc->partial = 0;
	mpmc->lower_limit = 0;
	mpmc->upper_limit = 255;

	mpmc->width = LCD_WIDTH;
	ret = of_property_read_u32(np, "width", &mpmc->width);
	if (ret < 0 && ret != -EINVAL) {
		dev_err(&pdev->dev, "error reading 'width' property\n");
		return ret;
	}
	mpmc->height = LCD_HEIGHT;
	ret = of_property_read_u32(np, "height", &mpmc->height);
	if (ret < 0 && ret != -EINVAL) {
		dev_err(&pdev->dev, "error reading 'height' property\n");
		return ret;
	}

	mpmc_var.xres = mpmc->width;
	mpmc_var.yres = mpmc->height;
	mpmc_var.xres_virtual = mpmc->width;
	mpmc_var.yres_virtual = mpmc->height;
	mpmc_fix.line_length = mpmc->width * (mpmc_var.bits_per_pixel / 8);

	mpmc->cd_gpio = of_get_named_gpio(np, "cd-gpio", 0);
	if (mpmc->cd_gpio < 0) {
		dev_err(&pdev->dev, "cd gpio not specified in devicetree\n");
		return ret;
	}

	mpmc->cs_gpio = of_get_named_gpio(np, "cs-gpio", 0);
	if (mpmc->cs_gpio < 0) {
		dev_err(&pdev->dev, "cs gpio not specified in devicetree\n");
		return ret;
	}

	/* The reset GPIO is optional. */
	mpmc->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);

	mutex_init(&mpmc->transfer_lock);
	init_waitqueue_head(&mpmc->vblank_wq);

	mpmc->dev = &pdev->dev;

	/* setup framebuffer */
	info = framebuffer_alloc(sizeof(u32) * 256, &pdev->dev);
	if (!info)
		return -ENOMEM;

	/* attach info to driver data */
	mpmc->info = info;

	info->pseudo_palette = (void *)(mpmc->info + 1); /* info->par; */
	info->par = mpmc;
	info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;

	info->var = mpmc_var;
	info->fix = mpmc_fix;

	info->fbops = &mpmc_ops;

	mpmc_defio.delay = CONFIG_FB_MPMC_DEFERRED_IO_DELAY;
	info->fbdefio = &mpmc_defio;

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret < 0)
		goto err_free_info;

	/* request gpio for CD line */
	ret = devm_gpio_request(&pdev->dev, mpmc->cd_gpio, "mpmc cd");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request CD gpio %d\n",
			mpmc->cd_gpio);
		goto err_free_cmap;
	}
	gpio_direction_output(mpmc->cd_gpio, 0);

	/* request gpio for CS line */
	ret = devm_gpio_request(&pdev->dev, mpmc->cs_gpio, "mpmc cs");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request CS gpio %d\n",
			mpmc->cs_gpio);
		goto err_free_cmap;
	}
	gpio_direction_output(mpmc->cs_gpio, 1);

	/* request gpio for reset line */
	if (mpmc->reset_gpio >= 0) {
		ret = devm_gpio_request(&pdev->dev, mpmc->reset_gpio,
					"mpmc reset");
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request reset gpio %d\n",
				mpmc->reset_gpio);
			goto err_free_cmap;
		}
		gpio_direction_output(mpmc->reset_gpio, 0);
	}

	info->screen_size = info->fix.line_length * info->var.yres_virtual;
	alloc_size = PAGE_ALIGN(info->screen_size);
	mpmc->nr_pages = alloc_size / PAGE_SIZE;

	/* setup fake framebuffer */
	ret = -ENOMEM;
	mpmc->alloc_size = alloc_size;
	mpmc->fb = vmalloc_32_user(alloc_size);
	if (!mpmc->fb)
		goto err_free_cmap;

	memset(mpmc->fb, 0xff, alloc_size);

	info->screen_base = (char __iomem *)mpmc->fb;
	info->fix.smem_start = (unsigned long)info->screen_base;
	info->fix.smem_len = alloc_size;

	mpmc->page_transfer = devm_kzalloc(&pdev->dev,
		sizeof(struct mpmc_transfer) * mpmc->nr_pages, GFP_KERNEL);

	if (!mpmc->page_transfer)
		goto err_free_fb;

	mpmc->pages = devm_kzalloc(&pdev->dev, mpmc->nr_pages * sizeof(int),
				   GFP_KERNEL);
	if (!mpmc->pages)
		goto err_free_fb;

	mpmc->tpages = devm_kzalloc(&pdev->dev, mpmc->nr_pages * sizeof(int),
				    GFP_KERNEL);
	if (!mpmc->pages)
		goto err_free_fb;

	mpmc->rows = devm_kzalloc(&pdev->dev, mpmc->nr_pages * sizeof(int),
				  GFP_KERNEL);
	if (!mpmc->pages)
		goto err_free_fb;

	mpmc->shadow_fb = vmalloc(alloc_size);
	if (!mpmc->shadow_fb)
		goto err_free_fb;

	memset(mpmc->shadow_fb, 0xff, alloc_size);

	nr_rows = (((PAGE_SIZE / (mpmc->info->var.bits_per_pixel / 8)) +
		  mpmc->width - 1) / mpmc->width) + 1;
	for (page = 0; page < mpmc->nr_pages; page++) {
		memset(&mpmc->page_transfer[page], 0,
		       sizeof(struct mpmc_transfer));

		row = (PAGE_SIZE * page) /
		      ((mpmc->info->var.bits_per_pixel / 8) * mpmc->width);
		if ((row + nr_rows) >= mpmc->height)
			nr_rows -= row + nr_rows - mpmc->height + 1;
		/* offset in framebuffer */
		mpmc->page_transfer[page].offset = row *
			mpmc->width * (info->var.bits_per_pixel / 8);
		/* offset in transfer buffer */
		mpmc->page_transfer[page].toffset = row * mpmc_fix.line_length;
		mpmc->page_transfer[page].row = row;
		mpmc->page_transfer[page].nr_rows = nr_rows;
	}

	/* attach driver data to the device */
	platform_set_drvdata(pdev, mpmc);

	val = readl(mpmc->regs + MPMCStaticConfig1);
	val &= ~(1 << 7); /* PB 0 */
	val &= ~(1 << 6); /* Chip select polarity active low (default) */
	val &= ~0x3; /* Memory width: 8 bit */
	writel(val, mpmc->regs + MPMCStaticConfig1);
	writel(0x0, mpmc->regs + MPMCStaticWaitWen1);
	writel(0x0, mpmc->regs + MPMCStaticWaitOen1);
	/* 2 might also work, 1 or 0 don't work */
	writel(mpmc->wait_wr, mpmc->regs + MPMCStaticWaitWr1);
	/* 3 = 5 HCLK cycle write access time, at 166MHz, i.e., 30ns */
	writel(0x0, mpmc->regs + MPMCStaticWaitTurn1);

	mpmc_parse_display_sequence(mpmc, "power-on");

	mpmc_do_transfer_rect(mpmc, 0, mpmc->height - 1, 0, mpmc->width - 1);

	/* register framebuffer userspace device */
	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register framebuffer\n");
		goto err_free_defio;
	}

	fb_deferred_io_init(mpmc->info);

	if (fb_prepare_logo(info, 0))
		fb_show_logo(info, 0);

	ret = sysfs_create_group(&pdev->dev.kobj, &mpmc_attribute_group);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to create mpmc sysfs file\n");
		goto err_unregister_framebuffer;
	}

	dev_info(&pdev->dev,
		 "fb%d: %s frame buffer device, %dx%d, %d bpp, %luk\n",
		 info->node, info->fix.id, info->var.xres, info->var.yres,
		 info->var.bits_per_pixel, info->screen_size >> 10);

	return 0;

err_unregister_framebuffer:
	unregister_framebuffer(mpmc->info);
err_free_defio:
	fb_deferred_io_cleanup(mpmc->info);
err_free_fb:
	vfree(mpmc->fb);
err_free_cmap:
	fb_dealloc_cmap(&info->cmap);
err_free_info:
	framebuffer_release(mpmc->info);

	return ret;
}

static int
mpmc_remove(struct platform_device *pdev)
{
	struct mpmc *mpmc = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &mpmc_attribute_group);
	fb_deferred_io_cleanup(mpmc->info);
	unregister_framebuffer(mpmc->info);
	fb_dealloc_cmap(&mpmc->info->cmap);
	framebuffer_release(mpmc->info);
	vfree(mpmc->shadow_fb);
	vfree(mpmc->fb);

	return 0;
}

static const struct of_device_id mpmc_dt_match[] = {
	{ .compatible = "dspg,mpmc-cputype", },
	{}
};
MODULE_DEVICE_TABLE(of, mpmc_dt_match);

static struct platform_driver mpmc_driver = {
	.driver = {
		.name	= "mpmc",
		.owner	= THIS_MODULE,
		.of_match_table = mpmc_dt_match,
	},
	.probe	= mpmc_probe,
	.remove	= mpmc_remove,
};

module_platform_driver(mpmc_driver);
