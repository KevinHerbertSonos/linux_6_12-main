/**
 * @file mipi_dsih_api.h
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
/*
	The Synopsys Software Driver and documentation (hereinafter "Software")
	is an unsupported proprietary work of Synopsys, Inc. unless otherwise
	expressly agreed to in writing between	Synopsys and you.

	The Software IS NOT an item of Licensed Software or Licensed Product under
	any End User Software License Agreement or Agreement for Licensed Product
	with Synopsys or any supplement	thereto.  Permission is hereby granted,
	free of charge, to any person obtaining a copy of this software annotated
	with this license and the Software, to deal in the Software without
	restriction, including without limitation the rights to use, copy, modify,
	merge, publish, distribute, sublicense,	and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject
	to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
	AND ANY	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
	INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE	POSSIBILITY OF SUCH
	DAMAGE.
 */
#ifndef MIPI_DSIH_API_H_
#define MIPI_DSIH_API_H_

/* general configuration functions */

#include "../src/includes.h"

int mipi_dsih_open(struct mipi_dsi_dev *dev);
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
void mipi_dsih_allow_return_to_lp(struct mipi_dsi_dev *dev, int hfp, int hbp,
				  int vactive, int vfp, int vbp, int vsync);
void mipi_dsih_video_mode(struct mipi_dsi_dev *dev, int en);

/* command mode functions */
int mipi_dsih_dcs_wr_cmd(struct mipi_dsi_dev *dev, uint8_t vc, uint8_t *params,
			 uint16_t param_length);
int mipi_dsih_gen_wr_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
			 uint8_t *params, uint16_t param_length);
int mipi_dsih_gen_wr_packet(struct mipi_dsi_dev *dev, uint8_t vc,
			    uint8_t data_type, uint8_t * params,
			    uint16_t param_length);
uint16_t mipi_dsih_dcs_rd_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
			      uint8_t command, uint8_t bytes_to_read,
			      uint8_t *read_buffer);
uint16_t mipi_dsih_gen_rd_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
			      uint8_t *params, uint16_t param_length,
			      uint8_t bytes_to_read, uint8_t *read_buffer);
uint16_t mipi_dsih_gen_rd_packet(struct mipi_dsi_dev *dev, uint8_t vc,
				 uint8_t data_type, uint8_t msb_byte,
				 uint8_t lsb_byte, uint8_t bytes_to_read,
				 uint8_t *read_buffer);
void mipi_dsih_cmd_mode(struct mipi_dsi_dev *dev, int en);
int mipi_dsih_active_mode(struct mipi_dsi_dev *dev);
void mipi_dsih_dcs_cmd_lp_transmission(struct mipi_dsi_dev *dev,
				       int long_write, int short_write,
				       int short_read);
void mipi_dsih_gen_cmd_lp_transmission(struct mipi_dsi_dev *dev,
				       int long_write, int short_write,
				       int short_read);
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
					       int all,
					       register_config_t *config,
					       uint16_t config_length);
uint32_t mipi_dsih_write_register_configuration(struct mipi_dsi_dev *dev,
						register_config_t *config,
						uint16_t config_length);
uint16_t mipi_dsih_check_dbi_fifos_state(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_check_ulpm_mode(struct mipi_dsi_dev *dev);
void stop_video_pattern(struct mipi_dsi_dev *dev);
void start_video_pattern(struct mipi_dsi_dev *dev, unsigned char orientation,
			 unsigned char pattern);

int
mipi_dsih_parse_display_sequence(struct mipi_dsi_dev *dev,
				 const char *sequence);

#endif	/* MIPI_DSIH_API_H_ */
