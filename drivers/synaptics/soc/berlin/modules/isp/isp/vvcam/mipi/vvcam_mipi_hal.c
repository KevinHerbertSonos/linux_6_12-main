// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
/****************************************************************************
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 VeriSilicon Holdings Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************
 *
 * The GPL License (GPL)
 *
 * Copyright (c) 2023 VeriSilicon Holdings Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 *****************************************************************************
 *
 * Note: This software is released under dual MIT and GPL licenses. A
 * recipient may use this file under the terms of either the MIT license or
 * GPL License. If you wish to use only one license not the other, you can
 * indicate your decision by deleting one of the above license notices in your
 * version of this file.
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "vvcam_mipi_driver.h"

/**
 *TPG_ISP_RST 0:reset 1:enable
 *TPG_CSI_RST 0:reset 1:enable
 *TPG_FRAME_GATE_MASK 0:ON 1:OFF
 *TPG_MUX_S 0:TPG 1:SENSOR
 */
#define TPG_ISP_RST_SHIFT              24
#define TPG_ISP_RST_MASK               0x01000000
#define TPG_CSI_RST_SHIFT              28
#define TPG_CSI_RST_MASK               0x10000210
#define TPG_FRAME_GATE_SHIFIT          30
#define TPG_FRAME_GATE_MASK            0x40000000
#define TPG_MUX_S_SHIFT                29
#define TPG_MUX_S_MASK                 0x20000000

/**
 *CSI_CTRL_MUX_T 0:self csi 1:other csi
 *CSI_CTRL_SHIFT_MODE 0:independent mode 1:other csi dependent mode
 */
#define CSI_CTRL_MUX_T_SHIFT           31
#define CSI_CTRL_MUX_T_MASK            0x80000000
#define CSI_CTRL_SHIFT_MODE_SHIFT      28
#define CSI_CTRL_SHIFT_MODE_MASK       0x10000000
#define CSI_CTRL_VC0_CTRL_SHIFIT       0
#define CSI_CTRL_VC0_CTRL_MASK         0x00000007
#define CSI_CTRL_VC1_CTRL_SHIFIT       8
#define CSI_CTRL_VC1_SHIFT_MASK        0x00000700
#define CSI_CTRL_VC2_CTRL_SHIFIT       16
#define CSI_CTRL_VC2_SHIFT_MASK        0x00070000
#define CSI_CTRL_VC3_CTRL_SHIFIT       24
#define CSI_CTRL_VC3_SHIFT_MASK        0x07000000

/**
 * MRV_CSI2_NUM_LANES
 * Config num lanes register [3:0] rw
 * 0000b - controller off
 * 0001b - 1 Lane
 * 0010b - 2 Lanes
 * 0011b - 3 Lanes
 * 0100b - 4 Lanes
 */
#define MRV_CSI2_NUM_LANES 0x0

/**
 * MRV_CSI2_LANES_CLK_EN
 * Configure lanes clock [0]
 * 0b - disable
 * 1b - enable
 */
#define MRV_CSI2_LANES_CLK_EN 0x4

/**
 * MRV_CSI2_LANES_DATA_EN
 * enable/disable lanes data [7:0]
 * setting bits to a '1' value enable data lane
 *[0]-data lane 0
 *[1]-data lane 1
 *[2]-data lane 2
 *[3]-data lane 3
 *[4]-data lane 4
 *[5]-data lane 5
 *[6]-data lane 6
 *[7]-data lane 7
 */
#define MRV_CSI2_LANES_DATA_EN 0x8

/**
 * MRV_CSI2_IGNORE_VC
 * Set this input causes the interface to ignore the Virtual Channel
 */
#define MRV_CSI2_IGNORE_VC        0x80
#define MRV_CSI2_FIFO_SEND_LEVEL  0x88
#define MRV_CSI2_VID_VSYNC        0x8c
#define MRV_CSI2_VID_HSYNC_FP     0x90
#define MRV_CSI2_VID_HSYNC        0x94
#define MRV_CSI2_VID_HSYNC_BP     0x98

#define IS_VALID_ID(id) (id != -1)

static vvcam_input_dev_attr_t default_config = {
    .mode = VVCAM_INPUT_MODE_MIPI,
    .image_rect = {
        .left = 0,
        .top =0,
        .width = 1920,
        .height = 1080,
    },
    .mipi_attr = {
        .itf_cfg[0] = {
            .enable = true,
            .data_type = VVCAM_RAW_12BIT,
        },
        .lane_id  = {0, 1, 2, 3, -1, -1, -1, -1},
    },
};

static void vvcam_mipi_write_reg(void __iomem *base,
			uint32_t addr, uint32_t value)
{
	writel(value, base + addr);
}

static uint32_t vvcam_mipi_read_reg(void __iomem *base, uint32_t addr)
{
	return readl(base + addr);
}

static int vvcam_mipi_csi2_cfg(struct vvcam_mipi_dev *mipi_dev,
                            vvcam_input_dev_attr_t mipi_cfg)
{
    int i = 0;
    uint8_t mipi_lanes = 0;
    uint32_t data_bits = 0;
    uint32_t csi_ctrl_value = 0;
    for (i = 0; i < MIPI_LANE_NUM_MAX; i++) {
        if (IS_VALID_ID(mipi_cfg.mipi_attr.lane_id[i])){
            mipi_lanes++;
        }
    }

    switch (mipi_lanes) {
    case 1:
        vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_NUM_LANES, 0x01);
        break;
    case 2:
        vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_NUM_LANES, 0x02);
        break;
    case 4:
        vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_NUM_LANES, 0x04);
        break;
    case 8:
        vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_NUM_LANES, 0x04);
        break;
    default:
        return -EINVAL;
    }

    switch (mipi_cfg.mipi_attr.itf_cfg[0].data_type) {
    case VVCAM_RAW_8BIT:
        data_bits =  8;
        break;
    case VVCAM_RAW_10BIT:
        data_bits =  10;
        break;
    case VVCAM_RAW_12BIT:
        data_bits =  12;
        break;
    case VVCAM_RAW_14BIT:
        data_bits =  14;
        break;
    case VVCAM_RAW_16BIT:
        data_bits =  16;
        break;
    case VVCAM_RAW_20BIT:
        data_bits =  20;
        break;
    case VVCAM_RAW_24BIT:
        data_bits =  24;
        break;
    default:
        return -EINVAL;
    }

    if (mipi_dev->bus_width < data_bits) {
        return -EINVAL;
    }
    csi_ctrl_value =  vvcam_mipi_read_reg(mipi_dev->csi_ctrl_reg, 0);
    csi_ctrl_value &= (~0x07);

    if ((mipi_dev->bus_width - data_bits) > 0x07)
        csi_ctrl_value |= 0x07;
    else {
        csi_ctrl_value |= (mipi_dev->bus_width - data_bits);
    }
    vvcam_mipi_write_reg(mipi_dev->csi_ctrl_reg, 0, csi_ctrl_value);

    return 0;
}

static int vvcam_mipi_csi2_set_stream(struct vvcam_mipi_dev *mipi_dev,
                                    uint32_t enable)
{
    int i = 0;
    uint32_t data_lane_en = 0;
    if (enable) {
        vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_LANES_CLK_EN, 1);
        for (i = 0; i < MIPI_LANE_NUM_MAX; i++) {
            if (IS_VALID_ID(mipi_dev->mipi_cfg.mipi_attr.lane_id[i])) {
                switch(mipi_dev->mipi_cfg.mipi_attr.lane_id[i]) {
                case 0:
                    data_lane_en |= (1 << 0);
                    break;
                case 1:
                    data_lane_en |= (1 << 1);
                    break;
                case 2:
                    data_lane_en |= (1 << 2);
                    break;
                case 3:
                    data_lane_en |= (1 << 3);
                    break;
                case 4:
                    data_lane_en |= (1 << 4);
                    break;
                case 5:
                    data_lane_en |= (1 << 5);
                    break;
                case 6:
                    data_lane_en |= (1 << 6);
                    break;
                case 7:
                    data_lane_en |= (1 << 7);
                    break;
                default:
                    break;
                }
            }
        }
        vvcam_mipi_write_reg(mipi_dev->base,
                        MRV_CSI2_LANES_DATA_EN, data_lane_en);
    } else {
        vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_LANES_CLK_EN, 0);
        vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_LANES_DATA_EN, 0);
    }
    return 0;
}

int vvcam_mipi_default_cfg(struct vvcam_mipi_dev *mipi_dev)
{
    uint32_t csi_ctrl_value = 0;
    uint32_t tpg_reg = 0;

    memcpy(&mipi_dev->mipi_cfg, &default_config, sizeof(vvcam_input_dev_attr_t));

    vvcam_mipi_reset(mipi_dev, 1);
    msleep(10);
    vvcam_mipi_reset(mipi_dev, 0);

    vvcam_mipi_set_stream(mipi_dev, 0);

    if ((((mipi_dev->id == 0) || (mipi_dev->id == 1)) && CSI_CTRL_MUX_T0T1) ||
        (((mipi_dev->id == 2) || (mipi_dev->id == 3)) && CSI_CTRL_MUX_T2T3)) {
        csi_ctrl_value = vvcam_mipi_read_reg(mipi_dev->csi_ctrl_reg, 0);
        csi_ctrl_value |= CSI_CTRL_MUX_T_MASK;
        vvcam_mipi_write_reg(mipi_dev->csi_ctrl_reg, 0, csi_ctrl_value);
    } else {
        csi_ctrl_value = vvcam_mipi_read_reg(mipi_dev->csi_ctrl_reg, 0);
        csi_ctrl_value &= ~CSI_CTRL_MUX_T_MASK;
        vvcam_mipi_write_reg(mipi_dev->csi_ctrl_reg, 0, csi_ctrl_value);
    }

#ifdef INPUT_CSI_MUX_8000L
    vvcam_mipi_write_reg(mipi_dev->csi_mux_reg, 0, 0x80000000);
#else
    vvcam_mipi_write_reg(mipi_dev->csi_mux_reg, 0, 0x00000000);
#endif

    tpg_reg = vvcam_mipi_read_reg(mipi_dev->tpg_reg, 0);
    tpg_reg  |= 0x210;
    tpg_reg &= ~TPG_FRAME_GATE_MASK;
    tpg_reg |= TPG_MUX_S_MASK;

    vvcam_mipi_write_reg(mipi_dev->tpg_reg, 0, tpg_reg);

    vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_IGNORE_VC, 0x01);
    vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_FIFO_SEND_LEVEL, 0x41);
    vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_VID_VSYNC, 0x20);
    vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_VID_HSYNC_FP, 0x20);
    vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_VID_HSYNC, 0x01);
    vvcam_mipi_write_reg(mipi_dev->base, MRV_CSI2_VID_HSYNC_BP, 0x20);

    mipi_dev->bus_width = VVCAM_CSI_BUS_WIDTH;
    vvcam_mipi_set_cfg(mipi_dev, mipi_dev->mipi_cfg);

    return 0;
}

int vvcam_mipi_reset(struct vvcam_mipi_dev *mipi_dev, uint32_t val)
{
    uint32_t tpg_reg = 0;

    tpg_reg = vvcam_mipi_read_reg(mipi_dev->tpg_reg, 0);

    if (val) {
        /* reset */
        tpg_reg &= ~TPG_CSI_RST_MASK;
    } else {
        /* unreset */
        tpg_reg |= TPG_CSI_RST_MASK;
    }

    vvcam_mipi_write_reg(mipi_dev->tpg_reg, 0, tpg_reg);
    return 0;
}

int vvcam_mipi_set_cfg(struct vvcam_mipi_dev *mipi_dev,
                    vvcam_input_dev_attr_t mipi_cfg)
{
    int ret = 0;

    switch (mipi_cfg.mode) {
    case VVCAM_INPUT_MODE_MIPI:
        ret = vvcam_mipi_csi2_cfg(mipi_dev, mipi_cfg);
        break;
    case VVCAM_INPUT_MODE_LVDS:
        ret = -EINVAL;
        break;
    case VVCAM_INPUT_MODE_BT1120:
        ret = -EINVAL;
        break;
    default:
        ret = -EINVAL;
        break;
    }

    if (ret == 0) {
        memcpy(&mipi_dev->mipi_cfg, &mipi_cfg, sizeof(mipi_cfg));
    }

    return ret;
}

int vvcam_mipi_set_stream(struct vvcam_mipi_dev *mipi_dev, uint32_t enable)
{
    int ret = 0;

    switch (mipi_dev->mipi_cfg.mode) {
    case VVCAM_INPUT_MODE_MIPI:
        ret = vvcam_mipi_csi2_set_stream(mipi_dev, enable);
        break;
    case VVCAM_INPUT_MODE_LVDS:
        ret = -EINVAL;
        break;
    case VVCAM_INPUT_MODE_BT1120:
        ret = -EINVAL;
        break;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}
