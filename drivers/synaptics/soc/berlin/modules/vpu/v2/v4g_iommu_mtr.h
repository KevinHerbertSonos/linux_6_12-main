// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 * Author: Hsia-Jun(Randy) Li <randy.li@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __V4G_IOMMU_MTR_H_
#define __V4G_IOMMU_MTR_H_

#include <linux/module.h>

#define BERLIN_VPU_MTR_ENABLE
#define BERLIN_VPU_IOMMU_EANBLE


#ifdef BERLIN_VPU_MTR_ENABLE

#define SYNA_V4G_PM_W_LOG2(modifier)	((modifier >> 24UL) & 0xfUL)
#define SYNA_V4G_PM_H_LOG2(modifier)	((modifier >> 28UL) & 0xfUL)

#ifdef BERLIN_VPU_IOMMU_EANBLE
#define SYNA_V4G_CFG_FROM_PIX_MOD(modifier, mmu_cfg, dec_cfg) \
	do { \
		dec_cfg->mtr_enable = ((modifier >> 36UL) & 0x1UL); \
		mmu_cfg->en = 1; \
		if ((modifier >> 20UL) & 0x1UL) { \
			mmu_cfg->page_mapping = 1; \
			mmu_cfg->pm_w_log2_minus6 = SYNA_V4G_PM_W_LOG2(modifier) - 6; \
			mmu_cfg->pm_h_log2_minus2 = SYNA_V4G_PM_H_LOG2(modifier) - 2; \
		} \
	} while (0)

#else /* BERLIN_VPU_IOMMU_EANBLE == 0*/
#define SYNA_V4G_CFG_FROM_PIX_MOD(modifier, mmu_cfg, dec_cfg) \
	do { \
		dec_cfg->mtr_enable = ((modifier >> 36UL) & 0x1UL); \
		mmu_cfg->en = 0; \
		if ((modifier >> 20UL) & 0x1UL) { \
			mmu_cfg->page_mapping = 1; \
			mmu_cfg->pm_w_log2_minus6 = SYNA_V4G_PM_W_LOG2(modifier) - 6; \
			mmu_cfg->pm_h_log2_minus2 = SYNA_V4G_PM_H_LOG2(modifier) - 2; \
		} \
	} while (0)
#endif //BERLIN_VPU_IOMMU_EANBLE

#else /* BERLIN_VPU_MTR_ENABLE == 0 */
#define SYNA_V4G_PM_W_LOG2(modifier)	(0)
#define SYNA_V4G_PM_H_LOG2(modifier)	(0)

#ifdef BERLIN_VPU_IOMMU_EANBLE
#define SYNA_V4G_CFG_FROM_PIX_MOD(modifier, mmu_cfg, dec_cfg) \
	do { \
		dec_cfg->mtr_enable = 0; \
		mmu_cfg->en = 1; \
		mmu_cfg->page_mapping = 0; \
		mmu_cfg->pm_w_log2_minus6 = 0; \
		mmu_cfg->pm_h_log2_minus2 = 0; \
	} while (0)

#else /* BERLIN_VPU_IOMMU_EANBLE == 0 */
#define SYNA_V4G_CFG_FROM_PIX_MOD(modifier, mmu_cfg, dec_cfg) \
	do { \
		dec_cfg->mtr_enable = 0; \
		mmu_cfg->en = 0; \
		mmu_cfg->page_mapping = 0; \
		mmu_cfg->pm_w_log2_minus6 = 0; \
		mmu_cfg->pm_h_log2_minus2 = 0; \
	} while (0)

#endif //BERLIN_VPU_IOMMU_EANBLE
#endif //BERLIN_VPU_MTR_ENABLE

#endif //__V4G_IOMMU_MTR_H_