/* drivers/input/touchscreen/ft5x06_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver.
 *
 * Copyright (c) 2010  Focal tech Ltd.
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

#include <linux/i2c.h>
#include <linux/input.h>
//#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
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
#include <linux/timer.h>
#include <linux/of_gpio.h>

#include <mach/gpio.h>
//#include <mach/map.h>
//#include <mach/regs-clock.h>
//#include <mach/regs-gpio.h>
//#include <plat/gpio-cfg.h>

#include "ft5x06_ts.h"

//#define FT5336_DOWNLOAD
//#define SYSFS_DEBUG
//#define FTS_APK_DEBUG
//#define FTS_CTL_IIC

#ifdef FTS_CTL_IIC
#include "focaltech_ctl.h"
#endif

#ifdef FT5336_DOWNLOAD
#include "ft5336_download_lib.h"
static struct i2c_client *g_i2c_client = NULL;
static unsigned char CTPM_MAIN_FW[]=
{
	#include "ft5336_all.i"
};
#endif

#ifdef SYSFS_DEBUG
#include "ft5x06_ex_fun.h"
#endif

struct ts_event {
	u16 au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:
					0 -- down; 1-- contact; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	u16 pressure;
	u8 touch_point;
};

struct ft5x0x_ts_data {
	int irq;
	unsigned int x_max;
	unsigned int y_max;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct ts_event event;
	int reset_gpio;
	#ifdef CONFIG_PM
	struct early_suspend *early_suspend;
	#endif
};

#define ANDROID_INPUT_PROTOCOL_B

//#define FT5X0X_RESET_PIN	S5PV210_GPB(2)
#define FT5X0X_RESET_PIN_NAME	"ft5x0x-reset"



/*
*ft5x0x_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
int ft5x0x_i2c_Read(struct i2c_client *client, char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
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
			dev_err(&client->dev, "f%s: i2c read error.\n",
				__func__);
	} else {
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
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}
/*write data by i2c*/
int ft5x0x_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

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
		dev_err(&client->dev, "%s i2c write error.\n", __func__);

	return ret;
}
#ifdef FT5336_DOWNLOAD
int ft5x0x_download_i2c_Read(unsigned char *writebuf,
		    int writelen, unsigned char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = 0x38,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = 0x38,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(g_i2c_client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&g_i2c_client->dev, "f%s: i2c read error.\n",
				__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = 0x38,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(g_i2c_client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&g_i2c_client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}
/*write data by i2c*/
int ft5x0x_download_i2c_Write(unsigned char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
		 .addr = 0x38,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	ret = i2c_transfer(g_i2c_client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&g_i2c_client->dev, "%s i2c write error.\n", __func__);

	return ret;
}

#endif
/*Read touch point information when the interrupt  is asserted.*/
static int ft5x0x_read_Touchdata(struct ft5x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	int i = 0;
	u8 pointid = FT_MAX_ID;

	ret = ft5x0x_i2c_Read(data->client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&data->client->dev, "%s read touchdata failed.\n",
			__func__);
		return ret;
	}
	memset(event, 0, sizeof(struct ts_event));

	event->touch_point = 0;
	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {
		pointid = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		if (pointid >= FT_MAX_ID)
			break;
		else
			event->touch_point++;
		event->au16_x[i] =
		    (s16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
		event->au16_y[i] =
		    (s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];
		event->au8_touch_event[i] =
		    buf[FT_TOUCH_EVENT_POS + FT_TOUCH_STEP * i] >> 6;
		event->au8_finger_id[i] =
		    (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		#if 0
		pr_info("id=%d event=%d x=%d y=%d\n", event->au8_finger_id[i],
			event->au8_touch_event[i], event->au16_x[i], event->au16_y[i]);
		#endif
	}

	event->pressure = FT_PRESS;

	return 0;
}

/*
*report the point information
*/
static void ft5x0x_report_value(struct ft5x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	int i;
	int uppoint = 0;

	/*protocol B*/	
	for (i = 0; i < event->touch_point; i++)
	{
		input_mt_slot(data->input_dev, event->au8_finger_id[i]);
		
		if (event->au8_touch_event[i]== 0 || event->au8_touch_event[i] == 2)
		{
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,
				true);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					 data->x_max - event->au16_x[i]);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					event->au16_y[i]);
		}
		else
		{
			uppoint++;
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,
				false);
		}
	}
	if(event->touch_point == uppoint)
		input_report_key(data->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(data->input_dev, BTN_TOUCH, event->touch_point > 0);
	input_sync(data->input_dev);


}

/*The ft5x0x device will signal the host about TRIGGER_FALLING.
*Processed when the interrupt is asserted.
*/
static irqreturn_t ft5x0x_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x0x_ts_data *ft5x0x_ts = dev_id;
	int ret = 0;
	disable_irq_nosync(ft5x0x_ts->irq);

	//pr_info("interrupt is coming\n");
	ret = ft5x0x_read_Touchdata(ft5x0x_ts);
	if (ret == 0)
		ft5x0x_report_value(ft5x0x_ts);

	enable_irq(ft5x0x_ts->irq);

	return IRQ_HANDLED;
}

#if 0
static void ft5x0x_reset_tp(int HighOrLow)
{
	pr_info("set tp reset pin to %d\n", HighOrLow);
	gpio_set_value(FT5X0X_RESET_PIN, HighOrLow);
}

static void ft5x0x_Enable_IRQ(struct i2c_client *client, int enable)
{
	if (FT5X0X_ENABLE_IRQ == enable)
		enable_irq(client->irq);
	else
		disable_irq_nosync(client->irq);
}
#endif

static int fts_init_gpio_hw(struct ft5x0x_ts_data *ft5x0x_ts)
{
	int ret;
	int floating_gpio;

	ft5x0x_ts->reset_gpio = of_get_named_gpio(ft5x0x_ts->client->dev.of_node,
						  "reset-gpio", 0);
	if (!gpio_is_valid(ft5x0x_ts->reset_gpio)) {
		pr_err("%s: reset GPIO not valid\n", __func__);
		return -1;
	}
	ret = devm_gpio_request_one(&ft5x0x_ts->client->dev,
				ft5x0x_ts->reset_gpio,
				GPIOF_OUT_INIT_HIGH,
				FT5X0X_RESET_PIN_NAME);
	if (ret) {
		pr_err("%s: request GPIO %s for reset failed, ret = %d\n",
				__func__, FT5X0X_RESET_PIN_NAME, ret);
		return ret;
	}

	/* On 455HD, the interrupt has been reworked and is forced to be also
	 * connected to this "floating_gpio" which has to be configured as
	 * input as not to influence the interrupt line
	 */
	floating_gpio = of_get_named_gpio(ft5x0x_ts->client->dev.of_node,
					  "floating-gpio", 0);
	if (gpio_is_valid(floating_gpio)) {
		ret = devm_gpio_request_one(&ft5x0x_ts->client->dev,
					    floating_gpio, GPIOF_IN,
					    "ts-int-floating");
		if (ret)
			pr_err("%s: could not request floating GPIO %d\n",
			       __func__, floating_gpio);
		else
			pr_info("%s: successfully requested floating GPIO %d\n",
				__func__, floating_gpio);
	}

	return ret;
}

#ifdef FT5336_DOWNLOAD

int ft5336_Enter_Debug(void)
{
	ft5x0x_reset_tp(0);
	msleep(4);
	ft5x0x_reset_tp(1);
	return ft5336_Lib_Enter_Download_Mode();
}
//if return 0, main flash is ok, else download.
int ft5336_IsDownloadMain(void)
{
	//add condition to check main flash
	return -1;
}
int ft5336_DownloadMain(void)
{
	unsigned short fwlen = 0;
	if (ft5336_Enter_Debug() < 0) {
		pr_err("-----enter debug mode failed\n");
		return -1;
	}
	fwlen = sizeof(CTPM_MAIN_FW);
	pr_info("----fwlen=%d\n", fwlen);

	//return ft6x06_Lib_DownloadMain(CTPM_MAIN_FW, fwlen);
	return ft5336_Lib_DownloadMain(CTPM_MAIN_FW, fwlen);
}
#endif


static int ft5x0x_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct ft5x0x_ts_data *ft5x0x_ts;
	struct input_dev *input_dev;
	int err = 0;
	unsigned char uc_reg_value;
	unsigned char uc_reg_addr;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ft5x0x_ts = kzalloc(sizeof(struct ft5x0x_ts_data), GFP_KERNEL);

	if (!ft5x0x_ts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, ft5x0x_ts);

	ft5x0x_ts->irq = of_get_named_gpio(client->dev.of_node, "int-gpio", 0);
	if (ft5x0x_ts->irq < 0 || !gpio_is_valid(ft5x0x_ts->irq)) {
		pr_err("%s: irq GPIO %d not valid\n", __func__,
		       ft5x0x_ts->irq);
		err = -1;
		goto exit_alloc_data_failed;
	}
	ft5x0x_ts->irq = gpio_to_irq(ft5x0x_ts->irq);
	if (ft5x0x_ts->irq < 0) {
		pr_err("%s: gpio to irq failed\n", __func__);
		err = ft5x0x_ts->irq;
		goto exit_alloc_data_failed;
	}

	ft5x0x_ts->client = client;
	err = of_property_read_u32(client->dev.of_node, "max-x",
				   &ft5x0x_ts->x_max);
	if (err || ft5x0x_ts->x_max < 1) {
		dev_err(&client->dev, "could not get 'max-x' property\n");
		goto exit_irq_request_failed;
	} else {
		ft5x0x_ts->x_max--;
	}
	err = of_property_read_u32(client->dev.of_node, "max-y",
				   &ft5x0x_ts->y_max);
	if (err || ft5x0x_ts->y_max < 1) {
		dev_err(&client->dev, "could not get 'max-y' property\n");
		goto exit_irq_request_failed;
	} else {
		ft5x0x_ts->y_max--;
	}
	client->irq = ft5x0x_ts->irq;
	pr_info("irq = %d\n", client->irq);
	//pr_info("FT5X0X_INT_PIN = %d\n", FT5X0X_INT_PIN);

	if(fts_init_gpio_hw(ft5x0x_ts)<0)
		goto exit_irq_request_failed;
#ifdef CONFIG_PM
#if 0
	err = gpio_request(pdata->reset, "ft5x0x reset");
	if (err < 0) {
		dev_err(&client->dev, "%s:failed to set gpio reset.\n",
			__func__);
		goto exit_request_reset;
	}
	#endif
#endif
	

	err = request_threaded_irq(client->irq, NULL, ft5x0x_ts_interrupt,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->dev.driver->name,
				   ft5x0x_ts);
	if (err < 0) {
		dev_err(&client->dev, "ft5x0x_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	disable_irq(client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ft5x0x_ts->input_dev = input_dev;
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, ft5x0x_ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, ft5x0x_ts->y_max, 0, 0);
	input_mt_init_slots(input_dev, CFG_MAX_TOUCH_POINTS,
			    INPUT_MT_DIRECT | INPUT_MT_POINTER);

	input_dev->name = FT5X0X_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"ft5x0x_ts_probe: failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	/*make sure CTP already finish startup process */
	msleep(150);
#ifdef SYSFS_DEBUG
		ft5x0x_create_sysfs(client);
#endif

#ifdef FTS_CTL_IIC
		if (ft_rw_iic_drv_init(client) < 0)
			dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",
					__func__);
#endif
#ifdef FTS_APK_DEBUG
	ft5x0x_create_apk_debug_channel(client);
#endif

#ifdef FT5336_DOWNLOAD
			g_i2c_client = client;
			FTS_I2c_Read_Function fun_i2c_read = ft5x0x_download_i2c_Read;
			FTS_I2c_Write_Function fun_i2c_write = ft5x0x_download_i2c_Write;
			Init_I2C_Read_Func(fun_i2c_read);
			Init_I2C_Write_Func(fun_i2c_write);
			 if(ft5336_IsDownloadMain() < 0) {
	 	#if 1
				pr_info("--------FTS---------download main\n");
				if(ft5336_DownloadMain()<0)
				{
					pr_err("---------FTS---------Download main failed\n");
				}
		#endif
			 } else
				pr_info("--------FTS---------no download main\n");
#endif


	/*get some register information */
	uc_reg_addr = FT5x0x_REG_FW_VER;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	//dev_dbg(&client->dev, "[FTS] Firmware version = 0x%x\n", uc_reg_value);
	pr_info( "[FTS] Firmware version = 0x%x\n", uc_reg_value);

	uc_reg_addr = FT5x0x_REG_POINT_RATE;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	//dev_dbg(&client->dev, "[FTS] report rate is %dHz.\n",
	//	uc_reg_value * 10);
	pr_info("[FTS] report rate is %dHz.\n", uc_reg_value * 10);

	uc_reg_addr = FT5X0X_REG_THGROUP;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	//dev_dbg(&client->dev, "[FTS] touch threshold is %d.\n",
	//	uc_reg_value * 4);
	pr_info("[FTS] touch threshold is %d.\n", uc_reg_value * 4);

	enable_irq(client->irq);
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, ft5x0x_ts);
#ifdef CONFIG_PM
#if 0
exit_request_reset:
	gpio_free(ft5x0x_ts->pdata->reset);
#endif
#endif

exit_irq_request_failed:
	i2c_set_clientdata(client, NULL);
	kfree(ft5x0x_ts);

exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

#if 0
#ifdef CONFIG_PM
static void ft5x0x_ts_suspend(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts = container_of(handler, struct ft5x0x_ts_data,
						early_suspend);

	dev_dbg(&ts->client->dev, "[FTS]ft5x0x suspend\n");
	disable_irq(ts->pdata->irq);
}

static void ft5x0x_ts_resume(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts = container_of(handler, struct ft5x0x_ts_data,
						early_suspend);

	dev_dbg(&ts->client->dev, "[FTS]ft5x0x resume.\n");
	gpio_set_value(ts->pdata->reset, 0);
	msleep(20);
	gpio_set_value(ts->pdata->reset, 1);
	enable_irq(ts->pdata->irq);
}
#else
#define ft5x0x_ts_suspend	NULL
#define ft5x0x_ts_resume		NULL
#endif
#endif

static int ft5x0x_ts_remove(struct i2c_client *client)
{
	struct ft5x0x_ts_data *ft5x0x_ts;
	ft5x0x_ts = i2c_get_clientdata(client);
	input_unregister_device(ft5x0x_ts->input_dev);

	#ifdef FTS_CTL_IIC
	ft_rw_iic_drv_exit();
	#endif
	#ifdef SYSFS_DEBUG
		ft5x0x_remove_sysfs(client);
	#endif

	#ifdef FTS_APK_DEBUG
		ft5x0x_release_apk_debug_channel();
	#endif

	kfree(ft5x0x_ts);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id ft5x0x_ts_id[] = {
	{FT5X0X_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

static const struct of_device_id fts_of_match[] = {
	{ .compatible = "focaltech,ft5x06", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, fts_of_match);

static struct i2c_driver ft5x0x_ts_driver = {
	.probe = ft5x0x_ts_probe,
	.remove = ft5x0x_ts_remove,
	.id_table = ft5x0x_ts_id,
#if 0
	.suspend = ft5x0x_ts_suspend,
	.resume = ft5x0x_ts_resume,
#endif
	.driver = {
		   .name = FT5X0X_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = fts_of_match,
		   },
};

static int __init ft5x0x_ts_init(void)
{
	int ret;
	ret = i2c_add_driver(&ft5x0x_ts_driver);
	if (ret) {
		printk(KERN_WARNING "Adding ft5x0x driver failed "
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",
			ft5x0x_ts_driver.driver.name);
	}
	return ret;
}

static void __exit ft5x0x_ts_exit(void)
{
	i2c_del_driver(&ft5x0x_ts_driver);
}

module_init(ft5x0x_ts_init);
module_exit(ft5x0x_ts_exit);

MODULE_AUTHOR("<luowj>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
