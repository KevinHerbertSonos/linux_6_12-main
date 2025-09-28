/*
 * video_if.c
 *
 *  Created on: Aug 31, 2011
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#include "video_if.h"

#include "mipi_displays.h"

static unsigned int zoom_g = 0;
static unsigned int panx_g = 0;
static unsigned int pany_g = 0;

unsigned long if_clk = VIDEO_IF_FREQ;

const char* sources[]= {"CSI-2", "HDMI Rx"};
const char* targets[]= {"LCD", "DSI", "HDMI Tx"};

static inline void video_if_write(struct mipi_dsi_dev *dev,u32 reg, u32 value)
{
//	pr_info("val %d addr %x\n",value,(unsigned int)dev->vid_bridge_addr + (reg << 2));
	iowrite32(value, dev->vid_bridge_addr + (reg << 2));
}

/* IO accessors */
static inline u32 video_if_read(struct mipi_dsi_dev *dev, u32 reg)
{
	return ioread32(dev->vid_bridge_addr + (reg << 2));
}

void video_if_write_part(struct mipi_dsi_dev *dev, uint32_t reg_address, uint32_t data, uint8_t shift, uint8_t width)
{
	uint32_t mask = (1 << width) - 1;
	uint32_t temp = video_if_read(dev,reg_address);

	temp &= ~(mask << shift);
	temp |= (data & mask) << shift;
	video_if_write(dev,reg_address, temp);
}

uint32_t video_if_read_part(struct mipi_dsi_dev *dev, uint32_t reg_address, uint8_t shift, uint8_t width)
{
	return (video_if_read(dev,reg_address) >> shift) & ((1 << width) - 1);
}
/**
 * Configures PAN coordinates in video interface
 * @param  pointer to structure holding the video interface information
 * @param x X axis value
 * @param y Y axis value
 */
void video_if_pan_coord(struct mipi_dsi_dev *dev,uint16_t x, uint16_t y)
{
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0);
	video_if_write(dev,VIDEO_IF_PANX, x);
	video_if_write(dev,VIDEO_IF_PANY, y);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 1);
}
/**
 * Shows where horizontally in the original image does the shown image start
 * @param  pointer to structure holding the video interface information
 * @return the X coordinate currently written to video interface PAN
 */
uint16_t video_if_get_x_coord(struct mipi_dsi_dev *dev)
{
	return (uint16_t)video_if_read(dev,VIDEO_IF_PANX);

}
/**
 * Shows where vertically in the original image does the shown image start
 * @param  pointer to structure holding the video interface information
 * @return the Y coordinate currently written to video interface PAN
 */
uint16_t video_if_get_y_coord(struct mipi_dsi_dev *dev)
{
	return (uint16_t)video_if_read(dev,VIDEO_IF_PANY);
}
/**
 * Modify the phase select of the video interface
 * @param  pointer to structure holding the video interface information
 * @param value of phase
 */
void video_if_phase(struct mipi_dsi_dev *dev,uint8_t value)
{
	video_if_write(dev, VIDEO_IF_PHASE_SEL, value);
}
/**
 * Get the phase select value configured in the video interface
 * @param  pointer to structure holding the video interface information
 * @return phase select
 */
uint8_t video_if_get_phase(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_PHASE_SEL);
}
/**
 * Change the video source of the video interface
 * @param  pointer to structure holding the video interface information
 * @param value indicator of the source
 */
void video_if_data_source(struct mipi_dsi_dev *dev,uint8_t value)
{
	video_if_write_part(dev, VIDEO_IF_SEL_DATA, value, 0, 1);
}
/**
 * Set the video source of the video interface
 * @param  pointer to structure holding the video interface information
 * @return indicator of the source
 */
uint8_t video_if_get_data_source(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_SEL_DATA, 0, 1);
}
/**
 * Set the RAW bayer sequence the video interface is assuming to receive
 * @param  pointer to structure holding the video interface information
 * @param RAW sequence indicator
 */
void video_if_raw_seq(struct mipi_dsi_dev *dev, uint8_t value)
{
	video_if_write(dev, VIDEO_IF_SWAP_BAY, value);
}
/**
 * Get the RAW bayer sequence the video interface is assuming to receive
 * @param  pointer to structure holding the video interface information
 * @return RAW sequence indicator
 */
uint8_t video_if_get_raw_seq(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_SWAP_BAY);
}
/**
 * Set video interface test mode
 * @param  pointer to structure holding the video interface information
 * @param value of test mode indicator
 */
void video_if_test_mode(struct mipi_dsi_dev *dev,uint8_t value)
{
	video_if_write(dev, VIDEO_IF_TESTMODE, value);
}

/**
 * Get video interface test mode
 * @param  pointer to structure holding the video interface information
 * @return value of test mode indicator
 */
uint8_t video_if_get_test_mode(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_TESTMODE);
}

/**
 * Set the zoom out the video interface to perform on image
 * @param  pointer to structure holding the video interface information
 * @param value of zoom out
 */
void video_if_zoom_out(struct mipi_dsi_dev *dev, uint8_t value)
{
	video_if_write(dev, VIDEO_IF_ZOOM_OUT, value);
}

/**
 * Get the zoom out the video interface is currently performing on image
 * @param  pointer to structure holding the video interface information
 * @return value of zoom out
 */
uint8_t video_if_get_zoom_out(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_ZOOM_OUT) / VIDEO_IF_ZOOM_OUT_STEP;
}

/**
 * Set the video output target
 * @param  pointer to structure holding the video interface information
 * @param value of the target indicator
 */
void video_if_data_target(struct mipi_dsi_dev *dev, uint8_t value)
{
	video_if_write_part(dev, VIDEO_IF_SEL_DATA, value, 1, 2); /* bits [2:1] */
}

/**
 * Get the video output target
 * @param  pointer to structure holding the video interface information
 * @return value of the target indicator
 */
uint8_t video_if_get_data_target(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_SEL_DATA, 1, 2);
}

/**
 * Set the horizontal line time in units of output clock
 * @param  pointer to structure holding the video interface information
 * @param value of the line time
 */
void video_if_line_time(struct mipi_dsi_dev *dev,uint32_t value)
{
	video_if_write(dev,VIDEO_IF_LINE_TIME, value);
}

/**
 * Get the horizontal line time in units of output clock
 * @param  pointer to structure holding the video interface information
 * @return value of the line time
 */
uint32_t video_if_get_line_time(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_LINE_TIME);
}

/**
 * Increase zoom (zoom in) in image according to zoom step
 * @param  pointer to structure holding the video interface information
 */
void video_if_increase_zoom(struct mipi_dsi_dev *dev)
{
	video_if_zoom_out(dev, video_if_get_zoom_out(dev) - VIDEO_IF_ZOOM_OUT_STEP);
}

/**
 * Decrease zoom (zoom out) in image according to zoom step
 * @param  pointer to structure holding the video interface information
 */
void video_if_decrease_zoom(struct mipi_dsi_dev *dev)
{
	video_if_zoom_out(dev, video_if_get_zoom_out(dev) + VIDEO_IF_ZOOM_OUT_STEP);
}

/**
 * Switch between video sources
 * @param  pointer to structure holding the video interface information
 */
void video_if_switch_source(struct mipi_dsi_dev *dev)
{
	video_if_data_source(dev, video_if_get_data_source(dev) + 1);
}

/**
 * Switch between video targets
 * @param  pointer to structure holding the video interface information
 */
void video_if_switch_target(struct mipi_dsi_dev *dev)
{
	video_if_data_target(dev, (video_if_get_data_target(dev) + 1) % 3);
}

/**
 * Move right in the image according to pan step
 * @param  pointer to structure holding the video interface information
 */
void video_if_pan_right(struct mipi_dsi_dev *dev)
{
	video_if_pan_coord(dev, video_if_get_x_coord(dev) + VIDEO_IF_PAN_STEP, video_if_get_y_coord(dev));
}

/**
 * Move left in the image according to pan step
 * @param  pointer to structure holding the video interface information
 */
void video_if_pan_left(struct mipi_dsi_dev *dev)
{
	uint16_t x = video_if_get_x_coord(dev);
	video_if_pan_coord(dev, (x <= VIDEO_IF_PAN_STEP)? x: x - VIDEO_IF_PAN_STEP, video_if_get_y_coord(dev));
}

/**
 * Move up in the image according to pan step
 * @param  pointer to structure holding the video interface information
 */
void video_if_pan_up(struct mipi_dsi_dev *dev)
{
	uint16_t y = video_if_get_y_coord(dev);
	video_if_pan_coord(dev, video_if_get_x_coord(dev), (y <= VIDEO_IF_PAN_STEP)? y: y - VIDEO_IF_PAN_STEP);
}

/**
 * Move down in the image according to pan step
 * @param  pointer to structure holding the video interface information
 */
void video_if_pan_down(struct mipi_dsi_dev *dev)
{
	video_if_pan_coord(dev, video_if_get_x_coord(dev), video_if_get_y_coord(dev) + VIDEO_IF_PAN_STEP);
}

/**
 * Set the output horizontal pixel size of the video interface
 * @param  pointer to structure holding the video interface information
 * @param value of horizontal pixel size
 */
void video_if_h_active(struct mipi_dsi_dev *dev, uint16_t value)
{
	video_if_write_part(dev, VIDEO_IF_CANVAS, value, 0, 16);
}

/**
 * Set the output vertical line size of the video interface
 * @param  pointer to structure holding the video interface information
 * @param value of vertical line size
 */
void video_if_v_active(struct mipi_dsi_dev *dev, uint16_t value)
{
	video_if_write_part(dev, VIDEO_IF_CANVAS, value, 16, 16);
}

/**
 * Get the output horizontal pixel size of the video interface
 * @param  pointer to structure holding the video interface information
 * @return horizontal pixel size
 */
uint16_t video_if_get_h_active(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_CANVAS, 0, 16);
}

/**
 * Get the output vertical line size of the video interface
 * @param  pointer to structure holding the video interface information
 * @return vertical line size
 */
uint16_t video_if_get_v_active(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_CANVAS, 16, 16);
}

/**
 * Set the frame frequency of video (and line time is automatically calculated)
 * @param  pointer to structure holding the video interface information
 * @param fps frames per second rate in [Hz]
 * @note This function can only be called after the V active and H active have
 * been set
 */
void video_if_fps(struct mipi_dsi_dev *dev, uint8_t fps)
{
	uint32_t line_time = 0;
	uint32_t v_total = video_if_get_v_active(dev) + video_if_get_vertical_blanking(dev) + video_if_get_vertical_sync(dev) + video_if_get_vertical_front_porch(dev);
	line_time = (1000 * VIDEO_IF_FREQ) /(fps * v_total); /* 1000 is used because IF_Freq is in KHz */
	video_if_line_time(dev,line_time);
}

/**
 * Get the frame frequency of video
 * @param  pointer to structure holding the video interface information
 * @return frames per second rate in [Hz]
 * @note This function can only be called after line time or fps has been set
 * been set
 */
uint8_t video_if_get_fps(struct mipi_dsi_dev *dev)
{
	uint32_t fps = 0;
	uint32_t v_total = video_if_get_v_active(dev) + video_if_get_vertical_blanking(dev) + video_if_get_vertical_sync(dev) + video_if_get_vertical_front_porch(dev);
	fps = (1000 * VIDEO_IF_FREQ) /(video_if_get_line_time(dev) * v_total); /* 1000 is used because IF_Freq is in KHz */
	return fps;
}

/**
 * Video interface set horizontal blanking
 * @param  pointer to structure holding the video interface information
 * @param hbp horizontal blanking in pixels
 */
void video_if_horizontal_blanking(struct mipi_dsi_dev *dev, uint16_t hbp)
{
	video_if_write_part(dev, VIDEO_IF_HBP_HSA, hbp, 16, 16);
}

/**
 * Video interface set horizontal sync
 * @param  pointer to structure holding the video interface information
 * @param hsa horizontal sync in pixels
 */
void video_if_horizontal_sync(struct mipi_dsi_dev *dev, uint16_t hsa)
{
	video_if_write_part(dev, VIDEO_IF_HBP_HSA, hsa, 0, 16);
}

/**
 * Video interface set vertical blanking
 * @param  pointer to structure holding the video interface information
 * @param vbp vertical blanking in lines
 */
void video_if_vertical_blanking(struct mipi_dsi_dev *dev, uint8_t vbp)
{
	video_if_write_part(dev, VIDEO_IF_VBP_VFP_VSA, vbp, 20, 12);
}

/**
 * Video interface set vertical front porch
 * @param  pointer to structure holding the video interface information
 * @param vfp vertical front porch in lines
 */
void video_if_vertical_front_porch(struct mipi_dsi_dev *dev, uint8_t vfp)
{
	video_if_write_part(dev, VIDEO_IF_VBP_VFP_VSA, vfp, 8, 11);
}

/**
 * Video interface set vertical sync
 * @param  pointer to structure holding the video interface information
 * @param vbp vertical sync in lines
 */
void video_if_vertical_sync(struct mipi_dsi_dev *dev, uint8_t vsa)
{
	video_if_write_part(dev, VIDEO_IF_VBP_VFP_VSA, vsa, 0, 8);
}

/**
 * Video interface set horizontal blanking
 * @param  pointer to structure holding the video interface information
 * @return horizontal blanking in pixels
 */
uint16_t video_if_get_horizontal_blanking(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_HBP_HSA, 16, 16);
}

/**
 * Video interface set horizontal sync
 * @param  pointer to structure holding the video interface information
 * @return horizontal sync in pixels
 */
uint16_t video_if_get_horizontal_sync(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_HBP_HSA, 0, 16);
}

/**
 * Video interface set vertical blanking
 * @param  pointer to structure holding the video interface information
 * @return vertical blanking in lines
 */
uint8_t video_if_get_vertical_blanking(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_VBP_VFP_VSA, 20, 12);
}

/**
 * Video interface set vertical front porch
 * @param  pointer to structure holding the video interface information
 * @return vertical front porch in lines
 */
uint8_t video_if_get_vertical_front_porch(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_VBP_VFP_VSA, 8, 11);
}

/**
 * Video interface set vertical sync
 * @param  pointer to structure holding the video interface information
 * @return vertical sync in lines
 */
uint8_t video_if_get_vertical_sync(struct mipi_dsi_dev *dev)
{
	return video_if_read_part(dev, VIDEO_IF_VBP_VFP_VSA, 0, 8);
}

/**
 * Get the Video Interface Version
 * @param  pointer to structure holding the video interface information
 * @return version
 */
uint8_t video_if_version(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_VERSION);
}

/**
 * Set color mode (bits per pixel and pixel configuration)
 * of the video interface output
 * @param  pointer to structure holding the video interface information
 * @param color code
 * @return 0 if no errors found
 * @note only color modes of 16 bpp and above are supported
 */
int video_if_set_color_mode(struct mipi_dsi_dev *dev, vif_color_coding_t color_coding)
{
	if (color_coding < VIF_COLOR_CODE_MAX) {
		video_if_write(dev, VIDEO_IF_DPICOLORMODE, color_coding);
		return 0;
	}
	return 1;
}

/**
 * Get the color mode code configured in the video interface
 * @param  pointer to structure holding the video interface information
 * @return vif_color_coding_t
 */
vif_color_coding_t video_if_get_color_mode(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_DPICOLORMODE);
}

/**
 * Predefined video resolutions lookup table
 */

/**
 * Get the number of bits per pixel being transmitted by the video interface
 * @param  pointer to structure holding the video interface information
 * @return number of bits per pixel
 */
uint8_t video_if_get_bits_per_pixel(struct mipi_dsi_dev *dev)
{
	uint8_t bpp = 0;
	uint8_t colour_mode = video_if_read(dev,VIDEO_IF_DPICOLORMODE);
	switch (colour_mode)
	{
	case 0:
	case 1:
	case 2:
		bpp = 16;
		break;
	case 3:
	case 4:
		bpp = 18;
		break;
	default:
		bpp = 24;
		break;
	}
	return bpp;
}

/**
 * Get the pixel configuration being transmitted by the video interface
 * @param  pointer to structure holding the video interface information
 * @return pixel configuration
 */
uint8_t video_if_get_pixel_config(struct mipi_dsi_dev *dev)
{
	uint8_t pixel_config = 0;
	uint8_t colour_mode = video_if_read(dev,VIDEO_IF_DPICOLORMODE);
	switch (colour_mode)
	{
	case 0:
		pixel_config = 1;
		break;
	case 1:
		pixel_config = 2;
		break;
	case 2:
		pixel_config = 3;
		break;
	case 3:
		pixel_config = 1;
		break;
	case 4:
		pixel_config = 2;
		break;
	default:
		pixel_config = 0;
		break;
	}
	return pixel_config;
}

/**
 * enable / disable eDPI mode
 * @param  pointer to structure holding the video interface information
 * @param enable
 */
void video_if_edpi(struct mipi_dsi_dev *dev, int enable)
{
	video_if_write(dev, VIDEO_IF_EDPICTRL, enable);
}
/**
 * whether eDPI interface is enabled in the video interface
 * @param  pointer to structure holding the video interface information
 * @return 1 when enabled
 */
int video_if_is_edpi(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_EDPICTRL);
}

/**
 * enable waiting for tearing effect signal
 * @param  pointer to structure holding the video interface information
 * @param enable
 */
void video_if_wait_for_tearing(struct mipi_dsi_dev *dev, int enable)
{
	video_if_write(dev, VIDEO_IF_TECONTROL, enable);
}

/**
 * get the status of waiting for tearing effect signal
 * @param  pointer to structure holding the video interface information
 * @return 1 if waiting for tearing is enabled
 */
int video_if_get_wait_for_tearing(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_TECONTROL);
}

/**
 * enable cea output from hdmi rx ip
 * @param  pointer to structure holding the video interface information
 * @param enable
 */
void video_if_pvo_cea(struct mipi_dsi_dev *dev, int enable_cea)
{
	video_if_write(dev, VIDEO_IF_PVO_CEAVID, enable_cea);
}

/**
 * get the status of cea output, if enabled or not
 * @param  pointer to structure holding the video interface information
 * @return 1 if cea output interface is enabled
 */
int video_if_is_cea(struct mipi_dsi_dev *dev)
{
	return video_if_read(dev,VIDEO_IF_PVO_CEAVID);
}

void video_if_adjust_pan_x(struct mipi_dsi_dev *dev, unsigned pan_x)
{
	video_if_write(dev,VIDEO_IF_PANX, pan_x);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x00);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x01);
}

void video_if_adjust_pan_y(struct mipi_dsi_dev *dev, unsigned pan_y)
{
	video_if_write(dev,VIDEO_IF_PANY, pan_y);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x00);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x01);
}

void video_if_update_zoom(struct mipi_dsi_dev *dev,int zoom)
{
	if (zoom == 1) {
		if (zoom_g < 10) {
			zoom_g ++;
			if (panx_g < 100)  // WA for zoom change screen cleaning on DSI
				panx_g++;
		}
	} else {
		if (zoom == 0) {
			if (zoom_g > 0) {
				zoom_g--;
				if (panx_g > 0)  // WA for zoom change screen cleaning on DSI
					panx_g--;
			}
		}
	}
}

void video_if_update_panx(struct mipi_dsi_dev *dev, int panx)
{
	if (panx == 1) {
		if (panx_g < 150)
			panx_g ++;
	} else {
		if (panx == 0)
			if (panx_g > 0)
				panx_g --;
	}
}

void video_if_update_pany(struct mipi_dsi_dev *dev, int pany)
{
	if (pany == 1) {
		if (pany_g < 150)
			pany_g ++;
	} else {
		if (pany == 0)
			if (pany_g > 0)
				pany_g --;
	}
}

void video_if_update_position(struct mipi_dsi_dev *dev)
{
	video_if_write(dev,VIDEO_IF_ZOOM_OUT,  zoom_g);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
	msleep(1);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);
	video_if_write(dev,VIDEO_IF_PAN_VALID, pany_g);
	video_if_write(dev,VIDEO_IF_PAN_VALID, panx_g);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
	msleep(1);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);
}

int video_if_config(struct mipi_dsi_dev *dev, unsigned mode)
{
	switch (mode) {
	case TREMOLO_S:
		/* 960 x 540 @45fps 60bpp */
		video_if_set_color_mode(dev, VIF_COLOR_CODE_24BIT);
		video_if_h_active(dev, 960);
		video_if_horizontal_sync(dev, 20);
		video_if_horizontal_blanking(dev, 20);
		video_if_v_active(dev, 540);
		video_if_vertical_sync(dev, 2);
		video_if_vertical_blanking(dev, 1);
		video_if_vertical_front_porch(dev, 2);
		video_if_fps(dev, 60);
		return 0;
	case TREMOLO_M:
		/* 840 x 480 24bpp @ 60fps*/
		video_if_set_color_mode(dev, VIF_COLOR_CODE_24BIT);
		video_if_h_active(dev,800);
		video_if_horizontal_sync(dev,20);
		video_if_horizontal_blanking(dev,20);
		video_if_v_active(dev,480);
		video_if_vertical_sync(dev,2);
		video_if_vertical_blanking(dev,1);
		video_if_vertical_front_porch(dev,16);
		video_if_fps(dev,60);
		video_if_pan_coord(dev,0, 0);
		return 0;
	default:
		return -1;
	}
}

void video_if_adjust_zoom_autocenter(struct mipi_dsi_dev *dev)
{
	unsigned h_source_count;
	unsigned h_zoom_lines;
	unsigned v_source_count;
	unsigned v_zoom_lines;
	unsigned zoom_factor;
	unsigned h_start;
	unsigned v_start;

	if(!video_if_get_data_source(dev)) {
		video_if_write(dev,VIDEO_IF_ZOOM_OUT, 0x0);
		video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_X, 0x0);
		video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_Y, 0x0);

		video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
		video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);

		return;
	}

	// toggle rx video output mute - to clear DSI screen
	video_if_write(dev,VIDEO_IF_SEL_DATA, 0x02);

	h_source_count = 640;
	v_source_count = 480;

	// reset zoom factor;
	video_if_write(dev,VIDEO_IF_ZOOM_OUT, 0x0);
	video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_X, 0x0);
	video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_Y, 0x0);

	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);

	zoom_factor = (unsigned)((h_source_count/(SCREEN_SIZE_X/2)));
	//pr_info("Zoom %u\n",zoom_factor);

	video_if_write(dev,VIDEO_IF_ZOOM_OUT, zoom_factor);
	h_zoom_lines = (unsigned)((h_source_count / (zoom_factor+1))); // bridge applies zoom_factor + 1
	v_zoom_lines = (unsigned)((v_source_count / (zoom_factor+1)));
	//pr_info( "Zoom H %u V %u\n",h_zoom_lines,v_zoom_lines);
	video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_X, 0x0);
	video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_Y, 0x0);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);

	if (h_zoom_lines < (SCREEN_SIZE_X/2)) {
		h_start = (unsigned)(((SCREEN_SIZE_X/2) - h_zoom_lines)/2);
		video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_X, h_start);
		video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
		video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);
		//pr_info( "HS %u\n",h_start);
	}
	if (v_zoom_lines < (SCREEN_SIZE_Y/2)) {
		v_start = (unsigned)(((SCREEN_SIZE_Y/2) - v_zoom_lines)/2);
		video_if_write(dev,VIDEO_IF_IMAGE_START_POSITION_Y, v_start);
		video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
		video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);
		//pr_info( "VS %u\n",v_start);
	}
	video_if_write(dev,VIDEO_IF_PANX, 0x00);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);
	video_if_write(dev,VIDEO_IF_PANY, 0x00);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x0);
	video_if_write(dev,VIDEO_IF_PAN_VALID, 0x1);

	video_if_write(dev,VIDEO_IF_SEL_DATA, 0x03);
}
