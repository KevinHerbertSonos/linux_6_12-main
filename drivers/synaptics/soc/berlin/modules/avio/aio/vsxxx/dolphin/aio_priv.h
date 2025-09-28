/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _AIO_PRIV_H_
#define _AIO_PRIV_H_

#define AIO_MIC_CH_NUM (AIO_MIC_CH3 + 1)
#define MAX_DMIC_MODULES (4)

// PCM channel number configuration
#define MIN_CHANNELS (2)
#define MAX_CHANNELS (8)

// Default PCM channel map
#define PCM_DEF_CHL_MAP (0xFF)

//Suspend-Resume ctrl yet to be implemented
#define AIO_PLL_POWER_CTRL(__on)	do {} while (0)

#define AVIO_DEFAULT_GBL_CONFIG \
	_AVIO_GBL_REG_(RA_avioGbl_AVPLL_CTRL0, 0x00), \
	_AVIO_GBL_REG_(RA_avioGbl_APLL_WRAP0, 0x260), \
	_AVIO_GBL_REG_(RA_avioGbl_APLL_WRAP1, 0x260), \
	_AVIO_GBL_REG_(RA_avioGbl_AVPLLA_CLK_EN, 0x4D), \
	_AVIO_GBL_REG_(RA_avioGbl_CTRL1, 0x2400),

// PDM to DHUB write size in bytes while in interleave mode & dummy byte inserted
#define PDM_DHUB_OCPF_CHUNK_SZ (32)
#define IS_INVALID_ID(id) ((id) > AIO_ID_MAX_NUM)

#define DMIC_CLK_TABLE_CONFIG                                                  \
	{ 8000, 196608000, 95, 64, 0, 1, 48, 1 },                              \
		{ 11025, 180633600, 63, 256, 0, 1, 8, 1 },                     \
		{ 16000, 196608000, 47, 192, 0, 1, 8, 1 },                     \
		{ 22050, 180633600, 31, 128, 0, 1, 8, 1 },                     \
		{ 32000, 196608000, 23, 96, 0, 1, 8, 1 },                      \
		{ 44100, 180633600, 15, 64, 0, 1, 8, 1 },                      \
		{ 48000, 196608000, 15, 64, 0, 1, 8, 1 },                      \
		{ 64000, 196608000, 11, 48, 0, 1, 8, 1 },                      \
		{ 88200, 180633600, 7, 32, 0, 1, 8, 1 },                       \
		{ 96000, 196608000, 7, 32, 0, 1, 8, 1 },

u32 get_intl_base_addr_extra(u32 id);
u32 aio_get_offset_extra_plus(u32 ID, u32 tsd);
u32 aio_get_addr_extra_plus(u32 id);
#endif
