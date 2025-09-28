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

/******************************************************************************
*
* File Name: focaltech_core.c
*
*    Author: Tsai HsiangYu
*
*   Created: 2015-03-02
*
******************************************************************************/
#include "focaltech_core.h"
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
//#include <linux/switch.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#if TPD_PROXIMITY
  #include <linux/hwmsensor.h>
  #include <linux/hwmsen_dev.h>
  #include <linux/sensors_io.h>
#endif

int apk_debug_flag				= 0;

#define FTS_NAME			"ft7421"
#if FT_ESD_PROTECT
	#define TPD_ESD_CHECK_CIRCLE	200
#endif
#if TPD_PROXIMITY
	#define APS_ERR(fmt, arg ...) \
		printk("<<proximity>> "fmt "\n", ## arg)
	#define TPD_PROXIMITY_DEBUG(fmt, arg ...) \
		printk("<<proximity>> "fmt "\n", ## arg)
	#define TPD_PROXIMITY_DMESG(fmt, arg ...) \
		printk("<<proximity>> "fmt "\n", ## arg)
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fts_ts_suspend(struct early_suspend *handler);
static void fts_ts_resume(struct early_suspend *handler);
static struct early_suspend focal_early_suspend =
{
	.level		= EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend	= fts_ts_suspend,
	.resume		= fts_ts_resume,
};
#endif

#if FT_ESD_PROTECT
static int count_irq				= 0;
static unsigned long esd_check_circle = TPD_ESD_CHECK_CIRCLE;
static u8 run_check_91_register			= 0;
#endif
#if TPD_PROXIMITY
static u8 tpd_proximity_flag			= 0;
/* add for tpd_proximity by wangdongfang */
static u8 tpd_proximity_flag_one		= 0;
/* 0 -> close ; 1 -> far away */
static u8 tpd_proximity_detect			= 1;
#endif
#if FT_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct *gtp_esd_check_workqueue = NULL;
#endif

static DEFINE_MUTEX(i2c_rw_access);

#if FT_ESD_PROTECT
static void gtp_esd_check_func(struct work_struct *);
#endif
unsigned char IC_FW;
struct fts_ts_data *fts_wq_data;
struct i2c_client *fts_i2c_client;
struct input_dev *fts_input_dev;

int
fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen,
	     char *readbuf, int readlen)
{
	int ret = 0;

	mutex_lock(&i2c_rw_access);
	if ((writelen > 0) && (readlen >= 0) && (NULL != client)) {
		struct i2c_msg msgs[] = {
			{
				.addr = client->addr,
				.flags = 0,
				.len = writelen,
				.buf = writebuf,
			},
			{
				.addr = client->addr,
				.flags = I2C_M_RD,
				.len = readlen,
				.buf = readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev,
				"%s: i2c read error %d\n",
				__func__, ret);
	} else {
		if ((readlen > 0) && (NULL != client)) {
			struct i2c_msg msgs[] = {
				{
					.addr = client->addr,
					.flags = I2C_M_RD,
					.len = readlen,
					.buf = readbuf,
				},
			};
			ret = i2c_transfer(client->adapter, msgs, 1);
			if (ret < 0)
				dev_err(&client->dev,
					"%s: i2c read error %d\n",
					__func__, ret);
		}
	}
	mutex_unlock(&i2c_rw_access);
	return ret;
}

int
fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret = 0;

	mutex_lock(&i2c_rw_access);
	if ((writelen > 0) && (NULL != client)) {
		struct i2c_msg msg[] = {
			{
				.addr = client->addr,
				.flags = 0,
				.len = writelen,
				.buf = writebuf,
			},
		};

		ret = i2c_transfer(client->adapter, msg, 1);
		if (ret < 0)
			dev_err(&client->dev,
				"%s: i2c write error.\n", __func__);
		mutex_unlock(&i2c_rw_access);
	}
	return ret;
}

int
fts_write_reg(struct i2c_client *client, u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = { 0 };

	buf[0] = regaddr;
	buf[1] = regvalue;

	return fts_i2c_write(client, buf, sizeof(buf));
}

int
fts_read_reg(struct i2c_client *client, u8 regaddr, u8 *regvalue)
{
	return fts_i2c_read(client, &regaddr, 1, regvalue, 1);
}

#if TPD_PROXIMITY
int
tpd_read_ps(void)
{
	tpd_proximity_detect;
	return 0;
}

static int
tpd_get_ps_value(void)
{
	return tpd_proximity_detect;
}

static int
tpd_enable_ps(int enable)
{
	u8 state, state2;
	int ret = -1;

	ret = fts_read_reg(fts_i2c_client, 0xB0, &state);
	if (ret < 0)
		dev_err(fts_wq_data->dev, "[Focal][Touch] read value fail");

	dev_dbg(fts_wq_data->dev,
		  "[proxi_5206]read: 999 0xb0's value is 0x%02X\n", state);
	if (enable) {
		state |= 0x01;
		tpd_proximity_flag = 1;
		TPD_PROXIMITY_DEBUG("[proxi_5206]ps function is on\n");
	} else {
		state &= 0x00;
		tpd_proximity_flag = 0;
		TPD_PROXIMITY_DEBUG("[proxi_5206]ps function is off\n");
	}

	ret = fts_write_reg(fts_i2c_client, 0xB0, state);
	if (ret < 0)
		dev_err(fts_wq_data->dev, "[Focal][Touch] write value fail");
	TPD_PROXIMITY_DEBUG("[proxi_5206]write: 0xB0's value is 0x%02X\n", state);

	ret = fts_read_reg(fts_i2c_client, 0xB0, &state2);
	if (ret < 0)
		dev_err(fts_wq_data->dev, "[Focal][Touch] read value fail");
	if (state != state2) {
		tpd_proximity_flag = 0;
		dev_err(fts_wq_data->dev,
			"[proxi_5206] ps fail! state = 0x%X, state2 = 0x%X\n",
			state, state2);
	}

	return 0;
}

int
tpd_ps_operate(void *self, uint32_t command, void *buff_in, int size_in,
	       void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value;

	hwm_sensor_data *sensor_data;

	TPD_DEBUG("[proxi_5206]command = 0x%02X\n", command);
	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		}
		// Do nothing
		break;

	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Enable sensor parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int*)buff_in;
			if (value)
				if ((tpd_enable_ps(1) != 0)) {
					APS_ERR("enable ps fail: %d\n", err);
					return -1;
				}
			else
				if ((tpd_enable_ps(0) != 0)) {
					APS_ERR("disable ps fail: %d\n", err);
					return -1;
				}
		}
		break;

	case SENSOR_GET_DATA:
		if ((buff_out == NULL) ||
		    (size_out < sizeof(hwm_sensor_data))) {
			APS_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			sensor_data = (hwm_sensor_data*)buff_out;

			if ((err = tpd_read_ps())) {
				err = -1;;
			} else {
				sensor_data->values[0] = tpd_get_ps_value();
				TPD_PROXIMITY_DEBUG("huang sensor_data->values[0] 1082 = %d\n",
						    sensor_data->values[0]);
				sensor_data->value_divide = 1;
				sensor_data->status =
					SENSOR_STATUS_ACCURACY_MEDIUM;
			}
		}
		break;
	default:
		APS_ERR("proxmy sensor operate function no this parameter %d!\n",
			command);
		err = -1;
		break;
	}

	return err;
}
#endif /* TPD_PROXIMITY */

#if FT_ESD_PROTECT
void
esd_switch(s32 on)
{
	if (on == 1)
		/* switch on esd */
		queue_delayed_work(gtp_esd_check_workqueue,
				   &gtp_esd_check_work, esd_check_circle);
	else
		/* switch off esd */
		cancel_delayed_work(&gtp_esd_check_work);
}

static void
force_reset_guitar(void)
{
}

#define A3_REG_VALUE				0x54
#define RESET_91_REGVALUE_SAMECOUNT		5
static u8 g_old_91_Reg_Value = 0x00;
static u8 g_first_read_91 = 0x01;
static u8 g_91value_same_count = 0;

static void
gtp_esd_check_func(struct work_struct *work)
{
	int i;
	int ret = -1;
	u8 data, data_old;
	u8 flag_error = 0;
	int reset_flag = 0;
	u8 check_91_reg_flag = 0;

	if (is_update)
		return;
	if (apk_debug_flag)
		return;

	run_check_91_register = 0;
	for (i = 0; i < 3; i++) {
		ret = fts_read_reg(fts_i2c_client, 0xA3, &data);
		if (ret < 0)
			dev_err(fts_wq_data->dev,
				"read value fail");
		if (ret == 1 && A3_REG_VALUE == data)
			break;
	}

	if (i >= 3) {
		force_reset_guitar();
		dev_dbg(fts_wq_data->dev,
			  "tpd reset. i >= 3 ret = %d A3_Reg_Value = 0x%02x\n",
			  ret, data);
		reset_flag = 1;
		goto FOCAL_RESET_A3_REGISTER;
	}

	/* esd check for count */
	ret = fts_read_reg(fts_i2c_client, 0x8F, &data);
	if (ret < 0)
		dev_err(fts_wq_data->dev, "read value fail");
	dev_dbg(fts_wq_data->dev, "0x8F:%d, count_irq is %d\n",
		  data, count_irq);

	flag_error = 0;
	if ((count_irq - data) > 10)
		if ((data + 200) > (count_irq + 10))
			flag_error = 1;

	if ((data - count_irq) > 10)
		flag_error = 1;

	if (1 == flag_error) {
		dev_err(fts_wq_data->dev,
			"reset.1 == flag_error...data=%d count_irq\n",
			data, count_irq);
		force_reset_guitar();
		reset_flag = 1;
		goto FOCAL_RESET_INT;
	}

	run_check_91_register = 1;
	ret = fts_read_reg(fts_i2c_client, 0x91, &data);
	if (ret < 0)
		dev_err(fts_wq_data->dev, "read value fail");
	dev_dbg(fts_wq_data->dev,
		  "91 register value = 0x%02x old value = 0x%02x\n",
		  data, g_old_91_Reg_Value);
	if (0x01 == g_first_read_91) {
		g_old_91_Reg_Value = data;
		g_first_read_91 = 0x00;
	} else {
		if (g_old_91_Reg_Value == data) {
			g_91value_same_count++;
			dev_dbg(fts_wq_data->dev,
				  "g_91value_same_count = %d\n",
				  g_91value_same_count);
			if (RESET_91_REGVALUE_SAMECOUNT ==
			    g_91value_same_count) {
				force_reset_guitar();
				dev_dbg(fts_wq_data->dev,
					  "reset. g_91value_same_count = 5\n");
				g_91value_same_count = 0;
				reset_flag = 1;
			}

			esd_check_circle = TPD_ESD_CHECK_CIRCLE / 2;
			g_old_91_Reg_Value = data;
		} else {
			g_old_91_Reg_Value = data;
			g_91value_same_count = 0;
			esd_check_circle = TPD_ESD_CHECK_CIRCLE;
		}
	}
FOCAL_RESET_INT:
FOCAL_RESET_A3_REGISTER:
	count_irq = 0;
	data = 0;

	ret = fts_write_reg(fts_i2c_client, 0x8F, data);
	if (ret < 0)
		dev_err(fts_wq_data->dev, "write value fail");
	if (!run_check_91_register)
		g_91value_same_count = 0;

#if TPD_PROXIMITY
	if (reset_flag == 1 && FT_PROXIMITY_ENABLE == tpd_proximity_flag)
		if (tpd_enable_ps(FT_PROXIMITY_ENABLE))
			return -1;
#endif
	queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work,
			   esd_check_circle);

	return;
}
#endif

static int
fts_read_touchdata(struct fts_ts_data *data)
{
	struct fts_event *event = &data->event;
	u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	int i = 0;
	u8 pointid = FTS_MAX_ID;

	ret = fts_i2c_read(data->client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&data->client->dev, "%s read touchdata failed.\n",
			__func__);
		return ret;
	}
#if REPORT_TOUCH_DEBUG
	for (i = 0; i < POINT_READ_BUF; i++)
		dev_dbg(&fts_i2c_client->dev,
			  "[fts] zax buf[%d] =(0x%02x)\n", i, buf[i]);
#endif
	memset(event, 0, sizeof(struct fts_event));
	event->touch_point = 0;
	event->touch_point_num = buf[FT_TOUCH_POINT_NUM] & 0x0F;
	for (i = 0; i < fts_updateinfo_curr.TPD_MAX_POINTS; i++) {
		pointid = (buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;
		else
			event->touch_point++;
		/* FIXME: had to change x and y here to make it work! */
		event->au16_y[i] =
			(s16)(buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] &
			      0x0F) << 8 |
			(s16)buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
		event->au16_x[i] =
			(s16)(buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] &
			      0x0F) << 8 |
			(s16)buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
		event->au16_x[i] = event->au16_x[i] > TOUCH_MAX_X ?
				   0 : TOUCH_MAX_X - event->au16_x[i];

		event->au8_touch_event[i] =
			buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] >> 6;
		event->au8_finger_id[i] =
			(buf[FTS_TOUCH_ID_POS + FTS_TOUCH_STEP * i]) >> 4;
#if 0
		dev_info(fts_wq_data->dev,
			 "%2.2d %2.2d\n", event->au16_x[0], event->au16_y[0]);
#endif
		event->pressure[i] =
			(buf[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);
		event->area[i] =
			(buf[FTS_TOUCH_MISC + FTS_TOUCH_STEP * i]) >> 4;
		if ((event->au8_touch_event[i] == 0 ||
		     event->au8_touch_event[i] == 2) &&
		    event->touch_point_num == 0)
			return 1;
#if REPORT_TOUCH_DEBUG
		dev_dbg(&fts_i2c_client->dev,
			  "[fts] zax data (id %d ,x (0x%02x),y (0x%02x))\n",
			  event->au8_finger_id[i], event->au16_x[i],
			  event->au16_y[i]);
#endif
	}
	return 0;
}

static int
fts_report_value(struct fts_ts_data *data)
{
	struct fts_event *event = &data->event;
	int i = 0;
	int j = 0;
	int up_point = 0;
	int touchs = 0;

	for (i = 0; i < event->touch_point; i++) {
		input_mt_slot(data->input_dev, event->au8_finger_id[i]);

		if (event->au8_touch_event[i] == 0 ||
		    event->au8_touch_event[i] == 2) {
			input_mt_report_slot_state(data->input_dev,
						   MT_TOOL_FINGER, true);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE,
					 event->pressure[i]);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					 event->area[i]);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					 event->au16_x[i]);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					 event->au16_y[i]);
			touchs |= BIT(event->au8_finger_id[i]);
			data->touchs |= BIT(event->au8_finger_id[i]);
#if REPORT_TOUCH_DEBUG
			dev_dbg(&fts_i2c_client->dev,
				"[fts] zax down (id %d ,x %d, y %d, pres %d, area %d)\n",
				event->au8_finger_id[i], event->au16_x[i],
				event->au16_y[i], event->pressure[i],
				event->area[i]);
#endif
		} else {
#if REPORT_TOUCH_DEBUG
			dev_dbg(&fts_i2c_client->dev,
				"[fts] zax up (id %d ,x %d, y %d, pres %d, area %d)\n",
				event->au8_finger_id[i], event->au16_x[i],
				event->au16_y[i], event->pressure[i],
				event->area[i]);
#endif
			up_point++;
			input_mt_report_slot_state(data->input_dev,
						   MT_TOOL_FINGER, false);
			data->touchs &= ~BIT(event->au8_finger_id[i]);
		}
	}

	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {
		if (BIT(i) & (data->touchs ^ touchs)) {

#if REPORT_TOUCH_DEBUG
			dev_err(&fts_i2c_client->dev,
				"[fts] zax add up  id %d\n", i);
#endif
			data->touchs &= ~BIT(i);
			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev,
						   MT_TOOL_FINGER, false);
		}
	}
#if REPORT_TOUCH_DEBUG
	dev_dbg(&fts_i2c_client->dev,
		"[fts] zax 1 touchs %d, data-touchs %d, touch_point %d, up_point %d\n ",
		touchs, data->touchs, event->touch_point, up_point);
#endif
	data->touchs = touchs;

	/* release all touches in final */
	if (!event->touch_point_num) {
		for (j = 0; j < CFG_MAX_TOUCH_POINTS; j++) {
			input_mt_slot(data->input_dev, j);
			input_mt_report_slot_state(data->input_dev,
						   MT_TOOL_FINGER, 0);
		}
		data->touchs = 0;
		input_report_key(data->input_dev, BTN_TOUCH, 0);
		input_sync(data->input_dev);
#if REPORT_TOUCH_DEBUG
		dev_dbg(&fts_i2c_client->dev,
			"[fts] zax end 2 touchs %d, data-touchs %d, touch_point %d, up_point %d\n ",
			touchs, data->touchs, event->touch_point, up_point);
#endif
		return 0;
	}

#if REPORT_TOUCH_DEBUG
	dev_dbg(&fts_i2c_client->dev,
		"[fts] zax 2 touchs %d, data-touchs %d, touch_point %d, up_point %d\n",
		touchs, data->touchs, event->touch_point, up_point);
#endif
	if (event->touch_point == up_point)
		input_report_key(data->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(data->input_dev, BTN_TOUCH, 1);

	input_sync(data->input_dev);
#if REPORT_TOUCH_DEBUG
	dev_err(&fts_i2c_client->dev, "[fts] zax input_synch\n");
#endif
	return 0;
}

static
irqreturn_t fts_ts_interrupt(int irq, void *dev_id)
{
	int ret = 0;

#if TPD_PROXIMITY
	int err;
	hwm_sensor_data sensor_data;
	u8 proximity_status;
#endif

	disable_irq_nosync(fts_wq_data->irq);
#if TPD_PROXIMITY
	if (tpd_proximity_flag == 1) {
		//i2c_smbus_read_i2c_block_data(i2c_client, 0xB0, 1, &state);
		ret = fts_read_reg(fts_i2c_client, 0xB0, &state);
		if (ret < 0)
			dev_err(fts_wq_data->dev, "read value fail");
		TPD_PROXIMITY_DEBUG("proxi_5206 0xB0 state value is 1131 0x%02X\n",
				    state);

		if (!(state & 0x01))
			tpd_enable_ps(1);

		ret = fts_read_reg(fts_i2c_client, 0x01, &proximity_status);
		if (ret < 0)
			dev_err(fts_wq_data->dev, "read value fail");
		TPD_PROXIMITY_DEBUG("proxi_5206 0x01 value is 1139 0x%02X\n",
				    proximity_status);

		if (proximity_status == 0xC0)
			tpd_proximity_detect = 0;
		else if (proximity_status == 0xE0)
			tpd_proximity_detect = 1;

		TPD_PROXIMITY_DEBUG("tpd_proximity_detect 1149 %d\n",
				    tpd_proximity_detect);

		if ((err = tpd_read_ps()))
			TPD_PROXIMITY_DMESG("proxi_5206 read ps data 1156 %d\n",
					    err);
		sensor_data.values[0] = tpd_get_ps_value();
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
		if ((err = hwmsen_get_interrupt_data(ID_PROXIMITY,
						     &sensor_data)))
			TPD_PROXIMITY_DMESG("proxi_5206 call hwmsen_get_interrupt_data failed  %d\n",
					    err);
	}
#endif

#if FTS_GESTURE_EN
	ret = fts_read_reg(fts_i2c_client, 0xd0, &state);
	if (ret < 0)
		dev_err(fts_wq_data->dev, "read value fail");
	if (state == 1) {
		fts_read_Gestruedata(fts_wq_data);
		enable_irq(fts_wq_data->irq);
	} else {
#endif
#if FT_ESD_PROTECT
		count_irq++;
#endif
#if FT_ESD_PROTECT
		esd_switch(0);
		apk_debug_flag = 1;
#endif
		ret = fts_read_touchdata(fts_wq_data);
		enable_irq(fts_wq_data->irq);
		if (ret == 0)
			fts_report_value(fts_wq_data);
#if FT_ESD_PROTECT
		esd_switch(1);
		apk_debug_flag = 0;
#endif
#if FTS_GESTURE_EN
	}
#endif
	return IRQ_HANDLED;
}

void
fts_reset_tp(int high_or_low)
{
	if (fts_wq_data->rst_gpio >= 0) {
		pr_info("[Focal] %s : set tp reset pin to %d\n", __func__,
			high_or_low);
		gpio_set_value(fts_wq_data->rst_gpio, high_or_low);
	}
}

static int
fts_init_gpio_hw(struct fts_ts_data *fts_wq_data)
{
	int ret = 0;

	ret = gpio_request(fts_wq_data->rst_gpio, FTS_RESET_PIN_NAME);
	if (ret) {
		dev_err(fts_wq_data->dev,
			"%s: request GPIO %s for reset failed %d\n",
		       __func__, FTS_RESET_PIN_NAME, ret);
		return ret;
	}
	/* change reset to high */
	ret = gpio_direction_output(fts_wq_data->rst_gpio, 1);
	if (ret) {
		dev_err(fts_wq_data->dev,
			"%s: set %s gpio to out put high failed %d\n",
		       __func__, FTS_RESET_PIN_NAME, ret);
		return ret;
	}

	return ret;
}

static void
fts_un_init_gpio_hw(struct fts_ts_data *fts_wq_data)
{
	gpio_free(fts_wq_data->rst_gpio);
}

static int
focaltech_parse_dt(struct fts_ts_data *dev_data)
{
	struct device_node *pnode = dev_data->dev->of_node;
	int ret;
	u32 tmp;
	int gpio;

	ret = of_property_read_u32(pnode, "x-max", &tmp);
	if (ret < 0) {
		dev_err(dev_data->dev,
			"invalid or missing 'x-max' property!\n");
		return -EINVAL;
	}
	dev_data->x_max = tmp;

	ret = of_property_read_u32(pnode, "y-max", &tmp);
	if (ret < 0) {
		dev_err(dev_data->dev,
			"invalid or missing 'y-max' property!\n");
		return -EINVAL;
	}
	dev_data->y_max = tmp;

	gpio = of_get_named_gpio(pnode, "int-gpio", 0);
	if (gpio < 0) {
		dev_err(dev_data->dev,
			"failed to get interrupt gpio\n");
		return -EINVAL;
	}
	if (!gpio_is_valid(gpio)) {
		dev_err(dev_data->dev, "interrupt gpio %d is not valid\n",
			gpio);
		return -EINVAL;
	}
	dev_data->irq = gpio;

	gpio = of_get_named_gpio(pnode, "rst-gpio", 0);
	if (gpio < 0) {
		dev_warn(dev_data->dev,
			"failed to get reset gpio\n");
		dev_data->rst_gpio = -1;
	} else if (!gpio_is_valid(gpio)) {
		dev_err(dev_data->dev, "reset gpio %d is not valid\n",
			tmp);
		return -EINVAL;
	} else {
		dev_data->rst_gpio = gpio;
	}

	return 0;
}

/************************************************************************
 * Name: fts_ts_probe
 * Brief: driver entrance function for initial/power on/create channel
 * Input: i2c info, device id
 * Output: no
 * Return: 0
 ***********************************************************************/
static int
fts_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct input_dev *input_dev;
	int err = 0;

	dev_info(&client->dev, "probing FT7421\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	fts_wq_data = kzalloc(sizeof(struct fts_ts_data), GFP_KERNEL);
	if (!fts_wq_data) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	fts_wq_data->dev = &client->dev;
	fts_wq_data->client = client;

	if (!client->dev.of_node) {
		dev_err(fts_wq_data->dev, "device not in device tree\n");
		return -EINVAL;
	}
	err = focaltech_parse_dt(fts_wq_data);
	if (err)
		return err;

	i2c_set_clientdata(client, fts_wq_data);

	fts_wq_data->init_success = 0;

	if (0 >= fts_wq_data->x_max)
		fts_wq_data->x_max = TOUCH_MAX_X;
	if (0 >= fts_wq_data->y_max)
		fts_wq_data->y_max = TOUCH_MAX_Y;

	if (fts_wq_data->rst_gpio >= 0 && fts_init_gpio_hw(fts_wq_data) < 0)
		goto exit_init_gpio;

	if (gpio_request(fts_wq_data->irq, FTS_INT_PIN_NAME)) {
		dev_err(&client->dev,
			"%s: gpio %d request for interrupt fail.\n",
			__func__, fts_wq_data->irq);
		goto exit_irq_request_failed;
	}
	gpio_direction_input(fts_wq_data->irq);

	fts_wq_data->client->irq = gpio_to_irq(fts_wq_data->irq);
	err = request_threaded_irq(fts_wq_data->client->irq, NULL,
				   fts_ts_interrupt,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   client->dev.driver->name, fts_wq_data);

	if (fts_wq_data->client->irq < 0) {
		dev_err(&client->dev,
			"[Focal][Touch] %s: request irq fail. \n", __func__);
		goto exit_irq_request_failed;
	}

	/* FIXME need mutex protect */
	disable_irq(fts_wq_data->client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"[Focal][Touch] %s: failed to allocate input device\n",
			__func__);
		goto exit_input_dev_alloc_failed;
	}

	fts_wq_data->input_dev = input_dev;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
	input_mt_init_slots(input_dev, CFG_MAX_TOUCH_POINTS, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, CFG_MAX_TOUCH_POINTS);
#endif
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0,
			     255 /*31*/, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
			     fts_wq_data->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
			     fts_wq_data->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0,
			     FTS_PRESS_MAX, 0, 0);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0))
	//for linux 3.8
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0,
			     (TPD_MAX_POINTS_5 - 1), 0, 0);
#endif
	input_dev->name = FTS_INPUT_DEV_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"%s: failed to register input device: %s\n",
			__func__, dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	/*make sure CTP already finish startup process */
	msleep(200);

#if SYSFS_DEBUG_EN
	dev_dbg(fts_wq_data->dev, "fts_create_sysfs start\n");
	fts_create_sysfs(client);

	dev_dbg(fts_wq_data->dev, "fts_create_sysfs end\n");

	mutex_init(&fts_wq_data->g_device_mutex);
#endif
	HidI2c_To_StdI2c(client);
	fts_i2c_client = client;
	fts_get_upgrade_array();
#if FTS_CTL_IIC_EN
	if (fts_rw_iic_drv_init(client) < 0)
		dev_err(&client->dev,
			"%s: create fts control iic driver failed\n",
			__func__);
#endif

	fts_input_dev = fts_wq_data->input_dev;
#if FTS_APK_DEBUG_EN
	fts_create_apk_debug_channel(client);
#endif
#ifdef FTS_AUTO_UPGRADEG
	is_update = true;
	fts_ctpm_auto_upgrade(client);
	is_update = false;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&focal_early_suspend);
#endif
	/*
	   uc_reg_addr = FTS_REG_FW_VER;
	   err = fts_i2c_read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	   if (err < 0)
	        fts_wq_data->init_success = 0;
	   else
	   {
	        fts_wq_data->init_success = 1;
	        printk("[FTS] Firmware version = 0x%x\n", uc_reg_value);
	        IC_FW = uc_reg_value;
	   }

	   uc_reg_addr = FTS_REG_POINT_RATE;
	   err = fts_i2c_read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	   if (err < 0)
	        fts_wq_data->init_success = 0;
	   else
	   {
	        fts_wq_data->init_success = 1;
	        printk("[FTS] report rate is %dHz.\n", uc_reg_value * 10);
	   }

	   uc_reg_addr = FTS_REG_THGROUP;
	   err = fts_i2c_read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	   if (err < 0)
	        fts_wq_data->init_success = 0;
	   else
	   {
	        fts_wq_data->init_success = 1;
	        printk("[FTS] touch threshold is %d.\n", uc_reg_value * 4);
	   }

	   uc_reg_addr = FTS_REG_VENDOR_ID;
	   err = fts_i2c_read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	   if (err < 0)
	        fts_wq_data->init_success = 0;
	   else
	   {
	        fts_wq_data->init_success = 1;
	        printk("[FTS] VENDOR ID = 0x%x\n", uc_reg_value);
	   }
	 */

#if FT_ESD_PROTECT
	INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
	gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
	queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work,
			   TPD_ESD_CHECK_CIRCLE);
#endif

#if FTS_GESTURE_EN
	fts_Gesture_init(input_dev);
#endif
#if TPD_PROXIMITY
	struct hwmsen_object obj_ps;
	//interrupt mode
	obj_ps.polling = 0;
	obj_ps.sensor_operate = tpd_ps_operate;

	if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
		APS_ERR("proxi_fts attach fail = %d\n", err);
	else
		APS_ERR("proxi_fts attach ok = %d\n", err);
#endif

	enable_irq(fts_wq_data->client->irq);
	dev_info(fts_wq_data->dev, "client name %s, irq %d\n",
		 client->name, client->irq);
	dev_info(fts_wq_data->dev,
		 "X-RES %d, Y-RES %d, RST gpio %d, gpio irq %d, client irq %d\n",
		 fts_wq_data->x_max, fts_wq_data->y_max, fts_wq_data->rst_gpio,
		 fts_wq_data->irq, fts_wq_data->client->irq);

	return 0;
exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, fts_wq_data);

exit_init_gpio:
	if (fts_wq_data->rst_gpio >= 0)
		fts_un_init_gpio_hw(fts_wq_data);

exit_irq_request_failed:
	i2c_set_clientdata(client, NULL);
	kfree(fts_wq_data);

exit_alloc_data_failed:
	dev_err(&client->dev, "[%s] alloc fts_ts_data failed\n", __func__);
exit_check_functionality_failed:
	return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void
fts_release_all_finger( void )
{
	struct fts_ts_data *ts = fts_wq_data;

#ifndef MT_PROTOCOL_B
	input_mt_sync( ts->input_dev );
#else
	for (finger_count = 0; finger_count < CFG_MAX_TOUCH_POINTS;
	     finger_count++) {
		input_mt_slot( ts->input_dev, finger_count);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER,
					   false);
	}
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
#endif
	input_sync(ts->input_dev );
}

static void
fts_ts_suspend(struct early_suspend *handler)
{
	struct fts_ts_data *ts = fts_wq_data;

#if TPD_PROXIMITY
	if (tpd_proximity_flag == 1) {
		tpd_proximity_flag_one = 1;
		return;
	}
#endif

#if FTS_GESTURE_EN
	dev_dbg(fts_wq_data->dev, "open gesture mode");

	if (fts_updateinfo_curr.CHIP_ID == 0x54 ||
	    fts_updateinfo_curr.CHIP_ID == 0x58 ||
	    fts_updateinfo_curr.CHIP_ID == 0x86) {
		fts_write_reg(ts->client, 0xd1, 0xff);
		fts_write_reg(ts->client, 0xd2, 0xff);
		fts_write_reg(ts->client, 0xd5, 0xff);
		fts_write_reg(ts->client, 0xd6, 0xff);
		fts_write_reg(ts->client, 0xd7, 0xff);
		fts_write_reg(ts->client, 0xd8, 0xff);
	}
	fts_write_reg(ts->client, 0xd0, 0x01);
	return;
#else
	dev_dbg(fts_wq_data->dev, "touch suspend");
	disable_irq(ts->intr_gpio);
#if FT_ESD_PROTECT
	cancel_delayed_work_sync(&gtp_esd_check_work);
#endif /* FT_ESD_PROTECT */

	disable_irq_nosync(ts->intr_gpio);

	if ((fts_updateinfo_curr.CHIP_ID == 0x59))
		fts_write_reg(ts->client, 0xa5, 0x02);
	else
		fts_write_reg(ts->client, 0xa5, 0x03);
	msleep(10);
	fts_release_all_finger();
#endif /* FTS_GESTURE_EN */
}

/************************************************************************
 * Name: fts_ts_resume
 * Brief: system wake up
 * Input: no use
 * Output: no
 * Return: no
 ***********************************************************************/
static void
fts_ts_resume(struct early_suspend *handler)
{
	struct fts_ts_data *ts = fts_wq_data;

	dev_dbg(&ts->client->dev, "[FTS]focaltech resume.\n");
#if TPD_PROXIMITY
	if (tpd_proximity_flag == 1)
		if (tpd_proximity_flag_one == 1) {
			tpd_proximity_flag_one = 0;
			return;
		}
#endif

#if FTS_GESTURE_EN
	fts_write_reg(ts->client, 0xD0, 0x00);
#endif
#if FT_ESD_PROTECT
	count_irq = 0;
	queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work,
			   TPD_ESD_CHECK_CIRCLE);
#endif
	if (ts->rst_gpio >= 0) {
		gpio_set_value(ts->rst_gpio, 0);
		msleep(20);
		gpio_set_value(ts->rst_gpio, 1);
	}
	msleep(300);
	enable_irq(ts->intr_gpio);
	msleep(30);

	fts_release_all_finger();
}
#endif /* CONFIG_HAS_EARLY_SUSPEND */

/************************************************************************
 * Name: fts_ts_remove
 * Brief: remove driver/channel
 * Input: i2c info
 * Output: no
 * Return: 0
 ***********************************************************************/
static int fts_ts_remove(struct i2c_client *client)
{
	struct fts_ts_data *fts_wq_data;

	fts_wq_data = i2c_get_clientdata(client);
	input_unregister_device(fts_wq_data->input_dev);

#ifdef CONFIG_PM
	if (fts_wq_data->rst_gpio >= 0)
		gpio_free(fts_wq_data->rst_gpio);
#endif
#if FTS_CTL_IIC_EN
	fts_rw_iic_drv_exit();
#endif
#if SYSFS_DEBUG_EN
	fts_remove_sysfs(client);
#endif
#if FTS_APK_DEBUG_EN
	fts_release_apk_debug_channel();
#endif
#if FT_ESD_PROTECT
	destroy_workqueue(gtp_esd_check_workqueue);
#endif
	fts_un_init_gpio_hw(fts_wq_data);

	free_irq(client->irq, fts_wq_data);

	kfree(fts_wq_data);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct of_device_id fts_of_match[] = {
	{ .compatible = "focaltech,ft7421", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, fts_of_match);

static const struct i2c_device_id fts_ts_id[] = {
	{ FTS_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, fts_ts_id);

static struct i2c_driver fts_ts_driver = {
	.probe		= fts_ts_probe,
	.remove		= fts_ts_remove,
	.id_table	= fts_ts_id,
	.driver		= {
		.name		= FTS_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = fts_of_match,
	},
};

module_i2c_driver(fts_ts_driver);

MODULE_AUTHOR("<OSTeam>");
MODULE_DESCRIPTION("FocalTech TouchScreen driver");
MODULE_LICENSE("GPL");
