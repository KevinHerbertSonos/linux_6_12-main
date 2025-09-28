/**
 * @file frame_buffer.c
 * @brief MIPI DSI controller driver
 *
 * Copyright (C) 2010 Synopsys, Inc. All rights reserved.
 *
 * @version 1.00a first release
 */

#include "frame_buffer.h"
#include "../include/mipi_fb.h"
#include "../api/mipi_dsih_api.h"
#include "../api/mipi_dsih_dphy.h"
#include "../api/mipi_dsih_hal.h"
#include "video_if.h"
#include "mipi_displays.h"

struct mipi_dsi_dev *pdev_[2];
struct mipi_dsi_dev *pdev;
int fb_count = 0;

void
init_frame_buffer(struct mipi_dsi_dev *dev)
{
	if (dev == NULL) {
		pr_err("%s:mipi_dsi_dev is NULL", FUNC_NAME);
		return;
	}

	// Init fb_fix_screeninfo
	strcpy(dev->fb.info.fix.id,"MIPI DSI");
	dev->fb.info.fix.type = FB_TYPE_PACKED_PIXELS;
	dev->fb.info.fix.visual = FB_VISUAL_TRUECOLOR;
	dev->fb.info.fix.xpanstep = 0;		// No HW panning
	dev->fb.info.fix.ypanstep = 0;		// No HW panning
	dev->fb.info.fix.ywrapstep = 0;		// No HW ywrap
	dev->fb.info.fix.accel = FB_ACCEL_NONE;	// No hardware accelerator
	dev->fb.info.fix.capabilities = FB_CAP_FOURCC;

	// Init fb_var_screeninfo
	dev->fb.info.var.xres = 640;
	dev->fb.info.var.yres = 480;
	dev->fb.info.var.xres_virtual = 640;
	dev->fb.info.var.yres_virtual = 480;
	dev->fb.info.var.xoffset = 0;
	dev->fb.info.var.yoffset = 0;
	dev->fb.info.var.bits_per_pixel = 16;	// Default RGB565
	dev->fb.info.var.grayscale = 0;
	dev->fb.info.var.activate = FB_ACTIVATE_NOW;
	dev->fb.info.var.height = -1;
	dev->fb.info.var.width = -1;
	dev->fb.info.var.pixclock = 20000;	// in pico-seconds
	dev->fb.info.var.left_margin = 64;
	dev->fb.info.var.right_margin = 64;
	dev->fb.info.var.upper_margin = 32;
	dev->fb.info.var.lower_margin = 32;
	dev->fb.info.var.hsync_len = 64;
	dev->fb.info.var.vsync_len = 2;
	dev->fb.info.var.vmode = FB_VMODE_NONINTERLACED;

	// Configure the operations
	dev->fb.ops.owner = THIS_MODULE;
	dev->fb.ops.fb_open = fb_mipi_open;
	dev->fb.ops.fb_release = fb_mipi_release;
	dev->fb.ops.fb_read = fb_mipi_read;
	dev->fb.ops.fb_write = fb_mipi_write;
	dev->fb.ops.fb_check_var = fb_mipi_check_var;
	dev->fb.ops.fb_set_par = fb_mipi_set_par;
	dev->fb.ops.fb_setcolreg = fb_mipi_setcolreg;
	dev->fb.ops.fb_setcmap = fb_mipi_setcmap;
	dev->fb.ops.fb_blank = fb_mipi_blank;
	dev->fb.ops.fb_pan_display = fb_mipi_pan_display;
	dev->fb.ops.fb_fillrect = fb_mipi_fillrect;
	dev->fb.ops.fb_copyarea = fb_mipi_copyarea;
	dev->fb.ops.fb_imageblit = fb_mipi_imageblit;
	dev->fb.ops.fb_cursor = fb_mipi_cursor;
	dev->fb.ops.fb_sync = fb_mipi_sync;
	dev->fb.ops.fb_ioctl = fb_mipi_ioctl;
	dev->fb.ops.fb_get_caps = fb_mipi_get_caps;
	dev->fb.ops.fb_destroy = fb_mipi_destroy;

	// Configure the info structure
	dev->fb.info.fbops = &dev->fb.ops;
	mutex_init(&dev->fb.info.lock);
	mutex_init(&dev->fb.info.mm_lock);
	dev->fb.info.state = FBINFO_STATE_SUSPENDED;

	// Update the internal device pointer
	pdev_[fb_count] = dev;
	fb_count++;
}

int bus32_write(uint32_t offset, uint32_t data)
{
	iowrite32(data,(void *)(pdev->core_addr + offset));

	return 0;
}

int bus32_read(uint32_t offset, uint32_t *data)
{
	*data = ioread32((void *)(pdev->core_addr + offset));

	return 0;
}

int
fb_mipi_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int param = 0;
	int tmp = 0;

	static unsigned rx_en = 0;
	static unsigned ack_en = 0;
	static unsigned eotp_tx_en = 0;
	static unsigned eotp_rx_en = 0;
	static unsigned ecc_en = 0;
	static unsigned cmd_hs = 0;

	fb_ioctl_data fb_data;
	dsi_basic_config_t dsi_config;
	cmd_write_t cmd_write;
	cmd_read_t cmd_read;
	tear_t tear;
	dphy_t dphy;
	presp_t	presp;

	pdev = pdev_[info->node - 1];

	if (pdev == NULL) {
		pr_err("%s:mipi_dsi_dev device is NULL\n", FUNC_NAME);
		return -EINVAL;
	}

	disable_irq(pdev->irq[0]);
	disable_irq(pdev->irq[1]);
	
	switch(cmd){
	case FB_MIPI_CORE_READ:
		if (copy_from_user(&fb_data, (void __user *)arg,
				sizeof(fb_ioctl_data)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		ret = bus32_read(fb_data.address, &fb_data.value);

		if (ret) {
			pr_info("%s:READ:  reg 0x%08x [-EIO]\n",
						FUNC_NAME, fb_data.address);

			enable_irq(pdev->irq[0]);
			enable_irq(pdev->irq[1]);

			return ret;
		}
		if (copy_to_user((void __user *)arg, &fb_data,
				sizeof(fb_ioctl_data)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");

		//pr_info("%s:READ:  reg 0x%08x - value 0x%08x\n",FUNC_NAME, fb_data.address, fb_data.value);
		break;

	case FB_MIPI_CORE_WRITE:
		if (copy_from_user(&fb_data, (void __user *)arg,
						sizeof(fb_ioctl_data)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		ret = bus32_write(fb_data.address, fb_data.value);
		if (ret){ 			
			enable_irq(pdev->irq[0]);
			enable_irq(pdev->irq[1]);
			return ret;
		}

		if (copy_to_user((void __user *)arg, &fb_data,
						sizeof(fb_ioctl_data)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		break;

	case FB_MIPI_BASE_ADDR:
		ret = (uint32_t)pdev->core_addr;
		if (copy_to_user((void __user *) arg, &ret, sizeof(ret)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		break;
/********************************************VIF**************************************/		
	case FB_MIPI_VIF_TEST:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_test_mode(pdev,param);		
		break;
	case FB_MIPI_VIF_HRES:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_h_active(pdev,param);		
		break;
	case FB_MIPI_VIF_HBP:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_horizontal_blanking(pdev,param);		
		break;
	case FB_MIPI_VIF_HSYNC:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_horizontal_sync(pdev,param);		
		break;
	case FB_MIPI_VIF_LTIME:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_line_time(pdev,param);		
		break;
	case FB_MIPI_VIF_FPS:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_fps(pdev,param);		
		break;
	case FB_MIPI_VIF_VRES:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_v_active(pdev,param);		
		break;
	case FB_MIPI_VIF_VBP:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_vertical_blanking(pdev,param);		
		break;
	case FB_MIPI_VIF_VFP:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_vertical_front_porch(pdev,param);		
		break;
		
	case FB_MIPI_VIF_VSYNC:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_vertical_sync(pdev,param);		
		break;
	case FB_MIPI_VIF_PSEL:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_phase(pdev,param);		
		break;
// 	case FB_MIPI_VIF_RAW:
// 		copy_from_user(&param, (void __user *)arg,
// 						sizeof(int));
// 		video_if_raw_seq(pdev,param);		
// 		break;
	case FB_MIPI_VIF_BPP:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_set_color_mode(pdev,param);		
		break;
	case FB_MIPI_VIF_PCONF:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_set_color_mode(pdev,param);		
		break;
		
	case FB_MIPI_VIF_EDPI:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_edpi(pdev,param);		
		break;
	case FB_MIPI_VIF_TE:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_wait_for_tearing(pdev,param);		
		break;
	case FB_MIPI_VIF_PRE:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		video_if_config(pdev,param);		
		break;
		

/**********************************************************************************/		
	case FB_MIPI_PATTERN_GENERATOR:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		switch(param){
			case 0:
				stop_video_pattern(pdev);
				break;
			case 1:
				start_video_pattern(pdev,0,0);
				break;
			case 2:
				start_video_pattern(pdev,1,0);
				break;
			case 3:
				start_video_pattern(pdev,0,1);
				break;
			default:
				pr_err("%s:Option not valid!\n", FUNC_NAME);
				break;
		}
				
		break;
/************************************DPI****************************************/
	case FB_MIPI_DPI_WRITE:
		if (copy_from_user(&(pdev->dpi_video), (void __user *)arg,
						sizeof(dsih_dpi_video_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");

		if (dsi_get_transition_times(pdev) == FALSE){
			pr_err("error configuring video - freq out of range\n");
			copy_dpi_param_changes(&pdev->dpi_video_old,&pdev->dpi_video);
		}
		else{
			ret = mipi_dsih_dpi_video(pdev);
			if (ret){
				pr_err("error configuring video %d\n",ret);
				copy_dpi_param_changes(&pdev->dpi_video_old,&pdev->dpi_video);
			}
			else{		
				mipi_dsih_reset_controller(pdev);
				copy_dpi_param_changes(&pdev->dpi_video,&pdev->dpi_video_old);
			}
		}			
		break;		
	case FB_MIPI_DPI_READ:
		if (copy_to_user((void __user *)arg, &(pdev->dpi_video),
				sizeof(dsih_dpi_video_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		break;	
	case FB_MIPI_DPI_PRE_CONFIG:
		if (copy_from_user(&dsi_config, (void __user *)arg,
						sizeof(dsi_basic_config_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		pre_video_mode(pdev,dsi_config.screen,dsi_config.lanes);
		
		if (dsi_get_transition_times(pdev) == FALSE){
			pr_err("error configuring video - freq out of range\n");
			copy_dpi_param_changes(&pdev->dpi_video_old,&pdev->dpi_video);
		}
		else{
			ret = mipi_dsih_dpi_video(pdev);
			if (ret){
				pr_err("error configuring video %d\n",ret);
				copy_dpi_param_changes(&pdev->dpi_video_old,&pdev->dpi_video);
			}
			else{		
				mipi_dsih_reset_controller(pdev);
				copy_dpi_param_changes(&pdev->dpi_video,&pdev->dpi_video_old);
			}
		}			
		break;	
/************************************EDPI****************************************/
	case FB_MIPI_EDPI_WRITE:
		if (copy_from_user(&(pdev->cmd_mode_video), (void __user *)arg,
						sizeof(dsih_cmd_mode_video_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");

		ret =  mipi_dsih_dphy_configure(pdev, pdev->cmd_mode_video.no_of_lanes, pdev->cmd_mode_video.byte_clock * 8);
		if (!ret)
			pr_err("PHY configuration error\n");
		ret = mipi_dsih_edpi_video(pdev);
		if (!ret)
			pr_err("eDPI configuration error\n");
		else
		{
			mipi_dsih_reset_controller(pdev);
			copy_edpi_param_changes(&pdev->cmd_mode_video, &pdev->cmd_mode_video_old);
		}
		break;		
	case FB_MIPI_EDPI_READ:
		if (copy_to_user((void __user *)arg, &(pdev->cmd_mode_video),
				sizeof(dsih_cmd_mode_video_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		break;	
	case FB_MIPI_EDPI_PRE_CONFIG:
		if (copy_from_user(&param, (void __user *)arg,
						sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		pre_command_mode(pdev,param);
		
		ret =  mipi_dsih_dphy_configure(pdev, pdev->cmd_mode_video.no_of_lanes, pdev->cmd_mode_video.byte_clock * 8);
		if (!ret)
			pr_err("PHY configuration error\n");
		ret = mipi_dsih_edpi_video(pdev);
		if (!ret)
			pr_err("eDPI configuration error\n");
		else
		{
			mipi_dsih_reset_controller(pdev);
			copy_edpi_param_changes(&pdev->cmd_mode_video, &pdev->cmd_mode_video_old);
		}
		
		break;	
/*******************************************************************************/
	case FB_MIPI_PLATFORM_INIT:
		if (copy_from_user(&dsi_config, (void __user *)arg,
							sizeof(dsi_basic_config_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		dsi_platform_init(pdev,dsi_config.screen,dsi_config.video_mode,dsi_config.lanes);
		break;	
/*******************************WRITE**************************************/		
	case FB_MIPI_DCS_WRITE:
		if (copy_from_user(&cmd_write, (void __user *)arg,
							sizeof(cmd_write_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		mipi_dsih_cmd_mode(pdev, 1);
		/* vc, opcode, payload */
		mipi_dsih_dcs_wr_cmd(pdev, cmd_write.vc, cmd_write.cmd_buffer, cmd_write.length);
		
		break;
		
	case FB_MIPI_GEN_WRITE:
		if (copy_from_user(&cmd_write, (void __user *)arg,
							sizeof(cmd_write_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		mipi_dsih_cmd_mode(pdev, 1);
		/* vc, opcode, payload */
		mipi_dsih_gen_wr_cmd(pdev, cmd_write.vc, cmd_write.cmd_buffer, cmd_write.length);
		
		break;

	case FB_MIPI_PKT_WRITE:
		if (copy_from_user(&cmd_write, (void __user *)arg,
							sizeof(cmd_write_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		mipi_dsih_cmd_mode(pdev, 1);
		/* vc, opcode, payload */
		mipi_dsih_gen_wr_packet(pdev, cmd_write.vc,cmd_write.data_type,cmd_write.cmd_buffer, cmd_write.length);
		
		break;
/**********************************READ**************************************/		
	case FB_MIPI_READ_DCS:
		if (copy_from_user(&cmd_read, (void __user *)arg,
							sizeof(cmd_read_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		cmd_read.read_bytes = mipi_dsih_dcs_rd_cmd(pdev, cmd_read.vc, cmd_read.command, cmd_read.bytes_to_read, cmd_read.cmd_buffer);
		
		if (copy_to_user((void __user *)arg, &cmd_read,
				sizeof(cmd_read_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
	break;
	
	case FB_MIPI_READ_GEN:
		if (copy_from_user(&cmd_read, (void __user *)arg,
							sizeof(cmd_read_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		if(cmd_read.command != 0)
			cmd_read.read_bytes = mipi_dsih_gen_rd_cmd(pdev, cmd_read.vc, (unsigned char*)cmd_read.command,1,cmd_read.bytes_to_read, cmd_read.cmd_buffer);
		else
			cmd_read.read_bytes = mipi_dsih_gen_rd_cmd(pdev, cmd_read.vc, 0 , 0, cmd_read.bytes_to_read, cmd_read.cmd_buffer);
		
		if (copy_to_user((void __user *)arg, &cmd_read,
				sizeof(cmd_read_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
	break;
	
	case FB_MIPI_READ_PACKET:
		if (copy_from_user(&cmd_read, (void __user *)arg,
							sizeof(cmd_read_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		cmd_read.read_bytes = mipi_dsih_gen_rd_packet(pdev, cmd_read.vc, cmd_read.command,cmd_read.param[1],cmd_read.param[0] ,cmd_read.bytes_to_read, cmd_read.cmd_buffer);
		
		if (copy_to_user((void __user *)arg, &cmd_read,
				sizeof(cmd_read_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
	break;
/***************************************************************************/			
	case FB_MIPI_RX:
		if (copy_from_user(&rx_en, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		mipi_dsih_enable_rx(pdev, rx_en);
		break;	
		
	case FB_MIPI_ACK:
		if (copy_from_user(&ack_en, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		rx_en = ack_en? 1: rx_en;
		mipi_dsih_peripheral_ack(pdev, ack_en);
		mipi_dsih_enable_rx(pdev, rx_en);
		break;	
		
	case FB_MIPI_PHY_PWR:
		if (copy_from_user(&param, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		if (param == 2)
			mipi_dsih_reset_phy(pdev);
		else
			mipi_dsih_shutdown_phy(pdev, !param);
		break;	

	case FB_MIPI_CORE_PWR:
		if (copy_from_user(&param, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		if (param == 2)
			mipi_dsih_reset_controller(pdev);			
		else
			mipi_dsih_shutdown_controller(pdev, !param);
		break;			
				
	case FB_MIPI_CMD_HS:
		if (copy_from_user(&cmd_hs, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		if (cmd_hs)
			mipi_dsih_dphy_enable_hs_clk(pdev, 1);

		mipi_dsih_dcs_cmd_lp_transmission(pdev, cmd_hs, cmd_hs, cmd_hs);
		mipi_dsih_gen_cmd_lp_transmission(pdev, cmd_hs, cmd_hs, cmd_hs);
		break;	
	
	case FB_MIPI_ECC:
		if (copy_from_user(&ecc_en, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		rx_en = ecc_en? 1: rx_en;
		mipi_dsih_ecc_rx(pdev, ecc_en);
		mipi_dsih_enable_rx(pdev, rx_en);
		break;	
		
	case FB_MIPI_EOTP_TX:
		if (copy_from_user(&eotp_tx_en, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		rx_en = eotp_tx_en? 1: rx_en;
		mipi_dsih_eotp_tx(pdev, eotp_tx_en);
		mipi_dsih_enable_rx(pdev, rx_en);
		break;
		
	case FB_MIPI_EOTP_RX:
		if (copy_from_user(&eotp_rx_en, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		rx_en = eotp_rx_en? 1: rx_en;
		mipi_dsih_eotp_rx(pdev, eotp_rx_en);
		mipi_dsih_enable_rx(pdev, rx_en);
		break;
		
	case FB_MIPI_CMODE:
		if (copy_from_user(&param, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		mipi_dsih_hal_dpi_color_mode_pol(pdev, param);
		break;
		
	case FB_MIPI_RESETTRIG:		
		tmp = (mipi_dsih_active_mode(pdev) == 2);
		/* disable video mode */
		mipi_dsih_cmd_mode(pdev, tmp);
		/* reset trigger is 1000 in the D-PHY*/
		ret = mipi_dsih_dphy_escape_mode_trigger(pdev, 0x8);
		if (ret)
			pr_err("error %d", ret);

		msleep(10);
		mipi_dsih_video_mode(pdev, tmp);
		break;
	case FB_MIPI_HSCLK:		
		if (copy_from_user(&param, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		mipi_dsih_dphy_enable_hs_clk(pdev, tmp);
		break;
	case FB_MIPI_ULPSSAFE:		
		if (copy_from_user(&param, (void __user *)arg,
							sizeof(int)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		

			if (param == 0)	{
				/* make sure phy is awake before exiting ulps	 */
				mipi_dsih_dphy_wakeup_pll(pdev);

				//enable clk_lane transmission
				mipi_dsih_dphy_enable_hs_clk(pdev, 1);

				ret = 0x00;
				ret = mipi_dsih_dphy_ulps_data_lanes(pdev, 0);
				ret = mipi_dsih_dphy_ulps_clk_lane(pdev, 0);
				//Disable command mode
				mipi_dsih_cmd_mode(pdev, 0);
				msleep(10);
				//Reset PHY
				mipi_dsih_reset_phy(pdev);
				msleep(10);
				//Reset Controller
				mipi_dsih_reset_controller(pdev);
			}
			else if (param == 1){ //ON MODE	
				mipi_dsih_cmd_mode(pdev, 1);//Enable command mode
				ret = 0x00;
				tmp = 0;
				while (ret!=0x01 && tmp < 5000)
				{
					//Check if generic command FIFO is empty
					ret = mipi_dsih_read_part(pdev, R_DSI_HOST_CMD_PKT_STATUS, 0, 1);
					tmp++;
				}
				ret = 0x00;
				tmp = 0;
				while (ret!=0x01 && tmp < 5000)
				{
					//Check if DBI command FIFO is empty
					ret = mipi_dsih_read_part(pdev, R_DSI_HOST_CMD_PKT_STATUS, 8, 1);
					tmp++;
				}
				//disable clk_lane transmission
				mipi_dsih_dphy_enable_hs_clk(pdev, 0);

				ret = mipi_dsih_dphy_ulps_data_lanes(pdev, 1);
				ret = mipi_dsih_dphy_ulps_clk_lane(pdev, 1);


				mipi_dsih_reset_controller(pdev);
			}			
		break;
	case FB_MIPI_TEAR:	
		
		if (copy_from_user(&tear, (void __user *)arg,
							sizeof(tear_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		if (tear.status == 1){	/* on */
			mipi_dsih_tear_effect_ack(pdev, 1);
			tmp = 0x35;
		}
		else{
			/* off */
			tmp = 0x34;
			mipi_dsih_tear_effect_ack(pdev, 0);
		}
		mipi_dsih_cmd_mode(pdev, 1);
		ret = mipi_dsih_dcs_wr_cmd(pdev, tear.vc, (unsigned char*)tmp, 1);
		
		break;
	case FB_MIPI_PHY:		
		if (copy_from_user(&dphy, (void __user *)arg,
							sizeof(dphy_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
			mipi_dsih_dphy_configure(pdev, dphy.lanes, dphy.output_freq);
			pdev->phy.lanes = dphy.lanes;
			pdev->phy.output_freq = dphy.output_freq;
		break;
	case FB_MIPI_PRESP:		
		if (copy_from_user(&presp, (void __user *)arg,
							sizeof(presp_t)))
			dev_warn(info->dev,
				 "dsi ioctl: could not copy/read all data\n");
		
		switch (presp.time){
			case 1:
				mipi_dsih_presp_timeout_low_power_write(pdev, presp.time);
				break;
			case 2:
				mipi_dsih_presp_timeout_low_power_read(pdev, presp.time);
				break;
			case 3:
				mipi_dsih_presp_timeout_high_speed_write(pdev, presp.time);
				break;
			case 4:
				mipi_dsih_presp_timeout_high_speed_read(pdev, presp.time);
				break;
			case 5:
				mipi_dsih_presp_timeout_bta(pdev, presp.time);
				break;
			default:
				pr_err("presp type not valid\n");
				break;
		}
		break;
		
	default:
			pr_err("%s:IOCTL unknown!\n", FUNC_NAME);
		break;
	}
	enable_irq(pdev->irq[0]);
	enable_irq(pdev->irq[1]);
	return 0;
}

int
fb_mipi_open(struct fb_info *info, int user)
{
	return 0;
}

int
fb_mipi_release(struct fb_info *info, int user)
{
	return 0;
}

ssize_t
fb_mipi_read(struct fb_info *info, char __user *buf, size_t count,
	     loff_t *ppos)
{
	return 0;
}

ssize_t
fb_mipi_write(struct fb_info *info, const char __user *buf, size_t count,
	      loff_t *ppos)
{
	return count;
}

/********************************************************************
 * Miscellaneous functions - They should be used only after the HW is
 * configured.
 ********************************************************************/
int
fb_mipi_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	return 0;
}

int
fb_mipi_set_par(struct fb_info *info)
{
	return 0;
}

int
fb_mipi_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue,
		unsigned transp, struct fb_info *info)
{
	uint32_t *pal = info->pseudo_palette;
	uint32_t cr = red >> (16 - info->var.red.length);
	uint32_t cg = green >> (16 - info->var.green.length);
	uint32_t cb = blue >> (16 - info->var.blue.length);
	uint32_t value;

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

int
fb_mipi_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	return 0;
}

int
fb_mipi_blank(int blank, struct fb_info *info)
{
	return 0;
}

int
fb_mipi_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	return 0;
}

void
fb_mipi_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	return;
}

void
fb_mipi_copyarea(struct fb_info *info, const struct fb_copyarea *region)
{

}

void
fb_mipi_imageblit(struct fb_info *info, const struct fb_image *image)
{
	return;
}

int
fb_mipi_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	return 0;
}

int
fb_mipi_sync(struct fb_info *info)
{
	return 0;
}

int
fb_mipi_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return 0;
}

void
fb_mipi_get_caps(struct fb_info *info, struct fb_blit_caps *caps,
		struct fb_var_screeninfo *var)
{
	return;
}

void
fb_mipi_destroy(struct fb_info *info)
{
	if (info->screen_base)
		iounmap(info->screen_base);
}
