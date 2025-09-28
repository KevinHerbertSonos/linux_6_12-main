/**
 * @file mipi_dsih_api.c
 * @brief DWC MIPI DSI Host driver
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#include <video/mipi_display.h>
#include <dt-bindings/dspg/mipi_dsih.h>

#include "mipi_dsih_api.h"
#include "mipi_dsih_hal.h"
#include "mipi_dsih_dphy.h"
/* whether to get debug messages (1) or not (0) */

#define PRECISION_FACTOR	1000
#define VIDEO_PACKET_OVERHEAD	6
#define NULL_PACKET_OVERHEAD	6
#define SHORT_PACKET		4
#define BLANKING_PACKET         6

/*#define MIPI_DSIH_CMD_DBG*/

/**
 * @short Start DW MIPI DSI controller
 * @param dev pointer to structure holding the DSI Host core information
 * @return error code
 * @note this function must be called before any other function in this API
 */
int mipi_dsih_open(struct mipi_dsi_dev *dev)
{
	pr_info("%s:DSI Open\n", FUNC_NAME);

	if (dev == NULL) {
		pr_err("%s:Device is null\n", FUNC_NAME);
		return -ENODEV;
	} else if (!mipi_dsih_dphy_open(dev)) {
		dev_err(dev->parent_dev, "%s:Error in DPHY Opening\n",
			FUNC_NAME);
		return ENXIO;
	} else {
		dev->hw_version = mipi_dsih_hal_get_version(dev);

		switch (dev->hw_version) {
		case DSI_VERSION_130:
			pr_info("HW Version 1.30a\n");
			break;
		case DSI_VERSION_131:
			pr_info("HW Version 1.31a\n");
			break;
		default:
			dev_err(dev->parent_dev,
				"Core Version not supported!!!\n Somethings might not work\n");
			break;
		}
	}

	dev->max_lanes = 4;	/* DWC MIPI D-PHY Bidir TSMC40LP has only 2 lanes */
	dev->max_bta_cycles = 4095;
	dev->color_mode_polarity = 1;
	dev->shut_down_polarity = 1;

	mipi_dsih_hal_power(dev, 0);
	mipi_dsih_hal_dpi_color_mode_pol(dev, !(dev->color_mode_polarity));
	mipi_dsih_hal_dpi_shut_down_pol(dev, !(dev->shut_down_polarity));
	mipi_dsih_hal_int_mask_0(dev, 0x0);
	mipi_dsih_hal_int_mask_1(dev, 0x0);

	if (!mipi_dsih_phy_bta_time(dev, dev->max_bta_cycles)) {
		dev_err(dev->parent_dev, "no_of_byte_cycles > 8000\n");
		return FALSE;
	}

	/* by default, return to LP during ALL, unless otherwise specified */
	mipi_dsih_hal_dpi_lp_during_hfp(dev, 1);
	mipi_dsih_hal_dpi_lp_during_hbp(dev, 1);
	mipi_dsih_hal_dpi_lp_during_vactive(dev, 1);
	mipi_dsih_hal_dpi_lp_during_vfp(dev, 1);
	mipi_dsih_hal_dpi_lp_during_vbp(dev, 1);
	mipi_dsih_hal_dpi_lp_during_vsync(dev, 1);
	/* by default, all commands are sent in LP */
	mipi_dsih_hal_dcs_wr_tx_type(dev, 0, 1);
	mipi_dsih_hal_dcs_wr_tx_type(dev, 1, 1);
	mipi_dsih_hal_dcs_wr_tx_type(dev, 3, 1);	/* long packet */
	mipi_dsih_hal_dcs_rd_tx_type(dev, 0, 1);
	mipi_dsih_hal_gen_wr_tx_type(dev, 0, 1);
	mipi_dsih_hal_gen_wr_tx_type(dev, 1, 1);
	mipi_dsih_hal_gen_wr_tx_type(dev, 2, 1);
	mipi_dsih_hal_gen_wr_tx_type(dev, 3, 1);	/* long packet */
	mipi_dsih_hal_gen_rd_tx_type(dev, 0, 1);
	mipi_dsih_hal_gen_rd_tx_type(dev, 1, 1);
	mipi_dsih_hal_gen_rd_tx_type(dev, 2, 1);
	/* by default, RX_VC = 0, NO EOTp, EOTn, BTA, ECC rx and CRC rx */
	mipi_dsih_hal_gen_rd_vc(dev, 0);
	mipi_dsih_hal_gen_eotp_rx_en(dev, 0);
	mipi_dsih_hal_gen_eotp_tx_en(dev, 0);
	mipi_dsih_hal_bta_en(dev, 0);
	mipi_dsih_hal_gen_ecc_rx_en(dev, 0);
	mipi_dsih_hal_gen_crc_rx_en(dev, 0);
	mipi_dsih_hal_power(dev, 1);
	/* dividing by 6 is aimed for default byte clock, 500MHz */
	mipi_dsih_hal_tx_escape_division(dev, 3);
	/* initialize pll so escape clocks could be generated at 864MHz, 1 lane */
	/* however the high speed clock will not be requested */

	if (!mipi_dsih_dphy_configure(dev, 1, DEFAULT_BYTE_CLOCK))
		return FALSE;

	return TRUE;
}

/**
 * @short Close DSI Host driver Free up resources and shutdown host controller and PHY
 * @param dev pointer to structure holding the DSI Host core information
 * @return error
 */
int mipi_dsih_close(struct mipi_dsi_dev *dev)
{
	if (dev == NULL) {
		pr_err("%s:Device is null\n", FUNC_NAME);
		return -ENODEV;
	}

	mipi_dsih_hal_int_mask_0(dev, 0x0);
	mipi_dsih_hal_int_mask_1(dev, 0x0);
	mipi_dsih_dphy_close(dev);
	mipi_dsih_hal_power(dev, 0);

	return TRUE;
}

/**
 * @short Enable return to low power mode inside video periods when timing allows
 * @param dev pointer to structure holding the DSI Host core information
 * @param hfp allow to return to lp inside horizontal front porch pixels
 * @param hbp allow to return to lp inside horizontal back porch pixels
 * @param vactive allow to return to lp inside vertical active lines
 * @param vfp allow to return to lp inside vertical front porch lines
 * @param vbp allow to return to lp inside vertical back porch lines
 * @param vsync allow to return to lp inside vertical sync lines
 */
void mipi_dsih_allow_return_to_lp(struct mipi_dsi_dev *dev, int hfp, int hbp,
				  int vactive, int vfp, int vbp, int vsync)
{
	if (dev != NULL) {
		mipi_dsih_hal_dpi_lp_during_hfp(dev, hfp);
		mipi_dsih_hal_dpi_lp_during_hbp(dev, hbp);
		mipi_dsih_hal_dpi_lp_during_vactive(dev, vactive);
		mipi_dsih_hal_dpi_lp_during_vfp(dev, vfp);
		mipi_dsih_hal_dpi_lp_during_vbp(dev, vbp);
		mipi_dsih_hal_dpi_lp_during_vsync(dev, vsync);

		return;
	}
}

/**
 * @short Set DCS command packet transmission to low power
 * @param dev pointer to structure holding the DSI Host core information
 * @param long_write command packets
 * @param short_write command packets with none and one parameters
 * @param short_read command packets with none parameters
 */
void mipi_dsih_dcs_cmd_lp_transmission(struct mipi_dsi_dev *dev, int long_write,
				       int short_write, int short_read)
{
	if (dev != NULL) {
		mipi_dsih_hal_dcs_wr_tx_type(dev, 0, short_write);
		mipi_dsih_hal_dcs_wr_tx_type(dev, 1, short_write);
		mipi_dsih_hal_dcs_wr_tx_type(dev, 3, long_write);	/* long packet */
		mipi_dsih_hal_dcs_rd_tx_type(dev, 0, short_read);
		return;
	}
}

/**
 * @short Set Generic interface packet transmission to low power
 * @param dev pointer to structure holding the DSI Host core information
 * @param long_write command packets
 * @param short_write command packets with none, one and two parameters
 * @param short_read command packets with none, one and two parameters
 */
void mipi_dsih_gen_cmd_lp_transmission(struct mipi_dsi_dev *dev, int long_write,
				       int short_write, int short_read)
{
	if (dev != NULL) {
		mipi_dsih_hal_gen_wr_tx_type(dev, 0, short_write);
		mipi_dsih_hal_gen_wr_tx_type(dev, 1, short_write);
		mipi_dsih_hal_gen_wr_tx_type(dev, 2, short_write);
		mipi_dsih_hal_gen_wr_tx_type(dev, 3, long_write);	/* long packet */
		mipi_dsih_hal_gen_rd_tx_type(dev, 0, short_read);
		mipi_dsih_hal_gen_rd_tx_type(dev, 1, short_read);
		mipi_dsih_hal_gen_rd_tx_type(dev, 2, short_read);
		return;
	}
}

/* packet handling */
/**
 *  Enable all receiving activities (applying a Bus Turn Around).
 *  - Disabling using this function will stop all acknowledges by the
 *  peripherals and no interrupts from low-level protocol error reporting
 *  will be able to rise.
 *  - Enabling any receiving function (command or frame ACKs, ECC,
 *  tear effect ACK or EoTp receiving) will enable this automatically,
 *  but it must be EXPLICITLY be disabled to disabled all kinds of
 *  receiving functionality.
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable or disable
 * @return error code
 */
int mipi_dsih_enable_rx(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_hal_bta_en(dev, enable);
	return TRUE;
}

/**
 * @short Enable command packet acknowledges by the peripherals
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable or disable
 * @return error code
 */
int mipi_dsih_peripheral_ack(struct mipi_dsi_dev *dev, int enable)
{
	if (dev != NULL) {
		mipi_dsih_hal_cmd_ack_en(dev, enable);

		if (enable)
			mipi_dsih_hal_bta_en(dev, 1);

		return TRUE;
	}
	return -ENODEV;
}

/**
 * @short Enable tearing effect acknowledges by the peripherals (wait for TE)
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable or disable
 * @return error code
 */
int mipi_dsih_tear_effect_ack(struct mipi_dsi_dev *dev, int enable)
{
	if (dev != NULL) {

		mipi_dsih_hal_tear_effect_ack_en(dev, enable);

		if (enable)
			mipi_dsih_hal_bta_en(dev, 1);

		return TRUE;
	}

	return -ENODEV;
}

/**
 * @short Enable the receiving of EoT packets at the end of LS transmission.
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable or disable
 * @return error code
 */
int mipi_dsih_eotp_rx(struct mipi_dsi_dev *dev, int enable)
{
	if (dev != NULL) {
		mipi_dsih_hal_gen_eotp_rx_en(dev, enable);
		if (enable)
			mipi_dsih_hal_bta_en(dev, 1);

		return TRUE;
	}
	return -ENODEV;
}

/**
 * @short Enable the listening to ECC bytes. This allows for recovering from
 * 1 bit errors. To report ECC events, the ECC events should be registered
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable or disable
 * @return error code
 */
int mipi_dsih_ecc_rx(struct mipi_dsi_dev *dev, int enable)
{
	if (dev != NULL) {

		mipi_dsih_hal_gen_ecc_rx_en(dev, enable);
		if (enable)
			mipi_dsih_hal_bta_en(dev, 1);

		return TRUE;
	}
	return -ENODEV;
}

/**
 * @short Enable the sending of EoT (End of Transmission) packets at the end of HS
 * transmission. It was made optional in the DSI spec. for retro-compatibility.
 * @param dev pointer to structure holding the DSI Host core information
 * @param enable or disable
 * @return error code
 */
int mipi_dsih_eotp_tx(struct mipi_dsi_dev *dev, int enable)
{
	if (dev != NULL) {
		mipi_dsih_hal_gen_eotp_tx_en(dev, enable);
		return TRUE;
	}
	return -ENODEV;
}

/**
 * @short Configure DPI video interface
 * @param dev pointer to structure holding the DSI Host core information
 * @param video_params pointer to video stream-to-send information
 * @return error code
 */
int mipi_dsih_dpi_video(struct mipi_dsi_dev *dev)
{
	int error = 0;
	uint16_t bytes_per_pixel_x100 = 0;	/* bpp x 100 because it can be 2.25 */
	uint16_t video_size = 0;
	uint32_t ratio_clock_xPF = 0;	/* holds dpi clock/byte clock times precision factor */
	uint16_t null_packet_size = 0;
	uint8_t video_size_step = 1;
	uint32_t hs_timeout = 0;
	uint32_t total_bytes = 0;
	uint32_t bytes_per_chunk = 0;
	uint32_t no_of_chunks = 0;
	uint32_t bytes_left = 0;
	uint32_t chunk_overhead = 0;
	int counter = 0;
	dsih_dpi_video_t *video_params;

	/* check DSI controller dev */
	if (dev == NULL)
		return -ENODEV;

	video_params = &dev->dpi_video;

	if (video_params->no_of_lanes > dev->max_lanes) {
		dev_err(dev->parent_dev, "no of lanes %d > max lanes %d\n",
			video_params->no_of_lanes, dev->max_lanes);
		return FALSE;
	}

	/* set up phy pll to required lane clock */
	mipi_dsih_phy_hs2lp_config(dev, video_params->max_hs_to_lp_cycles);
	mipi_dsih_phy_lp2hs_config(dev, video_params->max_lp_to_hs_cycles);
	mipi_dsih_phy_clk_hs2lp_config(dev,
				       video_params->max_clk_hs_to_lp_cycles);
	mipi_dsih_phy_clk_lp2hs_config(dev,
				       video_params->max_clk_lp_to_hs_cycles);
	mipi_dsih_non_continuous_clock(dev, video_params->non_continuous_clock);
	mipi_dsih_dphy_configure(dev, video_params->no_of_lanes,
				 video_params->byte_clock * 8);

	ratio_clock_xPF =
	    (video_params->byte_clock * PRECISION_FACTOR) /
	    (video_params->pixel_clock);
	video_size = video_params->h_active_pixels;
	/* set up ACKs and error reporting */
	mipi_dsih_hal_dpi_frame_ack_en(dev, video_params->receive_ack_packets);
	if (video_params->receive_ack_packets) {	/* if ACK is requested, enable BTA, otherwise leave as is */
		mipi_dsih_hal_bta_en(dev, 1);
	}
	/* mipi_dsih_hal_gen_cmd_mode_en(dev, 0); */
	mipi_dsih_hal_dpi_video_mode_en(dev, 1);
	/* get bytes per pixel and video size step (depending if loosely or not */
	switch (video_params->color_coding) {
	case COLOR_CODE_16BIT_CONFIG1:
	case COLOR_CODE_16BIT_CONFIG2:
	case COLOR_CODE_16BIT_CONFIG3:
		bytes_per_pixel_x100 = 200;
		video_size_step = 1;
		break;
	case COLOR_CODE_18BIT_CONFIG1:
	case COLOR_CODE_18BIT_CONFIG2:
		mipi_dsih_hal_dpi_18_loosely_packet_en(dev,
						       video_params->
						       is_18_loosely);
		bytes_per_pixel_x100 = 225;
		if (!video_params->is_18_loosely) {	/* 18bits per pixel and NOT loosely, packets should be multiples of 4 */
			video_size_step = 4;
			/* round up active H pixels to a multiple of 4 */
			for (; (video_size % 4) != 0; video_size++) {
				;
			}
		} else {
			video_size_step = 1;
		}
		break;
	case COLOR_CODE_24BIT:
		bytes_per_pixel_x100 = 300;
		video_size_step = 1;
		break;
	case COLOR_CODE_20BIT_YCC422_LOOSELY:
		bytes_per_pixel_x100 = 250;
		video_size_step = 2;
		/* round up active H pixels to a multiple of 2 */
		if ((video_size % 2) != 0) {
			video_size += 1;
		}
		break;
	case COLOR_CODE_24BIT_YCC422:
		bytes_per_pixel_x100 = 300;
		video_size_step = 2;
		/* round up active H pixels to a multiple of 2 */
		if ((video_size % 2) != 0) {
			video_size += 1;
		}
		break;
	case COLOR_CODE_16BIT_YCC422:
		bytes_per_pixel_x100 = 200;
		video_size_step = 2;
		/* round up active H pixels to a multiple of 2 */
		if ((video_size % 2) != 0) {
			video_size += 1;
		}
		break;
	case COLOR_CODE_30BIT:
		bytes_per_pixel_x100 = 375;
		video_size_step = 2;
		break;
	case COLOR_CODE_36BIT:
		bytes_per_pixel_x100 = 450;
		video_size_step = 2;
		break;
	case COLOR_CODE_12BIT_YCC420:
		bytes_per_pixel_x100 = 150;
		video_size_step = 2;
		/* round up active H pixels to a multiple of 2 */
		if ((video_size % 2) != 0) {
			video_size += 1;
		}
		break;
	default:
		dev_err(dev->parent_dev, "invalid color coding");
		return FALSE;
	}

	mipi_dsih_hal_dpi_color_coding(dev, video_params->color_coding);

	mipi_dsih_hal_dpi_video_mode_type(dev, video_params->video_mode);
	mipi_dsih_hal_dpi_hline(dev,
				(uint16_t) ((video_params->h_total_pixels *
					     ratio_clock_xPF) /
					    PRECISION_FACTOR));
	mipi_dsih_hal_dpi_hbp(dev,
			      ((video_params->h_back_porch_pixels *
				ratio_clock_xPF) / PRECISION_FACTOR));
	mipi_dsih_hal_dpi_hsa(dev,
			      ((video_params->h_sync_pixels * ratio_clock_xPF) /
			       PRECISION_FACTOR));
	mipi_dsih_hal_dpi_vactive(dev, video_params->v_active_lines);
	mipi_dsih_hal_dpi_vfp(dev,
			      video_params->v_total_lines -
			      (video_params->v_back_porch_lines +
			       video_params->v_sync_lines +
			       video_params->v_active_lines));
	mipi_dsih_hal_dpi_vbp(dev, video_params->v_back_porch_lines);
	mipi_dsih_hal_dpi_vsync(dev, video_params->v_sync_lines);
	mipi_dsih_hal_dpi_hsync_pol(dev, !video_params->h_polarity);
	mipi_dsih_hal_dpi_vsync_pol(dev, !video_params->v_polarity);
	mipi_dsih_hal_dpi_dataen_pol(dev, !video_params->data_en_polarity);
	/* HS timeout */
	hs_timeout =
	    ((video_params->h_total_pixels * video_params->v_active_lines) +
	     (DSIH_PIXEL_TOLERANCE * bytes_per_pixel_x100) / 100);
	for (counter = 0x80; (counter < hs_timeout) && (counter > 2); counter--) {
		if ((hs_timeout % counter) == 0) {
			mipi_dsih_hal_timeout_clock_division(dev, counter);
			mipi_dsih_hal_lp_rx_timeout(dev,
						    (uint16_t) (hs_timeout /
								counter));
			mipi_dsih_hal_hs_tx_timeout(dev,
						    (uint16_t) (hs_timeout /
								counter));
			break;
		}
	}
	/* TX_ESC_CLOCK_DIV must be less than 20000KHz */
	mipi_dsih_hal_tx_escape_division(dev, 6);
	/* video packetisation */
	if (video_params->video_mode == VIDEO_BURST_WITH_SYNC_PULSES) {	/* BURST */
		mipi_dsih_hal_dpi_null_packet_size(dev, 0);
		mipi_dsih_hal_dpi_chunks_no(dev, 1);
		mipi_dsih_hal_dpi_video_packet_size(dev, video_size);

		/* BURST by default, returns to LP during ALL empty periods - energy saving */
		mipi_dsih_hal_dpi_lp_during_hfp(dev, 1);
		mipi_dsih_hal_dpi_lp_during_hbp(dev, 1);
		mipi_dsih_hal_dpi_lp_during_vactive(dev, 1);
		mipi_dsih_hal_dpi_lp_during_vfp(dev, 1);
		mipi_dsih_hal_dpi_lp_during_vbp(dev, 1);
		mipi_dsih_hal_dpi_lp_during_vsync(dev, 1);

		pr_debug("burst video\n");
		pr_debug("h line time %d\n",
			 (uint16_t) ((video_params->h_total_pixels *
				      ratio_clock_xPF) / PRECISION_FACTOR));
		pr_debug("video_size %d\n", video_size);

	} else {		/* non burst transmission */
		pr_debug("non burst video\n");
		null_packet_size = 0;
		/* bytes to be sent - first as one chunk */
		bytes_per_chunk =
		    (bytes_per_pixel_x100 * video_params->h_active_pixels) /
		    100 + VIDEO_PACKET_OVERHEAD;
		/* bytes being received through the DPI interface per byte clock cycle */
		total_bytes =
		    (ratio_clock_xPF * video_params->no_of_lanes *
		     (video_params->h_total_pixels -
		      video_params->h_back_porch_pixels -
		      video_params->h_sync_pixels)) / PRECISION_FACTOR;
		/* check if the in pixels actually fit on the DSI link */
		if (total_bytes >= bytes_per_chunk) {
			chunk_overhead = total_bytes - bytes_per_chunk;
			/* overhead higher than 1 -> enable multi packets */
			if (chunk_overhead > 1) {	/* MULTI packets */
				for (video_size = video_size_step;
				     video_size < video_params->h_active_pixels;
				     video_size += video_size_step) {	/* determine no of chunks */
					if ((((video_params->h_active_pixels *
					       PRECISION_FACTOR) / video_size) %
					     PRECISION_FACTOR) == 0) {
						no_of_chunks =
						    video_params->
						    h_active_pixels /
						    video_size;
						bytes_per_chunk =
						    (bytes_per_pixel_x100 *
						     video_size) / 100 +
						    VIDEO_PACKET_OVERHEAD;
						if (total_bytes >=
						    (bytes_per_chunk *
						     no_of_chunks)) {
							bytes_left =
							    total_bytes -
							    (bytes_per_chunk *
							     no_of_chunks);
							break;
						}
					}
				}
				/* prevent overflow (unsigned - unsigned) */
				if (bytes_left >
				    (NULL_PACKET_OVERHEAD * no_of_chunks)) {
					null_packet_size =
					    (bytes_left -
					     (NULL_PACKET_OVERHEAD *
					      no_of_chunks)) / no_of_chunks;
					if (null_packet_size > MAX_NULL_SIZE) {	/* avoid register overflow */
						null_packet_size =
						    MAX_NULL_SIZE;
					}
				}
			} else {	/* no multi packets */
				no_of_chunks = 1;

				pr_debug("no multi no null video");
				pr_debug("h line time %d",
					 (uint16_t) ((video_params->
						      h_total_pixels *
						      ratio_clock_xPF) /
						     PRECISION_FACTOR));
				pr_debug("video_size %d", video_size);

				/* video size must be a multiple of 4 when not 18 loosely */
				for (video_size = video_params->h_active_pixels;
				     (video_size % video_size_step) != 0;
				     video_size++) {
					;
				}
			}
		} else {
			dev_err(dev->parent_dev,
				"resolution cannot be sent to display through current settings");
			error = FALSE;
		}
	}
	mipi_dsih_hal_dpi_chunks_no(dev, no_of_chunks);
	mipi_dsih_hal_dpi_video_packet_size(dev, video_size);
	mipi_dsih_hal_dpi_null_packet_size(dev, null_packet_size);

	mipi_dsih_hal_dpi_video_vc(dev, video_params->virtual_channel);
	mipi_dsih_dphy_no_of_lanes(dev, video_params->no_of_lanes);
	/* enable high speed clock */
	mipi_dsih_dphy_enable_hs_clk(dev, 1);

	return error;
}

/**
 * Send a DCS write command
 * It sends the User Command Set commands listed in the DCS specification and
 * not the Manufacturer Command Set. To send the Manufacturer Commands use the
 * packet on the generic packets sending function
 * function sets the packet data type automatically
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc destination virtual channel
 * @param params byte-addressed array of command parameters, including the
 * command itself
 * @param param_length length of the above array
 * @return error code
 * @note this function has an active delay to wait for the buffer to clear.
 * The delay is limited to DSIH_FIFO_ACTIVE_WAIT x register access time
 */
int mipi_dsih_dcs_wr_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
			 uint8_t * params, uint16_t param_length)
{
	uint8_t packet_type = 0;
	int i = 0;

	if (param_length > 2) {
		i = 2;
	}
	switch (params[i]) {
	case 0x39:
	case 0x38:
	case 0x34:
	case 0x29:
	case 0x28:
	case 0x21:
	case 0x20:
	case 0x13:
	case 0x12:
	case 0x11:
	case 0x10:
	case 0x01:
	case 0x00:
		packet_type = 0x05;	/* DCS short write no param */
		break;
	case 0x3A:
	case 0x36:
	case 0x35:
	case 0x26:
		packet_type = 0x15;	/* DCS short write 1 param */
		break;
	case 0x44:
	case 0x3C:
	case 0x37:
	case 0x33:
	case 0x30:
	case 0x2D:
	case 0x2C:
	case 0x2B:
	case 0x2A:
		packet_type = 0x39;	/* DCS long write/write_LUT command packet */
		break;
	default:
		dev_err(dev->parent_dev, "invalid DCS command");
		return FALSE;
	}
	return mipi_dsih_gen_wr_packet(dev, vc, packet_type, params,
				       param_length);
}

/**
 * Enable command mode
 * - This function shall be explicitly called before commands are send if they
 * are to be sent in command mode and not interlaced with video
 * - If video mode is ON, it will be turned off automatically
 * @param dev pointer to structure holding the DSI Host core information
 * @param en enable/disable
 */
void mipi_dsih_cmd_mode(struct mipi_dsi_dev *dev, int en)
{
	if (dev != NULL)
		mipi_dsih_hal_gen_cmd_mode_en(dev, en);
	else
		pr_err("%s:Device is null\n", FUNC_NAME);

}

/**
 * Enable video mode
 * - If command mode is ON, it will be turned off automatically
 * @param dev pointer to structure holding the DSI Host core information
 * @param en enable/disable
 */
void mipi_dsih_video_mode(struct mipi_dsi_dev *dev, int en)
{
	if (dev != NULL)
		mipi_dsih_hal_dpi_video_mode_en(dev, en);
	else
		pr_err("%s:Device is null\n", FUNC_NAME);
}

/**
 * Get the current active mode
 * - 1 command mode
 * - 2 DPI video mode
 */
int mipi_dsih_active_mode(struct mipi_dsi_dev *dev)
{
	if (dev == NULL)
		return -ENODEV;

	if (mipi_dsih_hal_gen_is_cmd_mode(dev)) {
		return 1;
	} else if (mipi_dsih_hal_dpi_is_video_mode(dev)) {
		return 2;
	}
	return TRUE;
}

/**
 * Send a generic write command
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc destination virtual channel
 * @param params byte-addressed array of command parameters
 * @param param_length length of the above array
 * @return error code
 * @note this function has an active delay to wait for the buffer to clear.
 * The delay is limited to DSIH_FIFO_ACTIVE_WAIT x register access time
 */
int mipi_dsih_gen_wr_cmd(struct mipi_dsi_dev *dev, uint8_t vc,
			 uint8_t * params, uint16_t param_length)
{
	uint8_t data_type = 0;

	if (dev == NULL)
		return -ENODEV;

	switch (param_length) {
	case 0:
		data_type = 0x03;
		break;
	case 1:
		data_type = 0x13;
		break;
	case 2:
		data_type = 0x23;
		break;
	default:
		data_type = 0x29;
		break;
	}
	return mipi_dsih_gen_wr_packet(dev, vc, data_type, params,
				       param_length);
}

/**
 * Send a packet on the generic interface
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc destination virtual channel
 * @param data_type type of command, inserted in first byte of header
 * @param params byte array of command parameters
 * @param param_length length of the above array
 * @return error code
 * @note this function has an active delay to wait for the buffer to clear.
 * The delay is limited to:
 * (param_length / 4) x DSIH_FIFO_ACTIVE_WAIT x register access time
 * @note the controller restricts the sending of .
 * This function will not be able to send Null and Blanking packets due to
 *  controller restriction
 */
int mipi_dsih_gen_wr_packet(struct mipi_dsi_dev *dev, uint8_t vc,
			    uint8_t data_type, uint8_t *params,
			    uint16_t param_length)
{
	/* active delay iterator */
	int timeout = 0;
	/* iterators */
	int i = 0;
	int j = 0;
	/* holds padding bytes needed */
	int compliment_counter = 0;
	uint8_t *payload = 0;
	/* temporary variable to arrange bytes into words */
	uint32_t temp = 0;
	uint16_t word_count = 0;

	if (dev == NULL) {
		pr_err("Null device\n");
		return -ENODEV;
	}

	if ((params == 0) && (param_length != 0)) {	/* pointer NULL */
		dev_err(dev->parent_dev, "Null params\n");
		return FALSE;
	}
	if (param_length > 200) {
		dev_err(dev->parent_dev, "param length too large");
		return FALSE;
	}
	if (param_length > 2) {	/* long packet - write word count to header, and the rest to payload */
		payload = params + (2 * sizeof(params[0]));
		word_count = (params[1] << 8) | params[0];
		if (word_count > 200) {
			dev_err(dev->parent_dev, "word count too large: %u",
				word_count);
			return FALSE;
		}
		if ((param_length - 2) < word_count) {
			dev_err(dev->parent_dev,
				"sent (%u) > input payload (%u). complemented with zeroes",
				word_count, param_length - 2);
			compliment_counter = (param_length - 2) - word_count;
		} else if ((param_length - 2) > word_count) {
			dev_err(dev->parent_dev,
				"Overflow - input > sent. payload truncated");
		}
		for (i = 0; i < (param_length - 2); i += j) {
			temp = 0;
			for (j = 0; (j < 4) && ((j + i) < (param_length - 2));
			     j++) {	/* temp = (payload[i + 3] << 24) | (payload[i + 2] << 16) | (payload[i + 1] << 8) | payload[i]; */
				temp |= payload[i + j] << (j * 8);
			}
			/* check if payload Tx fifo is not full */
			for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT;
			     timeout++) {
				if (mipi_dsih_hal_gen_packet_payload(dev, temp))
					break;
//                              mdelay(500);
			}
			if (!(timeout < DSIH_FIFO_ACTIVE_WAIT)) {
				dev->timeout++;
				pr_err("timeout! %u\n", dev->timeout);
				return FALSE;
			}
		}
		/* if word count entered by the user more than actual parameters received
		 * fill with zeroes - a fail safe mechanism, otherwise controller will
		 * want to send data from an empty buffer */
		for (i = 0; i < compliment_counter; i++) {
			/* check if payload Tx fifo is not full */
			for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT;
			     timeout++) {
				if (!mipi_dsih_hal_gen_packet_payload
				    (dev, 0x00))
					break;
			}
			if (!(timeout < DSIH_FIFO_ACTIVE_WAIT)) {
				pr_err("timeout!!\n");
				return FALSE;
			}
		}
	}
	for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT; timeout++) {
		/* check if payload Tx fifo is not full */
		if (!mipi_dsih_hal_gen_cmd_fifo_full(dev)) {
			if (param_length == 0) {
				return mipi_dsih_hal_gen_packet_header(dev, vc,
								data_type,
								0x0,
								0x0);

			} else if (param_length == 1) {
				return mipi_dsih_hal_gen_packet_header(dev, vc,
								data_type,
								0x0,
								params[0]);
			} else {
				return mipi_dsih_hal_gen_packet_header(dev, vc,
								data_type,
								params[1],
								params[0]);
			}
		}
		mdelay(10);
	}

	dev_err(dev->parent_dev, "command fifo full!\n");
	return FALSE;
}

struct dsi_cmd {
	uint8_t dcs_cmd;
	unsigned num_params;
	uint8_t *params;
};

enum init_state {
	INIT_CMD,
	INIT_NUM_PARAMS,
	INIT_PARAMS,
	INIT_MDELAY,
	INIT_UDELAY,
	INIT_RST,
};

static int
mipi_dsih_send(struct mipi_dsi_dev *dev, struct dsi_cmd *cmd)
{
	int ret;
#ifdef MIPI_DSIH_CMD_DBG
	int i;

	if (!cmd->num_params)
		printk("DCS command 0x%2.2X; #params: %3.3u, params: [",
		       MIPI_DSI_DCS_SHORT_WRITE, cmd->num_params);
	else if (cmd->num_params == 1)
		printk("DCS command 0x%2.2X; #params: %3.3u, params: [",
		       MIPI_DSI_DCS_SHORT_WRITE_PARAM, cmd->num_params);
	else
		printk("DCS command 0x%2.2X; #params: %3.3u, params: [",
		       MIPI_DSI_DCS_LONG_WRITE, cmd->num_params);

	for (i = 0; i < cmd->num_params; i++)
		printk(" %2.2X", cmd->params[i]);
	printk(" ]\n");
#endif
	if (cmd->num_params == 1) {
		ret = mipi_dsih_gen_wr_packet(dev, 0, MIPI_DSI_DCS_SHORT_WRITE,
					      cmd->params, cmd->num_params);
	} else if (cmd->num_params == 2) {
		ret = mipi_dsih_gen_wr_packet(dev, 0,
					      MIPI_DSI_DCS_SHORT_WRITE_PARAM,
					      cmd->params, cmd->num_params);
	} else {
		int cur;

		for (cur = cmd->num_params + 1; cur >= 2; cur--)
			cmd->params[cur] = cmd->params[cur - 2];
		cmd->params[0] = (u8)cmd->num_params;
		cmd->params[1] = (u16)cmd->num_params >> 8;

		ret = mipi_dsih_gen_wr_packet(dev, 0, MIPI_DSI_DCS_LONG_WRITE,
					      cmd->params,
					      cmd->num_params + 2);
	}

	return ret;
}

int
mipi_dsih_parse_display_sequence(struct mipi_dsi_dev *dev,
				 const char *sequence)
{
	struct device_node *pnode =
		of_parse_phandle(dev->parent_dev->of_node, "init-sequence", 0);
	struct property *prop;
	u32 key;
	const __be32 *p;
	enum init_state state = INIT_CMD;
	struct dsi_cmd cmd;
	unsigned cur_param = 0;
	int ret;

	if (!pnode)
		return -ENXIO;

	cmd.params = dev->cmd_buf;

	of_property_for_each_u32(pnode, sequence, prop, p, key) {
		switch (state) {
		case INIT_CMD:
			switch (key) {
			case DSIH_INIT_MDELAY:
				state = INIT_MDELAY;
				break;
			case DSIH_INIT_UDELAY:
				state = INIT_UDELAY;
				break;
			case DSIH_INIT_RST:
				state = INIT_RST;
				break;
			default:
				cur_param = 0;
				cmd.params[cur_param++] = key;
				state = INIT_NUM_PARAMS;
				break;
			}
			break;
		case INIT_NUM_PARAMS:
			if (key > MIPI_DSIH_MAX_PARAMS)
				return -EINVAL;
			cmd.num_params = key + 1;
			if (cmd.num_params > 1) {
				state = INIT_PARAMS;
			} else {
				ret = mipi_dsih_send(dev, &cmd);
				if (ret != TRUE)
					return -EIO;
				state = INIT_CMD;
			}
			break;
		case INIT_PARAMS:
			if (cur_param >= cmd.num_params)
				return -EIO;
			cmd.params[cur_param++] = key;
			if (cur_param == cmd.num_params) {
				ret = mipi_dsih_send(dev, &cmd);
				if (ret != TRUE)
					return -EIO;
				state = INIT_CMD;
			}
			break;
		case INIT_MDELAY:
			mdelay(key);
			state = INIT_CMD;
			break;
		case INIT_UDELAY:
			udelay(key);
			state = INIT_CMD;
			break;
		case INIT_RST:
			if (dev->reset_gpio_panel >= 0)
				gpio_set_value(dev->reset_gpio_panel,
					       !!key);
			state = INIT_CMD;
			break;
		}
	}

	of_node_put(pnode);

	return 0;
}

/**
 * Send a DCS READ command to peripheral
 * function sets the packet data type automatically
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc destination virtual channel
 * @param command DCS code
 * @param bytes_to_read no of bytes to read (expected to arrive at buffer)
 * @param read_buffer pointer to 8-bit array to hold the read buffer words
 * return read_buffer_length
 * @note this function has an active delay to wait for the buffer to clear.
 * The delay is limited to 2 x DSIH_FIFO_ACTIVE_WAIT
 * (waiting for command buffer, and waiting for receiving)
 * @note this function will enable BTA
 */
uint16_t mipi_dsih_dcs_rd_cmd(struct mipi_dsi_dev * dev, uint8_t vc,
			      uint8_t command, uint8_t bytes_to_read,
			      uint8_t * read_buffer)
{
	if (dev == NULL)
		return FALSE;

	switch (command) {
	case 0xA8:
	case 0xA1:
	case 0x45:
	case 0x3E:
	case 0x2E:
	case 0x0F:
	case 0x0E:
	case 0x0D:
	case 0x0C:
	case 0x0B:
	case 0x0A:
	case 0x08:
	case 0x07:
	case 0x06:
		/* COMMAND_TYPE 0x06 - DCS Read no params refer to DSI spec p.47 */
		return mipi_dsih_gen_rd_packet(dev, vc, 0x06, 0x0, command,
					       bytes_to_read, read_buffer);
	default:
		dev_err(dev->parent_dev, "invalid DCS command");
		return TRUE;
	}
	return TRUE;
}

/**
 * Send Generic READ command to peripheral
 * - function sets the packet data type automatically
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc destination virtual channel
 * @param params byte array of command parameters
 * @param param_length length of the above array
 * @param bytes_to_read no of bytes to read (expected to arrive at buffer)
 * @param read_buffer pointer to 8-bit array to hold the read buffer words
 * return read_buffer_length
 * @note this function has an active delay to wait for the buffer to clear.
 * The delay is limited to 2 x DSIH_FIFO_ACTIVE_WAIT
 * (waiting for command buffer, and waiting for receiving)
 * @note this function will enable BTA
 */
uint16_t mipi_dsih_gen_rd_cmd(struct mipi_dsi_dev * dev, uint8_t vc,
			      uint8_t * params, uint16_t param_length,
			      uint8_t bytes_to_read, uint8_t * read_buffer)
{
	uint8_t data_type = 0;

	if (dev == NULL)
		return FALSE;

	switch (param_length) {
	case 0:
		data_type = 0x04;
		return mipi_dsih_gen_rd_packet(dev, vc, data_type, 0x00, 0x00,
					       bytes_to_read, read_buffer);
	case 1:
		data_type = 0x14;
		return mipi_dsih_gen_rd_packet(dev, vc, data_type, 0x00,
					       params[0], bytes_to_read,
					       read_buffer);
	case 2:
		data_type = 0x24;
		return mipi_dsih_gen_rd_packet(dev, vc, data_type, params[1],
					       params[0], bytes_to_read,
					       read_buffer);
	default:
		return TRUE;
	}
}

/**
 * Send READ packet to peripheral using the generic interface
 * This will force command mode and stop video mode (because of BTA)
 * @param dev pointer to structure holding the DSI Host core information
 * @param vc destination virtual channel
 * @param data_type generic command type
 * @param lsb_byte first command parameter
 * @param msb_byte second command parameter
 * @param bytes_to_read no of bytes to read (expected to arrive at buffer)
 * @param read_buffer pointer to 8-bit array to hold the read buffer words
 * return read_buffer_length
 * @note this function has an active delay to wait for the buffer to clear.
 * The delay is limited to 2 x DSIH_FIFO_ACTIVE_WAIT
 * (waiting for command buffer, and waiting for receiving)
 * @note this function will enable BTA
 */
uint16_t mipi_dsih_gen_rd_packet(struct mipi_dsi_dev * dev, uint8_t vc,
				 uint8_t data_type, uint8_t msb_byte,
				 uint8_t lsb_byte, uint8_t bytes_to_read,
				 uint8_t * read_buffer)
{
	int timeout = 0;
	int counter = 0;
	int i = 0;
	int last_count = 0;
	uint32_t temp[1] = { 0 };

	if (dev == NULL)
		return FALSE;

	if (bytes_to_read < 1)
		return FALSE;

	if (read_buffer == NULL)
		return FALSE;

	/* make sure command mode is on */
	mipi_dsih_cmd_mode(dev, 1);
	/* make sure receiving is enabled */
	mipi_dsih_hal_bta_en(dev, 1);
	/* listen to the same virtual channel as the one sent to */
	mipi_dsih_hal_gen_rd_vc(dev, vc);
	for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT; timeout++) {	/* check if payload Tx fifo is not full */
		if (!mipi_dsih_hal_gen_cmd_fifo_full(dev)) {
			mipi_dsih_hal_gen_packet_header(dev, vc, data_type,
							msb_byte, lsb_byte);
			break;
		}
	}
	if (!(timeout < DSIH_FIFO_ACTIVE_WAIT)) {
		dev_err(dev->parent_dev, "tx rd command timed out");
		return FALSE;
	}
	/* loop for the number of words to be read */
	for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT; timeout++) {	/* check if command transaction is done */
		if (!mipi_dsih_hal_gen_rd_cmd_busy(dev)) {
			unsigned long timeout = jiffies +
				msecs_to_jiffies(DSIH_READ_DATA_WAIT_MS);

			while (mipi_dsih_hal_gen_read_fifo_empty(dev)) {
				if (time_after(jiffies, timeout)) {
					pr_info("rx buffer empty\n");
					return TRUE;
				}
				mdelay(10);
			}
			for (counter = 0;
			     (!mipi_dsih_hal_gen_read_fifo_empty(dev));
			     counter += 4) {
				mipi_dsih_hal_gen_read_payload(dev,
							       temp);
				if (counter < bytes_to_read) {
					for (i = 0; i < 4; i++) {
						if ((counter + i) <
						    bytes_to_read) {
							/* put 32 bit temp in 4 bytes of buffer passed by user */
							read_buffer
							    [counter +
							     i] =
							    (uint8_t)
							    (temp[0] >>
							     (i * 8));
							last_count =
							    i + counter;
						} else {
							if ((uint8_t)
							    (temp[0] >>
							     (i * 8)) !=
							    0x00) {
								last_count
								    =
								    i +
								    counter;
							}
						}
					}
				} else {
					last_count = counter;
					for (i = 0; i < 4; i++) {
						if ((uint8_t)
						    (temp[0] >> (i * 8))
						    != 0x00) {
							last_count =
							    i + counter;
						}
					}
				}
			}
			return (last_count >= (uint16_t)0xffff) ?
			       0xffff : (uint16_t)(last_count + 1);
		}
	}
	dev_err(dev->parent_dev, "rx command timed out");
	return FALSE;
}

/**
 * Dump values stored in the DSI host core registers
 * @param dev pointer to structure holding the DSI Host core information
 * @param all whether to dump all the registers, no register_config array need
 * be provided if dump is to standard IO
 * @param config array of register_config_t type where addresses and values are
 * stored
 * @param config_length the length of the config array
 * @return the number of the registers that were read
 */
uint32_t mipi_dsih_dump_register_configuration(struct mipi_dsi_dev * dev,
					       int all,
					       register_config_t * config,
					       uint16_t config_length)
{
	uint32_t _current = 0;
	uint16_t count = 0;

	if (dev == NULL)
		return -ENODEV;

	if (all) {		/* dump all registers */
		for (_current = R_DSI_HOST_VERSION;
		     _current <= R_DSI_HOST_PHY_TMR_LPCLK_CFG;
		     count++, _current +=
		     (R_DSI_HOST_PWR_UP - R_DSI_HOST_VERSION)) {
			if ((config_length == 0) || (config == 0) ||
			    count >= config_length) {	/* no place to write - write to STD IO */
				pr_info("DSI 0x%X:0x%X", _current,
					mipi_dsih_read_word(dev, _current));
			} else {
				config[count].addr = _current;
				config[count].data =
				    mipi_dsih_read_word(dev, _current);
			}
		}
	} else {
		if (config == 0) {
			dev_err(dev->parent_dev, "invalid buffer");
			return FALSE;
		} else {
			for (count = 0; count < config_length; count++) {
				config[count].data =
				    mipi_dsih_read_word(dev,
							config[count].addr);
			}
		}
	}
	return count;
}

/**
 * Write values to DSI host core registers
 * @param dev pointer to structure holding the DSI Host core information
 * @param config array of register_config_t type where register addresses and
 * their new values are stored
 * @param config_length the length of the config array
 * @return the number of the registers that were written to
 */
uint32_t mipi_dsih_write_register_configuration(struct mipi_dsi_dev * dev,
						register_config_t * config,
						uint16_t config_length)
{
	uint16_t count = 0;

	if (dev == NULL)
		return -ENODEV;

	for (count = 0; count < config_length; count++)
		mipi_dsih_write_word(dev, config[count].addr,
				     config[count].data);

	return count;
}

/**
 * Reset the DSI Host controller
 * - Sends a pulse to the shut down register
 * @param dev pointer to structure holding the DSI Host core information
 */
void mipi_dsih_reset_controller(struct mipi_dsi_dev *dev)
{
	mipi_dsih_hal_power(dev, 0);
	msleep(1);
	mipi_dsih_hal_power(dev, 1);
}

/**
 * Shut down the DSI Host controller
 * @param dev pointer to structure holding the DSI Host core information
 * @param shutdown (1) power up (0)
 */
void mipi_dsih_shutdown_controller(struct mipi_dsi_dev *dev, int shutdown)
{
	mipi_dsih_hal_power(dev, !shutdown);
}

/**
 * Reset the PHY module being controlled by the DSI Host controller
 * - Sends a pulse to the PPI reset signal
 * @param dev pointer to structure holding the DSI Host core information
 */
void mipi_dsih_reset_phy(struct mipi_dsi_dev *dev)
{
	mipi_dsih_dphy_reset(dev, 0);
	mipi_dsih_dphy_reset(dev, 1);
}

/**
 * Shut down the PHY module being controlled by the DSI Host controller
 * @param dev pointer to structure holding the DSI Host core information
 * @param shutdown (1) power up (0)
 */
void mipi_dsih_shutdown_phy(struct mipi_dsi_dev *dev, int shutdown)
{
	mipi_dsih_dphy_shutdown(dev, !shutdown);
}

/**
 * Configure the eDPI interface
 * - Programs the controller to receive pixels on the DPI interface and send them
 * as commands (write memory start and write memory continue) instead of a
 * video stream.
 * @param dev pointer to structure holding the DSI Host core information
 * @param video_params pointer to the command mode video parameters context
 * @param send_setup_packets whether to send the setup packets from given info. These are: 1 - set pixel format; 2 - set column address; 3 - set page address
 * @return error code
 */
int mipi_dsih_edpi_video(struct mipi_dsi_dev *dev)
{
	uint8_t buf[7] = { 0 };
	uint32_t bytes_per_pixel_x100 = 0;
	dsih_cmd_mode_video_t *video_params;

	video_params = &dev->cmd_mode_video;

	mipi_dsih_hal_dpi_video_vc(dev, video_params->virtual_channel);
	mipi_dsih_cmd_mode(dev, 1);
	mipi_dsih_hal_dpi_color_coding(dev, video_params->color_coding);
	/* define whether write memory commands will be sent in LP or HS */
	mipi_dsih_hal_dcs_wr_tx_type(dev, 3, video_params->lp);	/* long packet */
	if (video_params->send_setup_packets) {
		/* define pixel packing format - 1 param */
		buf[0] = 0x3A;
		/* colour depth: table 6 DCS spec. 3:1| 8:2| 12:3| 16:5| 18:6| 24:7 */
		switch (video_params->color_coding) {
		case 0:
		case 1:
		case 2:
			buf[1] = 5;
			break;
		case 3:
		case 4:
			buf[1] = 6;
			break;
		default:
			buf[1] = 7;
			break;
		}
		mipi_dsih_dcs_wr_cmd(dev, video_params->virtual_channel, buf,
				     2);
		/* set column address (left to right) - 4 param */
		buf[0] = 0x05;	/* cmd length */
		buf[1] = 0x00;
		buf[2] = 0x2A;	/* cmd opcode */
		buf[3] = (uint8_t) (video_params->h_start >> 8);	/* payload start */
		buf[4] = (uint8_t) (video_params->h_start);
		buf[5] = (uint8_t) (video_params->h_active_pixels >> 8);
		buf[6] = (uint8_t) (video_params->h_active_pixels);
		mipi_dsih_dcs_wr_cmd(dev, video_params->virtual_channel, buf,
				     7);
		/* set page address (top to bottom) 4 - param */
		buf[0] = 0x05;	/* cmd length */
		buf[1] = 0x00;
		buf[2] = 0x2B;	/* cmd opcode */
		buf[3] = (uint8_t) (video_params->v_start >> 8);	/* payload start */
		buf[4] = (uint8_t) (video_params->v_start);
		buf[5] = (uint8_t) (video_params->v_active_lines >> 8);
		buf[6] = (uint8_t) (video_params->v_active_lines);
		mipi_dsih_dcs_wr_cmd(dev, video_params->virtual_channel, buf,
				     7);
	}
	switch (video_params->color_coding) {
	case COLOR_CODE_16BIT_CONFIG1:
	case COLOR_CODE_16BIT_CONFIG2:
	case COLOR_CODE_16BIT_CONFIG3:
		bytes_per_pixel_x100 = 200;
		break;
	case COLOR_CODE_18BIT_CONFIG1:
	case COLOR_CODE_18BIT_CONFIG2:
		bytes_per_pixel_x100 = 225;
		break;
	case COLOR_CODE_24BIT:
		bytes_per_pixel_x100 = 300;
		break;
	case COLOR_CODE_20BIT_YCC422_LOOSELY:
		bytes_per_pixel_x100 = 250;
		break;
	case COLOR_CODE_24BIT_YCC422:
		bytes_per_pixel_x100 = 300;
		break;
	case COLOR_CODE_16BIT_YCC422:
		bytes_per_pixel_x100 = 200;
		break;
	case COLOR_CODE_30BIT:
		bytes_per_pixel_x100 = 375;
		break;
	case COLOR_CODE_36BIT:
		bytes_per_pixel_x100 = 450;
		break;
	case COLOR_CODE_12BIT_YCC420:
		bytes_per_pixel_x100 = 150;
		break;
	default:
		dev_err(dev->parent_dev, "invalid color coding");
		return FALSE;
		break;
	}
	if (video_params->te) {
		mipi_dsih_hal_bta_en(dev, video_params->te);
		/* enable tearing effect */
		mipi_dsih_tear_effect_ack(dev, video_params->te);
		buf[0] = 0x35;
		mipi_dsih_dcs_wr_cmd(dev, video_params->virtual_channel, buf,
				     1);
	}
	mipi_dsih_dphy_enable_hs_clk(dev, 1);
	if ((((WORD_LENGTH * FIFO_DEPTH) * 100) / bytes_per_pixel_x100) >
	    video_params->h_active_pixels) {
		mipi_dsih_hal_edpi_max_allowed_size(dev,
						    video_params->
						    h_active_pixels);
	} else {
		mipi_dsih_hal_edpi_max_allowed_size(dev,
						    (((WORD_LENGTH *
						       FIFO_DEPTH) * 100) /
						     bytes_per_pixel_x100));
	}

	return TRUE;
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
void mipi_dsih_presp_timeout_low_power_write(struct mipi_dsi_dev *dev,
					     uint16_t no_of_byte_cycles)
{
	mipi_dsih_hal_presp_timeout_low_power_write(dev, no_of_byte_cycles);
}

/**
 * Timeout for peripheral (for controller to stay still) after LP data
 * transmission read requests
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a low power read operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_presp_timeout_low_power_read(struct mipi_dsi_dev *dev,
					    uint16_t no_of_byte_cycles)
{
	mipi_dsih_hal_presp_timeout_low_power_read(dev, no_of_byte_cycles);
}

/**
 * Timeout for peripheral (for controller to stay still) after HS data
 * transmission write requests
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a high-speed write operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_presp_timeout_high_speed_write(struct mipi_dsi_dev *dev,
					      uint16_t no_of_byte_cycles)
{
	mipi_dsih_hal_presp_timeout_high_speed_write(dev, no_of_byte_cycles);
}

/**
 * Timeout for peripheral between HS data transmission read requests
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a high-speed read operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_presp_timeout_high_speed_read(struct mipi_dsi_dev *dev,
					     uint16_t no_of_byte_cycles)
{
	mipi_dsih_hal_presp_timeout_high_speed_read(dev, no_of_byte_cycles);
}

/**
 * Timeout for peripheral (for controller to stay still) after bus turn around
 * @param dev pointer to structure holding the DSI Host core information
 * @param no_of_byte_cycles period for which the DWC_mipi_dsi_host keeps the
 * link still, after sending a BTA operation. This period is
 * measured in cycles of lanebyteclk
 */
void mipi_dsih_presp_timeout_bta(struct mipi_dsi_dev *dev,
				 uint16_t no_of_byte_cycles)
{
	mipi_dsih_hal_presp_timeout_bta(dev, no_of_byte_cycles);
}

uint16_t mipi_dsih_check_dbi_fifos_state(struct mipi_dsi_dev *dev)
{
	uint16_t cnt = 0;
	while (cnt < 5000
	       && mipi_dsih_read_word(dev, R_DSI_HOST_CMD_PKT_STATUS) != 0x15) {
		cnt++;
	}
	return (mipi_dsih_read_word(dev, R_DSI_HOST_CMD_PKT_STATUS) !=
		0x15) ? -1 : 1;
}

uint16_t mipi_dsih_check_ulpm_mode(struct mipi_dsi_dev * dev)
{
	return (mipi_dsih_read_word(dev, R_DSI_HOST_PHY_STATUS) !=
		0x1528) ? -1 : 1;
}

/**
 * Stop Video Pattern Generator
 * @param dev pointer to structure holding the DSI Host core information
 */
void stop_video_pattern(struct mipi_dsi_dev *dev)
{
	mipi_dsih_hal_enable_vpg_act(dev, 0);

	mipi_dsih_reset_controller(dev);
}

/**
 * Start Video Pattern Generator
 * @param dev pointer to structure holding the DSI Host core information
 * @param orientation pattern orientiation
 * @param pattern type of pattern (BER or STANDARD)
 */
void start_video_pattern(struct mipi_dsi_dev *dev, unsigned char orientation,
			 unsigned char pattern)
{
	mipi_dsih_reset_controller(dev);

	mipi_dsih_hal_enable_vpg_act(dev, 0);

	mipi_dsih_hal_vpg_orientation_act(dev, orientation);

	mipi_dsih_hal_vpg_mode_act(dev, pattern);

	mipi_dsih_hal_enable_vpg_act(dev, 1);

	/*if (mipi_dsih_hal_read_state_shadow_registers(dev))
	 *      {
	 *              mipi_dsih_hal_request_registers_change(dev);
	 }*/
}
