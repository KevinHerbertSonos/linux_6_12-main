// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fb.h>

#include "vpp_api.h"
#include "vpp_mem.h"
#include "vpp_fb.h"
#include "tz_driver.h"
#include "avio_common.h"
#include "kernel_compatibility.h"

//Enable to push a black frame upon bootup-This is must when fastlogo.ta is used
#define SYNAFB_VPP_ENABLE_BLACKFRAME_ON_BOOTUP

static vpp_config_params vpp_config_param = { 0 };
static bool is_vpp_initialized;
static VPP_MEM fb_shm_handle;

static struct fb_var_screeninfo mrvl_fb_var = {
	.xres = MV_SYSTEM_WIDTH_DEFAULT,
	.yres =	MV_SYSTEM_HEIGHT_DEFAULT,
	.xres_virtual = MV_SYSTEM_WIDTH_DEFAULT,
	.yres_virtual = MV_SYSTEM_HEIGHT_DEFAULT,
	.bits_per_pixel = 32,
	.red = { 0, 8, 0 },
	.green = { 0, 8, 0 },
	.blue = { 0, 8, 0 },
	.activate = FB_ACTIVATE_NOW,
	.height = -1,
	.width = -1,
	.pixclock = 1487500,
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 64,
	.vsync_len = 2,
	.vmode = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo mrvl_fb_fix = {
	.id = "SYNAFB",
	.smem_start = 0,
	.smem_len = 0,
	.type = FB_TYPE_PACKED_PIXELS,
	.type_aux = 0,
	.visual = FB_VISUAL_TRUECOLOR,
	.xpanstep = 1,
	.ypanstep = 1,
	.ywrapstep = 1,
	.line_length = 32,
	.mmio_start = 0,
	.mmio_len = 0,
	.accel = FB_ACCEL_NONE,
};

static int mrvl_fb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info);
static int mrvl_fb_set_par(struct fb_info *info);
static int mrvl_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info);
static int mrvl_fb_blank(int blank, struct fb_info *info);
static int mrvl_fb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info);
static int mrvl_fb_mmap(struct fb_info *info,
			struct vm_area_struct *vma);
static int mrvl_fb_open(struct fb_info *info, int user);
static int mrvl_fb_release(struct fb_info *info, int user);

static struct fb_ops mrvl_fb_ops = {
	.owner          = THIS_MODULE,
	.fb_open        = mrvl_fb_open,
	.fb_release     = mrvl_fb_release,
	.fb_check_var   = mrvl_fb_check_var,
	.fb_set_par     = mrvl_fb_set_par,
	.fb_setcolreg	= mrvl_fb_setcolreg,
	.fb_blank       = mrvl_fb_blank,
	.fb_pan_display = mrvl_fb_pan_display,
	.fb_mmap        = mrvl_fb_mmap,
};

static atomic_t mrvl_fb_open_cnt = ATOMIC_INIT(0);

static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;
	return length;
}

static int mrvl_fb_release(struct fb_info *info, int user)
{
	if (atomic_dec_return(&mrvl_fb_open_cnt) < 0)
		return -EBUSY; /* already released */
	return 0;
}

static int mrvl_fb_setup(struct fb_info *info)
{
	int ret;
	VPP_MEM_LIST *shm_list = info->par;

	shm_list->dev = info->device;
	ret = VPP_MEM_InitMemory(shm_list);
	if (ret != 0) {
		pr_err("Failed to initialize memory\n");
		return -ENOMEM;
	}

	fb_shm_handle.size = (mrvl_fb_var.yres*vpp_config_param.fb_count) *
		mrvl_fb_fix.line_length;
	ret = VPP_MEM_AllocateMemory(shm_list, VPP_MEM_TYPE_DMA,
				&fb_shm_handle, GFP_KERNEL | __GFP_NOWARN);
	if (ret != 0) {
		VPP_MEM_DeInitMemory(shm_list);
		pr_err("Failed to allocate screenbase in Open\n");
		return -ENOMEM;
	}

	ret = MV_VPP_FB_Init(shm_list);
	if (ret) {
		VPP_MEM_FreeMemory(shm_list, VPP_MEM_TYPE_DMA, &fb_shm_handle);
		VPP_MEM_DeInitMemory(shm_list);
		pr_err("VPP Init failed\n");
		return -ENOMEM;
	}

	info->screen_base = (char *)fb_shm_handle.k_addr;
	mrvl_fb_fix.smem_start = (unsigned long)fb_shm_handle.k_addr;
	is_vpp_initialized = true;

	return 0;
}

static int mrvl_fb_open(struct fb_info *info, int user)
{
	int ret = 0;

	if (atomic_inc_return(&mrvl_fb_open_cnt) > 1)
		return -EBUSY; /* already open */

	if (!is_vpp_initialized) {
		ret = mrvl_fb_setup(info);
		if (ret) {
			pr_err("FB setup FAILED\n");
			return ret;
		}
#ifdef SYNAFB_VPP_ENABLE_BLACKFRAME_ON_BOOTUP
		MV_VPP_FB_DisplayFrame(&fb_shm_handle, 0, info->var.yres,
			info->var.xres, info->var.yres, info->fix.line_length);
#endif //SYNAFB_VPP_ENABLE_BLACKFRAME_ON_BOOTUP
	}

	return ret;
}

static int mrvl_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			 u_int transp, struct fb_info *info)
{
	return -EINVAL;
}

static int mrvl_fb_check_var(struct fb_var_screeninfo *var,
			 struct fb_info *info)
{
	unsigned long line_length;

	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;
	if (var->bits_per_pixel < 32)
		return -EINVAL;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	line_length =
		get_line_length(var->xres_virtual, var->bits_per_pixel);
	switch (var->bits_per_pixel) {
	case 32:/* RGBA 8888 */
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 16;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	}
	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

static int mrvl_fb_set_par(struct fb_info *info)
{
	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);
	return 0;
}

static int mrvl_fb_blank(int blank, struct fb_info *info)
{
	int ret = 0;

	pr_info("%s: blank %d\n", __func__, blank);
	switch (blank) {
	case FB_BLANK_UNBLANK:
		ret = MV_VPP_SetHdmiTxControl(1);
		break;
	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_POWERDOWN:
		ret = MV_VPP_SetHdmiTxControl(0);
		break;
	}
	return 0;
}

static int mrvl_fb_pan_display(struct fb_var_screeninfo *var,
			   struct fb_info *info)
{
	MV_VPP_FB_DisplayFrame(&fb_shm_handle, info->var.xoffset, info->var.yoffset,
			info->var.xres, info->var.yres, info->fix.line_length);
	return 0;
}

static int mrvl_fb_mmap(struct fb_info *info,
			struct vm_area_struct *vma)
{

	int result = 0;
	size_t req_size, fb_size;

	req_size = vma->vm_end - vma->vm_start;
	fb_size = info->fix.smem_len;
	if (req_size > fb_size) {
		pr_info("requested map is greater than framebuffer\n");
		return -EOVERFLOW;
	}

	if (info->screen_base == NULL) {
		pr_info("fb mmap failed!!!!\n");
		return -ENOMEM;
	}

	result = VPP_MEM_MapMemory(&fb_shm_handle, vma);

	return result;
}

static int mrvl_fb_probe(struct platform_device *dev)
{
	struct fb_info *info;
	int retval = 0;
	VPP_MEM_LIST *mem_list;
	int len = 0;
	int width, height;
	struct device_node *np = dev->dev.of_node;

	//Defer probe until dependent soc module/s are probed/initialized
	if (!is_vpp_driver_initialized())
		return -EPROBE_DEFER;

	if (of_property_read_u32(np, "frame-size-ndx", &vpp_config_param.frame_size_ndx))
		vpp_config_param.frame_size_ndx = 0; //default is 'MV_SYSTEM_WIDTH_DEFAULT * MV_SYSTEM_HEIGHT_DEFAULT'

	if (of_property_read_u32(np, "disp1-res-id", &vpp_config_param.disp1_res_id))
		vpp_config_param.disp1_res_id = -1;

	if (of_property_read_u32(np, "disp1-type", &vpp_config_param.disp1_out_type))
		vpp_config_param.disp1_out_type = 0;

	if (of_property_read_u32(np, "disp1-bitdepth", &vpp_config_param.disp1_bit_depth))
		vpp_config_param.disp1_bit_depth = OUTPUT_BIT_DEPTH_8BIT;

	if (of_property_read_u32(np, "disp1-colorformat", &vpp_config_param.disp1_colorformat))
		vpp_config_param.disp1_colorformat = OUTPUT_COLOR_FMT_RGB888;

	vpp_config_param.frame_rate = 0;
	pr_debug("mrvl_fb_probe %d> resID[%d]disptype[%d]\n",
		__LINE__, vpp_config_param.disp1_res_id, vpp_config_param.disp1_out_type);

	of_node_put(np);

	if ((vpp_config_param.disp1_out_type != VOUT_HDMI) && \
		(vpp_config_param.disp1_out_type != VOUT_DSI))
	{
		pr_err("Unsupported display out type [%d]\n",
			vpp_config_param.disp1_out_type);
		return -EINVAL;
	}

	if ((vpp_config_param.display_mode < VPP_VOUT_SINGLE_MODE_PRI) ||\
		(vpp_config_param.display_mode >= VPP_VOUT_MODE_MAX))
	{
		pr_err("Unsupported display Mode [%d]\n",
			vpp_config_param.display_mode);
		return -EINVAL;
	}

	vpp_config_param.enable_frame_buf_copy = 1;
#ifdef MRVL_FB_ENABLE_DOUBLE_BUFFERING
	vpp_config_param.fb_count = 2;
#else
	vpp_config_param.fb_count = 3;
#endif //MRVL_FB_ENABLE_DOUBLE_BUFFERING
	vpp_config_param.callback = NULL;
	vpp_config_param.data = NULL;
	vpp_config_param.active_planes = (1 << PLANE_GFX1);

	MV_VPP_FB_ConfigParams(&vpp_config_param);

	//Update the size of the frame buffer
	MV_VPP_FB_GetInputFrameSize(&width, &height);
	mrvl_fb_var.xres = mrvl_fb_var.xres_virtual = width;
	mrvl_fb_var.yres = mrvl_fb_var.yres_virtual = height;
	pr_info("mrvl_fb_probe: frame-rate:%d, frame-size-ndx:%d, frame-size:%dx%d\n",
		vpp_config_param.frame_rate, vpp_config_param.frame_size_ndx, width, height);

	len = get_line_length(mrvl_fb_var.xres_virtual,
			mrvl_fb_var.bits_per_pixel);
	if (len <= 0) {
		pr_err("Invalid Line Length\n");
		return -EINVAL;
	}

	mem_list = kzalloc(sizeof(VPP_MEM_LIST), GFP_KERNEL | __GFP_NOWARN);
	if (!mem_list) {
		pr_err("Failed to allocate memory for device\n");
		return -ENOMEM;
	}

	mrvl_fb_fix.line_length = len;
	pr_info("mrvl_fb_fix.line_length:%x\n",
			mrvl_fb_fix.line_length);
	info = framebuffer_alloc(sizeof(struct fb_info), &dev->dev);
	if (!info) {
		retval = -ENOMEM;
		goto err_free_shm_handle;
	}

	info->fbops = &mrvl_fb_ops;
	info->var = mrvl_fb_var;
	info->screen_base = NULL;
	mrvl_fb_fix.smem_start =  0;
	mrvl_fb_fix.smem_len = 0;
	mrvl_fb_fix.smem_len = mrvl_fb_var.yres*vpp_config_param.fb_count*
			mrvl_fb_fix.line_length;
	info->fix = mrvl_fb_fix;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0))
	info->flags = FBINFO_FLAG_DEFAULT;
#endif

	retval = register_framebuffer(info);
	if (retval < 0)
		goto err_free_fb;

	info->par = mem_list;

	platform_set_drvdata(dev, info);

	return 0;

err_free_fb:
	framebuffer_release(info);
err_free_shm_handle:
	kfree(mem_list);
	return retval;
}

static RET_TYPE mrvl_fb_remove(struct platform_device *dev)
{
	struct fb_info *info = platform_get_drvdata(dev);

	if (info) {
		VPP_MEM_LIST *shm_list = info->par;
		unregister_framebuffer(info);
		if (is_vpp_initialized) {
			MV_VPP_FB_Deinit();
			if (fb_shm_handle.handle)
				VPP_MEM_FreeMemory(shm_list, VPP_MEM_TYPE_DMA, &fb_shm_handle);
		}
		VPP_MEM_DeInitMemory(shm_list);
		framebuffer_release(info);
	}
	RETURN_VALUE;
}

static const struct of_device_id fb_mrvl_match[] = {
	{
		.compatible = "marvell,berlin-fb",
	},
};

static struct platform_driver mrvl_fb_driver = {
	.probe	= mrvl_fb_probe,
	.remove = mrvl_fb_remove,
	.driver = {
		.name = "mrvl_fb",
		.of_match_table = fb_mrvl_match,
	},
};

static int __init mrvl_fb_init(void)
{
	int ret = 0;
	if (!avio_util_get_quiescent_flag()) {
		ret = platform_driver_register(&mrvl_fb_driver);
	}
	return ret;
}
late_initcall(mrvl_fb_init);

static void __exit mrvl_fb_exit(void)
{
	platform_driver_unregister(&mrvl_fb_driver);
}
module_exit(mrvl_fb_exit);

MODULE_LICENSE("GPL");
