#ifndef __DW_DRM_DSI_H
#define __DW_DRM_DSI_H

#include <linux/types.h>
#include <linux/fb.h>

#include <drm/drm_mipi_dsi.h>

#define DW_DSI_MAX_PARAMS	200

struct mipi_phy_params {
	u32 clk_t_lpx;
	u32 clk_t_hs_prepare;
	u32 clk_t_hs_zero;
	u32 clk_t_hs_trial;
	u32 clk_t_wakeup;
	u32 data_t_lpx;
	u32 data_t_hs_prepare;
	u32 data_t_hs_zero;
	u32 data_t_hs_trial;
	u32 data_t_ta_go;
	u32 data_t_ta_get;
	u32 data_t_wakeup;
	u32 hstx_ckg_sel;
	u32 pll_fbd_div5f;
	u32 pll_fbd_div1f;
	u32 pll_fbd_2p;
	u32 pll_enbwt;
	u32 pll_fbd_p;
	u32 pll_fbd_s;
	u32 pll_pre_div1p;
	u32 pll_pre_p;
	u32 pll_vco_750M;
	u32 pll_lpf_rs;
	u32 pll_lpf_cs;
	u32 clklp2hs_time;
	u32 clkhs2lp_time;
	u32 lp2hs_time;
	u32 hs2lp_time;
	u32 clk_to_data_delay;
	u32 data_to_clk_delay;
	u32 lane_byte_clk_kHz;
	u32 clk_division;
};

struct dsi_hw_ctx {
	void __iomem *base;
	struct clk *pclk;
	struct clk *refclk;
	struct clk *apb_clk;
};

struct dw_dsi {
	struct fb_videomode		mode;
	struct dsi_hw_ctx		*ctx;
	struct mipi_phy_params		phy;
	int				reset_gpio_panel;
	struct device			*parent_dev;
	u32				timeout;

	u32				lanes;
	int				bpp;
	enum mipi_dsi_pixel_format	format;
	unsigned long			mode_flags;
	bool				enable;
	u8				cmd_buf[DW_DSI_MAX_PARAMS];
};

struct dsi_data {
	struct dw_dsi dsi;
	struct dsi_hw_ctx ctx;
};

struct dsi_phy_range {
	u32 min_range_kHz;
	u32 max_range_kHz;
	u32 pll_vco_750M;
	u32 hstx_ckg_sel;
};

static const struct dsi_phy_range dphy_range_info[] = {
	{   46875,    62500,   1,    7 },
	{   62500,    93750,   0,    7 },
	{   93750,   125000,   1,    6 },
	{  125000,   187500,   0,    6 },
	{  187500,   250000,   1,    5 },
	{  250000,   375000,   0,    5 },
	{  375000,   500000,   1,    4 },
	{  500000,   750000,   0,    4 },
	{  750000,  1000000,   1,    0 },
	{ 1000000,  1500000,   0,    0 }
};

#endif	/* __DW_DRM_DSI_H */
