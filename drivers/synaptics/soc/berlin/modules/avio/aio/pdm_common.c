// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/atomic.h>
#include <linux/printk.h>
#include <linux/err.h>

#include "aio_common.h"
#include "aio_hal.h"
#include "pdm_common.h"
#include "avio_sub_module.h"

int __weak aio_get_pdm_offset(u32 chanid)
{
	return -EINVAL;
}

int aio_pdm_ch_mute(void *hd, u32 chanid, bool mute)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset;
	int pdmch_offset;
	T32PDMCH_CTRL reg;

	pdmch_offset = aio_get_pdm_offset(chanid);
	if (pdmch_offset == -EINVAL)
		return -EINVAL;
	offset = pdmch_offset + RA_PDMCH_CTRL;

	reg.u32 = aio_read(aio, offset);
	reg.uCTRL_MUTE = mute;
	aio_write(aio, offset, reg.u32);

	return 0;
}
EXPORT_SYMBOL(aio_pdm_ch_mute);

int aio_pdm_rdfd_set(void *hd, u32 chanid, u16 rdlt, u16 fdlt)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset;
	int pdmch_offset;
	T32PDMCH_CTRL2 reg;

	pdmch_offset = aio_get_pdm_offset(chanid);
	if (pdmch_offset == -EINVAL)
		return -EINVAL;
	offset = pdmch_offset + RA_PDMCH_CTRL2;

	reg.u32 = aio_read(aio, offset);
	reg.uCTRL2_RDLT = rdlt;
	reg.uCTRL2_FDLT = fdlt;
	aio_write(aio, offset, reg.u32);

	return 0;
}
EXPORT_SYMBOL(aio_pdm_rdfd_set);

int aio_pdm_interleaved_mode_en(void *hd, u32 chanid, bool enable)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset;
	int ret = 0;
	T32PDM_INTLMODE reg;

	offset = RA_AIO_PDM + RA_PDM_INTLMODE;
	reg.u32 = aio_read(aio, offset);

	switch (chanid) {
	case AIO_MIC_CH0:
		reg.uINTLMODE_PORT0_EN = enable;
		break;
	case AIO_MIC_CH1:
		reg.uINTLMODE_PORT1_EN = enable;
		break;
	case AIO_MIC_CH2:
		reg.uINTLMODE_PORT2_EN = enable;
		break;
	case AIO_MIC_CH3:
		reg.uINTLMODE_PORT3_EN = enable;
		break;
	default:
		dev_err(aio->dev, "illegal pdm channd id %d\n", chanid);
		ret = -EINVAL;
		return ret;
	}

	aio_write(aio, offset, reg.u32);

	return ret;
}
EXPORT_SYMBOL(aio_pdm_interleaved_mode_en);

void aio_pdm_ctrl1_set(void *hd, u8 div, bool invclk_int,
			u8 rlsb, u8 rdm, u8 mode, u8 latch_mode)
{
	struct aio_priv *aio = hd_to_aio(hd);
	u32 offset;
	T32PDM_CTRL1 reg;

	offset = RA_AIO_PDM + RA_PDM_CTRL1;
	reg.u32 = aio_read(aio, offset);
	reg.uCTRL1_CLKDIV = div;
	reg.uCTRL1_INVCLK_INT = invclk_int;
	reg.uCTRL1_RLSB = rlsb;
	reg.uCTRL1_RDM = rdm;
	reg.uCTRL1_MODE = mode;
	reg.uCTRL1_LATCH_MODE = latch_mode;
	aio_write(aio, offset, reg.u32);
}

int aio_pdm_get_div(u32 fs)
{

	u32 div;

	// update bc_rates if any rates are added or removed.
	switch (fs) {
	case 8000:
	case 11025:
	case 12000:
		// normally AIO_DIV32 but we doubled the decimation factor
		div = AIO_DIV16;
		break;
	case 16000:
	case 22050:
	case 24000:
		// normally AIO_DIV16 but we doubled the decimation factor
		div = AIO_DIV8;
		break;
	case 32000:
	case 44100:
	case 48000:
	case 64000:
	case 88200:
	case 96000:
		div = AIO_DIV8;
		break;
	default:
		div = AIO_DIV8;
		break;
	}

	return div;
}

void __weak aio_pdm_set_ctrl1_div(void *hd, u32 fs)
{
	u32 div;

	div = aio_pdm_get_div(fs);
	aio_pdm_ctrl1_set(hd, div, 1, 1, 4, 0, 0);
}
EXPORT_SYMBOL(aio_pdm_set_ctrl1_div);

void __weak xfeed_set_pdm_sel(void *hd, u32 pdm_clk_sel, u32 pdmc_sel, u32 pdm_sel) { }
