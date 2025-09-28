// SPDX-License-Identifier: GPL-2.0
/* Copyright 2017 Synopsys Inc. */
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-phy.h"
#include "hrx-phy-fw.h"
#include "hrx-reg.h"

#define HDMIRX_PHY_FW_CONTENT_CHECK	0x8AA7
#define PHY_MAGIC_IDCODE			0x1C3834CD

/* TODO: Review FW version
 *
 * PHY FW Version: 6.01a_sup2
 * Date:           Mar 6, 2020
 */

static void hrx_enable_phy_ints(struct syna_hrx_v4l2_dev *hrx_dev, bool enable)
{
	if (enable) {
		hrx_reg_mask_write(hrx_dev, 0x1,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_TIMEOUT_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_TIMEOUT_MASK);
		hrx_reg_mask_write(hrx_dev, 0x1,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_READ_DONE_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_READ_DONE_MASK);
		hrx_reg_mask_write(hrx_dev, 0x1,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_WRITE_DONE_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_WRITE_DONE_MASK);
		hrx_reg_mask_write(hrx_dev, 0x1,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_SELECTIONMODE_DONE_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_SELECTIONMODE_DONE_MASK);
	} else {
		hrx_reg_mask_write(hrx_dev, 0x0,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_TIMEOUT_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_TIMEOUT_MASK);
		hrx_reg_mask_write(hrx_dev, 0x0,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_READ_DONE_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_READ_DONE_MASK);
		hrx_reg_mask_write(hrx_dev, 0x0,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_WRITE_DONE_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_WRITE_DONE_MASK);
		hrx_reg_mask_write(hrx_dev, 0x0,
			HDMI_MAINUNIT_2_INT_MASK_N,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_SELECTIONMODE_DONE_OFFSET,
			HDMI_MAINUNIT_2_INT_MASK_N_PHYCREG_CR_SELECTIONMODE_DONE_MASK);
	}
}

void hrx_phy_tmds_clock_ratio(struct syna_hrx_v4l2_dev *hrx_dev, bool enable)
{
	hrx_reg_mask_write(hrx_dev, enable,
		HDMI_PHY_CONFIG,
		HDMI_PHY_CONFIG_TMDS_CLOCK_RATIO_OFFSET,
		HDMI_PHY_CONFIG_TMDS_CLOCK_RATIO_MASK);
	hrx_dev->is_hdmi2 = enable;
}

static void hrx_phy_reset(struct syna_hrx_v4l2_dev *hrx_dev, bool enable)
{
	hrx_reg_mask_write(hrx_dev, enable,
		HDMI_PHY_CONFIG,
		HDMI_PHY_CONFIG_RESET_OFFSET,
		HDMI_PHY_CONFIG_RESET_MASK);
	hrx_dev->phy_reset = enable;
}

static int hrx_phy_configure_cr_select(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int timeout = 1000;
	u32 cr_selmode_done;

	hrx_reg_mask_write(hrx_dev, 3,
		HDMI_PHYCREG_CONFIG0,
		HDMI_PHYCREG_CR_PARA_SELECTION_MODE_OFFSET,
		HDMI_PHYCREG_CR_PARA_SELECTION_MODE_MASK);

	cr_selmode_done = hrx_reg_mask_read(hrx_dev, HDMI_MAINUNIT_2_INT_STATUS,
						HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_SELECTIONMODE_DONE_OFFSET,
						HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_SELECTIONMODE_DONE_MASK);
	while (!cr_selmode_done && timeout--) {
		mdelay(5);
		cr_selmode_done = hrx_reg_mask_read(hrx_dev, HDMI_MAINUNIT_2_INT_STATUS,
							HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_SELECTIONMODE_DONE_OFFSET,
							HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_SELECTIONMODE_DONE_MASK);
	}

	if (!cr_selmode_done)
		return -ETIMEDOUT;

	hrx_reg_mask_write(hrx_dev, 1,
		HDMI_MAINUNIT_2_INT_CLEAR,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_SELECTIONMODE_DONE_OFFSET,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_SELECTIONMODE_DONE_MASK);
	return 0;
}

static u32 hrx_phy_reg_read(struct syna_hrx_v4l2_dev *hrx_dev, u32 addr)
{
	T32HDMI_RX_PHYCREG_CONFIG1 PhyCreg_Cfg1;
	T32HDMI_RX_PHYCREG_CONTROL PhyCreg_Ctrl;
	T32HDMI_RX_PHYCREG_STATUS PhyCreg_Stat;
	u32 PhyCregCrReadDone = 0;
	int timeout = 100;

	hrx_reg_mask_write(hrx_dev, 1,
		HDMI_MAINUNIT_2_INT_CLEAR,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_READ_DONE_OFFSET,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_READ_DONE_MASK);

	PhyCreg_Cfg1.reg = hrx_reg_read(hrx_dev, HDMI_PHYCREG_CONFIG1);
	PhyCreg_Cfg1.uPHYCREG_CONFIG1_cr_para_addr = addr;
	hrx_reg_write(hrx_dev, PhyCreg_Cfg1.reg, HDMI_PHYCREG_CONFIG1);

	udelay(10);
	PhyCreg_Ctrl.reg = hrx_reg_read(hrx_dev, HDMI_PHYCREG_CONTROL);
	PhyCreg_Ctrl.uPHYCREG_CONTROL_cr_para_WRITE_P = 0;
	PhyCreg_Ctrl.uPHYCREG_CONTROL_cr_para_READ_P = 1;
	hrx_reg_write(hrx_dev, PhyCreg_Ctrl.reg, HDMI_PHYCREG_CONTROL);

	udelay(10);
	PhyCregCrReadDone = hrx_reg_mask_read(hrx_dev, HDMI_MAINUNIT_2_INT_STATUS,
		HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_READ_DONE_OFFSET,
		HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_READ_DONE_MASK);
	while (!PhyCregCrReadDone && timeout--) {
		udelay(10);
		PhyCregCrReadDone = hrx_reg_read(hrx_dev, HDMI_MAINUNIT_2_INT_STATUS) &
								HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_READ_DONE_MASK;
	}
	PhyCreg_Stat.reg = hrx_reg_read(hrx_dev, HDMI_PHYCREG_STATUS);
	return PhyCreg_Stat.uPHYCREG_STATUS_cr_para_rd_data;
}

static void hrx_phy_reg_write(struct syna_hrx_v4l2_dev *hrx_dev, u32 addr, u32 data)
{
	T32HDMI_RX_PHYCREG_CONFIG1 PhyCreg_Cfg1;
	T32HDMI_RX_PHYCREG_CONFIG2 PhyCreg_Cfg2;
	T32HDMI_RX_PHYCREG_CONTROL PhyCreg_Ctrl;
	u32 PhyCregCrWriteDone = 0;
	int timeout = 100;

	hrx_reg_mask_write(hrx_dev, 1, HDMI_MAINUNIT_2_INT_CLEAR,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_WRITE_DONE_OFFSET,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_WRITE_DONE_MASK);
	PhyCreg_Cfg1.reg = 0;
	PhyCreg_Cfg1.uPHYCREG_CONFIG1_cr_para_addr = addr;
	hrx_reg_write(hrx_dev, PhyCreg_Cfg1.reg, HDMI_PHYCREG_CONFIG1);

	PhyCreg_Cfg2.reg = 0;
	PhyCreg_Cfg2.uPHYCREG_CONFIG2_cr_para_wr_data = data;
	hrx_reg_write(hrx_dev, PhyCreg_Cfg2.reg, HDMI_PHYCREG_CONFIG2);

	PhyCreg_Ctrl.reg = 0;
	PhyCreg_Ctrl.uPHYCREG_CONTROL_cr_para_WRITE_P = 1;
	PhyCreg_Ctrl.uPHYCREG_CONTROL_cr_para_READ_P = 0;
	hrx_reg_write(hrx_dev, PhyCreg_Ctrl.reg, HDMI_PHYCREG_CONTROL);

	PhyCregCrWriteDone = hrx_reg_mask_read(hrx_dev,
		HDMI_MAINUNIT_2_INT_STATUS,
		HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_WRITE_DONE_OFFSET,
		HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_WRITE_DONE_MASK);
	while (!PhyCregCrWriteDone && timeout--) {
		udelay(50);
		PhyCregCrWriteDone = hrx_reg_read(hrx_dev, HDMI_MAINUNIT_2_INT_STATUS) &
								HDMI_MAINUNIT_2_STATUS_PHYCREG_CR_WRITE_DONE_MASK;
	}
	hrx_reg_mask_write(hrx_dev, 1,
		HDMI_MAINUNIT_2_INT_CLEAR,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_WRITE_DONE_OFFSET,
		HDMI_MAINUNIT_2_CLEAR_PHYCREG_CR_WRITE_DONE_MASK);
}

static void hrx_phy_hdmi_disable(struct syna_hrx_v4l2_dev *hrx_dev, u32 enable)
{
	hrx_reg_mask_write(hrx_dev, enable,
		HDMI_PHY_CONFIG,
		HDMI_PHY_CONFIG_HDMI_DISABLE_OFFSET,
		HDMI_PHY_CONFIG_HDMI_DISABLE_MASK);
}

static bool hrx_phy_hdmi_disable_ack(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return hrx_reg_read(hrx_dev, HDMI_PHY_STATUS) &
		HDMI_PHY_STATUS_HDMI_DISABLE_ACK;
}

static void hrx_phy_pddq(struct syna_hrx_v4l2_dev *hrx_dev, u32 enable)
{
	hrx_reg_mask_write(hrx_dev, enable,
		HDMI_PHY_CONFIG,
		HDMI_PHY_CONFIG_PDDQ_OFFSET,
		HDMI_PHY_CONFIG_PDDQ_MASK);
}

static bool hrx_phy_pddq_ack(struct syna_hrx_v4l2_dev *hrx_dev)
{

	return hrx_reg_read(hrx_dev, HDMI_PHY_STATUS) & HDMI_PHY_STATUS_PDDQ_ACK;
}

static void hrx_phy_reffreq(struct syna_hrx_v4l2_dev *hrx_dev, u32 val)
{
	hrx_reg_mask_write(hrx_dev, val,
		HDMI_PHY_CONFIG,
		HDMI_PHY_CONFIG_REFFREQ_SEL_OFFSET,
		HDMI_PHY_CONFIG_REFFREQ_SEL_MASK);
}

static void hrx_phy_rxdata_width(struct syna_hrx_v4l2_dev *hrx_dev, u32 val)
{
	hrx_reg_mask_write(hrx_dev, val,
		HDMI_PHY_CONFIG,
		HDMI_PHY_CONFIG_RXDATA_WIDTH_OFFSET,
		HDMI_PHY_CONFIG_RXDATA_WIDTH_MASK);
}

int hrx_phy_init(struct syna_hrx_v4l2_dev *hrx_dev, bool data_rate_6g, bool full_config)
{
	int timeout, val, ret;
	u32 tmp, index, extPhyAdd;
	T32HDMI_RX_WRAP_PHY_SRAM_CTRL_STATUS PhySramCtrlStat;
	T32HDMI_RX_WRAP_PHY_SRAM_CTRL PhySramCtrl;

	HRX_LOG(HRX_DRV_INFO, "configuring phy: Is_data_rate_6g=%d\n", data_rate_6g);
	hrx_phy_tmds_clock_ratio(hrx_dev, data_rate_6g);
	if (!full_config) {
		HRX_LOG(HRX_DRV_DEBUG, "not programming full PHY\n");
		return 0;
	}
	switch (hrx_dev->phy_cfg_clk) {
	case 24:
		val = 0x0;
		break;
	case 25:
		val = 0x1;
		break;
	case 27:
		val = 0x2;
		break;
	case 48:
		val = 0x3;
		break;
	case 50:
		val = 0x4;
		break;
	case 54:
		val = 0x5;
		break;
	case 100:
		val = 0x6;
		break;
	default:
		HRX_LOG(HRX_DRV_ERROR, "unsupported cfgclk (%d)\n", hrx_dev->phy_cfg_clk);
		return -EINVAL;
	}

	hrx_phy_reset(hrx_dev, 0);

	hrx_reg_mask_write(hrx_dev, 0x1,
		HDMI_GLOBAL_SWENABLE,
		HDMI_GLOBAL_SWENABLE_PHYCTRL_ENABLE_OFFSET,
		HDMI_GLOBAL_SWENABLE_PHYCTRL_ENABLE_MASK);

	hrx_enable_phy_ints(hrx_dev, true);

	ret = hrx_phy_configure_cr_select(hrx_dev);
	if (ret)
		HRX_LOG(HRX_DRV_ERROR, "Failed to configure cr selection ret =%d\n", ret);

	HRX_LOG(HRX_DRV_DEBUG, "PHY fw Check 0x%04X\n", hrx_phy_reg_read(hrx_dev, 0x6001));

	if (hrx_phy_reg_read(hrx_dev, 0x6001) == HDMIRX_PHY_FW_CONTENT_CHECK) {
		hrx_phy_hdmi_disable(hrx_dev, 1);
		mdelay(20);
		hrx_phy_hdmi_disable(hrx_dev, 0);

		/* Wait */
		timeout = 500;
		while (timeout-- && hrx_phy_hdmi_disable_ack(hrx_dev))
			mdelay(20);
		if (timeout < 0) {
			HRX_LOG(HRX_DRV_ERROR, "failed to wait for hdmi_disable_ack\n");
			return -ETIMEDOUT;
		}
		HRX_LOG(HRX_DRV_INFO, "PHY fw already loaded - don't load again\n");
		goto phy_exit;
	}

	hrx_phy_hdmi_disable(hrx_dev, 1);
	hrx_phy_pddq(hrx_dev, 1);

	if (full_config)
		hrx_phy_reset(hrx_dev, 1);

	mdelay(100);

	hrx_phy_rxdata_width(hrx_dev, 1);
	hrx_phy_reffreq(hrx_dev, val);
	hrx_phy_reset(hrx_dev, 0);
	mdelay(100);

	/* Sanity Check */
	tmp = hrx_phy_reg_read(hrx_dev, 0x0) | (hrx_phy_reg_read(hrx_dev, 0x1) << 16);
	if (tmp != PHY_MAGIC_IDCODE) {
		HRX_LOG(HRX_DRV_INFO, "Invalid ID code 0x%x (expected 0x%x)\n",
			tmp, PHY_MAGIC_IDCODE);
		//return -EINVAL;
	}
	HRX_LOG(HRX_DRV_INFO, "PHY ID code 0x%x (expected 0x%x)\n",
		tmp, PHY_MAGIC_IDCODE);

	timeout = 100;
	HRX_REG_WORD32_READ(HDMIRX_WRAP_REG_BASE + RA_HDMI_RX_WRAP_PHY_SRAM_CTRL_STATUS, &PhySramCtrlStat.reg);
	while ((!PhySramCtrlStat.uPHY_SRAM_CTRL_STATUS_sram_init_done) && timeout--) {
		mdelay(5);
		HRX_REG_WORD32_READ(HDMIRX_WRAP_REG_BASE + RA_HDMI_RX_WRAP_PHY_SRAM_CTRL_STATUS, &PhySramCtrlStat.reg);
	}

	HRX_LOG(HRX_DRV_INFO, "sram_init_done = %d\n", PhySramCtrlStat.uPHY_SRAM_CTRL_STATUS_sram_init_done);
	HRX_LOG(HRX_DRV_INFO, "PHY fw Check 0x%04X\n", hrx_phy_reg_read(hrx_dev, 0x6001));

	if (hrx_phy_reg_read(hrx_dev, 0x6001) != HDMIRX_PHY_FW_CONTENT_CHECK) {
		for (extPhyAdd = 0x6000, index = 0; extPhyAdd <= 0x6FFF ; extPhyAdd++, index++)
			hrx_phy_reg_write(hrx_dev, extPhyAdd, hrx_phy_fw[index]);
	}

	HRX_LOG(HRX_DRV_INFO, "Loaded new PHY FW\n");
	HRX_LOG(HRX_DRV_INFO, "PHY fw Check 0x%04X\n", hrx_phy_reg_read(hrx_dev, 0x6001));
	HRX_REG_WORD32_READ(HDMIRX_WRAP_REG_BASE + RA_HDMI_RX_WRAP_PHY_SRAM_CTRL, &PhySramCtrl.reg);
	PhySramCtrl.uPHY_SRAM_CTRL_sram_ext_ld_done = 1;
	HRX_REG_WORD32_WRITE(HDMIRX_WRAP_REG_BASE + RA_HDMI_RX_WRAP_PHY_SRAM_CTRL, PhySramCtrl.reg);
	mdelay(10);

	/* Leave configuration mode */
	hrx_phy_pddq(hrx_dev, 0);
	timeout = 1000;
	while (timeout-- && hrx_phy_pddq_ack(hrx_dev))
		mdelay(10);
	if (timeout < 0) {
		HRX_LOG(HRX_DRV_ERROR, "failed to wait for pddq_ack\n");
		return -ETIMEDOUT;
	}

	/* Enable Reception */
	hrx_phy_hdmi_disable(hrx_dev, 0);

	/* Wait */
	timeout = 500;
	while (timeout-- && hrx_phy_hdmi_disable_ack(hrx_dev))
		mdelay(20);
	if (timeout < 0) {
		HRX_LOG(HRX_DRV_ERROR, "failed to wait for hdmi_disable_ack\n");
		return -ETIMEDOUT;
	}

phy_exit:
	hrx_dev->is_hdmi2 = data_rate_6g;
	hrx_enable_phy_ints(hrx_dev, false);
	return 0;
}

void hrx_phy_re_init(struct syna_hrx_v4l2_dev *hrx_dev)
{
	HRX_LOG(HRX_DRV_INFO, "Re-initialize phy\n");

	hrx_phy_pddq(hrx_dev, 1);
	mdelay(2);
	hrx_phy_pddq(hrx_dev, 0);
}

bool has_clock(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u8 clock_status, i;

	clock_status = !(hrx_reg_read(hrx_dev, HDMI_CMU_STATUS) &
			HDMI_CMU_STATUS_TMDSQP_CK_OFF);
	if (!clock_status) {
		hrx_phy_tmds_clock_ratio(hrx_dev, hrx_is_hdmi2(hrx_dev));
		for (i = 0; i < 4; i++) {
			mdelay(5);
			clock_status = !(hrx_reg_read(hrx_dev, HDMI_CMU_STATUS) &
					HDMI_CMU_STATUS_TMDSQP_CK_OFF);
			HRX_LOG(HRX_DRV_DEBUG, "iter = %d clock_status = %d\n", i, clock_status);
			if (clock_status)
				break;
		}
	}
	return clock_status;
}

bool has_audio_clock(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return !(hrx_reg_read(hrx_dev, HDMI_CMU_STATUS) &
			HDMI_CMU_STATUS_AUDIO_CK_OFF);
}

bool is_scrambled(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return hrx_reg_read(hrx_dev, HDMI_SCDC_REGBANK_STATUS1) &
			HDMI_SCDC_REGBANK_STATUS1_SCRAMB_EN;
}

unsigned int is_frl(struct syna_hrx_v4l2_dev *hrx_dev)
{
	return hrx_reg_mask_read(hrx_dev, HDMI_SCDC_REGBANK_STATUS2,
			HDMI_SCDC_REGBANK_STATUS2_FRL_RATE_OFFSET,
			HDMI_SCDC_REGBANK_STATUS2_FRL_RATE_MASK);
}

u32 hrx_get_tmds_clk(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u32 tmds_clk = hrx_reg_read(hrx_dev, HDMI_CMU_TMDSQP_COUNT) * 1000;

	if (!is_frl(hrx_dev))
		tmds_clk *= 4;

	HRX_LOG(HRX_DRV_DEBUG, "tmds_clk = %d\n", tmds_clk);
	return tmds_clk;
}
