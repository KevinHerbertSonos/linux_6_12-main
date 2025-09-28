// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2024 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_reserved_mem.h>
#include <linux/uaccess.h>
#include <video/dspgfb.h>
#include <asm/neon.h>
#include <linux/of_address.h>
#ifdef CONFIG_FB_DSPG_EXT_TE
  #include <linux/mfd/syscon.h>
  #include <linux/regmap.h>
#endif
#include <linux/list.h>

static unsigned underruns;
module_param(underruns, uint, 0644);
MODULE_PARM_DESC(underruns, "Number of underruns that happened.");

static unsigned unhandled_ints;
module_param(unhandled_ints, uint, 0644);
MODULE_PARM_DESC(unhandled_ints, "Number of unhandled interrupts.");

#define DSPGFB "dspgfb"

#define INT_FRAME_START	1
#define INT_FRAME_DONE	2
#define INT_UNDERRUN	4

#define DSPG_DSI_CMD_MODE	(1 << 14)
#define DSPG_WAIT_FOR_TE	(1 << 15)

#define DSPG_SYSCON_DSI_CTRL		0x50
#define DSPG_SYSCON_TE_DELAY(x)		(((x) & 0x3FFFFF) << 10)
#define DSPG_SYSCON_TE_CONFG(x)		(((x) &      0x7) <<  3)

static u32 pseudo_palette[16];

struct dspgfb {
	void __iomem		*regs;
	struct platform_device	*pdev;
	struct fb_info		*info0;
	struct fb_info		*info1;
	int			cur_fb;
	int			dual_display;
	unsigned		gpio;
	struct fb_videomode	mode;
	int			irq;
	struct clk		*clk;
	struct reset_control	*reset;
	int			enable_gpio;
	enum of_gpio_flags	enable_flags;

	void __iomem		*vidmem;

	int			vskip;
	int			hskip;
	bool			rotate90;
	bool			rotate_anti;
	void __iomem		*vidmem2;
	unsigned long		rot_buff;

	struct dspgfb_pdata	pdata;
	bool			pending;
	wait_queue_head_t	wq;
	struct work_struct	clk_work;

	unsigned long		lp_pixclock;
	unsigned int		lp_fps;
	unsigned long		hp_pixclock;
	unsigned long		cur_rate;
	struct notifier_block	clk_change_nb;

	const struct dspgfb_panel	*panel;

	struct completion	vsync_completion;
};

#ifdef CONFIG_DMA_SHARED_BUFFER
static struct sg_table *
dspgfb_map_dma_buf(struct dma_buf_attachment *attach,
		   enum dma_data_direction dir)
{
	/* TODO */
	pr_warn("dspgfb: %s not implemented", __func__);

	return NULL;
}

static void
dspgfb_unmap_dma_buf(struct dma_buf_attachment *attach, struct sg_table *sgt,
		     enum dma_data_direction dir)
{
	/* TODO */
	pr_warn("dspgfb: %s not implemented", __func__);
}

void
dspgfb_dma_buf_release(struct dma_buf *dma_buf)
{
	/* TODO */
	pr_warn("dspgfb: %s not implemented", __func__);
}

static const struct dma_buf_ops dspgfb_dmabuf_ops = {
	.map_dma_buf = dspgfb_map_dma_buf,
	.unmap_dma_buf = dspgfb_unmap_dma_buf,
	.release = dspgfb_dma_buf_release,
};

static struct dma_buf *
dspgfb_get_dma_buf(struct fb_info *info)
{
	struct dspgfb *dspgfb = info->par;
	struct dma_buf *buf = NULL;
	struct dma_buf_export_info dma_info;
	size_t screen_size = dspgfb->info0->screen_size;

	printk("fbdev: getting dma buffer\n"); /* TODO remove */

	if (dspgfb->dual_display)
		screen_size += dspgfb->info1->screen_size;

	dma_info.exp_name = "dspgfb_dma";
	dma_info.owner = THIS_MODULE;
	dma_info.ops = &dspgfb_dmabuf_ops;
	dma_info.size = screen_size;
	dma_info.flags = O_RDWR;
	dma_info.resv = NULL;
	dma_info.priv = &dspgfb;

	buf = dma_buf_export(&dma_info);

	return buf;
}
#else
static struct dma_buf *
dspgfb_get_dma_buf(struct fb_info *info) { return NULL; }
#endif /* CONFIG_DMA_SHARED_BUFFER */

void
process_row_4x4(void *from, void *to, unsigned width, unsigned height);
void
process_row_4x4_anti(void *from, void *to, unsigned width, unsigned height);

#ifdef CONFIG_KERNEL_MODE_NEON
static void
dspgfb_rotate(void *from, void *to, unsigned width, unsigned height,
	      int direction)
{
	unsigned block_row;

	for (block_row = 0; block_row < height / 4; block_row++) {
		kernel_neon_begin();
		if (direction)
			process_row_4x4_anti(from + block_row * width * 16,
					to + (((width - 1) * height / 4) +
							block_row) * 16,
					width, height);
		else
			process_row_4x4(from + block_row * width * 16,
					to + (height / 4 - (block_row + 1))
							* 16,
					width, height);
		kernel_neon_end();
	}
}
#else
static void
dspgfb_rotate(u16 *src, u16 *dst, unsigned int width, unsigned int height,
	      int direction)
{
	int blocksize = 16;
	int i, j, k, l;

	for (i = 0; i < width; i += blocksize) {
		for (j = 0; j < height; j += blocksize) {
			// transpose the block beginning at [i,j]
			for (k = i; k < i + blocksize; k++) {
				for (l = j; l < j + blocksize; l++) {
					dst[l + k * height] =
							src[k + l * width];
				}
			}
		}
	}
}
#endif

unsigned long
dspgfb_readl(struct dspgfb *dspgfb, unsigned long addr)
{
	return ioread32(dspgfb->regs + addr);
}

void
dspgfb_writel(struct dspgfb *dspgfb, unsigned long addr, unsigned long val)
{
	iowrite32(val, dspgfb->regs + addr);
}

static void
dspgfb_hw_enable(struct dspgfb *dspgfb)
{
	dspgfb_writel(dspgfb, LCDC_REG_LCDCCR, 1);
}

static void
dspgfb_hw_disable(struct dspgfb *dspgfb)
{
	dspgfb_writel(dspgfb, LCDC_REG_LCDCCR, 0);
}

/* If the panel supports only 16bits, the framebuffer will hold rgb565.
 * If the panel supports rgb666 or rgb888, the frame buffer will hold rgb888
 * (in case of panel which supports rgb666, the LCDC will drop the 2
 * least significat bits of each color component))
 */
static void
dspg_bpp_to_info(struct fb_var_screeninfo *var, int bits_per_pixel,
		 int alpha_in_front)
{
	if (bits_per_pixel == 16) {
		var->bits_per_pixel = 16;
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
	} else if ((bits_per_pixel == 18) || (bits_per_pixel == 24)) {
		var->bits_per_pixel = 24;
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
	} else if (bits_per_pixel == 32) {
		var->bits_per_pixel = 32;

		if (alpha_in_front) {
			var->red.offset = 16;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
		} else {
			var->red.offset = 24;
			var->red.length = 8;
			var->green.offset = 16;
			var->green.length = 8;
			var->blue.offset = 8;
			var->blue.length = 8;
		}
	}
}

static void
dspgfb_hw_param_update(struct dspgfb *dspgfb)
{
	dspgfb_writel(dspgfb, LCDC_REG_PARUP, 1);
	if (dspgfb->pdata.type & cputype)
		dspgfb_writel(dspgfb, LCDC_REG_CDISPUPR, 1);
	if (dspgfb->pdata.type & dsitype)
		/* send single new frame */
		dspgfb_hw_enable(dspgfb);
}

static int
dspgfb_set_display_bpp(struct dspgfb *dspgfb, struct fb_info *info)
{
	unsigned int bpp = 0;

	switch (info->var.bits_per_pixel) {
	case 16:
		bpp = 0;
		break;
	case 18:
	case 24:
	case 32:
		bpp = 1;
		break;
	case 2:
		bpp = 3;
		break;
	default:
		return -EINVAL;
	}

	if (info->var.bits_per_pixel == 32) {
		bpp |= 1 << 4;
		if (dspgfb->pdata.move_alpha)
			bpp |= 1 << 3;
	}

	dspgfb_writel(dspgfb, LCDC_REG_INTMR, bpp);

	/* input data transfer sizes */
	dspgfb_writel(dspgfb, LCDC_REG_INDTR,
		      info->var.xres * info->var.bits_per_pixel / 32 - 1);

	dspgfb_writel(dspgfb, LCDC_REG_OFFAR0,
		      info->var.xres * info->var.bits_per_pixel / 8);
	dspgfb_writel(dspgfb, LCDC_REG_OFFAR1,
		      info->var.xres * info->var.bits_per_pixel / 8);

	return 0;
}

static void
dspgfb_set_hw_init(struct dspgfb *dspgfb)
{
	int dispir = 0, pancsr;
#ifdef CONFIG_FB_DSPG_EXT_TE
	struct regmap *map;
	struct reg_field field_s = REG_FIELD(DSPG_SYSCON_DSI_CTRL, 0, 31);
	struct regmap_field *field;
#endif

	if (dspgfb->pdata.type & cputype)
		dspgfb_writel(dspgfb, LCDC_REG_LCDCCR, 2); /* FIF0 Reset */

	/* clear all pending interrupts (may exist from u-boot) */
	dspgfb_writel(dspgfb, LCDC_REG_INTSR,
		      dspgfb_readl(dspgfb, LCDC_REG_INTSR));

	if (dspgfb->pdata.type & cputype || dspgfb->pdata.type & dsitype)
		dspgfb_writel(dspgfb, LCDC_REG_INTER, INT_FRAME_DONE);
	else
		dspgfb_writel(dspgfb, LCDC_REG_INTER,
			      INT_FRAME_START | INT_UNDERRUN);

	if (dspgfb->pdata.type & cputype)
		dispir = 4;
	if (dspgfb->pdata.type & dsitype)
		dispir = DSPG_DSI_CMD_MODE;
	else if (dspgfb->pdata.bits_per_pixel == 24)
		dispir = 1;
	else if (dspgfb->pdata.type & ccirntsctype)
		dispir |= BIT(6) | BIT(7);
	else if (dspgfb->pdata.type & ccirpaltype)
		dispir |= BIT(6);

#ifdef CONFIG_FB_DSPG_EXT_TE
	if (dspgfb->pdata.type & ext_te) {
		if (dspgfb->pdata.type & cputype ||
		    !(dspgfb->pdata.type & dsitype)) {
			dev_info(&dspgfb->pdev->dev,
				 "external TE line only with DSI type");
			dspgfb->pdata.type &= ~ext_te;
			goto after_syscon;
		}

		map = syscon_regmap_lookup_by_phandle(
						dspgfb->pdev->dev.of_node,
						"dvf,syscfg");
		if (IS_ERR(map)) {
			dev_warn(&dspgfb->pdev->dev,
				 "could not get syscon regmap");
			dspgfb->pdata.type &= ~ext_te;
			goto after_syscon;
		}
		field = devm_regmap_field_alloc(&dspgfb->pdev->dev, map,
						field_s);
		if (IS_ERR(field)) {
			dev_warn(&dspgfb->pdev->dev,
				 "could not get syscon register");
			dspgfb->pdata.type &= ~ext_te;
			goto after_syscon;
		}
		regmap_field_write(field,
				DSPG_SYSCON_TE_DELAY(dspgfb->pdata.te_delay) |
				DSPG_SYSCON_TE_CONFG(5));

after_syscon:
		if (dspgfb->pdata.type & ext_te)
			dispir |= DSPG_WAIT_FOR_TE;
	}
#endif

	dspgfb_writel(dspgfb, LCDC_REG_DISPIR, dispir);

	pancsr = 0x6 | (dspgfb->pdata.iclk) | (dspgfb->pdata.swap_rb << 4);
	if (dspgfb->pdata.mode.sync & FB_SYNC_VERT_HIGH_ACT)
		pancsr &= ~0x2;
	if (dspgfb->pdata.mode.sync & FB_SYNC_HOR_HIGH_ACT)
		pancsr &= ~0x4;

	dspgfb_writel(dspgfb, LCDC_REG_PANCSR, pancsr);
}

static void
dspgfb_set_display_background(struct dspgfb *dspgfb,
			      unsigned short background_color)
{
	dspgfb_writel(dspgfb, LCDC_REG_BACKCPR, background_color);
}

void __weak
dspg_te_callback(void) {
	pr_warn("using dummy TE callback\n");
}

static int
dspgfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct dspgfb *dspgfb = info->par;
	dma_addr_t addr;
	int res = 0;

	if (dspgfb->pdata.type & cputype ||
	    (dspgfb->pdata.type & dsitype && !(dspgfb->pdata.type & ext_te))) {
		if (dspgfb->pdata.type & dsitype)
			dspg_te_callback();

#if 0 /* not yet tested */
		if (!dspgfb->dual_display)
			dspgfb_writel(dspgfb, LCDC_REG_INTER,
				      dspgfb_readl(dspgfb, LCDC_REG_INTER) |
				      INT_FRAME_DONE);
#endif
		/* make sure last DMA towards cpu-type lcd is finished before
		 * starting new one
		 */
		res = wait_event_interruptible_timeout(dspgfb->wq,
						       !dspgfb->pending,
						       HZ / 10);
		if (res == 0)
			dev_warn(info->dev,
				 "timeout - no frame_end interrupt!\n");
		dspgfb->pending = true;
	} else if (dspgfb->pdata.type & ext_te) {
		wait_event_interruptible(dspgfb->wq, !dspgfb->pending);
		dspgfb->pending = true;
	}

	if (dspgfb->rotate90) {
		dspgfb_rotate(dspgfb->vidmem +
			      var->yoffset * info->fix.line_length,
			      dspgfb->vidmem2 +
			      var->yoffset * info->fix.line_length,
			      info->var.xres, info->var.yres,
			      dspgfb->rotate_anti);
		addr = dspgfb->rot_buff + var->yoffset * info->fix.line_length;
	} else {
		addr = info->fix.smem_start +
		       var->yoffset * info->fix.line_length;
	}

	if (dspgfb->dual_display) {
		if (info->node == 0) {
			dspgfb_writel(dspgfb, LCDC_REG_MSBAHBA0R, addr >> 16);
			dspgfb_writel(dspgfb, LCDC_REG_LSBAHBA0R,
				      addr & 0x0000ffff);
		} else {
			dspgfb_writel(dspgfb, LCDC_REG_MSBAHBA1R, addr >> 16);
			dspgfb_writel(dspgfb, LCDC_REG_LSBAHBA1R,
				      addr & 0x0000ffff);
		}
	} else if ((dspgfb_readl(dspgfb, LCDC_REG_DISPIDXR) & 0x1) == 1 &&
		   info->node == 0) {
		dspgfb_writel(dspgfb, LCDC_REG_MSBAHBA0R, addr >> 16);
		dspgfb_writel(dspgfb, LCDC_REG_LSBAHBA0R, addr & 0x0000ffff);
		dspgfb_writel(dspgfb, LCDC_REG_DISPIDXR, 0x0);
	} else if (info->node == 0) {
		dspgfb_writel(dspgfb, LCDC_REG_MSBAHBA1R, addr >> 16);
		dspgfb_writel(dspgfb, LCDC_REG_LSBAHBA1R, addr & 0x0000ffff);
		dspgfb_writel(dspgfb, LCDC_REG_DISPIDXR, 0x1);
	} else {
		dev_warn(info->dev,
			 "write to second buffer but no dual display!\n");
	}

	/* kick lcdc hw:
	 * for cpu-type and dsi-type lcd, DMA will be started
	 * for rgb-interface lcd, ahb-address will be updated on next frame
	 */
	dspgfb_hw_param_update(dspgfb);

	if (!(dspgfb->pdata.type & cputype || dspgfb->pdata.type & dsitype)) {
		/* wait for the start of the next frame
		 * after it - we are sure that LCDC is refreshing from the new
		 * address
		 */
		dspgfb->pending = true;

		if (!dspgfb->dual_display)
			dspgfb_writel(dspgfb, LCDC_REG_INTER,
				      dspgfb_readl(dspgfb, LCDC_REG_INTER) |
				      INT_FRAME_START);

		res = wait_event_interruptible_timeout(dspgfb->wq,
						       !dspgfb->pending,
						       HZ / 10);
		if (res == 0)
			dev_warn(info->dev,
				 "timeout - no frame_start interrupt!\n");
	}

	return res;
}

static int
dspgfb_setcolreg(unsigned regno, unsigned red, unsigned green,
		 unsigned blue, unsigned transp, struct fb_info *info)
{
	u32 *palette = info->pseudo_palette;

	if (regno >= 16)
		return -EINVAL;

	/* only FB_VISUAL_TRUECOLOR supported */
	red    >>= 16 - info->var.red.length;
	green  >>= 16 - info->var.green.length;
	blue   >>= 16 - info->var.blue.length;
	transp >>= 16 - info->var.transp.length;

	palette[regno] = (red    << info->var.red.offset)   |
			 (green  << info->var.green.offset) |
			 (blue   << info->var.blue.offset)  |
			 (transp << info->var.transp.offset);

	return 0;
}

static int
dspgfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	/* For now, don't check anything... */
	return 0;
}

static int
dspgfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct dspgfb *dspgfb = info->par;
	unsigned short background_color;
	int ret;

	switch (cmd) {
	case FBIOSET_BACKGROUND:
		if (copy_from_user(&background_color, argp,
				   sizeof(unsigned short)))
			return -EFAULT;

		dspgfb_set_display_background(dspgfb,
					      background_color);
		break;

	case FBIO_WAITFORVSYNC:
		if (!dspgfb->dual_display)
			dspgfb_writel(dspgfb, LCDC_REG_INTER,
				      dspgfb_readl(dspgfb, LCDC_REG_INTER) |
				      INT_FRAME_START);

		reinit_completion(&dspgfb->vsync_completion);
		ret = wait_for_completion_interruptible_timeout(
						&dspgfb->vsync_completion,
						msecs_to_jiffies(100));
		if (!ret)
			return -ETIMEDOUT;
		break;
	}

	return 0;
}

static unsigned short
dspgfb_get_rgb565(unsigned short red, unsigned short green,
		  unsigned short blue)
{
	return ((red & 0x1f) << 11) | ((green & 0x3f) << 5) | (blue & 0x1f);
}

static int
dspgfb_cmap(struct fb_cmap *cmap, struct fb_info *info)
{
	struct dspgfb *dspgfb = info->par;

	dspgfb_writel(dspgfb, LCDC_REG_PALR0, dspgfb_get_rgb565(cmap->red[0],
		      cmap->green[0], cmap->blue[0]));
	dspgfb_writel(dspgfb, LCDC_REG_PALR1, dspgfb_get_rgb565(cmap->red[1],
		      cmap->green[1], cmap->blue[1]));
	dspgfb_writel(dspgfb, LCDC_REG_PALR2, dspgfb_get_rgb565(cmap->red[2],
		      cmap->green[2], cmap->blue[2]));
	dspgfb_writel(dspgfb, LCDC_REG_PALR3, dspgfb_get_rgb565(cmap->red[3],
		      cmap->green[3], cmap->blue[3]));

	return 0;
}

static void
dspgfb_parse_dt_regs(struct dspgfb *dspgfb)
{
	struct device_node *np = dspgfb->pdev->dev.of_node;
	u32 parsed, reg;
	u32 val = 0;
	struct property *prop;
	const __be32 *p;
	u32 i = 0;

	of_property_for_each_u32(np, "dspg,reg_write", prop, p, parsed)
		if (i++ & 1) {
			reg = parsed;
			dspgfb_writel(dspgfb, reg, val);
		} else {
			val = parsed;
		}
}

static int
dspgfb_set_par(struct fb_info *info)
{
	struct dspgfb *dspgfb = info->par;
	unsigned long addr;
	int ret, xres, yres;
	long clk_rate = 0;

	if (dspgfb->rotate90)
		info = dspgfb->info0;

	/* To set value to 0, we have to write 1 to the register.
	 * If any of the margins or vsync or hsync values are set to 0
	 * in device tree, it will be set to 1 to have the expected result.
	 */
	if (!info->var.vsync_len) {
		dev_warn(info->device,
			 "parameter is 0! setting vsync_len to 1\n");
		info->var.vsync_len = 1;
	}
	if (!info->var.upper_margin) {
		dev_warn(info->device,
			 "parameter is 0! setting upper_margin to 1\n");
		info->var.upper_margin = 1;
	}
	if (!info->var.lower_margin) {
		dev_warn(info->device,
			 "parameter 0! setting lower_margin to 1\n");
		info->var.lower_margin = 1;
	}
	if (!info->var.hsync_len) {
		dev_warn(info->device,
			 "parameter 0! setting hsync_len to 1\n");
		info->var.hsync_len = 1;
	}
	if (!info->var.left_margin) {
		dev_warn(info->device,
			 "parameter 0! setting left_margin to 1\n");
		info->var.left_margin = 1;
	}
	if (!info->var.right_margin) {
		dev_warn(info->device,
			 "parameter 0! setting right_margin to 1\n");
		info->var.right_margin = 1;
	}

	dspgfb_hw_disable(dspgfb);
	dspgfb_writel(dspgfb, LCDC_REG_DISPCR, 0);

	xres = info->var.hsync_len + info->var.left_margin +
	       info->var.xres      + info->var.right_margin;
	yres = info->var.vsync_len + info->var.upper_margin +
	       info->var.yres      + info->var.lower_margin;

	if (info->var.pixclock)
		clk_rate = info->var.pixclock;
	else
		clk_rate = (xres + dspgfb->hskip) *
			   (yres + dspgfb->vskip) * info->mode->refresh;

	clk_rate = clk_round_rate(dspgfb->clk, clk_rate);
	if (clk_rate < 0)
		return clk_rate;
	dspgfb->cur_rate = clk_rate;
	clk_set_rate(dspgfb->clk, dspgfb->cur_rate);

	dspgfb->hp_pixclock = clk_rate;
	if (dspgfb->lp_pixclock >= dspgfb->hp_pixclock)
		dspgfb->lp_pixclock = 0;
	if (dspgfb->lp_fps >= info->mode->refresh)
		dspgfb->lp_fps = 0;
	if (dspgfb->lp_pixclock == 0 && dspgfb->lp_fps != 0)
		dspgfb->lp_pixclock = (xres + dspgfb->hskip) *
				      (yres + dspgfb->vskip) *
				      dspgfb->lp_fps;

	ret = dspgfb_set_display_bpp(dspgfb, info);
	if (ret < 0)
		return ret;

	if (dspgfb->pdata.type & ccirntsctype ||
	    dspgfb->pdata.type & ccirpaltype) {
		if (dspgfb->pdata.type & ccirntsctype) {
			dspgfb_writel(dspgfb, LCDC_REG_VATR, 524);
			dspgfb_writel(dspgfb, LCDC_REG_HSTR, 267);
		} else {
			dspgfb_writel(dspgfb, LCDC_REG_VATR, 624);
			dspgfb_writel(dspgfb, LCDC_REG_HSTR, 279);
		}
		dspgfb_writel(dspgfb, LCDC_REG_HFTR, 3);
		dspgfb_writel(dspgfb, LCDC_REG_HADSTR, 1439);
		dspgfb_writel(dspgfb, LCDC_REG_HAPWR, 719);
		dspgfb_writel(dspgfb, LCDC_REG_HETR, 3);
	} else {
		dspgfb_writel(dspgfb, LCDC_REG_VSTR, info->var.vsync_len - 1);
		dspgfb_writel(dspgfb, LCDC_REG_VFTR,
			      info->var.upper_margin - 1);
		dspgfb_writel(dspgfb, LCDC_REG_VATR,
			      info->var.yres - 1 + dspgfb->vskip);
		dspgfb_writel(dspgfb, LCDC_REG_VETR,
			      info->var.lower_margin - 1);
		dspgfb_writel(dspgfb, LCDC_REG_HSTR, info->var.hsync_len - 1);
		dspgfb_writel(dspgfb, LCDC_REG_HFTR,
			      info->var.left_margin - 1);
		dspgfb_writel(dspgfb, LCDC_REG_HADSTR,
			      info->var.xres - 1 + dspgfb->hskip);
		dspgfb_writel(dspgfb, LCDC_REG_HAPWR,
			      info->var.xres - 1 + dspgfb->hskip);
		dspgfb_writel(dspgfb, LCDC_REG_HETR,
			      info->var.right_margin - 1);
	}
	dspgfb_writel(dspgfb, LCDC_REG_INDTR,
		      info->var.xres * info->var.bits_per_pixel / 32 - 1);
	dspgfb_writel(dspgfb, LCDC_REG_INDXSR, info->var.xres - 1);
	dspgfb_writel(dspgfb, LCDC_REG_INDYSR, info->var.yres - 1);

	/* display position */
	dspgfb_writel(dspgfb, LCDC_REG_DISPXSPOSR, 0);
	if (dspgfb->pdata.type & ccirntsctype) {
		dspgfb_writel(dspgfb, LCDC_REG_DISPXEPOSR, info->var.xres);
		dspgfb_writel(dspgfb, LCDC_REG_DISPYSPOS1R,
			      19 + dspgfb->vskip / 2);
		dspgfb_writel(dspgfb, LCDC_REG_DISPYEPOS1R,
			      18 + info->var.yres / 2 + dspgfb->vskip / 2);
		dspgfb_writel(dspgfb, LCDC_REG_DISPYSPOS2R,
			      282 + dspgfb->vskip / 2);
		/* FIXME: this accidental typo (should be DISPYEPOS2R) is
		 *        actually the only way it works
		 */
		dspgfb_writel(dspgfb, LCDC_REG_DISPYSPOS2R,
			      281 + info->var.yres / 2 + dspgfb->vskip / 2);
	} else if (dspgfb->pdata.type & ccirpaltype) {
		dspgfb_writel(dspgfb, LCDC_REG_DISPXEPOSR, info->var.xres);
		dspgfb_writel(dspgfb, LCDC_REG_DISPYSPOS1R,
			      22 + dspgfb->vskip / 2);
		dspgfb_writel(dspgfb, LCDC_REG_DISPYEPOS1R,
			      21 + info->var.yres / 2 + dspgfb->vskip / 2);
		dspgfb_writel(dspgfb, LCDC_REG_DISPYSPOS2R,
			      335 + dspgfb->vskip / 2);
		/* FIXME: this accidental typo (should be DISPYEPOS2R) is
		 *        actually the only way it works
		 */
		dspgfb_writel(dspgfb, LCDC_REG_DISPYSPOS2R,
			      334 + info->var.yres / 2 + dspgfb->vskip / 2);
	} else {
		dspgfb_writel(dspgfb, LCDC_REG_DISPXEPOSR,
			      info->var.xres - 1);
		dspgfb_writel(dspgfb, LCDC_REG_DISPYEPOS1R,
			      info->var.yres - 1);
	}

	/* input buffer */
	if (info->node == 0) {
		addr = info->fix.smem_start;
		dspgfb_writel(dspgfb, LCDC_REG_MSBAHBA0R, addr >> 16);
		dspgfb_writel(dspgfb, LCDC_REG_LSBAHBA0R, addr & 0xffff);

		dspgfb_writel(dspgfb, LCDC_REG_OFFAR0, info->fix.line_length);

		dspgfb_writel(dspgfb, LCDC_REG_DISPIDXR, 0);
	} else {
		addr = info->fix.smem_start;
		dspgfb_writel(dspgfb, LCDC_REG_MSBAHBA1R, addr >> 16);
		dspgfb_writel(dspgfb, LCDC_REG_LSBAHBA1R, addr & 0xffff);

		dspgfb_writel(dspgfb, LCDC_REG_OFFAR1, info->fix.line_length);

		dspgfb_writel(dspgfb, LCDC_REG_DISPIDXR, 1);
	}

	dspgfb_parse_dt_regs(dspgfb);

	dspgfb_hw_param_update(dspgfb);

	dspgfb_writel(dspgfb, LCDC_REG_DISPCR, 1);

	dspgfb_hw_enable(dspgfb);

	return 0;
}

static struct fb_ops dspgfb_ops = {
	.owner          = THIS_MODULE,
	.fb_pan_display = dspgfb_pan_display,
	.fb_check_var   = dspgfb_check_var,
	.fb_ioctl       = dspgfb_ioctl,
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
	.fb_setcolreg   = dspgfb_setcolreg,
	.fb_setcmap     = dspgfb_cmap,
	.fb_set_par     = dspgfb_set_par,
	.fb_dmabuf_export = dspgfb_get_dma_buf,
};

static irqreturn_t dspgfb_irq(int irq, void *priv)
{
	struct dspgfb *dspgfb = priv;
	int stat, handled = 0;

	/* read & clear active interrupts */
	stat = dspgfb_readl(dspgfb, LCDC_REG_INTSR);
	dspgfb_writel(dspgfb, LCDC_REG_INTSR, stat);

	if (stat & INT_FRAME_DONE) {
		if (dspgfb->dual_display) {
			gpio_set_value(dspgfb->gpio, dspgfb->cur_fb);

			dspgfb->cur_fb = dspgfb->cur_fb ? 0 : 1;
			dspgfb_writel(dspgfb, LCDC_REG_DISPIDXR,
				      dspgfb->cur_fb);
			dspgfb_hw_param_update(dspgfb);
		}
	}

	if (stat & (INT_FRAME_START | INT_FRAME_DONE)) {
		if (dspgfb->pending) {
			dspgfb->pending = false;

			if (stat & INT_FRAME_START &&
			    dspgfb->lp_pixclock != 0 &&
			    dspgfb->cur_rate != dspgfb->hp_pixclock) {
				dspgfb->cur_rate = dspgfb->hp_pixclock;

				schedule_work(&dspgfb->clk_work);
			}

			wake_up_interruptible(&dspgfb->wq);
		} else {
			if (!dspgfb->dual_display)
				/* disabling frame done interrupt has not yet
				 * been tested
				 */
				dspgfb_writel(dspgfb, LCDC_REG_INTER,
					dspgfb_readl(dspgfb, LCDC_REG_INTER) &
					~(/*INT_FRAME_DONE |*/
					  INT_FRAME_START));

			if (stat & INT_FRAME_START &&
			    dspgfb->lp_pixclock != 0 &&
			    dspgfb->cur_rate != dspgfb->lp_pixclock) {
				dspgfb->cur_rate = dspgfb->lp_pixclock;

				schedule_work(&dspgfb->clk_work);
			}
		}
		complete(&dspgfb->vsync_completion);

		handled = 1;
	}

	/* handle error interrupts */
	if (stat & INT_UNDERRUN) {
		underruns++;

		handled = 1;
	}

	if (!handled)
		unhandled_ints++;

	return IRQ_HANDLED;
}

static void
dspgfb_clk_work(struct work_struct *work)
{
	struct dspgfb *dspgfb = container_of(work, struct dspgfb, clk_work);

	clk_set_rate(dspgfb->clk, dspgfb->cur_rate);
}

static int
dspgfb_clk_change_cb(struct notifier_block *nb,
		     unsigned long event, void *data)
{
	struct dspgfb *dspgfb = container_of(nb, struct dspgfb, clk_change_nb);

	switch (event) {
	case POST_RATE_CHANGE:
		schedule_work(&dspgfb->clk_work);

		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

struct panel_entry {
	struct device_node *np;
	const struct dspgfb_panel *panel;
	struct list_head list;
};

static DEFINE_SPINLOCK(panel_list_lock);
static LIST_HEAD(panel_list);
int
dspgfb_register_panel(const struct dspgfb_panel *panel, struct device_node *np)
{
	struct panel_entry *entry = kzalloc(sizeof(*entry), GFP_KERNEL);

	if (!entry)
		return -ENOMEM;

	entry->np = np;
	entry->panel = panel;

	spin_lock(&panel_list_lock);
	list_add_tail(&entry->list, &panel_list);
	spin_unlock(&panel_list_lock);

	return 0;
}

static int
dspgfb_intmr_to_bpp(unsigned long intmr)
{
	switch (intmr & 0x7) {
	case 0x0:
		return 16;
	case 0x1:
		if (intmr & (1 << 4))
			return 32;
		return 24;
	case 0x3:
		return 2;
	}

	return -1;
}

static int
dspgfb_probe(struct platform_device *pdev)
{
	struct dspgfb *dspgfb;
	struct dspgfb_pdata *pdata = pdev->dev.platform_data;
	struct fb_info *info0;
	struct fb_info *info1 = NULL;
	struct fb_videomode *mode_copy = NULL;
	struct resource *res;
	struct resource mem_res;
	struct device_node *np = pdev->dev.of_node;
	int ret = 0, val;
	unsigned long *memory_base;
	unsigned long *memory_base2;
	struct panel_entry *entry;
	struct device_node *panel_node;

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret)
		dev_info(&pdev->dev, "not using reserved memory area\n");

	dspgfb = devm_kzalloc(&pdev->dev, sizeof(*dspgfb), GFP_KERNEL);
	if (!dspgfb)
		return -ENOMEM;
	dspgfb->pdev = pdev;

	init_completion(&dspgfb->vsync_completion);

	panel_node = of_parse_phandle(np, "panel", 0);
	if (panel_node) {
		spin_lock(&panel_list_lock);
		list_for_each_entry(entry, &panel_list, list)
			if (entry->np == panel_node) {
				dspgfb->panel = entry->panel;
				break;
			}
		spin_unlock(&panel_list_lock);

		if (!dspgfb->panel) {
			dev_info(&pdev->dev,
				"panel not yet registered; deferring probe\n");
			return -EPROBE_DEFER;
		}

		dev_info(&pdev->dev,
			 "found panel %s\n", dspgfb->panel->name);
	}

	if (dspgfb->panel && dspgfb->panel->ops.reset_display)
		dspgfb->panel->ops.reset_display(dspgfb->panel);

	if (!pdata) {
		const char *tmp;

		ret = of_property_read_u32_array(np, "dspg,mode",
						 &dspgfb->pdata.mode.refresh,
						 13);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"invalid or missing 'dspg,mode' property!\n");
			return ret;
		}

		dspgfb->pdata.name = "unknown";
		ret = of_property_read_string(np, "dspg,panel_name",
					      &dspgfb->pdata.name);
		if (ret && ret != -EINVAL) {
			dev_err(&pdev->dev, "invalid 'dspg,panel_name'\n");
			return ret;
		}

		if (of_property_read_bool(np, "dspg,cputype")) {
			dspgfb->pdata.type = cputype;
		} else if (of_property_read_bool(np, "dspg,dsitype")) {
			dspgfb->pdata.type = dsitype;

			if (of_property_read_bool(np, "dspg,ext_te"))
				dspgfb->pdata.type |= ext_te;
		} else if (of_property_read_bool(np, "dspg,ccirntsctype")) {
			dspgfb->pdata.type = ccirntsctype;
		} else if (of_property_read_bool(np, "dspg,ccirpaltype")) {
			dspgfb->pdata.type = ccirpaltype;
		}

		if (dspgfb->pdata.type & ext_te) {
			if (!of_property_read_u32(np, "dspg,te_delay", &val)) {
				if (DSPG_SYSCON_TE_DELAY(val) == 0)
					dev_warn(&pdev->dev,
						 "te_delay 0; free running\n");
				dspgfb->pdata.te_delay = val;
			} else {
				/* 0 would circumvent the WAIT_FOR_TE logic */
				dspgfb->pdata.te_delay = 1;
			}
		}

		ret = of_property_read_u32(np, "dspg,vskip", &val);
		if (ret == 0)
			dspgfb->vskip = val;

		ret = of_property_read_u32(np, "dspg,hskip", &val);
		if (ret == 0)
			dspgfb->hskip = val;

		ret = of_property_read_u32(np, "dspg,bits_per_pixel", &val);
		if (ret < 0 || (val != 16 && val != 18 && val != 24)) {
			dev_err(&pdev->dev, "invalid 'dspg,bits_per_pixel'\n");
			return ret;
		}
		dspgfb->pdata.bits_per_pixel = val;

		ret = of_property_read_u32(np, "dspg,fb_bits_per_pixel", &val);
		if (ret < 0 || (val != 16 && val != 18 &&
				val != 24 && val != 32)) {
			dev_err(&pdev->dev,
				"invalid 'dspg,fb_bits_per_pixel': %d\n",
				val);
			return ret;
		}
		dspgfb->pdata.fb_bits_per_pixel = val;

		if (of_property_read_bool(np, "dspg,rotate90") &&
		    (cpu_has_neon() || dspgfb->pdata.fb_bits_per_pixel == 16))
			dspgfb->rotate90 = true;

		if (of_property_read_bool(np, "dspg,rotate-anti"))
			dspgfb->rotate_anti = true;

		ret = of_property_read_string(np, "dspg,data_order", &tmp);
		if (ret < 0 ||
		    (strcasecmp(tmp, "rgb") != 0 &&
		     strcasecmp(tmp, "bgr") != 0 &&
		     strcasecmp(tmp, "argb") != 0 &&
		     strcasecmp(tmp, "rgba") != 0 &&
		     strcasecmp(tmp, "abgr") != 0 &&
		     strcasecmp(tmp, "bgra") != 0)) {
			dev_err(&pdev->dev, "invalid 'dspg,data_order'\n");
			return ret;
		}
		if (strcasecmp(tmp, "rgb") == 0 ||
		    strcasecmp(tmp, "argb") == 0 ||
		    strcasecmp(tmp, "rgba") == 0)
			dspgfb->pdata.swap_rb = 0;
		else if (strcasecmp(tmp, "bgr") == 0 ||
			 strcasecmp(tmp, "abgr") == 0 ||
			 strcasecmp(tmp, "bgra") == 0)
			dspgfb->pdata.swap_rb = 1;

		if (strcasecmp(tmp, "rgba") == 0 ||
		    strcasecmp(tmp, "bgra") == 0)
			dspgfb->pdata.move_alpha = 0;
		else if (strcasecmp(tmp, "argb") == 0 ||
			 strcasecmp(tmp, "abgr") == 0)
			dspgfb->pdata.move_alpha = 1;

		val = 0;
		ret = of_property_read_u32(np, "dspg,dual_display", &val);
		if (ret && ret != -EINVAL) {
			dev_err(&pdev->dev, "invalid 'dspg,dual_display'\n");
			val = 0;
		}
		dspgfb->dual_display = val;

		if (dspgfb->dual_display) {
			val = of_get_named_gpio(np,
						"dspg,dual_display_gpio", 0);
			if (val < 0) {
				dev_err(&pdev->dev,
					"invalid 'dspg,dual_display_gpio'\n");
				dspgfb->dual_display = 0;
			}
			dspgfb->gpio = val;

			if (!gpio_is_valid(dspgfb->gpio)) {
				dev_err(&pdev->dev, "gpio invalid\n");
				dspgfb->dual_display = 0;
			} else {
				ret = gpio_request(dspgfb->gpio,
						   "hookswitch_gpio");
				if (ret < 0) {
					dev_err(&pdev->dev,
						"cannot request gpio\n");
					dspgfb->dual_display = 0;
				} else {
					gpio_direction_output(dspgfb->gpio, 0);
				}
			}
		}

		if (dspgfb->dual_display)
			dspgfb->pdata.mode.refresh *= 2;

		val = 0;
		ret = of_property_read_u32(np, "dspg,lp-pixclock", &val);
		dspgfb->lp_pixclock = val;

		if (dspgfb->lp_pixclock == 0) {
			val = 0;
			ret = of_property_read_u32(np, "dspg,lp-fps", &val);
			dspgfb->lp_fps = val;
		}

		val = 0;
		ret = of_property_read_u32(np, "dspg,iclk", &val);
		if ((ret && ret != -EINVAL) || (val != 0 && val != 1)) {
			dev_err(&pdev->dev, "invalid 'dspg,iclk'\n");
			return ret;
		}
		dspgfb->pdata.iclk = val;

		dspgfb->enable_gpio = of_get_named_gpio_flags(np,
							"enable-gpio", 0,
							&dspgfb->enable_flags);

		if (dspgfb->enable_gpio >= 0 &&
		    gpio_is_valid(dspgfb->enable_gpio))
			devm_gpio_request_one(&pdev->dev, dspgfb->enable_gpio,
					      (dspgfb->enable_flags &
					       OF_GPIO_ACTIVE_LOW) ?
					      GPIOF_OUT_INIT_LOW :
					      GPIOF_OUT_INIT_HIGH,
					      "enable");
		else
			dspgfb->enable_gpio = -1;
	} else {
		dspgfb->pdata = *pdata;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "need a memory resource\n");
		return -EINVAL;
	}

	dspgfb->irq = platform_get_irq(pdev, 0);
	if (dspgfb->irq < 0) {
		dev_err(&pdev->dev, "need an irq resource\n");
		return dspgfb->irq;
	}

	if (!devm_request_mem_region(&pdev->dev, res->start,
				     resource_size(res), DSPGFB)) {
		dev_err(&pdev->dev, "failed to request memory region\n");
		return -EBUSY;
	}

	dspgfb->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(dspgfb->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		return PTR_ERR(dspgfb->reset);
	}

	ret = reset_control_deassert(dspgfb->reset);
	if (ret) {
		dev_err(&pdev->dev, "cannot deassert reset\n");
		return ret;
	}

	dspgfb->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(dspgfb->clk)) {
		dev_err(&pdev->dev, "could not get clock\n");
		return PTR_ERR(dspgfb->clk);
	}
	clk_prepare_enable(dspgfb->clk);

	info0 = framebuffer_alloc(0, &pdev->dev);
	if (!info0) {
		dev_err(&pdev->dev, "out of memory\n");
		ret = -ENOMEM;
		goto err_clk;
	}
	dspgfb->info0 = info0;

	if (dspgfb->dual_display || dspgfb->rotate90) {
		info1 = framebuffer_alloc(0, &pdev->dev);
		if (!info1) {
			dev_err(&pdev->dev, "out of memory\n");
			ret = -ENOMEM;
			goto err_clk;
		}
		dspgfb->info1 = info1;

		if (dspgfb->rotate90) {
			mode_copy = devm_kzalloc(&pdev->dev,
						 sizeof(*mode_copy),
						 GFP_KERNEL);
			if (!mode_copy) {
				dev_err(&pdev->dev,
					"could not allocate videomode");
				dspgfb->rotate90 = false;
			}
		}
	}

	info0->pseudo_palette = pseudo_palette;
	info0->par = dspgfb;
	info0->flags = FBINFO_FLAG_DEFAULT;

	strcpy(info0->fix.id, DSPGFB);
	info0->fix.type = FB_TYPE_PACKED_PIXELS;
	info0->fix.visual = FB_VISUAL_TRUECOLOR;
	info0->fix.accel = FB_ACCEL_NONE;

	dspg_bpp_to_info(&info0->var, dspgfb->pdata.fb_bits_per_pixel,
			 dspgfb->pdata.move_alpha);
	info0->fix.line_length =
		dspgfb->pdata.mode.xres * info0->var.bits_per_pixel / 8;

	info0->fbops = &dspgfb_ops;
	info0->mode = &dspgfb->pdata.mode;
	fb_videomode_to_var(&info0->var, &dspgfb->pdata.mode);

	/* triple buffering */
	info0->fix.ypanstep = 1;
	info0->var.yres_virtual *= 3;

	info0->screen_size = info0->fix.line_length * info0->var.yres_virtual;
	info0->fix.smem_len = info0->screen_size;
	info0->var.activate = FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;

	ret = fb_alloc_cmap(&info0->cmap, 256, 0);
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to allocate color map\n");
		goto err_fb;
	}

	if (dspgfb->dual_display)
		/* only difference lies in buffer address */
		memcpy(info1, info0, sizeof(*info0));

	/* setup memory */
	np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (np) {
		unsigned long vscreen_size = info0->screen_size +
					     (dspgfb->dual_display ?
					      info1->screen_size : 0);

		ret = of_address_to_resource(np, 0, &mem_res);
		if (ret) {
			dev_info(&pdev->dev,
				"could not get resource from address\n");
			goto mem_fallback;
		} else if (resource_size(&mem_res) < vscreen_size) {
			dev_err(&pdev->dev, "reserved memory is too small\n");
			ret = -ENOMEM;
			goto err_cmap;
		}

		dspgfb->vidmem = devm_memremap(&pdev->dev, mem_res.start,
					       vscreen_size, MEMREMAP_WB);
		if (!dspgfb->vidmem) {
			dev_err(&pdev->dev,
				"could not allocate framebuffer (%lu bytes)\n",
				info0->screen_size + (dspgfb->dual_display ?
				info1->screen_size : 0));
			goto mem_fallback;
		}
		memory_base = (unsigned long *)mem_res.start;

		if (dspgfb->rotate90) {
			if (resource_size(&mem_res) >= 2 * vscreen_size) {
				dspgfb->vidmem2 =
					devm_memremap(&pdev->dev,
						      mem_res.start +
						      vscreen_size,
						      vscreen_size,
						      MEMREMAP_WB);
				memory_base2 =
					(unsigned long *)(mem_res.start +
							  vscreen_size);
			}

			if (!dspgfb->vidmem2) {
				dev_err(&pdev->dev,
					"could not allocate rotation buffer\n");
				dspgfb->rotate90 = false;
			} else {
				dspgfb->rot_buff = (unsigned long)memory_base2;
			}
		}
	}
mem_fallback:
	if (!dspgfb->vidmem) {
		unsigned long vscreen_size = info0->screen_size +
					     (dspgfb->dual_display ?
					      info1->screen_size : 0);
		dev_info(&pdev->dev, "using DMA shared pool\n");

		dspgfb->vidmem = dmam_alloc_coherent(&pdev->dev, vscreen_size,
						(dma_addr_t *)&memory_base,
						GFP_KERNEL & ~__GFP_ZERO);
		if (!dspgfb->vidmem) {
			dev_err(&pdev->dev,
				"could not allocate framebuffer (%lu bytes)\n",
				info0->screen_size + (dspgfb->dual_display ?
				info1->screen_size : 0));
			ret = -ENOMEM;
			goto err_cmap;
		}
		if (dspgfb->rotate90) {
			dspgfb->vidmem2 = dmam_alloc_coherent(&pdev->dev,
						vscreen_size,
						(dma_addr_t *)&memory_base2,
						GFP_KERNEL);

			if (!dspgfb->vidmem2) {
				dev_err(&pdev->dev,
					"could not allocate rotation buffer\n");
				dspgfb->rotate90 = false;
			} else {
				dspgfb->rot_buff = (unsigned long)memory_base2;
			}
		}
	}

	info0->fix.smem_start = (unsigned long)memory_base;
	info0->screen_base = dspgfb->vidmem;

	if (dspgfb->dual_display) {
		info1->fix.smem_start = (unsigned long)(memory_base +
							info0->screen_size);
		info1->screen_base = dspgfb->vidmem + info0->screen_size;
	}

	if (dspgfb->panel && dspgfb->panel->ops.power_on)
		dspgfb->panel->ops.power_on(dspgfb->panel, info0);

	/* init hardware */
	dspgfb->regs = devm_ioremap(&pdev->dev, res->start,
				    resource_size(res));
	if (!dspgfb->regs) {
		dev_err(&pdev->dev, "unable to map registers\n");
		ret = -ENOMEM;
		goto err_cmap;
	}

	dev_set_drvdata(&pdev->dev, dspgfb);
	init_waitqueue_head(&dspgfb->wq);
	INIT_WORK(&dspgfb->clk_work, dspgfb_clk_work);

	underruns = 0;
	unhandled_ints = 0;

	ret = devm_request_threaded_irq(&pdev->dev, dspgfb->irq, dspgfb_irq,
					NULL, 0, DSPGFB, dspgfb);
	if (ret < 0) {
		dev_err(&pdev->dev, "request irq %d failed\n", dspgfb->irq);
		goto err_cmap;
	}

	dspgfb_set_hw_init(dspgfb);

	/* preserve boot splash from bootloader */
	if (((dspgfb_readl(dspgfb, LCDC_REG_MSBAHBA0R) << 16) |
	     (dspgfb_readl(dspgfb, LCDC_REG_LSBAHBA0R) & 0xffff)) !=
	    (unsigned long)memory_base ||
	    dspgfb_intmr_to_bpp(dspgfb_readl(dspgfb, LCDC_REG_INTMR)) !=
	    info0->var.bits_per_pixel) {
		memset(info0->screen_base, 0, info0->screen_size);
		if (dspgfb->dual_display)
			memset(info1->screen_base, 0, info1->screen_size);
	}

	/* fb_set_var must be called after all other things, since it
	 * calls fb_pan_display which needs irq and wq to be
	 * initialized */
	ret = fb_set_var(info0, &info0->var);
	if (ret < 0)
		goto err_cmap;

	if (dspgfb->dual_display) {
		ret = fb_set_var(info1, &info1->var);
		if (ret < 0)
			goto err_cmap;
	}

	if (dspgfb->rotate90) {
		memcpy(info1, info0, sizeof(*info0));

		info1->mode = mode_copy;
		memcpy(mode_copy, info0->mode, sizeof(*mode_copy));
		swap(info1->mode->xres, info1->mode->yres);
		fb_videomode_to_var(&info1->var, info1->mode);
		info1->var.yres_virtual *= 2;
		info1->fix.line_length =
			info1->mode->xres * info1->var.bits_per_pixel / 8;
	}
	/* register framebuffer in userspace */
	if (dspgfb->rotate90) {
		ret = register_framebuffer(info1);
	} else {
		ret = register_framebuffer(info0);
	}
	if (ret < 0)
		goto err_cmap;
	if (dspgfb->dual_display) {
		ret = register_framebuffer(info1);
		if (ret < 0)
			goto err_cmap;
	}

	/* register parent clock change notifier */
	dspgfb->clk_change_nb.notifier_call = dspgfb_clk_change_cb;
	dspgfb->clk_change_nb.next = NULL;

	ret = clk_notifier_register(clk_get_parent(dspgfb->clk),
				    &dspgfb->clk_change_nb);
	if (ret) {
		dev_err(&pdev->dev, "could not register clock notifier\n");
		dspgfb->clk_change_nb.notifier_call = NULL;
	}

	ret = of_platform_populate(np, NULL, NULL, &pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "error populating subdevices\n");
		goto err_cmap;
	}

	if (dspgfb->rotate90)
		dev_info(&pdev->dev,
			 "fb%d: %s frame buffer, %s (%dx%d, %d bpp, %luk)\n",
			 info1->node, info1->fix.id, dspgfb->pdata.name,
			 info1->var.xres, info1->var.yres,
			 info1->var.bits_per_pixel, info1->screen_size >> 10);
	else
		dev_info(&pdev->dev,
			 "fb%d: %s frame buffer, %s (%dx%d, %d bpp, %luk)\n",
			 info0->node, info0->fix.id, dspgfb->pdata.name,
			 info0->var.xres, info0->var.yres,
			 info0->var.bits_per_pixel, info0->screen_size >> 10);
	if (dspgfb->dual_display) {
		dev_info(&pdev->dev,
			 "fb%d: %s frame buffer, %s (%dx%d, %d bpp, %luk)\n",
			 info1->node, info1->fix.id, dspgfb->pdata.name,
			 info1->var.xres, info1->var.yres,
			 info1->var.bits_per_pixel, info1->screen_size >> 10);
	}

	return 0;

err_cmap:
	fb_dealloc_cmap(&info0->cmap);
	if (dspgfb->dual_display)
		fb_dealloc_cmap(&info1->cmap);
err_fb:
	framebuffer_release(dspgfb->info0);
	if (dspgfb->dual_display)
		framebuffer_release(dspgfb->info1);
err_clk:
	clk_disable_unprepare(dspgfb->clk);

	return ret;
}

static int
dspgfb_remove(struct platform_device *pdev)
{
	struct dspgfb *dspgfb = dev_get_drvdata(&pdev->dev);

	fb_dealloc_cmap(&dspgfb->info0->cmap);
	framebuffer_release(dspgfb->info0);
	if (dspgfb->dual_display) {
		fb_dealloc_cmap(&dspgfb->info1->cmap);
		framebuffer_release(dspgfb->info1);
	}

	clk_disable_unprepare(dspgfb->clk);

	return 0;
}

#if defined(CONFIG_PM)
static int
dspgfb_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dspgfb *dspgfb = dev_get_drvdata(&pdev->dev);

	if (dspgfb->panel && dspgfb->panel->ops.power_off)
		dspgfb->panel->ops.power_off(dspgfb->panel, dspgfb->info0);

	if (dspgfb->enable_gpio >= 0)
		gpio_set_value(dspgfb->enable_gpio,
			(dspgfb->enable_flags & OF_GPIO_ACTIVE_LOW) ? 1 : 0);

	dspgfb_writel(dspgfb, LCDC_REG_DISPCR, 0);
	dspgfb_hw_param_update(dspgfb);

	disable_irq(dspgfb->irq);

	clk_disable(dspgfb->clk);

	return 0;
}

static int
dspgfb_dev_resume(struct platform_device *pdev)
{
	struct dspgfb *dspgfb = dev_get_drvdata(&pdev->dev);

	clk_enable(dspgfb->clk);

	enable_irq(dspgfb->irq);

	dspgfb_writel(dspgfb, LCDC_REG_DISPCR, 1);
	dspgfb_hw_param_update(dspgfb);
	dspgfb_hw_param_update(dspgfb);

	if (dspgfb->enable_gpio >= 0)
		gpio_set_value(dspgfb->enable_gpio,
			(dspgfb->enable_flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1);

	if (dspgfb->panel && dspgfb->panel->ops.power_on)
		dspgfb->panel->ops.power_on(dspgfb->panel, dspgfb->info0);

	return 0;
}
#else
#define dspgfb_dev_suspend NULL
#define dspgfb_dev_resume NULL
#endif /* CONFIG_PM */

/* TODO: integrate CPU-type panels as "dspg,dspgfb-cpu". Can use .data to pass
 * extra information if required...
 */
static const struct of_device_id dspgfb_of_ids[] = {
	{ .compatible = "dspg,dspgfb-lcd" },
	{ },
};

static struct platform_driver dspgfb_driver = {
	.remove = dspgfb_remove,
	.suspend = dspgfb_dev_suspend,
	.resume = dspgfb_dev_resume,
	.driver = {
		.name = DSPGFB,
		.owner = THIS_MODULE,
		.of_match_table = dspgfb_of_ids,
	},
	.probe = dspgfb_probe,
};
module_platform_driver(dspgfb_driver);

MODULE_DESCRIPTION("DSPG LCD Controller framebuffer driver");
MODULE_LICENSE("GPL");
