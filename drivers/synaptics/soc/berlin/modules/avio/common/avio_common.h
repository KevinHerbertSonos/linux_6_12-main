// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2020 Synaptics Incorporated */

#ifndef	__AVIO_COMMON_H__
#define	__AVIO_COMMON_H__

#include <linux/gpio/consumer.h>

#ifdef __cplusplus
extern "C" {
#endif

int is_avio_driver_initialized(void);

int avio_util_get_quiescent_flag(void);

int avio_module_mipirst_get_gpio_handle(struct device *dev,
			struct fwnode_handle *child, enum gpiod_flags flags);
int avio_module_mipirst_set_gpio_val(int val);

#ifdef __cplusplus
}
#endif

#endif /* __AVIO_COMMON_H__ */
