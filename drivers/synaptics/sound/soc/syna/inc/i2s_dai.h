/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */

#ifndef __I2S_H__
#define __I2S_H__

#include "aio_hal.h"
#include "aud_clk.h"

#define I2S_CLK_SRC_KCONTROL_NAME "clk_src"
#define I2S_XFEED_CLK_KCONTROL_NAME "clk_feed"
#define I2S_MCLK_DIV_KCONTROL_NAME "mclk_div"
#define I2S_MODE_KCONTROL_NAME "mode"
#define I2S_LPBK_KCONTROL_NAME "loopback"
#define I2S_SYNC_START_KCONTROL_NAME "sync_start"
#define I2S_USE_MM_MODE_KCONTROL_NAME "use_mm_mode"

enum aio_i2s_io {
	AIO_I2S_IO_TX,
	AIO_I2S_IO_RX,
	AIO_I2S_IO_MAX
};

enum aio_i2s_bclk_src {
	AIO_I2S_BCLK_SRC_EXTERNAL = 0x00,
	AIO_I2S_BCLK_SRC_MCLK = 0x01,
};

enum aio_i2s_rx_data_mode {
	AIO_I2S_RX_DATA_IN_PAD = 0x00,
	AIO_I2S_RX_DATA_LPBK_NEAR_PAD = 0x01,
};

enum aio_i2s_pause_resume {
	AIO_I2S_STATE_DISABLED = 0,
	AIO_I2S_STATE_ACTIVE = 1,
	AIO_I2S_STATE_RESUME = 2,
	AIO_I2S_STATE_PAUSE = 3,
	AIO_I2S_STATE_STOP = 4,
};

enum aio_i2s_run_mode {
	AIO_I2S_SET_STOP = 0x00,
	AIO_I2S_SET_START = 0x01,
	AIO_I2S_SET_START_ON_PAUSE = 0x02,
	AIO_I2S_SET_PAUSE = 0x03,
	AIO_I2S_SET_RESUME = 0x04,
};

struct i2s_io_params {
	unsigned int i2s_irq;
	u32 i2s_chid;
	struct aud_clk *clkrt;
	bool irq_requested;
	enum aio_i2s_pause_resume statePauseResume;
};

struct i2s_soc_dai {
	struct device *dev;
	const char *dev_name;
	u32 mode;
	struct aud_ctrl ctrl;
	bool is_master;
	bool useMMMode;
	bool output_mclk;
	bool disable_mic_mute;
	/*  sample_period: sample period in terms of bclk numbers.
	 *  Typically 32 is used. For some pcm mono format, 16 may be used
	 */
	int  sample_period;
	void *aio_handle;
	bool bUsePauseResume;
	u32 eAPRKTMode;
	u32 xFeedClkCfg;
	enum aio_bclk_src clkSrcSel;
	enum aio_i2s_rx_data_mode rxDataMode;
	enum aud_clk_id nodeClkId;
	struct i2s_io_params io_params[AIO_I2S_IO_MAX];
};
int i2s_soc_dai_run(void *i2s_hdl, bool isTx,
						enum aio_i2s_run_mode runMode, u32 uPSC);
void i2s_soc_dai_set_bclk(struct i2s_soc_dai *i2s_dev, u32 id, bool is_master, bool inv);
#endif //__I2S_H__
