// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "avio_dhub_cfg.h"
#include "hrx-aip-hal.h"
#include "hrx-drv.h"
#include "avio_dhub_drv.h"

#define CPU_ID 0

void aip_dhub_dataclear(struct syna_hrx_v4l2_dev *hrx_dev)
{
	if (hrx_dev->eSourceType == AIP_SOURCE_AVIF)
		DhubChannelClear(hrx_dev->dhub, avioDhubChMap_aio64b_MIC3_CH_W, NULL);
}

void aip_dhub_intr_enable(struct syna_hrx_v4l2_dev *hrx_dev, int enable)
{
	u32 chanId;
	HDL_semaphore *pSemHandle;
	u32 uiInstate;

	if (hrx_dev->eSourceType == AIP_SOURCE_AVIF) {
		chanId = avioDhubChMap_aio64b_MIC3_CH_W;
		pSemHandle = dhub_semaphore(hrx_dev->dhub);
		//Enable AVIF DHUB Interrupt
		if (!enable) {
			uiInstate = semaphore_chk_full(pSemHandle, chanId);
			if (uiInstate) {
				semaphore_pop(pSemHandle, chanId, 1);
				semaphore_clr_full(pSemHandle, chanId);
				HRX_LOG(AIP_INFO, "semaphore_pop and clr_full for intr_id = %d\n", chanId);
			}
		}
		HRX_LOG(AIP_INFO, "%s %d %s intr chanId =0x%x\n", __func__, __LINE__,
				enable ? "enabled":"disabled", chanId);
		semaphore_intr_enable(
				pSemHandle,     // semaphore handler
				chanId,
				0,              // empty
				enable ? 1:0,   // full
				0,              // almost_empty
				0,              // almost_full
				CPU_ID          // 0~2, depending on which CPU the interrupt is enabled for.
				);
	}
}
