/*
 * @file mipi_dsih_dphy.h
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef MIPI_DSIH_DPHY_H_
#define MIPI_DSIH_DPHY_H_

#include "../src/includes.h"

#define R_DPHY_LPCLK_CTRL	(0x94)
#define R_DPHY_RSTZ		(0xA0)
#define R_DPHY_IF_CFG		(0xA4)
#define R_DPHY_ULPS_CTRL	(0xA8)
#define R_DPHY_TX_TRIGGERS	(0xAC)
#define R_DPHY_STATUS		(0xB0)
#define R_DPHY_TST_CRTL0	(0xB4)
#define R_DPHY_TST_CRTL1	(0xB8)

/* obligatory functions - code can be changed for different phys*/
int mipi_dsih_dphy_open(struct mipi_dsi_dev *dev);
int mipi_dsih_dphy_configure(struct mipi_dsi_dev *dev, uint8_t no_of_lanes,
			     uint32_t output_freq);
int mipi_dsih_dphy_close(struct mipi_dsi_dev *dev);
void mipi_dsih_dphy_clock_en(struct mipi_dsi_dev *dev, int en);
void mipi_dsih_dphy_reset(struct mipi_dsi_dev *dev, int reset);
void mipi_dsih_dphy_shutdown(struct mipi_dsi_dev *dev, int powerup);
int mipi_dsih_dphy_get_force_pll(struct mipi_dsi_dev *dev);
void mipi_dsih_dphy_force_pll(struct mipi_dsi_dev *dev, int force);
int mipi_dsih_dphy_wakeup_pll(struct mipi_dsi_dev *dev);
void mipi_dsih_dphy_stop_wait_time(struct mipi_dsi_dev *dev,
				   uint8_t no_of_byte_cycles);
void mipi_dsih_dphy_no_of_lanes(struct mipi_dsi_dev *dev, uint8_t no_of_lanes);
uint8_t mipi_dsih_dphy_get_no_of_lanes(struct mipi_dsi_dev *dev);
void mipi_dsih_dphy_enable_hs_clk(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_dphy_escape_mode_trigger(struct mipi_dsi_dev *dev,
				       uint8_t trigger_request);
int mipi_dsih_dphy_ulps_data_lanes(struct mipi_dsi_dev *dev, int enable);
int mipi_dsih_dphy_ulps_clk_lane(struct mipi_dsi_dev *dev, int enable);
uint32_t mipi_dsih_dphy_status(struct mipi_dsi_dev *dev, uint16_t mask);

/* end of obligatory functions*/
void mipi_dsih_dphy_test_clock(struct mipi_dsi_dev *dev, int value);
void mipi_dsih_dphy_test_clear(struct mipi_dsi_dev *dev, int value);
void mipi_dsih_dphy_test_en(struct mipi_dsi_dev *dev, uint8_t on_falling_edge);
uint8_t mipi_dsih_dphy_test_data_out(struct mipi_dsi_dev *dev);
void mipi_dsih_dphy_test_data_in(struct mipi_dsi_dev *dev, uint8_t test_data);
void mipi_dsih_dphy_write(struct mipi_dsi_dev *dev, uint8_t address,
			  uint8_t * data, uint8_t data_length);
void mipi_dsih_dphy_write_word(struct mipi_dsi_dev *dev,
			       uint32_t reg_address, uint32_t data);
void mipi_dsih_dphy_write_part(struct mipi_dsi_dev *dev,
			       uint32_t reg_address, uint32_t data,
			       uint8_t shift, uint8_t width);
uint32_t mipi_dsih_dphy_read_word(struct mipi_dsi_dev *dev,
				  uint32_t reg_address);
uint32_t mipi_dsih_dphy_read_part(struct mipi_dsi_dev *dev,
				  uint32_t reg_address, uint8_t shift,
				  uint8_t width);
uint8_t dsi_get_transition_times(struct mipi_dsi_dev *dev);

/*Testchip*/
void mipi_dsih_dphy_set_base_dir_tx(struct mipi_dsi_dev *dev);

#endif	/* MIPI_DSIH_DPHY_H_ */
