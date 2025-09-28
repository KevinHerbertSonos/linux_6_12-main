// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2024 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/stddef.h>
#include <linux/export.h>
#include <linux/kernel.h>

#include "ctypes.h"
#include "avio_io.h"
#include "hal_dhub.h"
#include "drv_dhub.h"
#include "Galois_memmap.h"
#include "avioGbl.h"

void dhub2nd_suspend(void *hdl, int enable)
{
	DHUB_CTX *hDhubCtx = (DHUB_CTX *)hdl;
	unsigned int vppSramPwrCtl;
	unsigned int aioSramPwrCtl;
	unsigned int avioGblCtrl;

	AVIO_REG_WORD32_READ(hDhubCtx->avio_gbl_base + RA_avioGbl_VPP128bDHUB_SRAMPWR + RA_SRAMPWR_ctrl, &vppSramPwrCtl);
	AVIO_REG_WORD32_READ(hDhubCtx->avio_gbl_base + RA_avioGbl_AIO64bDHUB_SRAMPWR + RA_SRAMPWR_ctrl, &aioSramPwrCtl);
	AVIO_REG_WORD32_READ(hDhubCtx->avio_gbl_base +  RA_avioGbl_CTRL, &avioGblCtrl);

	if (enable) {
		AVIO_REG_WORD32_WRITE(hDhubCtx->avio_gbl_base + RA_avioGbl_VPP128bDHUB_SRAMPWR + RA_SRAMPWR_ctrl,
			(vppSramPwrCtl | 0x7));
		AVIO_REG_WORD32_WRITE(hDhubCtx->avio_gbl_base + RA_avioGbl_AIO64bDHUB_SRAMPWR + RA_SRAMPWR_ctrl,
			(aioSramPwrCtl | 0x7));
		AVIO_REG_WORD32_WRITE(hDhubCtx->avio_gbl_base + RA_avioGbl_CTRL,
			(avioGblCtrl | 0x5));
	} else {
		AVIO_REG_WORD32_WRITE(hDhubCtx->avio_gbl_base + RA_avioGbl_VPP128bDHUB_SRAMPWR + RA_SRAMPWR_ctrl,
			(vppSramPwrCtl & ~0x7));
		AVIO_REG_WORD32_WRITE(hDhubCtx->avio_gbl_base + RA_avioGbl_AIO64bDHUB_SRAMPWR + RA_SRAMPWR_ctrl,
			(aioSramPwrCtl & ~0x7));
		AVIO_REG_WORD32_WRITE(hDhubCtx->avio_gbl_base +  RA_avioGbl_CTRL,
			(avioGblCtrl & ~0x5));
	}
}