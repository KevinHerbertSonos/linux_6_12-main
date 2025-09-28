/*
 * DSPG CPLD led driver
 *
 * Based on leds-pca9532.c
 * Copyright (c) 2014, DSP Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

static const struct i2c_device_id cpld_led_id[] = {
	{ "cpld", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cpld_led_id);

#ifdef CONFIG_OF
static const struct of_device_id cpld_led_of_id[] = {
	{ .compatible = "cpld" },
	{ },
};
MODULE_DEVICE_TABLE(of, cpld_led_of_id);
#endif

static DEFINE_MUTEX(cpld_led_mutex);

struct cpld_led {
	struct i2c_client *client;
	struct work_struct work;
	enum led_brightness brightness;
	struct led_classdev led_cdev;
	int led_num; /* 0..15 */
	char name[32];
};

static void
cpld_led_work(struct work_struct *work)
{
	struct cpld_led *cpld_led = container_of(work, struct cpld_led, work);
	unsigned char val;
	int reg = 2 - (cpld_led->led_num >> 3);
	int shift = 1 << (cpld_led->led_num & 0x7);

	mutex_lock(&cpld_led_mutex);

	val = i2c_smbus_read_byte_data(cpld_led->client, reg);

	val &= ~shift;
	if (cpld_led->brightness == LED_OFF)
		val |= shift;

	i2c_smbus_write_byte_data(cpld_led->client, reg, val);

	mutex_unlock(&cpld_led_mutex);
}

static void
cpld_led_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	struct cpld_led *cpld_led;

	cpld_led = container_of(led_cdev, struct cpld_led, led_cdev);

	cpld_led->brightness = value;

	/* Must use workqueue for the actual I/O since I2C operations
	 * can sleep.  */
	schedule_work(&cpld_led->work);
}

static int
cpld_led_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cpld_led *cpld_led;
	int i, err;
	const char *prefix = NULL;
	struct device_node *np = client->dev.of_node;

	cpld_led = kcalloc(16, sizeof(*cpld_led), GFP_KERNEL);
	if (!cpld_led) {
		dev_err(&client->dev, "out of memory\n");
		return -ENOMEM;
	}

	i2c_set_clientdata(client, cpld_led);

	err = of_property_read_string(np, "prefix", &prefix);
	if (err && err != -EINVAL) {
		dev_err(&client->dev, "invalid 'prefix'\n");
		goto err_free;
	}

	for (i = 0; i < 16; i++) {
		cpld_led[i].client = client;
		cpld_led[i].led_num = i;

		if (prefix)
			snprintf(cpld_led[i].name, sizeof(cpld_led[i].name), "%s%d", prefix, i);
		else
			snprintf(cpld_led[i].name, sizeof(cpld_led[i].name), "cpld%d", i);

		cpld_led[i].led_cdev.name = cpld_led[i].name;
		cpld_led[i].led_cdev.max_brightness = 1;
		cpld_led[i].led_cdev.brightness_set = cpld_led_brightness_set;

		INIT_WORK(&cpld_led[i].work, cpld_led_work);

		err = led_classdev_register(&client->dev, &cpld_led[i].led_cdev);
		if (err < 0) {
			dev_err(&client->dev, "led_classdev register failed\n");
			goto err_register;
		}

		cpld_led_brightness_set(&cpld_led[i].led_cdev, 0);
	}

	return 0;

err_register:
	while (i--) {
		led_classdev_unregister(&cpld_led[i].led_cdev);
		cancel_work_sync(&cpld_led[i].work);
	}
err_free:
	kfree(cpld_led);

	return err;
}

static int cpld_led_remove(struct i2c_client *client)
{
	struct cpld_led *cpld_led = i2c_get_clientdata(client);
	int i;

	for (i = 0; i < 16; i++) {
		led_classdev_unregister(&cpld_led[i].led_cdev);
		cancel_work_sync(&cpld_led[i].work);
	}
	kfree(cpld_led);

	return 0;
}

static struct i2c_driver cpld_led_driver = {
	.driver = {
		.name	= "leds-cpld",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(cpld_led_of_id),
	},
	.probe	= cpld_led_probe,
	.remove	= cpld_led_remove,
	.id_table = cpld_led_id,
};

module_i2c_driver(cpld_led_driver);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("CPLD LED driver");
MODULE_LICENSE("GPL");
