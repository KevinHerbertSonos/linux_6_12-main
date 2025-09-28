/*
 * video_if.h
 *
 *  Created on: Aug 31, 2011
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef VIDEO_IF_H_
#define VIDEO_IF_H_

#include "includes.h"


/** video interface PAN X */
#define VIDEO_IF_PANX			(0x00UL)
/** video interface PAN Y */
#define VIDEO_IF_PANY			(0x01UL)
/** video interface phase selection */
#define VIDEO_IF_PHASE_SEL		(0x02UL)
/** video interface data source */
#define VIDEO_IF_SEL_DATA		(0x03UL)
/** video interface swap bayer */
#define VIDEO_IF_SWAP_BAY		(0x04UL)
/** video interface test mode */
#define VIDEO_IF_TESTMODE		(0x05UL)
/** video interface zoom out */
#define VIDEO_IF_ZOOM_OUT		(0x06UL)
/** video interface eDPI enable */
#define VIDEO_IF_EDPICTRL		(0x07UL)
/** video interface canvas */
#define VIDEO_IF_CANVAS			(0x08UL)
/** video interface horizontal sync */
#define VIDEO_IF_HBP_HSA		(0x09UL)
/** video interface line time */
#define VIDEO_IF_LINE_TIME		(0x0AUL)
/** video interface vertical blanking */
#define VIDEO_IF_VBP_VFP_VSA	(0x0BUL)
/** video interface colour mode configuration */
#define VIDEO_IF_DPICOLORMODE	(0x0CUL)
/** video interface version */
#define VIDEO_IF_VERSION		(0x0DUL)
/** video interface external eDPI enable */
#define VIDEO_IF_EXTRNAL_EDPI	(0x0EUL)
/** video interface PVO CEAVID Control */
#define VIDEO_IF_PVO_CEAVID		(0x0FUL)
/** video interface PAN valid */
#define VIDEO_IF_PAN_VALID		(0x10UL)
/** video interface wait for tearing effect */
#define VIDEO_IF_TECONTROL		(0x11UL)
/** video interface image start position X */
#define VIDEO_IF_IMAGE_START_POSITION_X			(0x13UL)
/** video interface image start position Y */
#define VIDEO_IF_IMAGE_START_POSITION_Y			(0x14UL)
/** video interface image Y */
#define VIDEO_IF_IMAGE_Y						(0x15UL)
/** video interface image word count */
#define VIDEO_IF_IMAGE_WORD_COUNT				(0x16UL)
/** video interface image data type */
#define VIDEO_IF_IMAGE_DATA_TYPE				(0x17UL)
/** video interface image data type */	
#define VIDEO_IF_LINE_PIXS_CNT 					(0x18UL)
/** video interface image data type */
#define VIDEO_IF_FRAME_LINES_CNT				(0x19UL)


#define VIDEO_IF_ZOOM_OUT_STEP	1
#define VIDEO_IF_PAN_STEP		40
#define VIDEO_IF_FREQ			100000 /* [KHz] */
#define VIDEO_IF_V_FRONT_PORCH	2
#define VIDEO_IF_V_BACK_PORCH	2
#define VIDEO_IF_V_SYNC			2

#define SCREEN_SIZE_X	640
#define SCREEN_SIZE_Y	480


/**
 * Color coding type (depth and pixel configuration)
 */
typedef enum
{
	VIF_COLOR_CODE_16BIT_CONFIG1 = 0,
	VIF_COLOR_CODE_16BIT_CONFIG2,
	VIF_COLOR_CODE_16BIT_CONFIG3,
	VIF_COLOR_CODE_18BIT_CONFIG1,
	VIF_COLOR_CODE_18BIT_CONFIG2,
	VIF_COLOR_CODE_24BIT,
	VIF_COLOR_CODE_MAX
}
vif_color_coding_t;

/****************************************/
/* Video Interface Controller (bridge)	*/
/****************************************/
void video_if_pan_coord(struct mipi_dsi_dev *dev,  uint16_t x, uint16_t y);

uint16_t  video_if_get_x_coord(struct mipi_dsi_dev *dev);

uint16_t  video_if_get_y_coord(struct mipi_dsi_dev *dev);

void video_if_phase(struct mipi_dsi_dev *dev,  uint8_t value);

uint8_t video_if_get_phase(struct mipi_dsi_dev *dev);

void video_if_data_source(struct mipi_dsi_dev *dev,  uint8_t value);

uint8_t   video_if_get_data_source(struct mipi_dsi_dev *dev);

void video_if_data_target(struct mipi_dsi_dev *dev,  uint8_t value);

uint8_t   video_if_get_data_target(struct mipi_dsi_dev *dev);

void video_if_raw_seq(struct mipi_dsi_dev *dev,  uint8_t value);

uint8_t   video_if_get_raw_seq(struct mipi_dsi_dev *dev);

void video_if_test_mode(struct mipi_dsi_dev *dev,  uint8_t value);

uint8_t video_if_get_test_mode(struct mipi_dsi_dev *dev);

uint8_t   video_if_get_zoom_out(struct mipi_dsi_dev *dev);

void video_if_zoom_out(struct mipi_dsi_dev *dev,  uint8_t value);

void video_if_line_time(struct mipi_dsi_dev *dev,  uint32_t value);

uint32_t video_if_get_line_time(struct mipi_dsi_dev *dev);

void video_if_h_active(struct mipi_dsi_dev *dev,  uint16_t value);

void video_if_v_active(struct mipi_dsi_dev *dev,  uint16_t value);

uint16_t video_if_get_h_active(struct mipi_dsi_dev *dev);

uint16_t video_if_get_v_active(struct mipi_dsi_dev *dev);

void video_if_increase_zoom(struct mipi_dsi_dev *dev);

void video_if_decrease_zoom(struct mipi_dsi_dev *dev);

void video_if_switch_source(struct mipi_dsi_dev *dev);

void video_if_switch_target(struct mipi_dsi_dev *dev);

void video_if_pan_right(struct mipi_dsi_dev *dev);

void video_if_pan_left(struct mipi_dsi_dev *dev);

void video_if_pan_up(struct mipi_dsi_dev *dev);

void video_if_pan_down(struct mipi_dsi_dev *dev);

void video_if_fps(struct mipi_dsi_dev *dev,  uint8_t fps);

void video_if_horizontal_blanking(struct mipi_dsi_dev *dev,  uint16_t hbp);

void video_if_horizontal_sync(struct mipi_dsi_dev *dev,  uint16_t hsa);

void video_if_vertical_blanking(struct mipi_dsi_dev *dev,  uint8_t vbp);

void video_if_vertical_front_porch(struct mipi_dsi_dev *dev, uint8_t vfp);

void video_if_vertical_sync(struct mipi_dsi_dev *dev,  uint8_t vsa);

uint16_t video_if_get_horizontal_blanking(struct mipi_dsi_dev *dev);

uint16_t video_if_get_horizontal_sync(struct mipi_dsi_dev *dev);

uint8_t video_if_get_vertical_blanking(struct mipi_dsi_dev *dev);

uint8_t video_if_get_vertical_front_porch(struct mipi_dsi_dev *dev);

uint8_t video_if_get_vertical_sync(struct mipi_dsi_dev *dev);

uint8_t video_if_version(struct mipi_dsi_dev *dev);

int video_if_set_color_mode(struct mipi_dsi_dev *dev,  vif_color_coding_t color_coding);

vif_color_coding_t video_if_get_color_mode(struct mipi_dsi_dev *dev);

uint8_t video_if_get_bits_per_pixel(struct mipi_dsi_dev *dev);

uint8_t video_if_get_pixel_config(struct mipi_dsi_dev *dev);

void video_if_edpi(struct mipi_dsi_dev *dev,  int enable);

int video_if_is_edpi(struct mipi_dsi_dev *dev);

void video_if_wait_for_tearing(struct mipi_dsi_dev *dev,  int enable);

int video_if_get_wait_for_tearing(struct mipi_dsi_dev *dev);

void video_if_pvo_cea(struct mipi_dsi_dev *dev, int enable_cea);

int video_if_is_cea(struct mipi_dsi_dev *dev);

void video_if_adjust_zoom(struct mipi_dsi_dev *dev);

void video_if_adjust_zoom_autocenter(struct mipi_dsi_dev *dev);

void video_if_update_zoom(struct mipi_dsi_dev *dev,int zoom);

void video_if_update_position(struct mipi_dsi_dev *dev);

void video_if_update_pany(struct mipi_dsi_dev *dev,int pany);

void video_if_update_panx(struct mipi_dsi_dev *dev,int panx);

int video_if_config(struct mipi_dsi_dev *dev,unsigned mode);

#endif /* VIDEO_IF_H_ */
