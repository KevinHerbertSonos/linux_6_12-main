/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2022 - 2024 Synaptics Incorporated.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __H1_IRQ_H__
#define __H1_IRQ_H__

#include "vpu_common.h"

enum {
	H1_IOCTL_POLL_INT,
};

int h1_irq_init(struct syna_vpu_srv *node);
void h1_irq_destroy(void);

#endif // __H1_IRQ_H__
