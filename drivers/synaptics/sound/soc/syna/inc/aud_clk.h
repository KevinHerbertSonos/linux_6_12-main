/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __I2S_CLK_TREE_H__
#define __I2S_CLK_TREE_H__

#include <sound/soc.h>
#include "aio_hal.h"

enum aud_clk_id {
	I2S_CLK_ID_INVALID,
	I2S_CLK_ID_I2S1,
	I2S_CLK_ID_I2S2,
	I2S_CLK_ID_I2S3,
	I2S_CLK_ID_I2S4,
	I2S_CLK_ID_I2S5,
	I2S_CLK_ID_MAX,
};

enum i2s_xfeed_clk {
	AIO_I2S_XFEED_I2S1_DI = 0,
	AIO_I2S_XFEED_I2S1_LPBK = 1,
	AIO_I2S_XFEED_I2S2_DI = 2,
	AIO_I2S_XFEED_I2S2_LPBK = 3,
	AIO_I2S_XFEED_I2S3_DI = 4,
	AIO_I2S_XFEED_I2S3_LPBK = 5,
	AIO_I2S_XFEED_I2S4_DI = 6,
	AIO_I2S_XFEED_I2S4_LPBK = 7,
	AIO_I2S_XFEED_I2S5_DI = 8,
	AIO_I2S_XFEED_I2S5_LPBK = 9,
};

enum aud_clk_state {
	AIO_I2S_CLK_NOT_USED,
	AIO_I2S_CLK_READY,
	AIO_I2S_CLK_CONFIGURED,
	AIO_I2S_CLK_IN_USE,
	AIO_I2S_CLK_CLOSED,
};

enum aio_bclk_src {
	AIO_BCLK_SRC_APLL = 0, /*master*/
	AIO_BCLK_SRC_XFEED, /*MS*/
	AIO_BCLK_SRC_PAD, /*slave*/
};

struct aud_clk_cfg {
	struct aud_ctrl aCtrl;
	enum aio_bclk_src bclk_src_sel;
};

struct aud_clk_ops {
	int (*config)(struct snd_pcm_substream *ss); //configure
	int (*start)(struct snd_pcm_substream *ss);
	int (*stop)(struct snd_pcm_substream *ss);
	bool (*isInUse)(struct snd_pcm_substream *ss);
};

struct aud_clk {
	enum aud_clk_id clkId;
	enum aud_clk_state clkstate;
	bool useTxClk;
	u32 ref_count;
	u32 clk_chid;
	struct aud_clk_cfg clkCfg;
	struct aud_clk_ops ops;
	void *clk_soc_dai;
};

void *aud_clk_add(enum aud_clk_id clkId, void *clk_i2s_dev);
bool aud_clk_remove(enum aud_clk_id clk);
struct aud_clk *aud_get_clk(enum aud_clk_id clk);
void aud_set_aprkt_en_flag(bool bSetReset, u32 value);
u32 aud_get_aprkt_en_flag(void);
#endif //__I2S_CLK_TREE_H__
