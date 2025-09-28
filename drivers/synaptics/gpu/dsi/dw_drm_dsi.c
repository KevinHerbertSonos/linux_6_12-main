/*
 * DesignWare MIPI DSI Host Controller v1.02 driver
 *
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2014-2016 Hisilicon Limited.
 *
 * Author:
 *	Xinliang Liu <z.liuxinliang@hisilicon.com>
 *	Xinliang Liu <xinliang.liu@linaro.org>
 *	Xinwei Kong <kong.kongxinwei@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of_gpio.h>
#include <linux/component.h>
#include <linux/of_graph.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/gpio.h>

#include "dw_drm_dsi.h"
#include "dw_dsi_reg.h"
#include "dw_dsi_cmd.h"

#define MAX_TX_ESC_CLK		10
#define ROUND_PRECISION		3
#define PHY_REF_CLK_RATE	19200000
#define PHY_REF_CLK_PERIOD_PS	(1000000000 / (PHY_REF_CLK_RATE / 1000))

static inline u32
round_div(u32 x, u32 y)
{
	u32 tmp = (x << ROUND_PRECISION) / y;
	return (tmp >> ROUND_PRECISION) + ((tmp >> (ROUND_PRECISION - 1)) & 1);
}

static u32
dsi_calc_phy_rate(u32 req_kHz, struct mipi_phy_params *phy)
{
	u32 ref_clk_ps = PHY_REF_CLK_PERIOD_PS;
	u32 tmp_kHz = req_kHz;
	u32 i = 0;
	u32 q_pll = 1;
	u32 m_pll = 0;
	u32 n_pll = 0;
	u32 r_pll = 1;
	u32 m_n = 0;
	u32 m_n_int = 0;
	u32 f_kHz = 0;
	u64 temp;
	u64 rem;

	/*
	 * Find a rate >= req_kHz.
	 */
	do {
		f_kHz = tmp_kHz;

		for (i = 0; i < ARRAY_SIZE(dphy_range_info); i++)
			if (f_kHz >= dphy_range_info[i].min_range_kHz &&
			    f_kHz <= dphy_range_info[i].max_range_kHz)
				break;

		if (i == ARRAY_SIZE(dphy_range_info)) {
			pr_err("%dkHz out of range\n", f_kHz);
			return 0;
		}

		phy->pll_vco_750M = dphy_range_info[i].pll_vco_750M;
		phy->hstx_ckg_sel = dphy_range_info[i].hstx_ckg_sel;

		if (phy->hstx_ckg_sel <= 7 &&
		    phy->hstx_ckg_sel >= 4)
			q_pll = 0x10 >> (7 - phy->hstx_ckg_sel);

		temp = f_kHz * (u64)q_pll * (u64)ref_clk_ps;
		m_n_int = div64_u64(temp, (u64)1000000000);
		div64_u64_rem(temp, (u64)1000000000, &rem);
		m_n = div64_u64(rem, (u64)100000000);

		if (m_n_int % 2 == 0) {
			if (m_n * 6 >= 50) {
				n_pll = 2;
				m_pll = (m_n_int + 1) * n_pll;
			} else if (m_n * 6 >= 30) {
				n_pll = 3;
				m_pll = m_n_int * n_pll + 2;
			} else {
				n_pll = 1;
				m_pll = m_n_int * n_pll;
			}
		} else {
			if (m_n * 6 >= 50) {
				n_pll = 1;
				m_pll = (m_n_int + 1) * n_pll;
			} else if (m_n * 6 >= 30) {
				n_pll = 1;
				m_pll = (m_n_int + 1) * n_pll;
			} else if (m_n * 6 >= 10) {
				n_pll = 3;
				m_pll = m_n_int * n_pll + 1;
			} else {
				n_pll = 2;
				m_pll = m_n_int * n_pll;
			}
		}

		if (n_pll == 1) {
			phy->pll_fbd_p = 0;
			phy->pll_pre_div1p = 1;
		} else {
			phy->pll_fbd_p = n_pll;
			phy->pll_pre_div1p = 0;
		}

		if (phy->pll_fbd_2p <= 7 && phy->pll_fbd_2p >= 4)
			r_pll = 0x10 >> (7 - phy->pll_fbd_2p);

		if (m_pll == 2) {
			phy->pll_pre_p = 0;
			phy->pll_fbd_s = 0;
			phy->pll_fbd_div1f = 0;
			phy->pll_fbd_div5f = 1;
		} else if (m_pll >= 2 * 2 * r_pll && m_pll <= 2 * 4 * r_pll) {
			phy->pll_pre_p = m_pll / (2 * r_pll);
			phy->pll_fbd_s = 0;
			phy->pll_fbd_div1f = 1;
			phy->pll_fbd_div5f = 0;
		} else if (m_pll >= 2 * 5 * r_pll && m_pll <= 2 * 150 * r_pll) {
			if (((m_pll / (2 * r_pll)) % 2) == 0) {
				phy->pll_pre_p =
					(m_pll / (2 * r_pll)) / 2 - 1;
				phy->pll_fbd_s =
					(m_pll / (2 * r_pll)) % 2 + 2;
			} else {
				phy->pll_pre_p =
					(m_pll / (2 * r_pll)) / 2;
				phy->pll_fbd_s =
					(m_pll / (2 * r_pll)) % 2;
			}
			phy->pll_fbd_div1f = 0;
			phy->pll_fbd_div5f = 0;
		} else {
			phy->pll_pre_p = 0;
			phy->pll_fbd_s = 0;
			phy->pll_fbd_div1f = 0;
			phy->pll_fbd_div5f = 1;
		}

		f_kHz = div64_u64((u64)1000000000 * (u64)m_pll,
			((u64)ref_clk_ps * (u64)n_pll * (u64)q_pll));

		if (f_kHz >= req_kHz)
			break;

		tmp_kHz += 10;

	} while (true);

	return f_kHz;
}

static void
dsi_get_phy_params(u32 phy_req_kHz, struct mipi_phy_params *phy)
{
	u32 ref_clk_ps = PHY_REF_CLK_PERIOD_PS;
	u32 phy_rate_kHz;
	u32 ui;

	memset(phy, 0, sizeof(*phy));

	phy_rate_kHz = dsi_calc_phy_rate(phy_req_kHz, phy);
	if (!phy_rate_kHz)
		return;

	ui = 1000000 / phy_rate_kHz;

	phy->clk_t_lpx = round_div(50, 8 * ui);
	phy->clk_t_hs_prepare = round_div(133, 16 * ui) - 1;

	phy->clk_t_hs_zero = round_div(262, 8 * ui);
	phy->clk_t_hs_trial = 2 * (round_div(60, 8 * ui) - 1);
	phy->clk_t_wakeup = round_div(1000000, (ref_clk_ps / 1000) - 1);
	if (phy->clk_t_wakeup > 0xff)
		phy->clk_t_wakeup = 0xff;
	phy->data_t_wakeup = phy->clk_t_wakeup;
	phy->data_t_lpx = phy->clk_t_lpx;
	phy->data_t_hs_prepare = round_div(125 + 10 * ui, 16 * ui) - 1;
	phy->data_t_hs_zero = round_div(105 + 6 * ui, 8 * ui);
	phy->data_t_hs_trial = 2 * (round_div(60 + 4 * ui, 8 * ui) - 1);
	phy->data_t_ta_go = 3;
	phy->data_t_ta_get = 4;

	phy->pll_enbwt = 1;
	phy->clklp2hs_time = round_div(407, 8 * ui) + 12;
	phy->clkhs2lp_time = round_div(105 + 12 * ui, 8 * ui);
	phy->lp2hs_time = round_div(240 + 12 * ui, 8 * ui) + 1;
	phy->hs2lp_time = phy->clkhs2lp_time;
	phy->clk_to_data_delay = 1 + phy->clklp2hs_time;
	phy->data_to_clk_delay = round_div(60 + 52 * ui, 8 * ui) +
				phy->clkhs2lp_time;

	phy->lane_byte_clk_kHz = phy_rate_kHz / 8;
	phy->clk_division =
		DIV_ROUND_UP(phy->lane_byte_clk_kHz, MAX_TX_ESC_CLK);
}

static u32
dsi_get_dpi_color_coding(enum mipi_dsi_pixel_format format)
{
	u32 val;

	/*
	 * TODO: only support RGB888 now, to support more
	 */
	switch (format) {
	case MIPI_DSI_FMT_RGB888:
		val = DSI_24BITS_1;
		break;
	default:
		val = DSI_24BITS_1;
		break;
	}

	return val;
}

/*
 * dsi phy reg write function
 */
static void
dsi_phy_tst_set(void __iomem *base, u32 reg, u32 val)
{
	u32 reg_write = 0x10000 + reg;

	/*
	 * latch reg first
	 */
	writel(reg_write, base + PHY_TST_CTRL1);
	writel(0x02, base + PHY_TST_CTRL0);
	writel(0x00, base + PHY_TST_CTRL0);

	/*
	 * then latch value
	 */
	writel(val, base + PHY_TST_CTRL1);
	writel(0x02, base + PHY_TST_CTRL0);
	writel(0x00, base + PHY_TST_CTRL0);
}

static void
dsi_set_phy_timer(void __iomem *base, struct mipi_phy_params *phy, u32 lanes)
{
	u32 val;

	/*
	 * Set lane value and phy stop wait time.
	 */
	val = (lanes - 1) | (PHY_STOP_WAIT_TIME << 8);
	writel(val, base + PHY_IF_CFG);

	/*
	 * Set phy clk division.
	 */
	val = readl(base + CLKMGR_CFG) | phy->clk_division;
	writel(val, base + CLKMGR_CFG);

	/*
	 * Set lp and hs switching params.
	 */
	dw_update_bits(base + PHY_TMR_CFG, 24, MASK(8), phy->hs2lp_time);
	dw_update_bits(base + PHY_TMR_CFG, 16, MASK(8), phy->lp2hs_time);
	dw_update_bits(base + PHY_TMR_LPCLK_CFG, 16, MASK(10),
		       phy->clkhs2lp_time);
	dw_update_bits(base + PHY_TMR_LPCLK_CFG, 0, MASK(10),
		       phy->clklp2hs_time);
	dw_update_bits(base + CLK_DATA_TMR_CFG, 8, MASK(8),
		       phy->data_to_clk_delay);
	dw_update_bits(base + CLK_DATA_TMR_CFG, 0, MASK(8),
		       phy->clk_to_data_delay);
}

static void
dsi_set_mipi_phy(void __iomem *base, struct mipi_phy_params *phy, u32 lanes)
{
	u32 delay_count;
	u32 val;
	u32 i;

	/* phy timer setting */
	dsi_set_phy_timer(base, phy, lanes);

	/*
	 * Reset to clean up phy tst params.
	 */
	writel(0, base + PHY_RSTZ);
	writel(0, base + PHY_TST_CTRL0);
	writel(1, base + PHY_TST_CTRL0);
	writel(0, base + PHY_TST_CTRL0);

	/*
	 * Clock lane timing control setting: TLPX, THS-PREPARE,
	 * THS-ZERO, THS-TRAIL, TWAKEUP.
	 */
	dsi_phy_tst_set(base, CLK_TLPX, phy->clk_t_lpx);
	dsi_phy_tst_set(base, CLK_THS_PREPARE, phy->clk_t_hs_prepare);
	dsi_phy_tst_set(base, CLK_THS_ZERO, phy->clk_t_hs_zero);
	dsi_phy_tst_set(base, CLK_THS_TRAIL, phy->clk_t_hs_trial);
	dsi_phy_tst_set(base, CLK_TWAKEUP, phy->clk_t_wakeup);

	/*
	 * Data lane timing control setting: TLPX, THS-PREPARE,
	 * THS-ZERO, THS-TRAIL, TTA-GO, TTA-GET, TWAKEUP.
	 */
	for (i = 0; i < lanes; i++) {
		dsi_phy_tst_set(base, DATA_TLPX(i), phy->data_t_lpx);
		dsi_phy_tst_set(base, DATA_THS_PREPARE(i),
				phy->data_t_hs_prepare);
		dsi_phy_tst_set(base, DATA_THS_ZERO(i), phy->data_t_hs_zero);
		dsi_phy_tst_set(base, DATA_THS_TRAIL(i), phy->data_t_hs_trial);
		dsi_phy_tst_set(base, DATA_TTA_GO(i), phy->data_t_ta_go);
		dsi_phy_tst_set(base, DATA_TTA_GET(i), phy->data_t_ta_get);
		dsi_phy_tst_set(base, DATA_TWAKEUP(i), phy->data_t_wakeup);
	}

	/*
	 * physical configuration: I, pll I, pll II, pll III,
	 * pll IV, pll V.
	 */
	dsi_phy_tst_set(base, PHY_CFG_I, phy->hstx_ckg_sel);
	val = (phy->pll_fbd_div5f << 5) + (phy->pll_fbd_div1f << 4) +
				(phy->pll_fbd_2p << 1) + phy->pll_enbwt;
	dsi_phy_tst_set(base, PHY_CFG_PLL_I, val);
	dsi_phy_tst_set(base, PHY_CFG_PLL_II, phy->pll_fbd_p);
	dsi_phy_tst_set(base, PHY_CFG_PLL_III, phy->pll_fbd_s);
	val = (phy->pll_pre_div1p << 7) + phy->pll_pre_p;
	dsi_phy_tst_set(base, PHY_CFG_PLL_IV, val);
	val = (5 << 5) + (phy->pll_vco_750M << 4) + (phy->pll_lpf_rs << 2) +
		phy->pll_lpf_cs;
	dsi_phy_tst_set(base, PHY_CFG_PLL_V, val);

	writel(PHY_ENABLECLK, base + PHY_RSTZ);
	udelay(1);
	writel(PHY_ENABLECLK | PHY_UNSHUTDOWNZ, base + PHY_RSTZ);
	udelay(1);
	writel(PHY_ENABLECLK | PHY_UNRSTZ | PHY_UNSHUTDOWNZ, base + PHY_RSTZ);
	usleep_range(1000, 1500);

	/*
	 * wait for phy's clock ready
	 */
	delay_count = 100;
	while (delay_count) {
		val = readl(base +  PHY_STATUS);
		if ((BIT(0) | BIT(2)) & val)
			break;

		udelay(1);
		delay_count--;
	}

	if (!delay_count)
		pr_info("phylock and phystopstateclklane is not ready.\n");
}

static void
dsi_set_mode_timing(void __iomem *base, u32 lane_byte_clk_kHz,
		    struct fb_videomode *mode,
		    enum mipi_dsi_pixel_format format)
{
	u32 hfp, hbp, hsw, vfp, vbp, vsw;
	u32 hline_time;
	u32 hsa_time;
	u32 hbp_time;
	u32 pixel_clk_kHz;
	int htot, vtot;
	u32 val;
	u64 tmp;

	val = dsi_get_dpi_color_coding(format);
	writel(val, base + DPI_COLOR_CODING);

	val = (mode->sync & FB_SYNC_HOR_HIGH_ACT ? 0 : 1) << 2;
	val |= (mode->sync & FB_SYNC_VERT_HIGH_ACT ? 0 : 1) << 1;
	writel(val, base +  DPI_CFG_POL);

	/*
	 * The DSI IP accepts vertical timing using lines as normal,
	 * but horizontal timing is a mixture of pixel-clocks for the
	 * active region and byte-lane clocks for the blanking-related
	 * timings.  hfp is specified as the total hline_time in byte-
	 * lane clocks minus hsa, hbp and active.
	 */
	pixel_clk_kHz = mode->pixclock / 1000;
	htot = mode->xres;
	vtot = mode->yres;
	hfp = mode->right_margin;
	hbp = mode->left_margin;
	hsw = mode->hsync_len;
	vfp = mode->lower_margin;
	vbp = mode->upper_margin;
	vsw = mode->vsync_len;
	if (vsw > 15) {
		pr_debug("vsw exceeded 15\n");
		vsw = 15;
	}

	hsa_time = (hsw * lane_byte_clk_kHz) / pixel_clk_kHz;
	hbp_time = (hbp * lane_byte_clk_kHz) / pixel_clk_kHz;
	tmp = (u64)htot * (u64)lane_byte_clk_kHz;
	hline_time = div64_u64(tmp + pixel_clk_kHz - 1, pixel_clk_kHz);

	/* all specified in byte-lane clocks */
	writel(hsa_time, base + VID_HSA_TIME);
	writel(hbp_time, base + VID_HBP_TIME);
	writel(hline_time, base + VID_HLINE_TIME);

	writel(vsw, base + VID_VSA_LINES);
	writel(vbp, base + VID_VBP_LINES);
	writel(vfp, base + VID_VFP_LINES);
	writel(mode->yres, base + VID_VACTIVE_LINES);
	writel(mode->xres, base + VID_PKT_SIZE);

	pr_debug("htot=%d, hfp=%d, hbp=%d, hsw=%d\n",
			 htot, hfp, hbp, hsw);
	pr_debug("vtol=%d, vfp=%d, vbp=%d, vsw=%d\n",
			 vtot, vfp, vbp, vsw);
	pr_debug("hsa_time=%d, hbp_time=%d, hline_time=%d\n",
			 hsa_time, hbp_time, hline_time);
}

static void
dsi_set_video_mode(void __iomem *base, unsigned long flags)
{
	u32 val;
	u32 mode_mask = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
	u32 non_burst_sync_pulse = MIPI_DSI_MODE_VIDEO |
		MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
	u32 non_burst_sync_event = MIPI_DSI_MODE_VIDEO;

	/*
	 * choose video mode type
	 */
	if ((flags & mode_mask) == non_burst_sync_pulse)
		val = DSI_NON_BURST_SYNC_PULSES;
	else if ((flags & mode_mask) == non_burst_sync_event)
		val = DSI_NON_BURST_SYNC_EVENTS;
	else
		val = DSI_BURST_SYNC_PULSES_1;
	writel(val, base + VID_MODE_CFG);

	writel(PHY_TXREQUESTCLKHS, base + LPCLK_CTRL);
	writel(DSI_VIDEO_MODE, base + MODE_CFG);
}

static inline u32
mipi_dsi_pixel_format_to_bpp(enum mipi_dsi_pixel_format format)
{
	switch (format) {
	case MIPI_DSI_FMT_RGB888:
		/* fall through */
	case MIPI_DSI_FMT_RGB666:
		return 24;
	case MIPI_DSI_FMT_RGB666_PACKED:
		return 18;
	case MIPI_DSI_FMT_RGB565:
		return 16;
	default:
		return 0;
	}
}

static void
dsi_mipi_init(struct dw_dsi *dsi)
{
	struct dsi_hw_ctx *ctx = dsi->ctx;
	struct mipi_phy_params *phy = &dsi->phy;
	struct fb_videomode *mode = &dsi->mode;
	u32 bpp = mipi_dsi_pixel_format_to_bpp(dsi->format);
	void __iomem *base = ctx->base;
	u32 dphy_req_kHz;

	/*
	 * count phy params
	 */
	dphy_req_kHz = mode->pixclock / 1000 * bpp / dsi->lanes;
	dsi_get_phy_params(dphy_req_kHz, phy);

	/* reset Core */
	writel(RESET, base + PWR_UP);

	/* set dsi phy params */
	dsi_set_mipi_phy(base, phy, dsi->lanes);

	/* set dsi mode timing */
	dsi_set_mode_timing(base, phy->lane_byte_clk_kHz, mode, dsi->format);

	/* set dsi video mode */
	dsi_set_video_mode(base, dsi->mode_flags);

	/* dsi wake up */
	writel(POWERUP, base + PWR_UP);

	pr_debug("lanes=%d, pixel_clk=%d kHz, bytes_freq=%d kHz\n",
			 dsi->lanes, mode->pixclock, phy->lane_byte_clk_kHz);
}

static int
dsi_encoder_enable(struct platform_device *pdev)
{
	struct dw_dsi *dsi = dev_get_drvdata(&pdev->dev);
	struct dsi_hw_ctx *ctx = dsi->ctx;
	struct reset_control *rst_ctrl;
	int ret;

	if (dsi->enable)
		return 0;

	rst_ctrl = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(rst_ctrl)) {
		dev_err(&pdev->dev, "could not get reset control\n");
		return PTR_ERR(rst_ctrl);
	}

	/* APB clock has to be disabled when releasing reset */
	clk_prepare(ctx->apb_clk);
	clk_disable(ctx->apb_clk);

	ret = reset_control_deassert(rst_ctrl);
	if (ret) {
		dev_err(&pdev->dev, "reset release failed\n");
		return ret;
	}
	clk_enable(ctx->apb_clk);

	ret = clk_prepare_enable(ctx->refclk);
	if (ret) {
		dev_err(&pdev->dev, "could not enable reference clock\n");
		return ret;
	}

	ret = clk_prepare_enable(ctx->pclk);
	if (ret) {
		pr_err("fail to enable pclk: %d\n", ret);
		return 0;
	}

	dsi_mipi_init(dsi);

	dsi->enable = true;

	return 0;
}

static int
dsi_encoder_disable(struct platform_device *pdev, pm_message_t state)
{
	struct dw_dsi *dsi = dev_get_drvdata(&pdev->dev);
	struct dsi_hw_ctx *ctx = dsi->ctx;
	void __iomem *base = ctx->base;

	if (!dsi->enable)
		return 0;

	writel(0, base + PWR_UP);
	writel(0, base + LPCLK_CTRL);
	writel(0, base + PHY_RSTZ);
	clk_disable_unprepare(ctx->pclk);

	dsi->enable = false;

	return 0;
}

enum init_state {
	INIT_CMD,
	INIT_NUM_PARAMS,
	INIT_PARAMS,
	INIT_MDELAY,
	INIT_UDELAY,
	INIT_RST,
};

static int
dw_dsi_parse_display_sequence(struct dw_dsi *dev, const char *sequence)
{
	struct device_node *pnode =
		of_parse_phandle(dev->parent_dev->of_node, "init-sequence", 0);
	struct property *prop;
	u32 key;
	const __be32 *p;
	enum init_state state = INIT_CMD;
	struct dsi_cmd cmd;
	unsigned cur_param = 0;

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
			if (key > DW_DSI_MAX_PARAMS)
				return -EINVAL;
			cmd.num_params = key + 1;
			if (cmd.num_params > 1) {
				state = INIT_PARAMS;
			} else {
				dw_dsi_send(dev, &cmd);
				state = INIT_CMD;
			}
			break;
		case INIT_PARAMS:
			if (cur_param >= cmd.num_params)
				return -EIO;
			cmd.params[cur_param++] = key;
			if (cur_param == cmd.num_params) {
				dw_dsi_send(dev, &cmd);
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
			else
				dev_warn(dev->parent_dev, "no have panel reset\n");
			state = INIT_CMD;
			break;
		}
	}

	of_node_put(pnode);

	return 0;
}

static int
dsi_parse_dt(struct platform_device *pdev, struct dw_dsi *dsi)
{
	struct dsi_hw_ctx *ctx = dsi->ctx;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *video_input;
	struct resource *res;
	int ret = 0;

	ctx->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(ctx->pclk)) {
		dev_err(&pdev->dev, "failed to get pclk clock\n");
		return PTR_ERR(ctx->pclk);
	}

	ctx->apb_clk = devm_clk_get(&pdev->dev, "apb");
	if (IS_ERR(ctx->apb_clk)) {
		dev_err(&pdev->dev, "failed to get pclk clock\n");
		return PTR_ERR(ctx->apb_clk);
	}

	ctx->refclk = devm_clk_get(&pdev->dev, "refclk");
	if (IS_ERR(ctx->refclk)) {
		dev_err(&pdev->dev, "failed to get pclk clock\n");
		return PTR_ERR(ctx->refclk);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ctx->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ctx->base)) {
		dev_err(&pdev->dev, "failed to remap dsi io region\n");
		return PTR_ERR(ctx->base);
	}

	dsi->reset_gpio_panel = of_get_named_gpio(np, "panel-reset", 0);
	if (gpio_is_valid(dsi->reset_gpio_panel)) {
		ret = devm_gpio_request(&pdev->dev, dsi->reset_gpio_panel,
					"dsi-panel-rst");
		if (ret) {
			dev_err(&pdev->dev, "failed to request panel reset\n");
			return ret;
		}
		gpio_direction_output(dsi->reset_gpio_panel, 0);
	}

	ret = of_property_read_u32(np, "dsi-lanes", &dsi->lanes);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"invalid or missing 'dsi-lanes' property\n");
		return -EIO;
	}

	ret = of_property_read_u32(np, "dsi-format", &dsi->format);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"invalid or missing 'dsi-format' property\n");
		return -EIO;
	}

	video_input = of_parse_phandle(np, "video-in", 0);
	if (!video_input) {
		dev_err(&pdev->dev, "could not parse 'video-in' phandle\n");
		return -EIO;
	}

	ret = of_property_read_u32(video_input, "dspg,bits_per_pixel",
				   &dsi->bpp);
	if (ret < 0 || (dsi->bpp != 16 && dsi->bpp != 18 && dsi->bpp != 24)) {
		dev_err(&pdev->dev, "invalid 'dspg,bits_per_pixel'\n");
		ret = -EIO;
		goto put;
	}

	ret = of_property_read_u32_array(video_input, "dspg,mode",
					 &dsi->mode.refresh, 13);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"invalid or missing 'dspg,mode' property!\n");
		goto put;
	}

	if (!dsi->mode.pixclock) {
		dsi->mode.pixclock =
			(dsi->mode.xres + dsi->mode.left_margin +
			 dsi->mode.right_margin + dsi->mode.hsync_len) *
			(dsi->mode.yres + dsi->mode.upper_margin +
			 dsi->mode.lower_margin + dsi->mode.vsync_len) *
			dsi->mode.refresh;
		if (!dsi->mode.pixclock)
			ret = -EIO;
	}
put:
	of_node_put(video_input);

	return ret;
}

static int
dsi_probe(struct platform_device *pdev)
{
	struct dsi_data *data;
	struct dw_dsi *dsi;
	struct dsi_hw_ctx *ctx;
	int ret;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("failed to allocate dsi data.\n");
		return -ENOMEM;
	}
	dsi = &data->dsi;
	ctx = &data->ctx;
	dsi->ctx = ctx;
	dsi->parent_dev = &pdev->dev;

	ret = dsi_parse_dt(pdev, dsi);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, data);

	dsi_encoder_enable(pdev);

	/* TODO set low power/command mode? */
	dw_dsi_parse_display_sequence(dsi, "reset-seq");
	dw_dsi_parse_display_sequence(dsi, "power-on");

	return 0;
}

static int
dsi_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id dsi_of_match[] = {
	{.compatible = "snps,dw-mipi-dsi-kirin"},
	{ }
};
MODULE_DEVICE_TABLE(of, dsi_of_match);

static struct platform_driver dsi_driver = {
	.probe		= dsi_probe,
	.remove		= dsi_remove,
	.suspend	= dsi_encoder_disable,
	.resume		= dsi_encoder_enable,
	.driver = {
		.name		= "kirin-dsi",
		.owner		= THIS_MODULE,
		.of_match_table	= dsi_of_match,
	},
};
module_platform_driver(dsi_driver);

MODULE_AUTHOR("Xinliang Liu <xinliang.liu@linaro.org>");
MODULE_AUTHOR("Xinliang Liu <z.liuxinliang@hisilicon.com>");
MODULE_AUTHOR("Xinwei Kong <kong.kongxinwei@hisilicon.com>");
MODULE_DESCRIPTION("DesignWare MIPI DSI Host Controller v1.02 driver");
MODULE_LICENSE("GPL v2");
