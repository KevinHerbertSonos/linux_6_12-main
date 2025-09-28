/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include "ispbe_api.h"

void ISPSS_BE_DNSCL3_Probe(struct ISPBE_CA_DRV_CTX *drv_ctx);
void ISPSS_BE_TILER_Probe(struct ISPBE_CA_DRV_CTX *drv_ctx);
void ISPSS_BE_DEWARP_Probe(struct ISPBE_CA_DRV_CTX *drv_ctx);

#define DNS_INPUT_FMT_NUM        8
#define DNS_OUTPUT_FMT_NUM       8

#endif
