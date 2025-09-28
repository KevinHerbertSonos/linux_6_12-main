/*
* INTERNAL USE ONLY
*
* Unpublished Work Copyright (C) 2013-2017 Synaptics Incorporated.
* All rights reserved.
*
* This file contains information that is proprietary to Synaptics
* Incorporated ("Synaptics"). The holder of this file shall treat all
* information contained herein as confidential, shall use the
* information only for its intended purpose, and shall not duplicate,
* disclose, or disseminate any of this information in any manner
* unless Synaptics has otherwise provided express, written
* permission.
*
* Use of the materials may require a license of intellectual property
* from a third party or from Synaptics. This file conveys no express
* or implied licenses to any intellectual property rights belonging
* to Synaptics.
*/
#include "includes.h"
#include "dsih_core.h"
#include "DPHYTX.h"

void mipi_dphy_BiuCtrlPHYEn(dphy_t *phy, int en);
void mipi_dphy_shutdown(dphy_t *phy, int shutdown);
void mipi_dphy_resetz(dphy_t *phy, int resetz);
void mipi_dphy_enable_lanes(dphy_t *phy, int lanes);
void mipi_dphy_EnableClkBIU(dphy_t *phy, int en);
void mipi_dphy_CfgClkFreqRange(dphy_t *phy, int range);
void mipi_dphy_pll_shadow_control_en(dphy_t *phy, int en);
void mipi_dphy_pll_clksel(dphy_t *phy, int clksel);
void mipi_dphy_stopstate_wait(dphy_t *phy, int lanes);


void mipi_dphy_BiuCtrlPHYEn(dphy_t *phy, int en)
{
	T32DPHYTX_DPHY_CTL0 ctrl;

	ioread32(phy->base + RA_DPHYTX_DPHY_CTL0, &ctrl.u32);
	ctrl.uDPHY_CTL0_BiuCtrlPhyEn = en;
	iowrite32(phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);

	return;
}

void mipi_dphy_shutdown(dphy_t *phy, int shutdown)
{
	T32DPHYTX_DPHY_CTL1 ctrl;

	ioread32(phy->base + RA_DPHYTX_DPHY_CTL1, &ctrl.u32);
	ctrl.uDPHY_CTL1_shutdownz = shutdown;
	iowrite32(phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
	return;
}

void mipi_dphy_resetz(dphy_t *phy, int resetz)
{
	T32DPHYTX_DPHY_CTL1 ctrl;

	ioread32(phy->base + RA_DPHYTX_DPHY_CTL1, &ctrl.u32);
	ctrl.uDPHY_CTL1_rstz = resetz;
	iowrite32(phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
	return;
}

void mipi_dphy_enable_lanes(dphy_t *phy, int lanes)
{
	int lane_mask = 0x1;
	T32DPHYTX_DPHY_CTL1 ctrl;

	ioread32(phy->base + RA_DPHYTX_DPHY_CTL1, &ctrl.u32);

	if (lanes & lane_mask) {
	   ctrl.uDPHY_CTL1_enable_0 =  1;
	} else {
		ctrl.uDPHY_CTL1_enable_0 =  0;
	}

	lane_mask = (lane_mask << 1);
	if (lanes & lane_mask) {
		ctrl.uDPHY_CTL1_enable_1 =  1;
	} else {
		ctrl.uDPHY_CTL1_enable_1 =  0;
	}

	lane_mask = (lane_mask << 1);
	if (lanes & lane_mask) {
		ctrl.uDPHY_CTL1_enable_2 =  1;
	} else {
		ctrl.uDPHY_CTL1_enable_2 =  0;
	}

	lane_mask = (lane_mask << 1);
	if (lanes & lane_mask) {
		ctrl.uDPHY_CTL1_enable_1 =  1;
	} else {
		ctrl.uDPHY_CTL1_enable_1 =  0;
	}

	iowrite32(phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);

	return;
}

void mipi_dphy_EnableClkBIU(dphy_t *phy, int en)
{
	T32DPHYTX_DPHY_CTL1 ctrl;

	ioread32(phy->base + RA_DPHYTX_DPHY_CTL1, &ctrl.u32);
	ctrl.uDPHY_CTL1_enableclkBIU = en;
	iowrite32( phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
	return;
}

void mipi_dphy_CfgClkFreqRange(dphy_t *phy, int range)
{
	T32DPHYTX_DPHY_CTL1 ctrl;

	ioread32( phy->base + RA_DPHYTX_DPHY_CTL1, &ctrl.u32);
	ctrl.uDPHY_CTL1_cfgclkfreqrange = range;
	iowrite32( phy->base + RA_DPHYTX_DPHY_CTL1, ctrl.u32);
	return;
}

void mipi_dphy_pll_shadow_control_en(dphy_t *phy, int en)
{
	T32DPHYTX_DPHY_PLL2 ctrl;

	ioread32(phy->base + RA_DPHYTX_DPHY_PLL2, &ctrl.u32);
	ctrl.uDPHY_PLL2_pll_shadow_control = en;
	iowrite32( phy->base + RA_DPHYTX_DPHY_PLL2, ctrl.u32);
}

void mipi_dphy_pll_clksel(dphy_t *phy, int clksel)
{
	T32DPHYTX_DPHY_PLL2 ctrl;

	ioread32(phy->base + RA_DPHYTX_DPHY_PLL2, &ctrl.u32);
	ctrl.uDPHY_PLL2_clksel = clksel;
	iowrite32( phy->base + RA_DPHYTX_DPHY_PLL2, ctrl.u32);
}

void mipi_dphy_stopstate_wait(dphy_t *phy, int lanes)
{
	T32DPHYTX_DPHY_RB0 rb0, cond;
	unsigned int wait = 0xF00;

	lanes = (1<<lanes)-1;
	cond.u32 = 0;
	cond.uDPHY_RB0_stopstateclk = 1;
	cond.uDPHY_RB0_stopstatedata_0 = (lanes & 0x1)?1:0;
	cond.uDPHY_RB0_stopstatedata_1 = (lanes & 0x2)?1:0;
	cond.uDPHY_RB0_stopstatedata_2 = (lanes & 0x4)?1:0;
	cond.uDPHY_RB0_stopstatedata_3 = (lanes & 0x8)?1:0;
	do {
		ioread32(phy->base + RA_DPHYTX_DPHY_RB0, &rb0.u32);
		rb0.u32 = (rb0.u32 & 0x3D000);
		if (!wait)
			break;
		wait = wait - 1;
	} while (rb0.u32 != cond.u32);
}
