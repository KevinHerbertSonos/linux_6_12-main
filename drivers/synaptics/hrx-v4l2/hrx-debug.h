// SPDX-License-Identifier: GPL-2.0
// Copyright 2021 Synaptics Incorporated

#ifndef HRX_DEBUG_H
#define HRX_DEBUG_H

#include <linux/debugfs.h>

#define HRX_DRV_ERROR	0x0
#define VIP_ERROR		0x0
#define AIP_ERROR		0x0

#define HRX_DRV_INFO	0x1
#define HRX_DRV_DEBUG	0x2
#define HRX_DRV_ALL		0x3

#define VIP_INFO		0x10
#define VIP_DEBUG		0x20
#define VIP_ALL			0x30

#define AIP_INFO		0x100
#define AIP_DEBUG		0x200
#define AIP_ALL			0x300

#define HRX_LOG(type, arg...)			\
do {									\
	if ((!(type)) || (type & debug))	\
		pr_info("[hrx-drv] "arg);					\
} while (0)

extern int debug;

int hrx_debug_create(void);
void hrx_debug_remove(void);

#endif //HRX_DEBUG_H
