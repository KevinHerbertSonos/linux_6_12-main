// SPDX-License-Identifier: GPL-2.0
/**
 * Copyright (C) 2017 Synopsys, Inc.
 *
 * @file dsih_dphy.c
 * @brief Synopsys MIPI D-PHY functions
 * included as a part of Synopsys MIPI DSI Host controller driver
 *
 * @author Luis Oliveira <luis.oliveira@synopsys.com>
 */

#include "dsih_dphy.h"
#include "dphy_wrap.h"
//#include "mipi_hal.h"

#define PRECISION_FACTOR (1000)

/*
 * Reference clock frequency divided by
 * Input Frequency Division Ratio LIMITS
 */
#ifdef GEN_3
#define DPHY_DIV_UPPER_LIMIT (8000)
#define DPHY_DIV_LOWER_LIMIT (2000)
#define MIN_OUTPUT_FREQ (80)
#else /*ifndef GEN_3*/
#define DPHY_DIV_UPPER_LIMIT (40000)
#define MIN_OUTPUT_FREQ (80)
#define DPHY_DIV_LOWER_LIMIT (5000)
#endif /*GEN_3*/

/*
 * Initialise D-PHY module and power up
 *
 * @param phy pointer to structure which holds information about the d-phy
 * module
 * @return error code
 */
int mipi_dsih_dphy_open(struct mipi_dsi_dev *dev)
{
	mipi_dsih_dphy_reset(dev, 0);
	mipi_dsih_dphy_stop_wait_time(dev, 0x1C);
	mipi_dsih_dphy_no_of_lanes(dev, 1);
	mipi_dsih_dphy_clock_en(dev, 1);
	mipi_dsih_dphy_shutdown(dev, 1);
	mipi_dsih_dphy_reset(dev, 1);

	return TRUE;
}

/*
 * Configure D-PHY and PLL module to desired operation mode
 *
 * @param phy pointer to structure which holds information about the
 *	d-phy module
 * @param no_of_lanes active
 * @param output_freq desired high speed frequency
 * @return error code
 */
#ifdef GEN_2
int mipi_dsih_dphy_configure(struct mipi_dsi_dev *dev, uint8_t no_of_lanes,
							 uint32_t output_freq)
{
	uint32_t loop_divider = 0;  /* (M) */
	uint32_t input_divider = 1; /* (N) */
	uint8_t data[4];			/* maximum data for now are 4 bytes per test mode*/
	uint8_t no_of_bytes = 0;
	uint8_t i = 0;   /* iterator */
	uint8_t range = 0; /* ranges iterator */
	int flag = 0;
	dphy_t *phy = &dev->phy;

	struct {
		uint32_t loop_div; /* upper limit of loop divider range */
		uint8_t vco_range;
		uint8_t cp_current;   /* icpctrl */
		uint8_t lpf_resistor; /* lpfctrl */
	}

	loop_bandwidth[] =
		{/*
		 * Gen 2 associates the charge pump current and LPF
		 * resistor with the output frequency ranges
		 * (and thus we simplify here to use the counter/pointer
		 * of the following structure)
		 */
		 {80, 0x00, 0x01, 0x02},
		 {90, 0x00, 0x01, 0x02},
		 {100, 0x00, 0x01, 0x02},
		 {110, 0x00, 0x01, 0x01},
		 {120, 0x00, 0x01, 0x01},
		 {130, 0x00, 0x01, 0x01},
		 {140, 0x00, 0x01, 0x01},
		 {150, 0x00, 0x09, 0x02},
		 {160, 0x00, 0x09, 0x02},
		 {170, 0x00, 0x09, 0x02},
		 {180, 0x00, 0x09, 0x02},
		 {200, 0x01, 0x02, 0x02},
		 {205, 0x01, 0x02, 0x02},
		 {220, 0x01, 0x02, 0x02},
		 {235, 0x01, 0x02, 0x02},
		 {250, 0x01, 0x09, 0x04},
		 {275, 0x01, 0x09, 0x04},
		 {300, 0x02, 0x01, 0x01},
		 {325, 0x02, 0x01, 0x01},
		 {350, 0x02, 0x01, 0x01},
		 {400, 0x02, 0x06, 0x04},
		 {450, 0x02, 0x06, 0x04},
		 {500, 0x03, 0x06, 0x08},
		 {550, 0x03, 0x06, 0x08},
		 {600, 0x03, 0x06, 0x04},
		 {650, 0x03, 0x06, 0x04},
		 {700, 0x04, 0x06, 0x04},
		 {750, 0x04, 0x06, 0x04},
		 {800, 0x04, 0x06, 0x04},
		 {850, 0x04, 0x06, 0x04},
		 {900, 0x05, 0x0B, 0x10},
		 {950, 0x05, 0x0B, 0x10},
		 {1000, 0x05, 0x0B, 0x10},
		 {1050, 0x05, 0x0B, 0x10},
		 {1100, 0x06, 0x0B, 0x08},
		 {1150, 0x06, 0x0B, 0x08},
		 {1200, 0x06, 0x0B, 0x08},
		 {1250, 0x06, 0x0B, 0x08},
		 {1300, 0x07, 0x0B, 0x08},
		 {1350, 0x07, 0x0B, 0x08},
		 {1400, 0x07, 0x0B, 0x08},
		 {1450, 0x07, 0x0B, 0x08},
		 {1500, 0x07, 0x0B, 0x08}};
	uint32_t delta = 0;
	uint32_t tmp_loop_divider = 0;
	unsigned step = 0;

	struct {
		uint32_t freq;   /* upper margin of frequency range */
		uint8_t hs_freq;   /* hsfreqrange */
		uint8_t vco_range; /* vcorange */
	} ranges[] =
		{
			{90, 0x00, 0x00},
			{100, 0x10, 0x00},
			{110, 0x20, 0x00},
			{130, 0x01, 0x00},
			{140, 0x11, 0x00},
			{150, 0x21, 0x00},
			{170, 0x02, 0x00},
			{180, 0x12, 0x00},
			{200, 0x22, 0x00},
			{220, 0x03, 0x01},
			{240, 0x13, 0x01},
			{250, 0x23, 0x01},
			{270, 0x04, 0x01},
			{300, 0x14, 0x01},
			{330, 0x05, 0x02},
			{360, 0x15, 0x02},
			{400, 0x25, 0x02},
			{450, 0x06, 0x02},
			{500, 0x16, 0x02},
			{550, 0x07, 0x03},
			{600, 0x17, 0x03},
			{650, 0x08, 0x03},
			{700, 0x18, 0x03},
			{750, 0x09, 0x04},
			{800, 0x19, 0x04},
			{850, 0x29, 0x04},
			{900, 0x39, 0x04},
			{950, 0x0A, 0x05},
			{1000, 0x1A, 0x05},
			{1050, 0x2A, 0x05},
			{1100, 0x3A, 0x05},
			{1150, 0x0B, 0x06},
			{1200, 0x1B, 0x06},
			{1250, 0x2B, 0x06},
			{1300, 0x3B, 0x06},
			{1350, 0x0C, 0x07},
			{1400, 0x1C, 0x07},
			{1450, 0x2C, 0x07},
			{1500, 0x3C, 0x07}};
	if (phy == NULL)
		return MIPI_RET(ENODEV);

	if (output_freq < MIN_OUTPUT_FREQ)
		return FALSE;

	loop_divider =
		((output_freq * (phy->reference_freq / DPHY_DIV_LOWER_LIMIT)) / phy->reference_freq);

	/* here delta will account for the rounding */
	delta =
		((loop_divider * phy->reference_freq) / (phy->reference_freq / DPHY_DIV_LOWER_LIMIT)) - output_freq;

	for (input_divider = 1 + (phy->reference_freq / DPHY_DIV_UPPER_LIMIT);
		 ((phy->reference_freq / input_divider) >= DPHY_DIV_LOWER_LIMIT) && (!flag);
		 input_divider++) {
		tmp_loop_divider =
			((output_freq * input_divider) / (phy->reference_freq));

		if ((tmp_loop_divider % 2) == 0) {
			/* if even */
			if (output_freq == (tmp_loop_divider * (phy->reference_freq / input_divider))) {
				/* exact values found */
				flag = 1;
				loop_divider = tmp_loop_divider;

				delta =
					output_freq - (tmp_loop_divider * (phy->reference_freq / input_divider));

				/* variable was incremented
				 * before exiting the loop */
				input_divider--;
			}
			if ((output_freq - (tmp_loop_divider * (phy->reference_freq / input_divider))) < delta) {
				/* values found with smaller delta */
				loop_divider = tmp_loop_divider;

				delta =
					output_freq - (tmp_loop_divider * (phy->reference_freq / input_divider));
				step = 1;
			}
		} else {
			tmp_loop_divider += 1;
			if (output_freq == (tmp_loop_divider * (phy->reference_freq / input_divider))) {
				/* exact values found */
				flag = 1;
				loop_divider = tmp_loop_divider;

				delta =
					(tmp_loop_divider * (phy->reference_freq / input_divider)) - output_freq;

				/* variable was incremented
				 * before exiting the loop */
				input_divider--;
			}
			if (((tmp_loop_divider * (phy->reference_freq / input_divider)) - output_freq) < delta) {
				/* values found with smaller delta */
				loop_divider = tmp_loop_divider;

				delta =
					(tmp_loop_divider * (phy->reference_freq / input_divider)) - output_freq;
				step = 0;
			}
		}
	}
	if (!flag) {
		input_divider =
			step + (loop_divider * phy->reference_freq) / output_freq;

		mipi_dbg_print(MIPI_DEBUG,"D-PHY: Approximated Frequency: %d KHz\n",
				 (loop_divider * (phy->reference_freq / input_divider)) / 8);
	}
	/*
	 * get the PHY in power down mode (shutdownz = 0)
	 * and reset it (rstz = 0) to avoid transient periods in PHY
	 * operation during re-configuration procedures.
	 */
	mipi_dsih_dphy_reset(dev, 0);
	mipi_dsih_dphy_clock_en(dev, 0);
	mipi_dsih_dphy_shutdown(dev, 0);

	/* provide an initial active-high test clear pulse in TESTCLR */
	mipi_dsih_dphy_test_clear(dev, 1);
	mipi_dsih_dphy_test_clear(dev, 0);

	/* find ranges */
	for (range = 0;
		 (range < (sizeof(ranges) / sizeof(ranges[0]))) && ((output_freq / 1000) > ranges[range].freq);
		 range++) {
		;
	}

	if (range >= (sizeof(ranges) / sizeof(ranges[0]))) {
		return FALSE;
	}

	/* set up board depending on environment if any */
	mipi_dsih_dphy_set_base_dir_tx(dev);

	/* setup digital part */
	/* hs frequency range [7]|[6:1]|[0]*/
	data[0] = (0 << 7) | (ranges[range].hs_freq << 1) | 0;
	mipi_dsih_dphy_write(dev, 0x44, data, 1);

	/* setup PLL */
	/* vco range  [7]|[6:3]|[2:1]|[0] */
	data[0] =
		(1 << 7) | (ranges[range].vco_range << 3) | (0x0 << 2) | (0 << 1) | 1;
	mipi_dsih_dphy_write(dev, 0x10, data, 1);

	/* for all Gen2 testchips,
	 * bypass LP TX enable idle low power */
	data[0] = 0x80;
	mipi_dsih_dphy_write(dev, 0x32, data, 1);
	mipi_dsih_dphy_write(dev, 0x42, data, 1);
	mipi_dsih_dphy_write(dev, 0x52, data, 1);
	mipi_dsih_dphy_write(dev, 0x82, data, 1);
	mipi_dsih_dphy_write(dev, 0x92, data, 1);

	if ((loop_divider % 2) != 0) {
		/*
		 * only odd integers are allowed
		 * (1 will be subtracted upon writing, see below)
		 */
		loop_divider -= 1;
	}
	/*
	 * Gen 2 associates the charge pump current and LPF resistor
	 * with the output frequency ranges (and thus we simplify here
	 * to use the counter/pointer of the following structure)
	 */
	i = range;
	data[0] = (0x00 << 6) | (0x01 << 5) | (0x01 << 4);
	mipi_dsih_dphy_write(dev, 0x19, data, 1);

	/* PLL Lock bypass|charge pump current [7:4]|[3:0] */
	data[0] = (0x00 << 4) | (loop_bandwidth[i].cp_current << 0);
	mipi_dsih_dphy_write(dev, 0x11, data, 1);

	/* bypass CP default|
	 * bypass LPF default|
	 * LPF resistor [7]|[6]|[5:0] */
	data[0] =
		(0x01 << 7) | (0x01 << 6) | (loop_bandwidth[i].lpf_resistor << 0);
	mipi_dsih_dphy_write(dev, 0x12, data, 1);

	/* PLL input divider ratio [7:0] */
	data[0] = input_divider - 1;
	mipi_dsih_dphy_write(dev, 0x17, data, 1);

	/* pll loop divider (code 0x18) takes only 2 bytes
	 * (10 bits in data) */
	no_of_bytes = 2;
	for (i = 0; i < no_of_bytes; i++) {
		/* 7 is dependent on no_of_bytes make sure 5 bits only
		 * of value are written at a time */
		data[i] =
			((uint8_t)((((loop_divider - 2) >> (5 * i)) & 0x1F) | (i << 7)));
	}
	/* PLL loop divider ratio -
	 * SET no|reserved|feedback divider [7]|[6:5]|[4:0] */
	mipi_dsih_dphy_write(dev, 0x18, data, no_of_bytes);
	mipi_dsih_dphy_no_of_lanes(dev, no_of_lanes);
	mipi_dsih_dphy_stop_wait_time(dev, 0x1C);
	mipi_dsih_dphy_clock_en(dev, 1);
	mipi_dsih_dphy_shutdown(dev, 1);
	mipi_dsih_dphy_reset(dev, 1);

	return TRUE;
}
#endif
#ifdef GEN_3
int mipi_dsih_dphy_configure(struct mipi_dsi_dev *dev, uint8_t no_of_lanes,
							 uint32_t output_data_rate)
{
	unsigned long loop_divider = 0;  /* (M) */
	unsigned long input_divider = 1; /* (N) */
	unsigned long vco_divider = 1;   /* (VCO) */
	unsigned long delta = 0;
	unsigned long tmp_loop_divider = 0;
	unsigned long output_freq = output_data_rate / 2;
	unsigned int step = 0;
	int flag = 0;
	uint8_t data[4]; /* maximum data for now are 4 bytes per test mode */
	uint8_t range = 0;
	uint8_t no_of_bytes = 0;
	uint8_t i = 0;
	dphy_t *phy = &dev->phy;

	struct {
		uint32_t data_rate; /* upper margin of frequency range */
		uint8_t hs_freq;	/* hsfreqrange */
		uint8_t vco_range;  /* vcorange */
	} ranges[] =
	{
		{  80, 0x00, 0x3F},
		{  90, 0x10, 0x3F},
		{ 100, 0x20, 0x3F},
		{ 110, 0x30, 0x37},
		{ 120, 0x01, 0x37},
		{ 130, 0x11, 0x37},
		{ 140, 0x21, 0x37},
		{ 150, 0x31, 0x37},
		{ 160, 0x02, 0x37},
		{ 170, 0x12, 0x2F},
		{ 180, 0x22, 0x2F},
		{ 190, 0x32, 0x2F},
		{ 205, 0x03, 0x2F},
		{ 220, 0x13, 0x27},
		{ 235, 0x23, 0x27},
		{ 250, 0x33, 0x27},
		{ 275, 0x04, 0x27},
		{ 300, 0x14, 0x27},
		{ 325, 0x25, 0x27},
		{ 350, 0x35, 0x1F},
		{ 400, 0x05, 0x1F},
		{ 450, 0x16, 0x17},
		{ 500, 0x26, 0x17},
		{ 550, 0x37, 0x17},
		{ 600, 0x07, 0x17},
		{ 650, 0x18, 0x17},
		{ 700, 0x28, 0x0F},
		{ 750, 0x39, 0x0F},
		{ 800, 0x09, 0x0F},
		{ 850, 0x19, 0x0F},
		{ 900, 0x29, 0x07},
		{ 950, 0x3A, 0x07},
		{1000, 0x0A, 0x07},
		{1050, 0x1A, 0x07},
		{1100, 0x2A, 0x07},
		{1150, 0x3B, 0x07},
		{1200, 0x0B, 0x07},
		{1250, 0x1B, 0x07},
		{1300, 0x2B, 0x07},
		{1350, 0x3C, 0x03},
		{1400, 0x0C, 0x03},
		{1450, 0x1C, 0x03},
		{1500, 0x2C, 0x03},
		{1550, 0x3D, 0x03},
		{1600, 0x0D, 0x03},
		{1650, 0x1D, 0x03},
		{1700, 0x2E, 0x03},
		{1750, 0x3E, 0x03},
		{1800, 0x0E, 0x03},
		{1850, 0x1E, 0x03},
		{1900, 0x2F, 0x03},
		{1950, 0x3F, 0x03},
		{2000, 0x0F, 0x03},
		{2050, 0x40, 0x03},
		{2100, 0x41, 0x03},
		{2150, 0x42, 0x03},
		{2200, 0x43, 0x03},
		{2250, 0x44, 0x03},
		{2300, 0x45, 0x01},
		{2350, 0x46, 0x01},
		{2400, 0x47, 0x01},
		{2450, 0x48, 0x01},
		{2500, 0x49, 0x01}
	};

	if (phy == NULL)
		return MIPI_RET(ENODEV);

	if (output_freq < MIN_OUTPUT_FREQ)
		return FALSE;

	/* find ranges */
	for (range = 0;
		 (range < (sizeof(ranges) / sizeof(ranges[0]))) && ((output_data_rate / 1000) > ranges[range].data_rate);
		 range++) {
		;
	}

	if (range >= (sizeof(ranges) / sizeof(ranges[0]))) {
		return FALSE;
	}

	if ((ranges[range].vco_range >> 4) == 3)
		vco_divider = 8;
	else if ((ranges[range].vco_range >> 4) == 2)
		vco_divider = 4;
	else
		vco_divider = 2;

	if (ranges[range].data_rate > 640)
		vco_divider = 1;

	output_freq = output_freq * vco_divider;

	loop_divider =
		((output_freq * (phy->reference_freq / DPHY_DIV_LOWER_LIMIT)) / phy->reference_freq);

	/* here delta will account for the rounding */
	delta =
		((loop_divider * phy->reference_freq) / (phy->reference_freq / DPHY_DIV_LOWER_LIMIT)) - output_freq;

	for (input_divider = 1 + (phy->reference_freq / DPHY_DIV_UPPER_LIMIT);
		 ((phy->reference_freq / input_divider) >= DPHY_DIV_LOWER_LIMIT) && (!flag);
		 input_divider++) {
		tmp_loop_divider = ((output_freq * input_divider) / (phy->reference_freq));
		if ((tmp_loop_divider % 2) == 0) {

			/* if even */
			if (output_freq == (tmp_loop_divider * (phy->reference_freq / input_divider))) {

				/* exact values found */
				flag = 1;
				loop_divider = tmp_loop_divider;

				delta =
					output_freq - (tmp_loop_divider * (phy->reference_freq / input_divider));

				/* variable was incremented
				 * before exiting the loop */
				input_divider--;
			}
			if ((output_freq - (tmp_loop_divider * (phy->reference_freq / input_divider))) < delta) {

				/* values found with smaller delta */
				loop_divider = tmp_loop_divider;

				delta =
					output_freq - (tmp_loop_divider * (phy->reference_freq / input_divider));
				step = 1;
			}
		} else {
			tmp_loop_divider += 1;
			if (output_freq == (tmp_loop_divider * (phy->reference_freq / input_divider))) {

				/* exact values found */
				flag = 1;
				loop_divider = tmp_loop_divider;
				delta =
					(tmp_loop_divider * (phy->reference_freq / input_divider)) - output_freq;

				/* variable was incremented before exiting the loop */
				input_divider--;
			}
			if (((tmp_loop_divider * (phy->reference_freq / input_divider)) - output_freq) < delta) {

				/* values found with smaller delta */
				loop_divider = tmp_loop_divider;
				delta =
					(tmp_loop_divider * (phy->reference_freq / input_divider)) - output_freq;
				step = 0;
			}
		}
	}
	if (!flag)
		input_divider = step + (loop_divider * phy->reference_freq) / output_freq;

	/*
	 * Get the PHY in power down mode
	 * (shutdownz = 0) and reset it (rstz = 0) to avoid transient
	 * periods in PHY operation during re-configuration procedures
	 */
	mipi_dsih_dphy_reset(dev, 0);
	mipi_dsih_dphy_clock_en(dev, 0);
	mipi_dsih_dphy_shutdown(dev, 0);

	mipi_dsih_dphy_test_clear(dev, 1);
	mipi_dsih_dphy_test_clear(dev, 0);

	mipi_dphy_pll_clksel(phy, PLL_OUTPUT_CLOCK_GENERATE);

	/* hs frequency range [7]|[6:0]*/
	data[0] = (1 << 7) | (ranges[range].hs_freq ) ;
	mipi_dsih_dphy_write(dev, 0x44, data, 1);

	/*PLL lock configurations*/
	data[0] = 0x2;//pll_th1[9:0]
	mipi_dsih_dphy_write(dev, 0x14, data, 1);
	data[0] = 0x80;
	mipi_dsih_dphy_write(dev, 0x14, data, 1);
	data[0] = 0x60;//pll_th2[7:0]
	mipi_dsih_dphy_write(dev, 0x15, data, 1);
	data[0] = 0x3; //pll_th3[7:0]
	mipi_dsih_dphy_write(dev, 0x16, data, 1);
	data[0] = 0x2; //pll_lock_sel = 1
	mipi_dsih_dphy_write(dev, 0x1D, data, 1);

	if (ranges[range].data_rate <= 1500) {
		if (ranges[range].data_rate > 1000) {
			/*166ps*/
			data[0] = (0 << 6);
			mipi_dsih_dphy_write(dev, 0xA0, data, 1); //sr_range = 0
			data[0] =  0x18;//920 = 0x398
			mipi_dsih_dphy_write(dev, 0xA4, data, 1);
			data[0] =  0x87;
			mipi_dsih_dphy_write(dev, 0xA4, data, 1);
		} else {
			/*225ps*/
			data[0] = (1 << 6);
			mipi_dsih_dphy_write(dev, 0xA0, data, 1); //sr_range = 1
			data[0] = 0x11; //657 = 0x291
			mipi_dsih_dphy_write(dev, 0xA4, data, 1);
			data[0] = 0x85;
			mipi_dsih_dphy_write(dev, 0xA4, data, 1);
		}
		data[0] = 0x01;
		mipi_dsih_dphy_write(dev, 0xA3, data, 1);

		/* PLL Analog Programmability Control */
		data[0] = 0x01;
		mipi_dsih_dphy_write(dev, 0x1F, data, 1);
		data[0] = (1 << 6);
		mipi_dsih_dphy_write(dev, 0x4A, data, 1);
	} else {
		data[0] = (1 << 6);
		mipi_dsih_dphy_write(dev, 0x4A, data, 1);
		data[0] = (1 << 1);
		mipi_dsih_dphy_write(dev, 0xA0, data, 1); //disable slew rate calibration algorithm
		data[0] = 0x00;
		mipi_dsih_dphy_write(dev, 0xA3, data, 1);
		data[0] = 0x01;
		mipi_dsih_dphy_write(dev, 0x1F, data, 1);
	}

	data[0] = ((phy->reference_freq/1000)-17)*4;//28;(Fcfg_clk(MHz)-17)*4]
	mipi_dphy_CfgClkFreqRange(phy, data[0]);
	mipi_dphy_pll_shadow_control_en(phy, 1);

	/* setup digital part */
	/* PLL Proportional Charge Pump control*/
	if (ranges[range].data_rate >= 2300)
		data[0] =  0x09;
	else
		data[0] =  0x08;
	mipi_dsih_dphy_write(dev, 0x0E, data, 1);
	/* PLL Integral Charge Pump control*/
	data[0] =  0x00;
	mipi_dsih_dphy_write(dev, 0x0F, data, 1);
	/* PLL Charge Pump Bias Control*/
	data[0] =  0x10;
	mipi_dsih_dphy_write(dev, 0x1C, data, 1);
	/* PLL GMP Control and Digital Testability*/
	data[0] =  0x00;
	mipi_dsih_dphy_write(dev, 0x13, data, 1);
	/* setup PLL
	 * Reserved | pll_vco_cntrl_ovr_en | pll_vco_cntrl_ovr*/
	data[0] = (1 << 6) | (ranges[range].vco_range);
	mipi_dsih_dphy_write(dev, 0x12, data, 1);

	data[0] = (0x00 << 6) | (0x01 << 5) | (0x01 << 4);
	mipi_dsih_dphy_write(dev, 0x19, data, 1);

	/* PLL input divider ratio [7:0] */
	data[0] = input_divider - 1;
	mipi_dsih_dphy_write(dev, 0x17, data, 1);
	/* pll loop divider (code 0x18) takes only 2 bytes (10 bits in data) */
	no_of_bytes = 2;
	/* 7 is dependent on no_of_bytes make sure 5 bits only of value are written at a time */
	for (i = 0; i < no_of_bytes; i++)
		data[i] = ((uint8_t)((((loop_divider - 2) >> (5 * i)) & 0x1F) | (i << 7) ));

	/* PLL loop divider ratio - SET no|reserved|feedback divider [7]|[6:5]|[4:0] */
	mipi_dsih_dphy_write(dev, 0x18, data, no_of_bytes);

	mipi_dsih_dphy_no_of_lanes(dev, no_of_lanes);
	mipi_dsih_dphy_stop_wait_time(dev, 0x1E);

	/*Enable deskew pattern for 10us*/
	if (ranges[range].data_rate > 1500) {
		mipi_dphy_stopstate_wait(phy, no_of_lanes);
		mipi_dsih_dphy_deskew_en(dev, 1);
		mdelay(1);
		mipi_dsih_dphy_deskew_en(dev, 0);
	}
	mipi_dsih_dphy_clock_en(dev, 1);
	mipi_dsih_dphy_shutdown(dev, 1);
	mipi_dsih_dphy_reset(dev, 1);

	return TRUE;
}
#endif

/*
 * Close and power down D-PHY module
 *
 * @param phy pointer to structure which holds information about the
 * d-phy module
 * @return error code
 */
int mipi_dsih_dphy_close(struct mipi_dsi_dev *dev)
{
	dphy_t *phy = &dev->phy;

	if (phy == NULL)
		return MIPI_RET(ENODEV);

	mipi_dsih_dphy_reset(dev, 0);
	mipi_dsih_dphy_reset(dev, 1);
	mipi_dsih_dphy_shutdown(dev, 0);

	return TRUE;
}
/*
 * Enable clock lane module
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param en
 */
void mipi_dsih_dphy_clock_en(struct mipi_dsi_dev *dev, int en)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_RSTZ, en, 2, 1);
}
/*
 * Reset D-PHY module
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param reset
 */
void mipi_dsih_dphy_reset(struct mipi_dsi_dev *dev, int reset)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_RSTZ, reset, 1, 1);
}
/**
 * Power up/down D-PHY module
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param powerup (1) shutdown (0)
 */
void mipi_dsih_dphy_shutdown(struct mipi_dsi_dev *dev, int powerup)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_RSTZ, powerup, 0, 1);
}
/*
 * Force D-PHY PLL to stay on while in ULPS
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param force (1) disable (0)
 * @note To follow the programming model, use wakeup_pll function
 */
void mipi_dsih_dphy_force_pll(struct mipi_dsi_dev *dev, int force)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_RSTZ, force, 3, 1);
}
/**
 * Get force D-PHY PLL module
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @return force value
 */
int mipi_dsih_dphy_get_force_pll(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_dphy_read_part(dev, R_DPHY_RSTZ, 3, 1);
}
/**
 * Wake up or make sure D-PHY PLL module is awake
 * This function must be called after going into ULPS and before exiting it
 * to force the DPHY PLLs to wake up. It will wait until the DPHY status is
 * locked. It follows the procedure described in the user guide.
 * This function should be used to make sure the PLL is awake, rather than
 * the force_pll above.
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @return error code
 * @note this function has an active wait
 */
int mipi_dsih_dphy_wakeup_pll(struct mipi_dsi_dev *dev)
{
	unsigned i = 0;
	if (mipi_dsih_dphy_status(dev, 0x1) == 0) {
		mipi_dsih_dphy_force_pll(dev, 1);
		for (i = 0; i < DSIH_PHY_ACTIVE_WAIT; i++) {
			if (mipi_dsih_dphy_status(dev, 0x1)) {
				break;
			}
		}
		if (mipi_dsih_dphy_status(dev, 0x1) == 0) {
			return FALSE;
		}
	}
	return TRUE;
}
/**
 * Configure minimum wait period for HS transmission request after a stop state
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param no_of_byte_cycles [in byte (lane) clock cycles]
 */
void mipi_dsih_dphy_stop_wait_time(struct mipi_dsi_dev *dev, uint8_t no_of_byte_cycles)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_IF_CFG, no_of_byte_cycles, 8, 8);
}
/**
 * Set number of active lanes
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param no_of_lanes
 */
void mipi_dsih_dphy_no_of_lanes(struct mipi_dsi_dev *dev, uint8_t no_of_lanes)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_IF_CFG, no_of_lanes - 1, 0, 2);
}
/**
 * Get number of currently active lanes
 *
 * @param dev pointer to structure which holds information about the d-phy
 *  module
 * @return number of active lanes
 */
uint8_t mipi_dsih_dphy_get_no_of_lanes(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_dphy_read_part(dev, R_DPHY_IF_CFG, 0, 2);
}
/**
 * Request the PHY module to start transmission of high speed clock.
 * This causes the clock lane to start transmitting DDR clock on the
 * lane interconnect.
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param enable
 * @note this function should be called explicitly by user always except for
 * transmitting
 */
void mipi_dsih_dphy_enable_hs_clk(struct mipi_dsi_dev *dev, int enable)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_LPCLK_CTRL, enable, 0, 1);
}
/**
 * One bit is asserted in the trigger_request (4bits) to cause the lane module
 * to cause the associated trigger to be sent across the lane interconnect.
 * The trigger request is synchronous with the rising edge of the clock.
 * @note: Only one bit of the trigger_request is asserted at any given time, the
 * remaining must be left set to 0, and only when not in LPDT or ULPS modes
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param trigger_request 4 bit request
 */
int mipi_dsih_dphy_escape_mode_trigger(struct mipi_dsi_dev *dev, uint8_t trigger_request)
{
	uint8_t sum = 0;
	int i = 0;

	for (i = 0; i < 4; i++) {
		sum += ((trigger_request >> i) & 1);
	}
	if (sum == 1) {
		/* clear old trigger */
		mipi_dsih_dphy_write_part(dev,
								  R_DPHY_TX_TRIGGERS, 0x00, 0, 4);
		mipi_dsih_dphy_write_part(dev,
								  R_DPHY_TX_TRIGGERS, trigger_request, 0, 4);

		for (i = 0; i < DSIH_PHY_ACTIVE_WAIT; i++) {

			if (mipi_dsih_dphy_status(dev, 0x0010)) {
				break;
			}
		}

		mipi_dsih_dphy_write_part(dev,
								  R_DPHY_TX_TRIGGERS, 0x00, 0, 4);
		if (i >= DSIH_PHY_ACTIVE_WAIT) {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
/**
 * ULPS mode request/exit on all active data lanes.
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param enable (request 1/ exit 0)
 * @return error code
 * @note this is a blocking function. wait upon exiting the ULPS will exceed 1ms
 */
int mipi_dsih_dphy_ulps_data_lanes(struct mipi_dsi_dev *dev, int enable)
{
	int timeout;
	/* mask 1 0101 0010 0000 */
	uint16_t data_lanes_mask = 0;

	if (enable) {
		mipi_dsih_dphy_write_part(dev,
								  R_DPHY_ULPS_CTRL, 1, 2, 1);
		return TRUE;
	} else {
		if (mipi_dsih_dphy_status(dev, 0x1) == 0)
			return FALSE;

		mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 1, 3, 1);
		switch (mipi_dsih_dphy_get_no_of_lanes(dev)) {
			case 3:
			{
				data_lanes_mask |= (1 << 12);
			}
			break;
			case 2:
			{
				data_lanes_mask |= (1 << 10);
			}
			break;

			case 1:
			{
				data_lanes_mask |= (1 << 8);
			}
			break;

			case 0:
			{
				data_lanes_mask |= (1 << 5);
			}
			break;

			default:
				data_lanes_mask |= 0;
				break;
		}
		for (timeout = 0; timeout < DSIH_PHY_ACTIVE_WAIT; timeout++) {
			/* verify that the DPHY has left ULPM */

			if (mipi_dsih_dphy_status(dev, data_lanes_mask) == data_lanes_mask) {
				break;
			}
			mdelay(5);
		}

		if (mipi_dsih_dphy_status(dev, data_lanes_mask) != data_lanes_mask) {
			mipi_dbg_print(MIPI_DEBUG,"stat %x, mask %x",
					 mipi_dsih_dphy_status(dev, data_lanes_mask),
					 data_lanes_mask);
			return FALSE;
		}
		mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 0, 2, 1);
		mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 0, 3, 1);
	}
	return TRUE;
}
/*
 * ULPS mode request/exit on Clock Lane.
 *
 * @param dev pointer to structure which holds information about the
 * d-phy module
 * @param enable 1 or disable 0 of the Ultra Low Power State of the clock lane
 * @return error code
 * @note this is a blocking function. wait upon exiting the ULPS will exceed 1ms
 */
int mipi_dsih_dphy_ulps_clk_lane(struct mipi_dsi_dev *dev, int enable)
{
	int timeout;
	/* mask 1000 */
	uint16_t clk_lane_mask = 0x0008;

	if (enable) {
		/* mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 0, 0, 1); */
		mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 1, 0, 1);
	} else {
		if (mipi_dsih_dphy_status(dev, 0x1) == 0)
			return FALSE;

		mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 1, 1, 1);

		for (timeout = 0; timeout < DSIH_PHY_ACTIVE_WAIT; timeout++) {
			/* verify that the DPHY has left ULPM */
			if (mipi_dsih_dphy_status(dev, clk_lane_mask) == clk_lane_mask) {
				mipi_dbg_print(MIPI_DEBUG,"stat %x, mask %x",
						 mipi_dsih_dphy_status(dev, clk_lane_mask),
						 clk_lane_mask);
				break;
			}
			mdelay(5);
		}
		if (mipi_dsih_dphy_status(dev, clk_lane_mask) != clk_lane_mask)
			return FALSE;

		mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 0, 0, 1);
		mipi_dsih_dphy_write_part(dev, R_DPHY_ULPS_CTRL, 0, 1, 1);
	}
	return TRUE;
}
/*
 * Get D-PHY PPI status
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param mask
 * @return status
 */
uint32_t mipi_dsih_dphy_status(struct mipi_dsi_dev *dev, uint16_t mask)
{
	return mipi_dsih_dphy_read_word(dev, R_DPHY_STATUS) & mask;
}
/*
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param value
 */
void mipi_dsih_dphy_test_clock(struct mipi_dsi_dev *dev, int value)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_TST_CRTL0, value, 1, 1);
}
/**
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param value
 */
void mipi_dsih_dphy_test_clear(struct mipi_dsi_dev *dev, int value)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_TST_CRTL0, value, 0, 1);
}
/**
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param on_falling_edge
 */
void mipi_dsih_dphy_test_en(struct mipi_dsi_dev *dev, uint8_t on_falling_edge)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_TST_CRTL1, on_falling_edge, 16, 1);
}
/**
 * @param dev pointer to structure which holds information about the d-phy
 * module
 */
uint8_t mipi_dsih_dphy_test_data_out(struct mipi_dsi_dev *dev)
{
	return mipi_dsih_dphy_read_part(dev, R_DPHY_TST_CRTL1, 8, 8);
}
/**
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param test_data
 */
void mipi_dsih_dphy_test_data_in(struct mipi_dsi_dev *dev, uint8_t test_data)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_TST_CRTL1, test_data, 0, 8);
}
/**
 * Write to D-PHY module (encapsulating the digital interface)
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param address offset inside the D-PHY digital interface
 * @param data array of bytes to be written to D-PHY
 * @param data_length of the data array
 */
#ifdef GEN_2
void mipi_dsih_dphy_write(struct mipi_dsi_dev *dev, uint16_t address,
						  uint8_t *data,
						  uint8_t data_length)
{
	unsigned i = 0;

	if (data != 0) {

		mipi_dbg_print(MIPI_DEBUG,"TEST CODE: ADDR %X DATA %X\n", address, data[0]);
		/* set the TESTCLK input high in preparation
		 * to latch in the desired test mode */
		mipi_dsih_dphy_test_clock(dev, 1);
		/* set the desired test code in the input 8-bit bus
		 * TESTDIN[7:0] */
		mipi_dsih_dphy_test_data_in(dev, (uint8_t)address);
		/* set TESTEN input high  */
		mipi_dsih_dphy_test_en(dev, 1);
		/* drive the TESTCLK input low; the falling edge
		 * captures the chosen test code into the transceiver
		 */
		mipi_dsih_dphy_test_clock(dev, 0);
		/* set TESTEN input low to disable further test mode
		 * code latching  */
		mipi_dsih_dphy_test_en(dev, 0);
		/* start writing MSB first */
		for (i = data_length; i > 0; i--) {
			/* set TESTDIN[7:0] to the desired test data
			 * appropriate to the chosen test mode */
			mipi_dsih_dphy_test_data_in(dev, data[i - 1]);
			/* pulse TESTCLK high to capture this test data
			 * into the macrocell; repeat these two steps
			 * as necessary */
			mipi_dsih_dphy_test_clock(dev, 1);
			mipi_dsih_dphy_test_clock(dev, 0);
		}
	}
}
#endif

#ifdef GEN_3
void mipi_dsih_dphy_write(struct mipi_dsi_dev *dev, uint16_t address,
						  uint8_t *data,
						  uint8_t data_length)
{
	unsigned i = 0;

	//mipi_dbg_print(MIPI_DEBUG,"TEST CODE: ADDR %X DATA %X\n", address, data[0]);
	if (data != 0) {
		/* set the TESTCLK input high in preparation to latch in the desired test mode */
		mipi_dsih_dphy_test_clock(dev, 1);
		/* set the desired test code in the input 8-bit bus TESTDIN[7:0] */
		mipi_dsih_dphy_test_data_in(dev, (uint8_t)address);
		/* set TESTEN input high  */
		mipi_dsih_dphy_test_en(dev, 1);
		/* drive the TESTCLK input low; the falling edge captures the chosen test code into the transceiver */
		mipi_dsih_dphy_test_clock(dev, 0);
		/* set TESTEN input low to disable further test mode code latching  */
		mipi_dsih_dphy_test_en(dev, 0);
		/* start writing MSB first */
		for (i = data_length; i > 0; i--) {
			/* set TESTDIN[7:0] to the desired test data appropriate to the chosen test mode */
			mipi_dsih_dphy_test_data_in(dev, data[i - 1]);
			/* pulse TESTCLK high to capture this test data into the macrocell; repeat these two steps as necessary */
			mipi_dsih_dphy_test_clock(dev, 1);
			mipi_dsih_dphy_test_clock(dev, 0);
		}
	}
}
#endif

void mipi_dsih_dphy_delay(struct mipi_dsi_dev *dev, int value)
{
	return;
}
void mipi_dsih_dphy_mmcm_pclk(struct mipi_dsi_dev *dev, uint32_t pclk)
{
	//FIXME: Pixel clock configuration is done in VPP
}
/* abstracting BSP */
/**
 * Write to whole register to D-PHY module (encapsulating the bus interface)
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param reg_address offset
 * @param data 32-bit word
 */
void mipi_dsih_dphy_write_word(struct mipi_dsi_dev *dev, uint32_t reg_address, uint32_t data)
{
	iowrite32(dev->core_addr + reg_address, data);
}
/**
 * Write bit field to D-PHY module (encapsulating the bus interface)
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param reg_address offset
 * @param data bits to be written to D-PHY
 * @param shift from the right hand side of the register (big endian)
 * @param width of the bit field
 */
void mipi_dsih_dphy_write_part(struct mipi_dsi_dev *dev, uint32_t reg_address, uint32_t data, uint8_t shift, uint8_t width)
{
	uint32_t mask = 0;
	uint32_t temp = 0;

	mask = (1 << width) - 1;
	temp = mipi_dsih_dphy_read_word(dev, reg_address);
	temp &= ~(mask << shift);
	temp |= (data & mask) << shift;
	mipi_dsih_dphy_write_word(dev, reg_address, temp);
}
/**
 * Read whole register from D-PHY module (encapsulating the bus interface)
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param reg_address offset
 * @return data 32-bit word
 */
uint32_t mipi_dsih_dphy_read_word(struct mipi_dsi_dev *dev, uint32_t reg_address)
{
   	uint32_t val;

	ioread32(dev->core_addr + reg_address, &val);

	return val;
}
/**
 * Read bit field from D-PHY module (encapsulating the bus interface)
 *
 * @param dev pointer to structure which holds information about the d-phy
 * module
 * @param reg_address offset
 * @param shift from the right hand side of the register (big endian)
 * @param width of the bit field
 * @return data bits to be written to D-PHY
 */
uint32_t mipi_dsih_dphy_read_part(struct mipi_dsi_dev *dev, uint32_t reg_address, uint8_t shift, uint8_t width)
{
	return (mipi_dsih_dphy_read_word(dev, reg_address) >> shift) & ((1 << width) - 1);
}

/** Test chip requirement - set chip direction (basedir_N) to Tx & force PLL
 *  signals this is the sequence followed by the test interface of the PHY
 *  found in phy_write function in the PHY API
 *
 *  @param dphy pointer to dphy context structure
 *  @param param
 */
void mipi_dsih_dphy_set_base_dir_tx(struct mipi_dsi_dev *dev)
{
	uint8_t data[4]; /* maximum data for now are 4 bytes per test mode*/
	data[0] = 0x01;
	mipi_dsih_dphy_write(dev, 0xb0, data, 1);
}

/**
 * Maximum times that the D-PHY lanes take to go from high-speed to low-power
 * transmission measured in lane byte clock cycles. PHY dependent
 *
 * @param video pointer to DPI configuration parameters instance
 * @return error code
 */
uint16_t dsi_get_transition_times(struct mipi_dsi_dev *dev)
{
	dsih_dpi_video_t *video;
	unsigned range = 0;
	struct {
		uint16_t freq;
		uint16_t clk_lp2hs;
		uint8_t clk_hs2lp;
		uint8_t data_lp2hs;
		uint8_t data_hs2lp;
	}
#ifdef GEN_2
	phy_transitions[] =
		{
			{90, 32, 20, 26, 13}, {100, 35, 23, 28, 14}, {110, 32, 22, 26, 13}, {130, 31, 20, 27, 13}, {140, 33, 22, 26, 14}, {150, 33, 21, 26, 14}, {170, 32, 20, 27, 13}, {180, 36, 23, 30, 15}, {200, 40, 22, 33, 15}, {220, 40, 22, 33, 15}, {240, 44, 24, 36, 16}, {250, 48, 24, 38, 17}, {270, 48, 24, 38, 17}, {300, 50, 27, 41, 18}, {330, 56, 28, 45, 18}, {350, 59, 28, 48, 19}, {400, 61, 30, 50, 20}, {450, 67, 31, 55, 21}, {500, 73, 31, 59, 22}, {550, 79, 36, 63, 24}, {600, 83, 37, 68, 25}, {650, 90, 38, 73, 27}, {700, 95, 40, 77, 28}, {750, 102, 40, 84, 28}, {800, 106, 42, 87, 30}, {850, 113, 44, 93, 31}, {900, 118, 47, 98, 32}, {950, 124, 47, 102, 34}, {1000, 130, 49, 107, 35}, {1050, 135, 51, 111, 37}, {1100, 139, 51, 114, 38}, {1150, 146, 54, 120, 40}, {1200, 153, 57, 125, 41}, {1250, 158, 58, 130, 42}, {1300, 163, 58, 135, 44}, {1350, 168, 60, 140, 45}, {1400, 172, 64, 144, 47}, {1450, 176, 65, 148, 48}, {1500, 181, 66, 153, 50}

		};
#endif
#ifdef GEN_3
	phy_transitions[] =
		{
			{80, 21, 17, 15, 10},
			{90, 23, 17, 16, 10},
			{100, 22, 17, 16, 10},
			{110, 25, 18, 17, 11},
			{120, 26, 20, 18, 11},
			{130, 27, 19, 19, 11},
			{140, 27, 19, 19, 11},
			{150, 28, 20, 20, 12},
			{160, 30, 21, 22, 13},
			{170, 30, 21, 23, 13},
			{180, 31, 21, 23, 13},
			{190, 32, 22, 24, 13},
			{205, 35, 22, 25, 13},
			{220, 37, 26, 27, 15},
			{235, 38, 28, 27, 16},
			{250, 41, 29, 30, 17},
			{275, 43, 29, 32, 18},
			{300, 45, 32, 35, 19},
			{325, 48, 33, 36, 18},
			{350, 51, 35, 40, 20},
			{400, 59, 37, 44, 21},
			{450, 65, 40, 49, 23},
			{500, 181,66, 153,50},/*{500, 71, 41, 54, 24},*/
			{550, 77, 44, 57, 26},
			{600, 82, 46, 64, 27},
			{650, 87, 48, 67, 28},
			{700, 94, 52, 71, 29},
			{750, 99, 52, 75, 31},
			{800, 105, 55, 82, 32},
			{850, 110, 58, 85, 32},
			{900, 115, 58, 88, 35},
			{950, 120, 62, 93, 36},
			{1000, 128, 63, 99, 38},
			{1050, 132, 65, 102, 38},
			{1100, 138, 67, 106, 39},
			{1150, 146, 69, 112, 42},
			{1200, 151, 71, 117, 43},
			{1250, 153, 74, 120, 45},
			{1300, 160, 73, 124, 46},
			{1350, 165, 76, 130, 47},
			{1400, 172, 78, 134, 49},
			{1450, 177, 80, 138, 49},
			{1500, 183, 81, 143, 52},
			{1550, 191, 84, 147, 52},
			{1600, 194, 85, 152, 52},
			{1650, 201, 86, 155, 53},
			{1700, 208, 88, 161, 53},
			{1750, 212, 89, 165, 53},
			{1800, 220, 90, 171, 54},
			{1850, 223, 92, 175, 54},
			{1900, 231, 91, 180, 55},
			{1950, 236, 95, 185, 56},
			{2000, 243, 97, 190, 56},
			{2050, 248, 99, 194, 58},
			{2100, 252, 100,199, 59},
			{2150, 259, 102,204, 61},
			{2200, 266, 105,210, 62},
			{2250, 269, 109,213, 63},
			{2300, 272, 109,217, 65},
			{2350, 281, 112,225, 66},
			{2400, 283, 115,226, 66},
			{2450, 282, 115,226, 67},
			{2500, 281, 118,227, 67}
		};
#endif

	/* check DSI controller dev */
	if (dev == NULL)
		return MIPI_RET(ENODEV);

	video = &dev->dpi_video;

	/* find ranges */
	for (range = 0;
		 (range < (sizeof(phy_transitions) / sizeof(phy_transitions[0]))) && (((video->byte_clock * 8) / 1000) > phy_transitions[range].freq);
		 range++) {
		;
	}
	if (range >= (sizeof(phy_transitions) / sizeof(phy_transitions[0]))) {
		return FALSE;
	}
	video->max_hs_to_lp_cycles = phy_transitions[range].data_hs2lp;
	video->max_lp_to_hs_cycles = phy_transitions[range].data_lp2hs;
	video->max_clk_hs_to_lp_cycles = phy_transitions[range].clk_hs2lp;
	video->max_clk_lp_to_hs_cycles = phy_transitions[range].clk_lp2hs;
	return TRUE;
}

/*
 * Enable/Disable PHY deskew pattern for > 1.5Gbps
 */
void mipi_dsih_dphy_deskew_en(struct mipi_dsi_dev *dev, int en)
{
	mipi_dsih_dphy_write_part(dev, R_DPHY_PHY_CAL, en, 0, 1);
}
