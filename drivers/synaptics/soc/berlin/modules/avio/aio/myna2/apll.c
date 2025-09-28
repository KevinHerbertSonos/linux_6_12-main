// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "aio_hal.h"
#include "aio_common.h"

static struct mclk_info clk_data[] = {
	{AIO_APLL_0, AIO_APLL_OUT, 294912000, AIO_CLK_D3_SWITCH_NOR, 12, 32, 24576000, \
	{12000, 24000, 48000, 96000, 192000}},
	{AIO_APLL_0, AIO_APLL_OUT, 294912000, AIO_CLK_D3_SWITCH_NOR,  6, 32, 49152000, \
	{384000}},
	{AIO_APLL_0, AIO_APLL_OUT, 294912000, AIO_CLK_D3_SWITCH_NOR, 18, 32, 16384000, \
	{8000, 16000, 32000, 64000}},
	{AIO_APLL_1, AIO_APLL_OUT, 270950400, AIO_CLK_D3_SWITCH_NOR,  6, 32, 45158400, \
	{11025, 22050, 44100, 88200, 176400}},
};

int aio_i2s_mclk_cfg(u32 fs, u32 sample_period, struct mclk_info **mclk)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(clk_data); ++i) {
		for (j = 0; j < ARRAY_SIZE(clk_data[i].sample_rate); ++j) {
			if (clk_data[i].sample_rate[j] && fs == clk_data[i].sample_rate[j]) {
				*mclk = &clk_data[i];
				return 0;
			}
		}
	}
	return -EINVAL;
}
EXPORT_SYMBOL(aio_i2s_mclk_cfg);
