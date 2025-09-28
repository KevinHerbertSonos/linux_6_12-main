// SPDX-License-Identifier: GPL-2.0
/**
 * Copyright (C) 2018 Synopsys, Inc.
 *
 * @file dsih_hal.c
 * @brief Hardware Abstraction Level of Synopsys MIPI DSI HOST controller
 * included as a part of Synopsys MIPI DSI Host controller driver
 *
 * @author Luis Oliveira <luis.oliveira@synopsys.com>
 */

#include "dsih_hal.h"

/**
 * Write a 32-bit word to the DSI Host core
 * @param dev pointer to structure holding the DSI Host core information
 * @param reg_address register offset in core
 * @param data 32-bit word to be written to register
 */
void mipi_dsih_write_word(struct mipi_dsi_dev *dev, uint32_t reg_address, uint32_t data)
{
	if(dev == NULL) {
		mipi_dbg_print(MIPI_ERROR,"%s:Device is null\n", FUNC_NAME);
		return;
	}

	iowrite32(dev->core_addr + reg_address, data);
}
/**
 * Write a bit field o a 32-bit word to the DSI Host core
 * @param dev pointer to structure holding the DSI Host core information
 * @param reg_address register offset in core
 * @param data to be written to register
 * @param shift bit shift from the left (system is BIG ENDIAN)
 * @param width of bit field
 */
void mipi_dsih_write_part(struct mipi_dsi_dev *dev, uint32_t reg_address, uint32_t data, uint8_t shift, uint8_t width)
{
	uint32_t mask = (1 << width) - 1;
	uint32_t temp = mipi_dsih_read_word(dev, reg_address);

	temp &= ~(mask << shift);
	temp |= (data & mask) << shift;
	mipi_dsih_write_word(dev, reg_address, temp);
}
/**
 * Write a 32-bit word to the DSI Host core
 * @param dev pointer to structure holding the DSI Host core information
 * @param reg_address offset of register
 * @return 32-bit word value stored in register
 */
uint32_t mipi_dsih_read_word(struct mipi_dsi_dev *dev, uint32_t reg_address)
{
	uint32_t ret;

	if(dev == NULL){
		mipi_dbg_print(MIPI_ERROR,"%s:Device is null\n", FUNC_NAME);
		return MIPI_RET(ENODEV);
	}
	ioread32(dev->core_addr + reg_address, &ret);

	return ret;
}
/**
 * Write a 32-bit word to the DSI Host core
 * @param dev pointer to structure holding the DSI Host core information
 * @param reg_address offset of register in core
 * @param shift bit shift from the left (system is BIG ENDIAN)
 * @param width of bit field
 * @return bit field read from register
 */
uint32_t mipi_dsih_read_part(struct mipi_dsi_dev *dev, uint32_t reg_address, uint8_t shift, uint8_t width)
{
	return (mipi_dsih_read_word(dev, reg_address) >> shift) & ((1 << width) - 1);
}
/**
 * Get DSI Host core version
 * @param dev pointer to structure holding the DSI Host core information
 * @return ascii number of the version
 */
uint32_t mipi_dsih_hal_get_version(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_word(dev, R_DSI_HOST_VERSION);
}
/**
 * Modify power status of DSI Host core
 * @param dev pointer to structure holding the DSI Host core information
 * @param on (1) or off (0)
 */
void mipi_dsih_hal_power(struct mipi_dsi_dev *dev, int on)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PWR_UP, on, 0, 1);
}
/**
 * Get the power status of the DSI Host core
 * @param dev pointer to structure holding the DSI Host core information
 * @return power status
 */
int mipi_dsih_hal_get_power(struct mipi_dsi_dev *dev)
{
	return (int)(mipi_dsih_read_part(dev, R_DSI_HOST_PWR_UP, 0, 1));
}
/**
 * Write transmission escape timeout
 * a safe guard so that the state machine would reset if transmission
 * takes too long
 * @param dev pointer to structure holding the DSI Host core information
 * @param tx_escape_division
 */
void mipi_dsih_hal_tx_escape_division(struct mipi_dsi_dev *dev, uint8_t tx_escape_division)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_CLK_MGR, tx_escape_division, 0, 8);
}
/**
 * Write the DPI video virtual channel destination
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc virtual channel
 */
void mipi_dsih_hal_dpi_video_vc(struct mipi_dsi_dev *dev, uint8_t vc)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_DPI_VCID, (uint32_t)(vc), 0, 2);
}
/**
 * Get the DPI video virtual channel destination
 * @param dev pointer to structure holding the DSI Host core information
 * @return virtual channel
 */
uint8_t mipi_dsih_hal_dpi_get_video_vc(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_DPI_VCID, 0, 2);
}
/**
 * Set DPI video color coding
 * @param dev pointer to structure holding the DSI Host core information
 * @param color_coding enum (configuration and color depth)
 * @return error code
 */
int mipi_dsih_hal_dpi_color_coding(struct mipi_dsi_dev *dev, dsih_color_coding_t color_coding)
{
	if (color_coding > COLOR_CODE_MAX){
		mipi_dbg_print(MIPI_ERROR,"invalid colour configuration");
		return FALSE;
	}
	else
		mipi_dsih_write_part(dev, R_DSI_HOST_DPI_COLOR_CODE, color_coding, 0, 4);

	return TRUE;
}
/**
 * Get DPI video color coding
 * @param dev pointer to structure holding the DSI Host core information
 * @return color coding enum (configuration and color depth)
 */
dsih_color_coding_t mipi_dsih_hal_dpi_get_color_coding(struct mipi_dsi_dev *dev)
{
	return (dsih_color_coding_t)(mipi_dsih_read_part(dev, R_DSI_HOST_DPI_COLOR_CODE, 0, 4));
}
/**
 * Get DPI video color depth
 * @param dev pointer to structure holding the DSI Host core information
 * @return number of bits per pixel
 */
uint8_t mipi_dsih_hal_dpi_get_color_depth(struct mipi_dsi_dev *dev)
{
	uint8_t color_depth = 0;
	switch (mipi_dsih_read_part(dev, R_DSI_HOST_DPI_COLOR_CODE, 0, 4))
	{
	case 0:
	case 1:
	case 2:
		color_depth = 16;
		break;
	case 3:
	case 4:
		color_depth = 18;
		break;
	case 5:
		color_depth = 24;
		break;
	case 6:
		color_depth = 20;
		break;
	case 7:
		color_depth = 24;
		break;
	case 8:
		color_depth = 16;
		break;
	case 9:
		color_depth = 30;
		break;
	case 10:
		color_depth = 36;
		break;
	case 11:
		color_depth = 12;
		break;
	default:
		break;
	}
	return color_depth;
}
/**
 * Get DPI video pixel configuration
 * @param dev pointer to structure holding the DSI Host core information
 * @return pixel configuration
 */
uint8_t mipi_dsih_hal_dpi_get_color_config(struct mipi_dsi_dev *dev)
{
	uint8_t color_config = 0;
	switch (mipi_dsih_read_part(dev, R_DSI_HOST_DPI_COLOR_CODE, 0, 4))
	{
	case 0:
		color_config = 1;
		break;
	case 1:
		color_config = 2;
		break;
	case 2:
		color_config = 3;
		break;
	case 3:
		color_config = 1;
		break;
	case 4:
		color_config = 2;
		break;
	default:
		color_config = 0;
		break;
	}
	return color_config;
}
/**
 * Set DPI loosely packetisation video (used only when color depth = 18
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 */
void mipi_dsih_hal_dpi_18_loosely_packet_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_DPI_COLOR_CODE, enable, 8, 1);
}
/**
 * Set DPI color mode pin polarity
 * @param dev pointer to structure holding the DSI Host core information
 * @param active_low (1) or active high (0)
 */
void mipi_dsih_hal_dpi_color_mode_pol(struct mipi_dsi_dev *dev, int active_low)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_DPI_CFG_POL, active_low, 4, 1);
}
/**
 * Set DPI shut down pin polarity
 * @param dev pointer to structure holding the DSI Host core information
 * @param active_low (1) or active high (0)
 */
void mipi_dsih_hal_dpi_shut_down_pol(struct mipi_dsi_dev *dev, int active_low)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_DPI_CFG_POL, active_low, 3, 1);
}
/**
 * Set DPI horizontal sync pin polarity
 * @param dev pointer to structure holding the DSI Host core information
 * @param active_low (1) or active high (0)
 */
void mipi_dsih_hal_dpi_hsync_pol(struct mipi_dsi_dev *dev, int active_low)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_DPI_CFG_POL, active_low, 2, 1);
}
/**
 * Set DPI vertical sync pin polarity
 * @param dev pointer to structure holding the DSI Host core information
 * @param active_low (1) or active high (0)
 */
void mipi_dsih_hal_dpi_vsync_pol(struct mipi_dsi_dev *dev, int active_low)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_DPI_CFG_POL, active_low, 1, 1);
}
/**
 * Set DPI data enable pin polarity
 * @param dev pointer to structure holding the DSI Host core information
 * @param active_low (1) or active high (0)
 */
void mipi_dsih_hal_dpi_dataen_pol(struct mipi_dsi_dev *dev, int active_low)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_DPI_CFG_POL, active_low, 0, 1);
}
/**
 * Set the command transmission to be in low-power mode.
 * @param dev pointer to structure holding the DSI Host core information
 * @return lpm command transmission
 */
void mipi_dsih_hal_dpi_lp_cmd_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 15, 1);
}
/**
 * Enable FRAME BTA ACK
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_frame_ack_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 14, 1);
}
/**
 * Enable return to low power mode inside horizontal front porch periods when
 *  timing allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_lp_during_hfp(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 13, 1);
}
/**
 * Enable return to low power mode inside horizontal back porch periods when
 *  timing allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_lp_during_hbp(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 12, 1);
}
/**
 * Enable return to low power mode inside vertical active lines periods when
 *  timing allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_lp_during_vactive(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 11, 1);
}
/**
 * Enable return to low power mode inside vertical front porch periods when
 *  timing allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_lp_during_vfp(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 10, 1);
}
/**
 * Enable return to low power mode inside vertical back porch periods when
 * timing allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_lp_during_vbp(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 9, 1);
}
/**
 * Enable return to low power mode inside vertical sync periods when
 *  timing allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_lp_during_vsync(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 8, 1);
}
/**
 * Set DPI video mode type (burst/non-burst - with sync pulses or events)
 * @param dev pointer to structure holding the DSI Host core information
 * @param type
 * @return error code
 */
int mipi_dsih_hal_dpi_video_mode_type(struct mipi_dsi_dev *dev, dsih_video_mode_t type)
{
	if (type < 3)
		mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, type, 0, 2);
	else{
		mipi_dbg_print(MIPI_ERROR,"undefined type");
		return FALSE;
	}

	return TRUE;
}
/**
 * Enable/disable DPI video mode
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_dpi_video_mode_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_MODE_CFG, enable? 0: 1, 0, 1);
}
/**
 * Get the status of video mode, whether enabled or not in core
 * @param dev pointer to structure holding the DSI Host core information
 * @return status
 */
int mipi_dsih_hal_dpi_is_video_mode(struct mipi_dsi_dev *dev)
{
	return (mipi_dsih_read_part(dev, R_DSI_HOST_MODE_CFG, 0, 1) == 0);
}
/**
 * Write the null packet size - will only be taken into account when null
 * packets are enabled.
 * @param dev pointer to structure holding the DSI Host core information
 * @param size of null packet
 * @return error code
 */
int mipi_dsih_hal_dpi_null_packet_size(struct mipi_dsi_dev *dev, uint16_t size)
{
	if (size < (1 << 13)) /* 13-bit field */
		mipi_dsih_write_part(dev, R_DSI_HOST_VID_NULL_SIZE, size, 0, 13);
	else
		return FALSE;


	return TRUE;
}
/**
 * Write no of chunks to core - taken into consideration only when multi packet
 * is enabled
 * @param dev pointer to structure holding the DSI Host core information
 * @param no of chunks
 */
int mipi_dsih_hal_dpi_chunks_no(struct mipi_dsi_dev *dev, uint16_t no)
{
	if (no < (1 << 13))
		mipi_dsih_write_part(dev, R_DSI_HOST_VID_NUM_CHUNKS, no, 0, 13);
	else
		return FALSE;

	return TRUE;
}
/**
 * Write video packet size. obligatory for sending video
 * @param dev pointer to structure holding the DSI Host core information
 * @param size of video packet - containing information
 * @return error code
 */
int mipi_dsih_hal_dpi_video_packet_size(struct mipi_dsi_dev *dev, uint16_t size)
{
	if (size < (1 << 14)) /* 14-bit field */
		mipi_dsih_write_part(dev, R_DSI_HOST_VID_PKT_SIZE, size, 0, 14);
	else
		return FALSE;

	return TRUE;
}
/**
 * Specifiy the size of the packet memory write start/continue
 * @param dev pointer to structure holding the DSI Host core information
 * @ size of the packet
 * @note when different than zero (0) eDPI is enabled
 */
void mipi_dsih_hal_edpi_max_allowed_size(struct mipi_dsi_dev *dev, uint16_t size)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_EDPI_CMD_SIZE, size, 0, 16);
}
/**
 * Enable tear effect acknowledge
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_tear_effect_ack_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, enable, 0, 1);
}
/**
 * Enable packets acknowledge request after each packet transmission
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable (1) - disable (0)
 */
void mipi_dsih_hal_cmd_ack_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, enable, 1, 1);
}
/**
 * Set DCS command packet transmission to transmission type
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_param of command
 * @param lp transmit in low power
 * @return error code
 */
int mipi_dsih_hal_dcs_wr_tx_type(struct mipi_dsi_dev *dev, unsigned no_of_param, int lp)
{
	switch (no_of_param){
	case 0:
		/* This bit configures the DCS short write packet with zero
		 * parameter command transmission type */
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 16, 1);
		break;
	case 1:
		/* This bit configures the DCS short write packet with one
		 * parameter command transmission type */
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 17, 1);
		break;
	default:
		/* This bit configures the DCS long write packet command
		* transmission type */
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 19, 1);
		break;
	}

	return TRUE;
}
/**
 * Set DCS read command packet transmission to transmission type
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_param of command
 * @param lp transmit in low power
 * @return error code
 */
int mipi_dsih_hal_dcs_rd_tx_type(struct mipi_dsi_dev *dev, unsigned no_of_param, int lp)
{
	switch (no_of_param){
	case 0:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 18, 1);
		break;
	default:
		mipi_dbg_print(MIPI_ERROR,"undefined DCS Read packet type");
		return FALSE;
	}

	return TRUE;
}
/**
 * Set generic write command packet transmission to transmission type
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_param of command
 * @param lp transmit in low power
 * @return error code
 */
int mipi_dsih_hal_gen_wr_tx_type(struct mipi_dsi_dev *dev, unsigned no_of_param, int lp)
{
	switch (no_of_param){
	case 0:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 8, 1);
		break;
	case 1:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 9, 1);
		break;
	case 2:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 10, 1);
		break;
	default:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 14, 1);
		break;
	}

	return TRUE;
}
/**
 * Set generic command packet transmission to transmission type
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_param of command
 * @param lp transmit in low power
 * @return error code
 */
int mipi_dsih_hal_gen_rd_tx_type(struct mipi_dsi_dev *dev, unsigned no_of_param, int lp)
{
	switch (no_of_param){
	case 0:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 11, 1);
		break;
	case 1:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 12, 1);
		break;
	case 2:
		mipi_dsih_write_part(dev, R_DSI_HOST_CMD_MODE_CFG, lp, 13, 1);
		break;
	default:
		mipi_dbg_print(MIPI_ERROR,"undefined Generic Read packet type");
		return FALSE;
	}

	return TRUE;
}

/**
 * Enable command mode (Generic interface)
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 */
void mipi_dsih_hal_gen_cmd_mode_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_MODE_CFG, enable? 1: 0, 0, 1);
}
/**
 * Retrieve the controller's status of whether command mode is ON or not
 * @param dev pointer to structure holding the DSI Host core information
 * @return whether command mode is ON
 */
int mipi_dsih_hal_gen_is_cmd_mode(struct mipi_dsi_dev *dev)
{
	return (mipi_dsih_read_part(dev, R_DSI_HOST_MODE_CFG, 0, 1) == 1);
}
/**
 * Configure the Horizontal Line time
 * @param dev pointer to structure holding the DSI Host core information
 * @param time taken to transmit the total of the horizontal line
 */
void mipi_dsih_hal_dpi_hline(struct mipi_dsi_dev *dev, uint16_t time)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_HLINE_TIME, time, 0, 15);
}
/**
 * Configure the Horizontal back porch time
 * @param dev pointer to structure holding the DSI Host core information
 * @param time taken to transmit the horizontal back porch
 */
void mipi_dsih_hal_dpi_hbp(struct mipi_dsi_dev *dev, uint16_t time)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_HBP_TIME, time, 0, 12);
}
/**
 * Configure the Horizontal sync time
 * @param dev pointer to structure holding the DSI Host core information
 * @param time taken to transmit the horizontal sync
 */
void mipi_dsih_hal_dpi_hsa(struct mipi_dsi_dev *dev, uint16_t time)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_HSA_TIME, time, 0, 12);
}
/**
 * Configure the vertical active lines of the video stream
 * @param dev pointer to structure holding the DSI Host core information
 * @param lines
 */
void mipi_dsih_hal_dpi_vactive(struct mipi_dsi_dev *dev, uint16_t lines)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_VACTIVE_LINES, lines, 0, 14);
}
/**
 * Configure the vertical front porch lines of the video stream
 * @param dev pointer to structure holding the DSI Host core information
 * @param lines
 */
void mipi_dsih_hal_dpi_vfp(struct mipi_dsi_dev *dev, uint16_t lines)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_VFP_LINES, lines, 0, 10);
}
/**
 * Configure the vertical back porch lines of the video stream
 * @param dev pointer to structure holding the DSI Host core information
 * @param lines
 */
void mipi_dsih_hal_dpi_vbp(struct mipi_dsi_dev *dev, uint16_t lines)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_VBP_LINES, lines, 0, 10);
}
/**
 * Configure the vertical sync lines of the video stream
 * @param dev pointer to structure holding the DSI Host core information
 * @param lines
 */
void mipi_dsih_hal_dpi_vsync(struct mipi_dsi_dev *dev, uint16_t lines)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_VSA_LINES, lines, 0, 10);
}
/**
 * configure timeout divisions (so they would have more clock ticks)
 * @param dev pointer to structure holding the DSI Host core information
 * @param byte_clk_division_factor no of hs cycles before transiting back to LP in
 *  (lane_clk / byte_clk_division_factor)
 */
void mipi_dsih_hal_timeout_clock_division(struct mipi_dsi_dev *dev, uint8_t byte_clk_division_factor)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_CLK_MGR, byte_clk_division_factor, 8, 8);
}
/**
 * Configure the Low power receive time out
 * @param dev pointer to structure holding the DSI Host core information
 * @param count (of byte cycles)
 */
void mipi_dsih_hal_lp_rx_timeout(struct mipi_dsi_dev *dev, uint16_t count)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_TO_CNT_CFG, count, 0, 16);
}
/**
 * Configure a high speed transmission time out7
 * @param dev pointer to structure holding the DSI Host core information
 * @param count (byte cycles)
 */
void mipi_dsih_hal_hs_tx_timeout(struct mipi_dsi_dev *dev, uint16_t count)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_TO_CNT_CFG, count, 16, 16);
}
/**
 * Get the error 0 interrupt register status
 * @param dev pointer to structure holding the DSI Host core information
 * @param mask the mask to be read from the register
 * @return error status 0 value
 */
uint32_t mipi_dsih_hal_int_status_0(struct mipi_dsi_dev *dev, uint32_t mask)
{
	return (mipi_dsih_read_word(dev, R_DSI_HOST_INT_ST0) & mask);
}
/**
 * Get the error 1 interrupt register status
 * @param dev pointer to structure holding the DSI Host core information
 * @param mask the mask to be read from the register
 * @return error status 1 value
 */
uint32_t mipi_dsih_hal_int_status_1(struct mipi_dsi_dev *dev, uint32_t mask)
{
	return (mipi_dsih_read_word(dev, R_DSI_HOST_INT_ST1) & mask);
}
/**
 * Configure MASK (hiding) of interrupts coming from error 0 source
 * @param dev pointer to structure holding the DSI Host core information
 * @param mask to be written to the register
 */
void mipi_dsih_hal_int_mask_0(struct mipi_dsi_dev *dev, uint32_t mask)
{
	mipi_dsih_write_word(dev, R_DSI_HOST_INT_MSK0, mask);
}
/**
 * Get the ERROR MASK  0 register status
 * @param dev pointer to structure holding the DSI Host core information
 * @param mask the bits to read from the mask register
 */
uint32_t mipi_dsih_hal_int_get_mask_0(struct mipi_dsi_dev *dev, uint32_t mask)
{
	return (mipi_dsih_read_word(dev, R_DSI_HOST_INT_MSK0) & mask);
}
/**
 * Configure MASK (hiding) of interrupts coming from error 0 source
 * @param dev pointer to structure holding the DSI Host core information
 * @param mask the mask to be written to the register
 */
void mipi_dsih_hal_int_mask_1(struct mipi_dsi_dev *dev, uint32_t mask)
{
	mipi_dsih_write_word(dev, R_DSI_HOST_INT_MSK1, mask);
}
/**
 * Get the ERROR MASK  1 register status
 * @param dev pointer to structure holding the DSI Host core information
 * @param mask the bits to read from the mask register
 */
uint32_t mipi_dsih_hal_int_get_mask_1(struct mipi_dsi_dev *dev, uint32_t mask)
{
	return (mipi_dsih_read_word(dev, R_DSI_HOST_INT_MSK1) & mask);
}

/**
 * Force Interrupt coming from INT 0
 * @param dev pointer to structure holding the DSI Host core information
 * @param force interrupts to be forced
 */
void mipi_dsih_hal_force_int_0(struct mipi_dsi_dev *dev, uint32_t force)
{
	mipi_dsih_write_word(dev, R_DSI_HOST_INT_FORCE0, force);
}

/**
 * Force Interrupt coming from INT 0
 * @param dev pointer to structure holding the DSI Host core information
 * @param force interrupts to be forced
 */
void mipi_dsih_hal_force_int_1(struct mipi_dsi_dev *dev, uint32_t force)
{
	mipi_dsih_write_word(dev, R_DSI_HOST_INT_FORCE1, force);
}

/* DBI NOT IMPLEMENTED */
void mipi_dsih_hal_dbi_out_color_coding(struct mipi_dsi_dev *dev, uint8_t color_depth, uint8_t option);
void mipi_dsih_hal_dbi_in_color_coding(struct mipi_dsi_dev *dev, uint8_t color_depth, uint8_t option);
void mipi_dsih_hal_dbi_lut_size(struct mipi_dsi_dev *dev, uint8_t size);
void mipi_dsih_hal_dbi_partitioning_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dbi_dcs_vc(struct mipi_dsi_dev *dev, uint8_t vc);
void mipi_dsih_hal_dbi_max_cmd_size(struct mipi_dsi_dev *dev, uint16_t size);
void mipi_dsih_hal_dbi_cmd_size(struct mipi_dsi_dev *dev, uint16_t size);
int mipi_dsih_hal_dbi_rd_cmd_busy(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_read_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_read_fifo_empty(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_write_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_write_fifo_empty(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_cmd_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_cmd_fifo_empty(struct mipi_dsi_dev *dev);

/**
 * Write command header in the generic interface
 * (which also sends DCS commands) as a subset
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc of destination
 * @param packet_type (or type of DCS command)
 * @param ls_byte (if DCS, it is the DCS command)
 * @param ms_byte (only parameter of short DCS packet)
 * @return error code
 */
int mipi_dsih_hal_gen_packet_header(struct mipi_dsi_dev *dev, uint8_t vc, uint8_t packet_type, uint8_t ms_byte, uint8_t ls_byte)
{
	if (vc < 4)
	{
		mipi_dsih_write_part(dev, R_DSI_HOST_GEN_HDR, (ms_byte <<  16) | (ls_byte << 8 ) | ((vc << 6) | packet_type), 0, 24);
		return TRUE;
	}
	return  FALSE;
}
/**
 * Write the payload of the long packet commands
 * @param dev pointer to structure holding the DSI Host core information
 * @param payload array of bytes of payload
 * @return error code
 */
int mipi_dsih_hal_gen_packet_payload(struct mipi_dsi_dev *dev, uint32_t payload)
{
	if (mipi_dsih_hal_gen_write_fifo_full(dev))
		return FALSE;

	mipi_dsih_write_word(dev, R_DSI_HOST_GEN_PLD_DATA, payload);
	return TRUE;

}
/**
 * Write the payload of the long packet commands
 * @param dev pointer to structure holding the DSI Host core information
 * @param payload pointer to 32-bit array to hold read information
 * @return error code
 */
int  mipi_dsih_hal_gen_read_payload(struct mipi_dsi_dev *dev, uint32_t* payload)
{
	*payload = mipi_dsih_read_word(dev, R_DSI_HOST_GEN_PLD_DATA);
	return TRUE;
}

/**
 * Configure the read back virtual channel for the generic interface
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc to listen to on the line
 */
void mipi_dsih_hal_gen_rd_vc(struct mipi_dsi_dev *dev, uint8_t vc)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_GEN_VCID, vc, 0, 2);
}
/**
 * Enable EOTp reception
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 */
void mipi_dsih_hal_gen_eotp_rx_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PCKHDL_CFG, enable, 1, 1);
}
/**
 * Enable EOTp transmission
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 */
void mipi_dsih_hal_gen_eotp_tx_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PCKHDL_CFG, enable, 0, 1);
}
/**
 * Enable Bus Turn-around request
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 */
void mipi_dsih_hal_bta_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PCKHDL_CFG, enable, 2, 1);
}
/**
 * Enable ECC reception, error correction and reporting
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 */
void mipi_dsih_hal_gen_ecc_rx_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PCKHDL_CFG, enable, 3, 1);
}
/**
 * Enable CRC reception, error reporting
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 */
void mipi_dsih_hal_gen_crc_rx_en(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PCKHDL_CFG, enable, 4, 1);
}
/**
 * Get status of read command
 * @param dev pointer to structure holding the DSI Host core information
 * @return 1 if busy
 */
int mipi_dsih_hal_gen_rd_cmd_busy(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 6, 1);
}
/**
 * Get the FULL status of generic read payload fifo
 * @param dev pointer to structure holding the DSI Host core information
 * @return 1 if fifo full
 */
int mipi_dsih_hal_gen_read_fifo_full(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 5, 1);
}
/**
 * Get the EMPTY status of generic read payload fifo
 * @param dev pointer to structure holding the DSI Host core information
 * @return 1 if fifo empty
 */
int mipi_dsih_hal_gen_read_fifo_empty(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 4, 1);
}
/**
 * Get the FULL status of generic write payload fifo
 * @param dev pointer to structure holding the DSI Host core information
 * @return 1 if fifo full
 */
int mipi_dsih_hal_gen_write_fifo_full(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 3, 1);
}
/**
 * Get the EMPTY status of generic write payload fifo
 * @param dev pointer to structure holding the DSI Host core information
 * @return 1 if fifo empty
 */
int mipi_dsih_hal_gen_write_fifo_empty(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 2, 1);
}
/**
 * Get the FULL status of generic command fifo
 * @param dev pointer to structure holding the DSI Host core information
 * @return 1 if fifo full
 */
int mipi_dsih_hal_gen_cmd_fifo_full(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 1, 1);
}
/**
 * Get the EMPTY status of generic command fifo
 * @param dev pointer to structure holding the DSI Host core information
 * @return 1 if fifo empty
 */
int mipi_dsih_hal_gen_cmd_fifo_empty(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 0, 1);
}
/* only if DPI */
/**
 * Configure how many cycles of byte clock would the PHY module take
 * to switch data lane from high speed to low power
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles
 * @return error code
 */
int mipi_dsih_phy_hs2lp_config(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PHY_TMR_CFG, no_of_byte_cycles, 16, 10);
	return TRUE;
}
/**
 * Configure how many cycles of byte clock would the PHY module take
 * to switch the data lane from to low power high speed
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles
 * @return error code
 */
int mipi_dsih_phy_lp2hs_config(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PHY_TMR_CFG, no_of_byte_cycles, 0, 10);
	return TRUE;
}
/**
 * Configure how many cycles of byte clock would the PHY module take
 * to switch clock lane from high speed to low power
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles
 * @return error code
 */
int mipi_dsih_phy_clk_hs2lp_config(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PHY_TMR_LPCLK_CFG, no_of_byte_cycles, 16, 10);
	return TRUE;
}
/**
 * Configure how many cycles of byte clock would the PHY module take
 * to switch clock lane from to low power high speed
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles
 * @return error code
 */
int mipi_dsih_phy_clk_lp2hs_config(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_PHY_TMR_LPCLK_CFG, no_of_byte_cycles, 0, 10);
	return TRUE;
}
/**
 * Configure how many cycles of byte clock would the PHY module take
 * to turn the bus around to start receiving
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles
 * @return error code
 */
int mipi_dsih_phy_bta_time(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	if (no_of_byte_cycles < 0x8000){ /* 15-bit field */
		if(dev->hw_version == DSI_VERSION_131)
			mipi_dsih_write_part(dev, PHY_TMR_RD_CFG, no_of_byte_cycles, 0, 15);
		else
			mipi_dsih_write_part(dev, R_DSI_HOST_PHY_TMR_CFG, no_of_byte_cycles, 0, 15);
	}
	else
		return FALSE;

	return TRUE;
}
/**
 * Enable the automatic mechanism to stop providing clock in the clock
 * lane when time allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable
 * @return error code
 */
void mipi_dsih_non_continuous_clock(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_LPCLK_CTRL, enable, 1, 1);
}
/**
 * Get the status of the automatic mechanism to stop providing clock in the
 * clock lane when time allows
 * @param dev pointer to structure holding the DSI Host core information
 * @return status 1 (enabled) 0 (disabled)
 */
int mipi_dsih_non_continuous_clock_status(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_LPCLK_CTRL, 1, 1);
}
/* PRESP Time outs */
/**
 * Timeout for peripheral (for controller to stay still) after LP data
 * transmission write requests
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a low power write operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_hal_presp_timeout_low_power_write(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_LP_WR_TO_CNT, no_of_byte_cycles, 0, 16);
}
/**
 * Timeout for peripheral (for controller to stay still) after LP data
 * transmission read requests
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a low power read operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_hal_presp_timeout_low_power_read(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_LP_RD_TO_CNT, no_of_byte_cycles, 0, 16);
}
/**
 * Timeout for peripheral (for controller to stay still) after HS data
 * transmission write requests
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a high-speed write operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_hal_presp_timeout_high_speed_write(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_HS_WR_TO_CNT, no_of_byte_cycles, 0, 16);
}
/**
 * Timeout for peripheral between HS data transmission read requests
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a high-speed read operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_hal_presp_timeout_high_speed_read(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_HS_RD_TO_CNT, no_of_byte_cycles, 0, 16);
}
/**
 * Timeout for peripheral (for controller to stay still) after bus turn around
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a BTA operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_hal_presp_timeout_bta(struct mipi_dsi_dev *dev, uint16_t no_of_byte_cycles)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_BTA_TO_CNT, no_of_byte_cycles, 0, 16);
}

/**
 * Change Pattern orientation
 * @param dev pointer to structure holding the DSI Host core information
 * @param orientation choose between horizontal or vertical pattern
 */
void mipi_dsih_hal_vpg_orientation_act(struct mipi_dsi_dev *dev,uint8_t orientation)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, orientation, 24, 1);
}

/**
 * Change Pattern Type
 * @param dev pointer to structure holding the DSI Host core information
 * @param mode choose between normal or BER pattern
 */
void mipi_dsih_hal_vpg_mode_act(struct mipi_dsi_dev *dev,uint8_t mode)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, mode, 20, 1);
}

/**
 * Change Video Pattern Generator
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable enable video pattern generator
 */
void mipi_dsih_hal_enable_vpg_act(struct mipi_dsi_dev *dev,uint8_t enable)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_MODE_CFG, enable, 16, 1);
}

/**
 * Function to activate shadow registers functionality
 * @param dev pointer to structure holding the DSI Host core information
 * @param activate activate or deactivate shadow registers
 */
void mipi_dsih_hal_activate_shadow_registers(struct mipi_dsi_dev *dev,uint8_t activate)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_SHADOW_CTRL, activate, 0, 1);
}

/**
 * Function to read shadow registers functionality state
 * @param dev pointer to structure holding the DSI Host core information
 */
uint8_t mipi_dsih_hal_read_state_shadow_registers(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_SHADOW_CTRL, 0, 1);
}

/**
 * Request registers change
 * @param dev pointer to structure holding the DSI Host core information
 */
void mipi_dsih_hal_request_registers_change(struct mipi_dsi_dev *dev)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_SHADOW_CTRL, 1, 8, 1);
}

/**
 * Use external pin as control to registers change
 * @param dev pointer to structure holding the DSI Host core information
 * @param external choose between external or internal control
 */
void mipi_dsih_hal_external_pin_registers_change(struct mipi_dsi_dev *dev,uint8_t external)
{
	mipi_dsih_write_part(dev, R_DSI_HOST_VID_SHADOW_CTRL, external, 16, 1);
}

/**
 * Get the DPI video virtual channel destination
 * @param dev pointer to structure holding the DSI Host core information
 * @return virtual channel
 */
uint8_t mipi_dsih_hal_get_dpi_video_vc_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_DPI_VCID_ACT, 0, 2);
}

/**
 * Get loosely packed variant to 18-bit configurations status
 * @param dev pointer to structure holding the DSI Host core information
 * @return loosely status
 */
uint8_t mipi_dsih_hal_get_loosely18_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_DPI_COLOR_CODING_ACT, 8, 1);
}

/**
 * Get DPI video color coding
 * @param dev pointer to structure holding the DSI Host core information
 * @return color coding enum (configuration and color depth)
 */
dsih_color_coding_t mipi_dsih_hal_get_dpi_color_coding_act(struct mipi_dsi_dev *dev)
{
	return (dsih_color_coding_t)(mipi_dsih_read_part(dev, R_DSI_HOST_DPI_COLOR_CODING_ACT, 0, 4));
}

/**
 * See if the command transmission is in low-power mode.
 * @param dev pointer to structure holding the DSI Host core information
 * @return lpm command transmission
 */
uint8_t mipi_dsih_hal_get_lp_cmd_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 15, 1);
}

/**
 * See if there is a request for an acknowledge response at
 * the end of a frame.
 * @param dev pointer to structure holding the DSI Host core information
 * @return  acknowledge response
 */
uint8_t mipi_dsih_hal_get_frame_bta_ack_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 14, 1);
}

/**
 * Get the return to low-power inside the Horizontal Front Porch (HFP)
 * period when timing allows.
 * @param dev pointer to structure holding the DSI Host core information
 * @return return to low-power
 */
uint8_t mipi_dsih_hal_get_lp_hfp_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 13, 1);
}

/**
 * Get the return to low-power inside the Horizontal Back Porch (HBP) period
 * when timing allows.
 * @param dev pointer to structure holding the DSI Host core information
 * @return return to low-power
 */
uint8_t mipi_dsih_hal_get_lp_hbp_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 12, 1);
}

/**
 * Get the return to low-power inside the Vertical Active (VACT) period when
 * timing allows.
 * @param dev pointer to structure holding the DSI Host core information
 * @return return to low-power
 */
uint8_t mipi_dsih_hal_get_lp_vact_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 11, 1);
}

/**
 * Get the return to low-power inside the Vertical Front Porch (VFP) period
 * when timing allows.
 * @param dev pointer to structure holding the DSI Host core information
 * @return return to low-power
 */
uint8_t mipi_dsih_hal_get_lp_vfp_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 10, 1);
}

/**
 * Get the return to low-power inside the Vertical Back Porch (VBP) period
 * when timing allows.
 * @param dev pointer to structure holding the DSI Host core information
 * @return return to low-power
 */
uint8_t mipi_dsih_hal_get_lp_vbp_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 9, 1);
}

/**
 * Get the return to low-power inside the Vertical Sync Time (VSA) period
 * when timing allows.
 * @param dev pointer to structure holding the DSI Host core information
 * @return return to low-power
 */
uint8_t mipi_dsih_hal_get_lp_vsa_en_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 8, 1);
}

/**
 * Get the video mode transmission type
 * @param dev pointer to structure holding the DSI Host core information
 * @return video mode transmission type
 */
uint8_t mipi_dsih_hal_get_vid_mode_type_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_MODE_CFG_ACT, 0, 2);
}

/**
 * Get the number of pixels in a single video packet
 * @param dev pointer to structure holding the DSI Host core information
 * @return video packet size
 */
uint16_t mipi_dsih_hal_get_vid_pkt_size_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_PKT_SIZE_ACT,0, 14);
}

/**
 * Get the number of chunks to be transmitted during a Line period
 * (a chunk is pair made of a video packet and a null packet).
 * @param dev pointer to structure holding the DSI Host core information
 * @return num_chunks number of chunks to use
 */
uint16_t mipi_dsih_hal_get_vid_num_chunks_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_NUM_CHUNKS_ACT,0, 13);
}

/**
 * Get the number of bytes inside a null packet
 * @param dev pointer to structure holding the DSI Host core information
 * @return size of null packets
 */
uint16_t mipi_dsih_hal_get_vid_null_size_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_NULL_SIZE_ACT,0, 13);
}

/**
 * Get the Horizontal Synchronism Active period in lane byte clock cycles.
 * @param dev pointer to structure holding the DSI Host core information
 * @return video HSA time
 */
uint16_t mipi_dsih_hal_get_vid_hsa_time_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_HSA_TIME_ACT, 0, 12);
}

/**
 * Get the Horizontal Back Porch period in lane byte clock cycles.
 * @param dev pointer to structure holding the DSI Host core information
 * @return video HBP time
 */
uint16_t mipi_dsih_hal_get_vid_hbp_time_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_HBP_TIME_ACT, 0, 12);
}

/**
 * Get the size of the total line time (HSA+HBP+HACT+HFP)
 * counted in lane byte clock cycles.
 * @param dev pointer to structure holding the DSI Host core information
 * @return overall time for each video line
 */
uint16_t mipi_dsih_hal_get_vid_hline_time_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_HLINE_TIME_ACT, 0, 15);
}

/**
 * Get the Vertical Synchronism Active period measured in number of horizontal lines.
 * @param dev pointer to structure holding the DSI Host core information
 * @return VSA period
 */
uint16_t mipi_dsih_hal_get_vsa_lines_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_VSA_LINES_ACT, 0, 10);
}

/**
 * Get the Vertical Back Porch period measured in number of horizontal lines.
 * @param dev pointer to structure holding the DSI Host core information
 * @return VBP period
 */
uint16_t mipi_dsih_hal_get_vbp_lines_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_VBP_LINES_ACT, 0, 10);
}

/**
 * Get the Vertical Front Porch period measured in number of horizontal lines.
 * @param dev pointer to structure holding the DSI Host core information
 * @return VFP period
 */
uint16_t mipi_dsih_hal_get_vfp_lines_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_VFP_LINES_ACT, 0, 10);
}

/**
 * Get the Vertical Active period measured in number of horizontal lines
 * @param dev pointer to structure holding the DSI Host core information
 * @return vertical resolution of video.
 */
uint16_t mipi_dsih_hal_get_v_active_lines_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_VID_VACTIVE_LINES_ACT, 0, 14);
}

/**
 * See if the next VSS packet includes 3D control payload
 * @param dev pointer to structure holding the DSI Host core information
 * @return include 3D control payload
 */
uint8_t mipi_dsih_hal_get_send_3d_cfg_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_SDF_3D_ACT, 16, 1);
}

/**
 * Get the left/right order:
 * @param dev pointer to structure holding the DSI Host core information
 * @return left/right order:
 */
uint8_t mipi_dsih_hal_get_right_left_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_SDF_3D_ACT,5, 1);
}

/**
 * See if there is a second VSYNC pulse between left and right Images,
 * when 3D image format is frame-based:
 * @param dev pointer to structure holding the DSI Host core information
 * @return second vsync
 */
uint8_t mipi_dsih_hal_get_second_vsync_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_SDF_3D_ACT, 4, 1);
}

/**
 * Get the 3D image format
 * @param dev pointer to structure holding the DSI Host core information
 * @return 3D image format
 */
uint8_t mipi_dsih_hal_get_format_3d_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_SDF_3D_ACT,2, 2);
}

/**
 * Get the 3D mode on/off and display orientation
 * @param dev pointer to structure holding the DSI Host core information
 * @return 3D mode
 */
uint8_t mipi_dsih_hal_get_mode_3d_act(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_read_part(dev, R_DSI_HOST_SDF_3D_ACT, 0, 2);
}

void mipi_dsih_hal_auto_ulps_entry(struct mipi_dsi_dev *dev, int delay)
{
	mipi_dsih_write_part(dev, R_DSI_AUTO_ULPS_ENTRY_DELAY, delay, 0 ,31);
}

void mipi_dsih_hal_auto_ulps_wakeup(struct mipi_dsi_dev *dev,
		int twakeup_clk_div, int twakeup_cnt)
{
	mipi_dsih_write_part(dev, R_DSI_AUTO_ULPS_WAKEUP_TIME,
			twakeup_clk_div, 0 ,15);
	mipi_dsih_write_part(dev, R_DSI_AUTO_ULPS_WAKEUP_TIME,
			twakeup_cnt, 16 ,31);
}

void mipi_dsih_hal_auto_ulps_mode(struct mipi_dsi_dev *dev, int mode)
{
	mipi_dsih_write_part(dev, R_DSI_AUTO_ULPS_MODE, mode, 0 ,1);
}

void mipi_dsih_hal_pll_off_in_ulps(struct mipi_dsi_dev *dev,
		int pll_off_ulps, int pre_pll_off_req)
{
	mipi_dsih_write_part(dev, R_DSI_AUTO_ULPS_MODE,
			pll_off_ulps, 16 ,1);
	mipi_dsih_write_part(dev, R_DSI_AUTO_ULPS_MODE,
			pre_pll_off_req, 17 ,1);
}

