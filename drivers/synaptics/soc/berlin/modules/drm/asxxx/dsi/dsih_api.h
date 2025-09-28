/* SPDX-License-Identifier: GPL-2.0 */
/**
 * Copyright (C) 2018 Synopsys, Inc.
 *
 * @file dsih_api.h
 * @brief Synopsys MIPI DSI Host controller driver API
 * included as a part of Synopsys MIPI DSI Host controller driver
 *
 * @author Luis Oliveira <luis.oliveira@synopsys.com>
 */

#ifndef MIPI_DSIH_API_H_
#define MIPI_DSIH_API_H_

#include "includes.h"

/* general configuration functions */
int mipi_dsih_open(struct mipi_dsi_dev *dev, dsih_color_coding_t color_coding);
int mipi_dsih_close(struct mipi_dsi_dev *dev);
void mipi_dsih_reset_controller(struct mipi_dsi_dev *dev);
void mipi_dsih_shutdown_controller(struct mipi_dsi_dev *dev, int shutdown);
void mipi_dsih_reset_phy(struct mipi_dsi_dev *dev);
void mipi_dsih_shutdown_phy(struct mipi_dsi_dev *dev, int shutdown);

/* packet handling */
int mipi_dsih_enable_rx(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_peripheral_ack(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_tear_effect_ack(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_eotp_rx(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_ecc_rx(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_eotp_tx(struct mipi_dsi_dev *dev, int enable);

/* video mode functions */
int mipi_dsih_dpi_video(struct mipi_dsi_dev *dev);
void mipi_dsih_allow_return_to_lp(struct mipi_dsi_dev *dev,
	int hfp, int hbp, int vactive, int vfp, int vbp, int vsync);

void mipi_dsih_video_mode(struct mipi_dsi_dev *dev, int en);

/* command mode functions */
int mipi_dsih_dcs_wr_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
	uint8_t * params, uint16_t param_length);

int mipi_dsih_gen_wr_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
	uint8_t * params, uint16_t param_length);

int mipi_dsih_gen_wr_packet(struct mipi_dsi_dev *dev, uint8_t vc,
	uint8_t data_type, uint8_t * params, uint16_t param_length);

uint16_t mipi_dsih_dcs_rd_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
	uint8_t command, uint8_t bytes_to_read, uint8_t * read_buffer);

uint16_t mipi_dsih_gen_rd_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
	uint8_t * params, uint16_t param_length, uint8_t bytes_to_read,
	uint8_t * read_buffer);

uint16_t mipi_dsih_gen_rd_packet(struct mipi_dsi_dev *dev, uint8_t vc,
	uint8_t data_type, uint8_t msb_byte, uint8_t lsb_byte,
	uint8_t bytes_to_read, uint8_t * read_buffer);

void mipi_dsih_cmd_mode(struct mipi_dsi_dev *dev, int en);

int mipi_dsih_active_mode(struct mipi_dsi_dev *dev);

void mipi_dsih_dcs_cmd_lp_transmission(struct mipi_dsi_dev *dev,
	int long_write, int short_write, int short_read);

void mipi_dsih_gen_cmd_lp_transmission(struct mipi_dsi_dev *dev,
	int long_write, int short_write,int short_read);

int mipi_dsih_edpi_video(struct mipi_dsi_dev *dev);

/* PRESP Time outs */
void mipi_dsih_presp_timeout_low_power_write(struct mipi_dsi_dev *dev,
		uint16_t no_of_byte_cycles);
void mipi_dsih_presp_timeout_low_power_read(struct mipi_dsi_dev *dev,
		uint16_t no_of_byte_cycles);
void mipi_dsih_presp_timeout_high_speed_write(struct mipi_dsi_dev *dev,
		uint16_t no_of_byte_cycles);
void mipi_dsih_presp_timeout_high_speed_read(struct mipi_dsi_dev *dev,
		uint16_t no_of_byte_cycles);
void mipi_dsih_presp_timeout_bta(struct mipi_dsi_dev *dev,
	uint16_t no_of_byte_cycles);

/* read/write register functions */
uint32_t mipi_dsih_dump_register_configuration(struct mipi_dsi_dev *dev,
	int all, register_config_t * config, uint16_t config_length);

uint32_t mipi_dsih_write_register_configuration(struct mipi_dsi_dev *dev,
	register_config_t * config, uint16_t config_length);

uint16_t mipi_dsih_check_dbi_fifos_state(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_check_ulpm_mode(struct mipi_dsi_dev *dev);

void stop_video_pattern(struct mipi_dsi_dev *dev);
void start_video_pattern(struct mipi_dsi_dev *dev,
	unsigned char orientation, unsigned char pattern);

int mipi_dsi_init(struct mipi_dsi_dev *dev);

#endif	/* MIPI_DSIH_API_H_ */
