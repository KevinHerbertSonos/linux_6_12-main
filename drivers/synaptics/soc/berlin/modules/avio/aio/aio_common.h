/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2019-2020 Synaptics Incorporated */
#ifndef _AIO_COMMON_H_
#define _AIO_COMMON_H_
#include "drv_aio.h"
#include "aio_hal.h"

#define	CutTo(x, b)	((x) & (bSETMASK(b) - 1))

struct aio_handle {
	char name[64];
	void *aio;
};

int aio_read(struct aio_priv *aio, u32 offset);
void aio_write(struct aio_priv *aio, u32 offset, u32 val);
int aio_read_gbl(struct aio_priv *aio, u32 offset);
void aio_write_gbl(struct aio_priv *aio, u32 offset, u32 val);
struct aio_priv *hd_to_aio(void *hd);
struct aio_priv *get_aio(void);
int aio_i2s_mclk_cfg(u32 fs, u32 tcf, struct mclk_info **mclk);
int aio_misc_enable_audio_timer(void *hd, bool en);
int aio_misc_get_audio_timer(void *hd, u32 *val);
int aio_misc_enable_sampinfo(void *hd, u32 idx, bool en);
int aio_misc_set_sampinfo_req(void *hd, u32 idx, bool en);
int aio_misc_get_audio_counter(void *hd, u32 idx, u32 *c);
int aio_misc_get_audio_timestamp(void *hd, u32 idx, u32 *t);
int aio_misc_sw_rst(void *hd, u32 option, u32 val);
int aio_misc_set_loopback_clk_gate(void *hd, u32 idx, u32 en);
int aio_misc_sw_rst_extra(void *hd, u32 option, u32 val);
int aio_misc_set_loopback_clk_gate_extra(void *hd, u32 idx, u32 en);
int aio_spdifi_enable_refclk(void *hd);
#endif
