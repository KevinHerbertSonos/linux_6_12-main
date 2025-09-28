/* SPDX-License-Identifier: MIT OR GPL-2.0-or-later */
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

#ifndef __VVCAM_DW_H__
#define __VVCAM_DW_H__
#include "vvcam_event.h"

enum {
    VVCAM_EID_DWE_INT            = 0x00,
    VVCAM_EID_VSE_MI_MIS0        = 0x01,
    VVCAM_EID_VSE_MI_MIS1        = 0x02,
    VVCAM_EID_VSE1_MI_MIS0       = 0x03,
    VVCAM_EID_VSE1_MI_MIS1       = 0x04,
    VVCAM_EID_DW_FE_MIS          = 0x05,
    VVCAM_EID_DW_MAX                   ,
};

typedef struct {
    uint32_t addr;
    uint32_t value;
} vvcam_dw_reg_t;

typedef struct {
    uint64_t base;
    uint64_t size;
} vvcam_dw_reg_mem_t;

#define VVCAM_DW_IOC_MAGIC 'v'
#define VVCAM_DWE_RESET             _IO(VVCAM_DW_IOC_MAGIC, 0x01)
#define VVCAM_DWE_READ_REG          _IOWR(VVCAM_DW_IOC_MAGIC, 0x02, vvcam_dw_reg_t)
#define VVCAM_DWE_WRITE_REG         _IOW(VVCAM_DW_IOC_MAGIC, 0x03, vvcam_dw_reg_t)
#define VVCAM_VSE_RESET             _IO(VVCAM_DW_IOC_MAGIC, 0x04)
#define VVCAM_VSE_READ_REG          _IOWR(VVCAM_DW_IOC_MAGIC, 0x05, vvcam_dw_reg_t)
#define VVCAM_VSE_WRITE_REG         _IOW(VVCAM_DW_IOC_MAGIC, 0x06, vvcam_dw_reg_t)
#define VVCAM_DW_SUBSCRIBE_EVENT    _IOW(VVCAM_DW_IOC_MAGIC, 0x07, vvcam_subscription_t)
#define VVCAM_DW_UNSUBSCRIBE_EVENT  _IOW(VVCAM_DW_IOC_MAGIC, 0x08, vvcam_subscription_t)
#define VVCAM_DW_DQEVENT            _IOR(VVCAM_DW_IOC_MAGIC, 0x09, vvcam_event_t)
#define VVCAM_DW_REG_MEM            _IOR(VVCAM_DW_IOC_MAGIC, 0x0A, vvcam_dw_reg_mem_t)

#endif
