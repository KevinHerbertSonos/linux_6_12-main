// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#ifndef _AVIO_DRIVER_H_
#define _AVIO_DRIVER_H_
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "avio_type.h"
#include "avio_ioctl.h"
#include "avio_memmap.h"
#include "avio_common.h"

typedef struct _AVIO_CTX_ {
	struct resource *pAvioRes;
	UINT32 avio_base;
	UINT32 avio_size;
	void *avio_virt_base;
	UINT32 regmap_handle;
	struct gpio_desc 	*mipirst;

	unsigned char isTeeEnabled;

	struct semaphore resume_sem;
} AVIO_CTX;

typedef struct fastlogo_info_t {
	u32 status : 1;   // Successfully displayed logo or not
	u32 partition : 1; //logo from partition A/B
	u32 cpcb0ResId : 8;     //CpCb0 resolution -- may be additional bits for depth/format
	u32 cpcb1ResId : 8;     //CpCb1 resolution
	u32 reserved : 14;
} FASTLOGO_INFO;

typedef union  avio_fastlogo_info_u {
	u32 fl_disp_info;
	struct fastlogo_info_t u;
} avio_fastlogo_info;

struct avio_device_t {
	unsigned char *dev_name;
	struct cdev cdev;
	struct class *dev_class;
	const struct file_operations *fops;
	struct mutex mutex;
	int major;
	int minor;
	struct proc_dir_entry *dev_procdir;
	void *private_data;

	struct mutex avio_mutex;

	int (*dev_init)(struct avio_device_t*, unsigned int);
	int (*dev_exit)(struct avio_device_t*, unsigned int);
};

int avio_module_avio_probe(struct platform_device *pdev);
avio_fastlogo_info avio_get_fastlogo_status(void);
void avio_set_fastlogo_status(int status);

#endif //_AVIO_DRIVER_H_
