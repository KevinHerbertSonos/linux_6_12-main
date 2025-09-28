/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __BERLIN_UITL_H__
#define __BERLIN_UITL_H__

#include <linux/kernel.h>
#include <sound/soc.h>
#include <sound/control.h>
#include "i2s_dai.h"

u32 berlin_get_dfm(u32 word_w);
u32 berlin_get_cfm(u32 word_s);
int berlin_set_pll(void *aio_handle, u32 apll_id, u32 fs);
u32 berlin_get_div(u32 fs);
u32 berlin_get_sample_resolution(u32 word_w);
u32 berlin_get_sample_period_in_bclk(u32 word_s);
u32 berlin_get_bclk_div(u32 fs, u32 bclk);
#endif
