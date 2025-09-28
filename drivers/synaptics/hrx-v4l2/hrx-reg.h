// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef HRX_REG_H
#define HRX_REG_H

#include "hrx-drv.h"

#define HRX_REG_WORD32_READ GA_REG_WORD32_READ
#define HRX_REG_WORD32_WRITE GA_REG_WORD32_WRITE

#define GLB_REG_READ32		HRX_REG_WORD32_READ
#define GLB_REG_WRITE32		HRX_REG_WORD32_WRITE


void hrx_reg_write(struct syna_hrx_v4l2_dev *hrx_dev, u32 reg, u32 val);
u32 hrx_reg_read(struct syna_hrx_v4l2_dev *hrx_dev, u32 reg);
void hrx_reg_mask_write(struct syna_hrx_v4l2_dev *hrx_dev, u32 data,
	u32 reg, u32 shift, u32 mask);
u32 hrx_reg_mask_read(struct syna_hrx_v4l2_dev *hrx_dev, u32 reg,
	u32 shift, u32 mask);
u32 hrx_reg_get_int_val(struct syna_hrx_v4l2_dev *hrx_dev, u32 stat_reg,
	u32 mask_reg);
void vip_reg_write(struct syna_hrx_v4l2_dev *hrx_dev, u32 reg, u32 val);
u32 vip_reg_read(struct syna_hrx_v4l2_dev *hrx_dev, u32 reg);
void glb_reg_write(u32 reg, u32 val);
u32 glb_reg_read(u32 reg);

typedef struct _tag_HRX_REG_INFO {
	char *addr_name;
	UINT32 addr;
} HRX_REG_INFO;

void hrx_dump_reg(struct seq_file *s);

#endif //HRX_REG_H
