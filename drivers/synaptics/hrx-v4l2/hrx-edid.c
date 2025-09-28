// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "edid.h"

#include "hrx-edid.h"

#define EDID_SIZE 256
// #define PRINT_EDID

static int syna_hrx_enable_edid_ctrl(struct syna_hrx_v4l2_dev *hrx_dev)
{
	T32EDID_I2C_CTRL reg;

	reg.u32 = readl(hrx_dev->edid_regs + RA_EDID_I2C_CTRL);
	reg.uI2C_CTRL_slv_addr = 0x50;
	reg.uI2C_CTRL_en = 1;
	reg.uI2C_CTRL_cpu_wr_done_ac_dis = 1;
	writel(reg.u32, hrx_dev->edid_regs + RA_EDID_I2C_CTRL);
	return 0;
}

static int syna_hrx_enable_edid_readforall(struct syna_hrx_v4l2_dev *hrx_dev)
{
	T32EDID_I2C_CTRL reg;

	reg.u32 = readl(hrx_dev->edid_regs + RA_EDID_I2C_CTRL);
	reg.uI2C_CTRL_en = 1;
	reg.uI2C_CTRL_cpu_wr_done =  1;
	reg.uI2C_CTRL_cpu_wr_done_ac_dis = 1;
	writel(reg.u32, hrx_dev->edid_regs + RA_EDID_I2C_CTRL);
	return 0;
}

static int syna_hrx_enable_edid_write_done(struct syna_hrx_v4l2_dev *hrx_dev)
{
	T32EDID_I2C_CTRL reg;

	reg.u32 = readl(hrx_dev->edid_regs + RA_EDID_I2C_CTRL);
	reg.uI2C_CTRL_slv_addr = 0x50;
	reg.uI2C_CTRL_en = 1;
	reg.uI2C_CTRL_cpu_wr_done =  1;
	writel(reg.u32, hrx_dev->edid_regs + RA_EDID_I2C_CTRL);
	return 0;
}

static int syna_hrx_read_edid_from_dt(struct syna_hrx_v4l2_dev *hrx_dev, u8 *edid)
{
	const u8 *edid_addr;
	int size;

	edid_addr = of_get_property(dev_of_node(hrx_dev->dev),
		"edid", &size);
	if ((edid_addr == NULL) || (EDID_SIZE != size)) {
		HRX_LOG(HRX_DRV_ERROR, "Failed to get edid property from devicetree\n");
		return -1;
	}
	else {
		memcpy(edid, edid_addr, EDID_SIZE);
	}

	return 0;
}

void syna_hrx_read_edid(struct syna_hrx_v4l2_dev *hrx_dev, u8 *edid)
{
	int i = 0;

	for (i = 0; i < EDID_SIZE; i++)
		edid[i] = (u8) readl(hrx_dev->edid_regs + (4*i));

#ifdef PRINT_EDID
	syna_hrx_print_edid(edid);
#endif
}

void syna_hrx_write_edid(struct syna_hrx_v4l2_dev *hrx_dev, u8 *edid)
{
	int i = 0;

	syna_hrx_enable_edid_ctrl(hrx_dev);

	for (i = 0; i < EDID_SIZE; i++)
		writel(edid[i], hrx_dev->edid_regs + (4*i));

	syna_hrx_enable_edid_write_done(hrx_dev);
	syna_hrx_enable_edid_readforall(hrx_dev);
}

void syna_hrx_write_default_edid(struct syna_hrx_v4l2_dev *hrx_dev)
{
	u8 edid[EDID_SIZE];

	if (syna_hrx_read_edid_from_dt(hrx_dev, edid)!= 0) {
		HRX_LOG(HRX_DRV_ERROR, "could not write default edid\n");
		return;
	}
	syna_hrx_write_edid(hrx_dev, edid);
}

void syna_hrx_print_edid(u8 *edid)
{
	int i = 0;

	for (i = 0; i < EDID_SIZE; i += 16) {
		HRX_LOG(HRX_DRV_INFO, "EDID\n");
		HRX_LOG(HRX_DRV_INFO, "[%2d]: %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n", i,
			edid[i], edid[i+1], edid[i+2], edid[i+3], edid[i+4], edid[i+5], edid[i+6], edid[i+7],
			edid[i+8], edid[i+9], edid[i+10], edid[i+11], edid[i+12], edid[i+13], edid[i+14], edid[i+15]);
	}
}

int syna_hrx_validate_edid(u8 *edid)
{
	int i = 0, checksum = 0;

	if (!edid)
		goto error;

	for (i = 0; i < 256; i++)
		checksum += edid[i];

	if ((0 == (checksum % 256)) && (checksum != 0))
		return 1;

error:
	return 0;
}
