/*
 * @file mipi_dsih_hal.h
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

#ifndef MIPI_DSIH_HAL_H_
#define MIPI_DSIH_HAL_H_

#include "../src/includes.h"

#define R_DSI_HOST_VERSION			(0x00)
#define R_DSI_HOST_PWR_UP			(0x04)
#define R_DSI_HOST_CLK_MGR			(0x08)
#define R_DSI_HOST_DPI_VCID			(0x0C)
#define R_DSI_HOST_DPI_COLOR_CODE		(0x10)
#define R_DSI_HOST_DPI_CFG_POL			(0x14)
#define R_DSI_HOST_DPI_LP_CMD_TIM		(0x18)
#define R_DSI_HOST_DBI_VCID			(0x1C)
#define R_DSI_HOST_DBI_CFG			(0x20)
#define R_DSI_HOST_DBI_PARTITION_EN		(0x24)
#define R_DSI_HOST_DBI_CMDSIZE			(0x28)
#define R_DSI_HOST_PCKHDL_CFG			(0x2C)
#define R_DSI_HOST_GEN_VCID			(0x30)
#define R_DSI_HOST_MODE_CFG			(0x34)
#define R_DSI_HOST_VID_MODE_CFG			(0x38)
#define R_DSI_HOST_VID_PKT_SIZE			(0x3C)
#define R_DSI_HOST_VID_NUM_CHUNKS		(0x40)
#define R_DSI_HOST_VID_NULL_SIZE		(0x44)
#define R_DSI_HOST_VID_HSA_TIME			(0x48)
#define R_DSI_HOST_VID_HBP_TIME			(0x4C)
#define R_DSI_HOST_VID_HLINE_TIME		(0x50)
#define R_DSI_HOST_VID_VSA_LINES		(0x54)
#define R_DSI_HOST_VID_VBP_LINES		(0x58)
#define R_DSI_HOST_VID_VFP_LINES		(0x5C)
#define R_DSI_HOST_VID_VACTIVE_LINES		(0x60)
#define R_DSI_HOST_EDPI_CMD_SIZE		(0x64)
#define R_DSI_HOST_CMD_MODE_CFG			(0x68)
#define R_DSI_HOST_GEN_HDR			(0x6C)
#define R_DSI_HOST_GEN_PLD_DATA			(0x70)
#define R_DSI_HOST_CMD_PKT_STATUS		(0x74)
#define R_DSI_HOST_TO_CNT_CFG			(0x78)
#define R_DSI_HOST_HS_RD_TO_CNT			(0x7C)
#define R_DSI_HOST_LP_RD_TO_CNT			(0x80)
#define R_DSI_HOST_HS_WR_TO_CNT			(0x84)
#define R_DSI_HOST_LP_WR_TO_CNT			(0x88)
#define R_DSI_HOST_BTA_TO_CNT			(0x8C)

#define R_DSI_HOST_SDF_3D			(0x90)

#define R_DSI_HOST_LPCLK_CTRL			(0x94)
#define R_DSI_HOST_PHY_TMR_LPCLK_CFG		(0x98)
#define R_DSI_HOST_PHY_TMR_CFG			(0x9C)
#define R_DSI_HOST_INT_ST0			(0xBC)
#define R_DSI_HOST_INT_ST1			(0xC0)
#define R_DSI_HOST_INT_MSK0			(0xC4)
#define R_DSI_HOST_INT_MSK1			(0xC8)
#define R_DSI_HOST_PHY_STATUS			(0xB0)

#define R_DSI_HOST_INT_FORCE0			(0x0D8)
#define R_DSI_HOST_INT_FORCE1			(0x0DC)

#define PHY_TMR_RD_CFG				(0xF4)	/* NEW Reg for 1.31a */

#define R_DSI_HOST_VID_SHADOW_CTRL		(0x100)

#define R_DSI_HOST_DPI_VCID_ACT			(0x10C)
#define R_DSI_HOST_DPI_COLOR_CODING_ACT		(0x110)
#define R_DSI_HOST_DPI_LP_CMD_TIM_ACT		(0x118)
#define R_DSI_HOST_VID_MODE_CFG_ACT		(0x138)
#define R_DSI_HOST_VID_PKT_SIZE_ACT		(0x13C)
#define R_DSI_HOST_VID_NUM_CHUNKS_ACT		(0x140)
#define R_DSI_HOST_VID_NULL_SIZE_ACT		(0x144)
#define R_DSI_HOST_VID_HSA_TIME_ACT		(0x148)
#define R_DSI_HOST_VID_HBP_TIME_ACT		(0x14C)
#define R_DSI_HOST_VID_HLINE_TIME_ACT		(0x150)
#define R_DSI_HOST_VID_VSA_LINES_ACT		(0x154)
#define R_DSI_HOST_VID_VBP_LINES_ACT		(0x158)
#define R_DSI_HOST_VID_VFP_LINES_ACT		(0x15C)
#define R_DSI_HOST_VID_VACTIVE_LINES_ACT	(0x160)
#define R_DSI_HOST_SDF_3D_ACT			(0x190)
uint32_t mipi_dsih_hal_get_version(struct mipi_dsi_dev *dev);
void mipi_dsih_hal_power(struct mipi_dsi_dev *dev, int on);
int mipi_dsih_hal_get_power(struct mipi_dsi_dev *dev);
void mipi_dsih_hal_tx_escape_division(struct mipi_dsi_dev *dev,
				      uint8_t tx_escape_division);
void mipi_dsih_hal_dpi_video_vc(struct mipi_dsi_dev *dev, uint8_t vc);
uint8_t mipi_dsih_hal_dpi_get_video_vc(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dpi_color_coding(struct mipi_dsi_dev *dev,
				   dsih_color_coding_t color_coding);
dsih_color_coding_t mipi_dsih_hal_dpi_get_color_coding(struct mipi_dsi_dev
						       *dev);
uint8_t mipi_dsih_hal_dpi_get_color_depth(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_dpi_get_color_config(struct mipi_dsi_dev *dev);
void mipi_dsih_hal_dpi_18_loosely_packet_en(struct mipi_dsi_dev *dev,
					    int enable);
void mipi_dsih_hal_dpi_color_mode_pol(struct mipi_dsi_dev *dev, int active_low);
void mipi_dsih_hal_dpi_shut_down_pol(struct mipi_dsi_dev *dev, int active_low);
void mipi_dsih_hal_dpi_hsync_pol(struct mipi_dsi_dev *dev, int active_low);
void mipi_dsih_hal_dpi_vsync_pol(struct mipi_dsi_dev *dev, int active_low);
void mipi_dsih_hal_dpi_dataen_pol(struct mipi_dsi_dev *dev, int active_low);
void mipi_dsih_hal_dpi_frame_ack_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dpi_lp_during_hfp(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dpi_lp_during_hbp(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dpi_lp_during_vactive(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dpi_lp_during_vfp(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dpi_lp_during_vbp(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dpi_lp_during_vsync(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_hal_dpi_video_mode_type(struct mipi_dsi_dev *dev,
				      dsih_video_mode_t type);
void mipi_dsih_hal_dpi_video_mode_en(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_hal_dpi_is_video_mode(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dpi_null_packet_size(struct mipi_dsi_dev *dev, uint16_t size);
int mipi_dsih_hal_dpi_chunks_no(struct mipi_dsi_dev *dev, uint16_t no);
int mipi_dsih_hal_dpi_video_packet_size(struct mipi_dsi_dev *dev,
					uint16_t size);
void mipi_dsih_hal_tear_effect_ack_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_cmd_ack_en(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_hal_dcs_wr_tx_type(struct mipi_dsi_dev *dev,
				 unsigned no_of_param, int lp);
int mipi_dsih_hal_dcs_rd_tx_type(struct mipi_dsi_dev *dev,
				 unsigned no_of_param, int lp);
int mipi_dsih_hal_gen_wr_tx_type(struct mipi_dsi_dev *dev,
				 unsigned no_of_param, int lp);
int mipi_dsih_hal_gen_rd_tx_type(struct mipi_dsi_dev *dev,
				 unsigned no_of_param, int lp);
void mipi_dsih_hal_max_rd_size_type(struct mipi_dsi_dev *dev, int lp);
void mipi_dsih_hal_gen_cmd_mode_en(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_hal_gen_is_cmd_mode(struct mipi_dsi_dev *dev);
void mipi_dsih_hal_dpi_hline(struct mipi_dsi_dev *dev, uint16_t time);
void mipi_dsih_hal_dpi_hbp(struct mipi_dsi_dev *dev, uint16_t time);
void mipi_dsih_hal_dpi_hsa(struct mipi_dsi_dev *dev, uint16_t time);
void mipi_dsih_hal_dpi_vactive(struct mipi_dsi_dev *dev, uint16_t lines);
void mipi_dsih_hal_dpi_vfp(struct mipi_dsi_dev *dev, uint16_t lines);
void mipi_dsih_hal_dpi_vbp(struct mipi_dsi_dev *dev, uint16_t lines);
void mipi_dsih_hal_dpi_vsync(struct mipi_dsi_dev *dev, uint16_t lines);
void mipi_dsih_hal_edpi_max_allowed_size(struct mipi_dsi_dev *dev,
					 uint16_t size);
int mipi_dsih_hal_gen_packet_header(struct mipi_dsi_dev *dev, uint8_t vc,
				    uint8_t packet_type, uint8_t ms_byte,
				    uint8_t ls_byte);
int mipi_dsih_hal_gen_packet_payload(struct mipi_dsi_dev *dev,
				     uint32_t payload);
int mipi_dsih_hal_gen_read_payload(struct mipi_dsi_dev *dev,
				   uint32_t * payload);
void mipi_dsih_hal_timeout_clock_division(struct mipi_dsi_dev *dev,
					  uint8_t byte_clk_division_factor);
void mipi_dsih_hal_lp_rx_timeout(struct mipi_dsi_dev *dev, uint16_t count);
void mipi_dsih_hal_hs_tx_timeout(struct mipi_dsi_dev *dev, uint16_t count);
uint32_t mipi_dsih_hal_int_status_0(struct mipi_dsi_dev *dev, uint32_t mask);
uint32_t mipi_dsih_hal_int_status_1(struct mipi_dsi_dev *dev, uint32_t mask);
void mipi_dsih_hal_int_mask_0(struct mipi_dsi_dev *dev, uint32_t mask);
void mipi_dsih_hal_int_mask_1(struct mipi_dsi_dev *dev, uint32_t mask);
uint32_t mipi_dsih_hal_int_get_mask_0(struct mipi_dsi_dev *dev, uint32_t mask);
uint32_t mipi_dsih_hal_int_get_mask_1(struct mipi_dsi_dev *dev, uint32_t mask);
void mipi_dsih_hal_force_int_0(struct mipi_dsi_dev *dev, uint32_t force);
void mipi_dsih_hal_force_int_1(struct mipi_dsi_dev *dev, uint32_t force);

/* DBI command interface */
void mipi_dsih_hal_dbi_out_color_coding(struct mipi_dsi_dev *dev,
					uint8_t color_depth, uint8_t option);
void mipi_dsih_hal_dbi_in_color_coding(struct mipi_dsi_dev *dev,
				       uint8_t color_depth, uint8_t option);
void mipi_dsih_hal_dbi_lut_size(struct mipi_dsi_dev *dev, uint8_t size);
void mipi_dsih_hal_dbi_partitioning_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_dbi_dcs_vc(struct mipi_dsi_dev *dev, uint8_t vc);
void mipi_dsih_hal_dbi_cmd_size(struct mipi_dsi_dev *dev, uint16_t size);
void mipi_dsih_hal_dbi_max_cmd_size(struct mipi_dsi_dev *dev, uint16_t size);
int mipi_dsih_hal_dbi_rd_cmd_busy(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_read_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_read_fifo_empty(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_write_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_write_fifo_empty(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_cmd_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_dbi_cmd_fifo_empty(struct mipi_dsi_dev *dev);

/* Generic command interface */
void mipi_dsih_hal_gen_rd_vc(struct mipi_dsi_dev *dev, uint8_t vc);
void mipi_dsih_hal_gen_eotp_rx_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_gen_eotp_tx_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_bta_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_gen_ecc_rx_en(struct mipi_dsi_dev *dev, int enable);
void mipi_dsih_hal_gen_crc_rx_en(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_hal_gen_rd_cmd_busy(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_gen_read_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_gen_read_fifo_empty(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_gen_write_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_gen_write_fifo_empty(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_gen_cmd_fifo_full(struct mipi_dsi_dev *dev);
int mipi_dsih_hal_gen_cmd_fifo_empty(struct mipi_dsi_dev *dev);

/* only if DPI */
int mipi_dsih_phy_hs2lp_config(struct mipi_dsi_dev *dev,
			       uint8_t no_of_byte_cycles);
int mipi_dsih_phy_lp2hs_config(struct mipi_dsi_dev *dev,
			       uint8_t no_of_byte_cycles);
int mipi_dsih_phy_clk_lp2hs_config(struct mipi_dsi_dev *dev,
				   uint8_t no_of_byte_cycles);
int mipi_dsih_phy_clk_hs2lp_config(struct mipi_dsi_dev *dev,
				   uint8_t no_of_byte_cycles);
int mipi_dsih_phy_bta_time(struct mipi_dsi_dev *dev,
			   uint16_t no_of_byte_cycles);
void mipi_dsih_non_continuous_clock(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_non_continuous_clock_status(struct mipi_dsi_dev *dev);

/* PRESP Time outs */
void mipi_dsih_hal_presp_timeout_low_power_write(struct mipi_dsi_dev *dev,
						 uint16_t no_of_byte_cycles);
void mipi_dsih_hal_presp_timeout_low_power_read(struct mipi_dsi_dev *dev,
						uint16_t no_of_byte_cycles);
void mipi_dsih_hal_presp_timeout_high_speed_write(struct mipi_dsi_dev *dev,
						  uint16_t no_of_byte_cycles);
void mipi_dsih_hal_presp_timeout_high_speed_read(struct mipi_dsi_dev *dev,
						 uint16_t no_of_byte_cycles);
void mipi_dsih_hal_presp_timeout_bta(struct mipi_dsi_dev *dev,
				     uint16_t no_of_byte_cycles);

/* bsp abstraction */
void mipi_dsih_write_word(struct mipi_dsi_dev *dev, uint32_t reg_address,
			  uint32_t data);
void mipi_dsih_write_part(struct mipi_dsi_dev *dev, uint32_t reg_address,
			  uint32_t data, uint8_t shift, uint8_t width);
uint32_t mipi_dsih_read_word(struct mipi_dsi_dev *dev, uint32_t reg_address);
uint32_t mipi_dsih_read_part(struct mipi_dsi_dev *dev, uint32_t reg_address,
			     uint8_t shift, uint8_t width);

/*Video Pattern generation abstraction */
void mipi_dsih_hal_enable_vpg_act(struct mipi_dsi_dev *dev, uint8_t enable);
void mipi_dsih_hal_vpg_orientation_act(struct mipi_dsi_dev *dev,
				       uint8_t orientation);
void mipi_dsih_hal_vpg_mode_act(struct mipi_dsi_dev *dev, uint8_t mode);

/* shadow registers */
void mipi_dsih_hal_activate_shadow_registers(struct mipi_dsi_dev *dev,
					     uint8_t activate);
uint8_t mipi_dsih_hal_read_state_shadow_registers(struct mipi_dsi_dev *dev);
void mipi_dsih_hal_request_registers_change(struct mipi_dsi_dev *dev);
void mipi_dsih_hal_external_pin_registers_change(struct mipi_dsi_dev *dev,
						 uint8_t external);
uint8_t mipi_dsih_hal_get_dpi_video_vc_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_loosely18_en_act(struct mipi_dsi_dev *dev);
dsih_color_coding_t mipi_dsih_hal_get_dpi_color_coding_act(struct
							   mipi_dsi_dev
							   *dev);
uint8_t mipi_dsih_hal_get_lp_cmd_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_frame_bta_ack_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_lp_hfp_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_lp_hbp_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_lp_vact_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_lp_vfp_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_lp_vbp_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_lp_vsa_en_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_vid_mode_type_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vid_pkt_size_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vid_num_chunks_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vid_null_size_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vid_hsa_time_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vid_hbp_time_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vid_hline_time_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vsa_lines_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vbp_lines_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_vfp_lines_act(struct mipi_dsi_dev *dev);
uint16_t mipi_dsih_hal_get_v_active_lines_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_send_3d_cfg_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_right_left_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_second_vsync_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_format_3d_act(struct mipi_dsi_dev *dev);
uint8_t mipi_dsih_hal_get_mode_3d_act(struct mipi_dsi_dev *dev);

#endif	/* MIPI_DSI_API_H_ */
