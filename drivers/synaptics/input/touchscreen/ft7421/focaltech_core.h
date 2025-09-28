/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __FOCALTECH_CORE_H
#define __FOCALTECH_CORE_H
/*******************************************************************************
* File Name: focaltech_core.h
*    Author: Xu YongFeng
*   Created: 2015-01-29
*******************************************************************************/

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
//#include <linux/switch.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/unistd.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0))
	#if defined(MODULE) || defined(CONFIG_HOTPLUG)
		#define __devexit_p(x)		x
	#else
		#define __devexit_p(x)		NULL
	#endif
	/* used for HOTPLUG */
	#define __devinit	__section(.devinit.text) \
		__cold notrace
	#define __devinitdata	__section(.devinit.data)
	#define __devinitconst	__section(.devinit.rodata)
	#define __devexit	__section(.devexit.text) __exitused \
		__cold notrace
	#define __devexitdata	__section(.devexit.data)
	#define __devexitconst	__section(.devexit.rodata)
#endif

/* driver config */
#define CFG_MAX_TOUCH_POINTS	10
#define FTS_PRESS_MAX		0xFF
#define FTS_PRESS		0x08
#define INTEL_EN		1
#define FTS_INPUT_DEV_NAME	"focal-touchscreen"
#define FTS_MAX_ID		0x0F
#define FTS_TOUCH_STEP		6
#define FTS_TOUCH_X_H_POS	3
#define FTS_TOUCH_X_L_POS	4
#define FTS_TOUCH_Y_H_POS	5
#define FTS_TOUCH_Y_L_POS	6
#define FTS_TOUCH_XY_POS	7
#define FTS_TOUCH_MISC		8
#define FTS_TOUCH_EVENT_POS	3
#define FTS_TOUCH_ID_POS	5
#define FT_TOUCH_POINT_NUM	2
#define POINT_READ_BUF		(3 + FTS_TOUCH_STEP * CFG_MAX_TOUCH_POINTS)

/* register addresses */
#define FTS_REG_CHIP_ID			0xA3
#define FTS_REG_FW_VER			0xA6
#define FTS_REG_POINT_RATE		0x88
#define FTS_REG_THGROUP			0x80
#define FTS_REG_VENDOR_ID		0xA8
//#define FT_TP				0
//#define FTS_ENABLE_IRQ		0
#define FTS_DISABLE_IRQ			0
#define SYSFS_DEBUG_EN			1
#define FTS_APK_DEBUG_EN		1
#define FTS_CTL_IIC_EN			1
#define CONFIG_PM_EN			1
#define FT_ESD_PROTECT			0
#define REPORT_TOUCH_DEBUG		0
#define TPD_PROXIMITY			0
#define FTS_GESTURE_EN			0
#define TOUCH_MAX_X			720 /* FIXME why not 800? */
#define TOUCH_MAX_Y			1280
#define ANDROID_INPUT_PROTOCOL_B
#define FTS_RESET_PIN_NAME		"ft3417-rst"
#define FTS_INT_PIN_NAME		"ft3417-int"
#define TPD_MAX_POINTS_5		5
//#define IC_FT5X46			5
//#define DEVICE_IC_TYPE		IC_FT5X46
#define FTS_DRV_VERSION			"drv: Intel_2.1_20150122"
#define FTS_UPGRADE_ERASE_DELAY		1500
#define FTS_PACKET_LENGTH		120
#define FTS_RESET_TP			13
#define FTS_SET_START_TEST_INFO		15
#define FTS_ENABLE_IRQ			16
#define LEN_FLASH_ECC_MAX		0xFFFE
#define FT_PROXIMITY_ENABLE		0

struct fts_upgrade_info {
	u8 CHIP_ID;
	u8 TPD_MAX_POINTS;
	u8 AUTO_CLB;
	/* delay of write FTS_UPGRADE_AA */
	u16 delay_aa;
	/* delay of write FTS_UPGRADE_55 */
	u16 delay_55;
	u8 upgrade_id_1;
	u8 upgrade_id_2;
	u16 delay_readid;
	u16 delay_erase_flash;
};

struct fts_event {
	/* x coordinate */
	u16 au16_x[CFG_MAX_TOUCH_POINTS];
	/* y coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];
	/* touch event: 0 -- down; 1-- contact; 2 -- contact */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];
	/* touch ID */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];
	/* touch weight */
	u8 au8_finger_weight[CFG_MAX_TOUCH_POINTS];
	u8 pressure[CFG_MAX_TOUCH_POINTS];
	u8 area[CFG_MAX_TOUCH_POINTS];
	u8 touch_point;
	u8 touch_point_num;
};

struct fts_ts_data {
	unsigned int irq;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int init_success;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct fts_event event;
	struct mutex g_device_mutex;
#ifdef CONFIG_PM
	struct early_suspend *early_suspend;
#endif
	u8 fw_ver[3];
	u8 fw_vendor_id;
	int touchs;

	int rst_gpio;
	int intr_gpio;
	uint16_t version;

	struct device *dev;
};

extern struct fts_upgrade_info fts_updateinfo_curr;
extern struct i2c_client *fts_i2c_client;
extern struct input_dev *fts_input_dev;
extern struct fts_ts_data *fts_wq_data;
extern int fts_ctpm_auto_upgrade(struct i2c_client *client);
extern int fts_ctpm_auto_clb(struct i2c_client *client);
extern int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client,
					     char *firmware_name);
extern int fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue);
extern int fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue);
extern void fts_get_upgrade_array(void);
extern int fts_rw_iic_drv_init(struct i2c_client *client);
extern void fts_rw_iic_drv_exit(void);
extern int fts_Gesture_init(struct input_dev *input_dev);
extern int fts_read_gesture_data(void);
extern int fts_ctpm_get_i_file_ver(void);
int fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen,
		 char *readbuf, int readlen);
int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen);
extern void fts_reset_tp(int HighOrLow);
/* zax + */
extern int fts_create_sysfs(struct i2c_client *client);
extern int fts_remove_sysfs(struct i2c_client *client);
extern int HidI2c_To_StdI2c(struct i2c_client *client);
int fts_create_apk_debug_channel(struct i2c_client *client);
void fts_release_apk_debug_channel(void);
int fts_ctpm_fw_preupgrade_hwreset(struct i2c_client *client, u8 *pbt_buf,
				   u32 dw_lenth);
int fts_ctpm_fw_preupgrade(struct i2c_client *client, u8 *pbt_buf,
			   u32 dw_lenth);
extern int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client);
#if FT_ESD_PROTECT
extern void esd_switch(s32 on);
#endif
/* zax - */

#define _FTS_DBG
#ifdef _FTS_DBG
	#define FTS_DBG(fmt, args ...) printk("[FTS]" fmt, ## args)
#else
	#define FTS_DBG(fmt, args ...) do {} while (0)
#endif

#endif /* __FOCALTECH_CORE_H */
