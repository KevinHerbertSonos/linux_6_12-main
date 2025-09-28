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
#ifndef __DPHY_WRAP_H__
#define __DPHY_WRAP_H__

#include "dsih_core.h"

enum{
	PLL_OUTPUT_CLOCK_STOP = 0,
	PLL_OUTPUT_CLOCK_GENERATE = 1,
	PLL_OUTPUT_CLOCK_EXTERNAL = 2,
	PLL_OUTPUT_CLOCK_FORBIDDEN = 3,
};

void mipi_dphy_pll_clksel(dphy_t *phy, int clksel);
void mipi_dphy_pll_shadow_control_en(dphy_t *phy, int en);
void mipi_dphy_CfgClkFreqRange(dphy_t *phy, int range);
void mipi_dphy_stopstate_wait(dphy_t *phy, int lanes);
void mipi_dphy_shutdown(dphy_t *phy, int shutdown);

#endif
