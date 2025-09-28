/*
 * This file is part of DSPG Technologies' TPS65132B2 driver.
 *
 * The TPS65132B2 driver is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * The TPS65132B2 driver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the TPS65132B2 driver.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tps65132b2.h"

#include <linux/errno.h>

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define TPS6513_DEBUG_I2C

static int
tps65132b2_read_reg(const struct i2c_client *client, u8 reg, u8 *data,
		    int eeprom);

static int
tps65132b2_write_reg(const struct i2c_client *client, u8 reg, u8 data);

static int
tps65132b2_read_dac(const struct i2c_client *client, u8 *data)
{
	int ret;
	u8 send[2];
	struct i2c_msg msg[2];

	send[0] = REG_CONTROL;
	send[1] = 0x0;

	tps65132b2_write_reg(client, REG_CONTROL, 0x0);

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 1;
	msg[0].buf = (char *)(send + 1);

	msg[1].addr = client->addr;
	msg[1].flags = client->flags & I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = TPS6513_REGS;
	msg[1].buf = data;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2)
		return -EIO;

	return TPS6513_REGS;
}

static int
tps65132b2_write_reg(const struct i2c_client *client, u8 reg, u8 data)
{
	int ret;
	u8 trans_data[2];

	trans_data[0] = reg;
	trans_data[1] = data;

	ret = i2c_master_send(client, trans_data, 2);
#ifdef TPS6513_DEBUG_I2C
	if (reg != REG_CONTROL) {
		dev_info(&client->dev, "writing %2.2x to [%2.2x] returned %d\n",
			 data, reg, ret);
	}
#endif
	if (ret != 2)
		return -EIO;
#ifdef TPS6513_DEBUG_I2C
	if (reg != REG_CONTROL) {
		ret = tps65132b2_read_reg(client, reg, trans_data, 0);
		dev_info(&client->dev, "read back %2.2x\n", trans_data[0]);
	}
#endif
	return ret;
}

/* copy DAC register contents to EEPROM */
static int
tps65132b2_save_regs(const struct i2c_client *client)
{
	int ret = 0;

	dev_info(&client->dev, "writing EEPROM\n");

	ret = tps65132b2_write_reg(client, REG_CONTROL, WRITE_EEPROM);

	/* I2C is unresponsive for 50ms */
	if (!ret)
		msleep(50);

	return ret;
}

static int
tps65132b2_read_reg(const struct i2c_client *client, u8 reg, u8 *data,
		    int eeprom)
{
	int ret;

	/* DAC or EEPROM selection */
	ret = tps65132b2_write_reg(client, REG_CONTROL, !!eeprom);
	if (ret < 0)
		return ret;
	ret = i2c_master_send(client, (void *)&reg, 1);
	if (ret != 1)
		return -EIO;
	ret = i2c_master_recv(client, data, 1);
	if (ret != 1)
		return -EIO;

	return 0;
}

static int
tps65132b2_parse_dt(struct tps65132b2_data *dev_data)
{
	struct device_node *pnode = dev_data->dev->of_node;
	int ret;
	u32 tmp;

	ret = of_property_read_u32(pnode, "out-p", &tmp);
	if (ret < 0 || tmp < 40 || tmp > 60) {
		dev_err(dev_data->dev,
			"invalid of missing 'out-p' property\n");
		return -EINVAL;
	}
	dev_data->pos_out = tmp;

	ret = of_property_read_u32(pnode, "out-n", &tmp);
	if (ret < 0 || tmp < 40 || tmp > 60) {
		dev_err(dev_data->dev,
			"invalid of missing 'out-n' property\n");
		return -EINVAL;
	}
	dev_data->neg_out = tmp;

	ret = of_property_read_u32(pnode, "up-seq", &tmp);
	if (ret < 0 || tmp > 0x3) {
		dev_err(dev_data->dev,
			"invalid of missing 'up-seq' property\n");
		return -EINVAL;
	}
	dev_data->up_seq = (u8)tmp;

	ret = of_property_read_u32(pnode, "down-seq", &tmp);
	if (ret < 0 || tmp > 0x3) {
		dev_err(dev_data->dev,
			"invalid of missing 'down-seq' property\n");
		return -EINVAL;
	}
	dev_data->down_seq = (u8)tmp;

	tmp = 1;
	ret = of_property_read_u32(pnode, "up-dly-p", &tmp);
	if (ret < 0 || tmp > 0x3) {
		dev_err(dev_data->dev,
			"invalid of missing 'up-dly-p' property\n");
		return -EINVAL;
	}
	dev_data->up_dly_p = (u8)tmp;

	if (dev_data->up_seq) {
		tmp = 1;
		ret = of_property_read_u32(pnode, "up-dly-n", &tmp);
		if (ret < 0 || tmp > 0x3) {
			dev_err(dev_data->dev,
				"invalid of missing 'up-dly-n' property\n");
			return -EINVAL;
		}
		dev_data->up_dly_n = (u8)tmp;
	} else {
		dev_data->up_dly_n = 0;
	}

	tmp = 1;
	ret = of_property_read_u32(pnode, "down-dly-p", &tmp);
	if (ret < 0 || tmp > 0x3) {
		dev_err(dev_data->dev,
			"invalid of missing 'down-dly-p' property\n");
		return -EINVAL;
	}
	dev_data->down_dly_p = (u8)tmp;

	if (dev_data->down_seq) {
		ret = of_property_read_u32(pnode, "down-dly-n", &tmp);
		if (ret < 0 || tmp > 0x3) {
			dev_err(dev_data->dev,
				"invalid of missing 'down-dly-n' property\n");
			return -EINVAL;
		}
		dev_data->down_dly_n = (u8)tmp;
	} else {
		dev_data->down_dly_n = 0;
	}

	return 0;
}

static u8
tps65132b2_get_dly_value(u8 ms)
{
	if (ms >= 3)
		return DLYX_10_MS;
	if (ms >= 2)
		return DLYX_5_MS;
	if (ms >= 1)
		return DLYX_1_MS;
	return DLYX_0_MS;
}

static int
tps65132b2_set_delays(const struct tps65132b2_data *dev_data, u8 *registers)
{
	u8 tmp, cmp, dly_n1, dly_p1, dly_n2, dly_p2;

	dly_n1 = tps65132b2_get_dly_value(dev_data->up_dly_n);
	dly_p1 = tps65132b2_get_dly_value(dev_data->up_dly_p);
	dly_n2 = tps65132b2_get_dly_value(dev_data->down_dly_n);
	dly_p2 = tps65132b2_get_dly_value(dev_data->down_dly_p);

	tps65132b2_read_reg(dev_data->i2c_client, REG_DLYX, &tmp, 0);
	tmp = registers[REG_DLYX];
	cmp = DLYN1(dly_n1) | DLYP1(dly_p1) | DLYN2(dly_n2) | DLYP2(dly_p2);
	if (tmp != cmp) {
		tps65132b2_write_reg(dev_data->i2c_client, REG_DLYX, cmp);
		registers[REG_DLYX] = cmp;
		return 1;
	}

	return 0;
}

static int
tps65132b2_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err, config_change = 0;
	struct tps65132b2_data *dev_data;
	u8 cur, cmp;
	u8 registers[TPS6513_REGS];

	dev_info(&client->dev, "%s: Entry\n", __FUNCTION__);

	dev_data = devm_kzalloc(&client->dev, sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		dev_err(&client->dev, "failed to allocate driver structure\n");
		return -ENOMEM;
	}

	dev_data->dev = &client->dev;
	dev_data->i2c_client = client;

	/* get ressources */
	if (!client->dev.of_node) {
		dev_err(dev_data->dev, "device not in device tree\n");
		return -EINVAL;
	}

	err = tps65132b2_parse_dt(dev_data);
	if (err)
		return err;

	/* allow EEPROM content to be copied to DAC registers */
	udelay(20);

	if (tps65132b2_read_dac(dev_data->i2c_client, registers) !=
	    TPS6513_REGS)
		return -EINVAL;

	dev_info(dev_data->dev, "DAC: [%2.2x] [%2.2x] [%2.2x] [%2.2x]\n",
		 registers[0], registers[1], registers[2], registers[3]);

	cur = registers[REG_VPOS];
	if ((cur & VOUT_MASK) != VOUT(dev_data->pos_out)) {
		err = tps65132b2_write_reg(client, REG_VPOS,
					   VOUT(dev_data->pos_out));
		if (err < 0)
			return err;
		registers[REG_VPOS] = VOUT(dev_data->pos_out);
		config_change = 1;
	}

	cur = registers[REG_VNEG];
	if ((cur & VOUT_MASK) != VOUT(dev_data->neg_out)) {
		err = tps65132b2_write_reg(client, REG_VNEG,
					   VOUT(dev_data->neg_out));
		if (err < 0)
			return err;
		registers[REG_VNEG] = VOUT(dev_data->neg_out);
		config_change = 1;
	}

	/* SEQU and SEQD */
	cur = registers[REG_DIV];
	cmp = DIV_SEQU(dev_data->up_seq) | DIV_SEQD(dev_data->down_seq) |
	      DIV_APPS(1) /* tablet */;
	if (cur != cmp) {
		err = tps65132b2_write_reg(client, REG_DIV, cmp);
		if (err < 0)
			return err;
		registers[REG_DIV] = cmp;
		config_change = 1;
	}

	/* DLYx */
	config_change |= tps65132b2_set_delays(dev_data, registers);

	if (config_change)
		tps65132b2_save_regs(client);

	tps65132b2_read_dac(dev_data->i2c_client, registers);

	dev_info(dev_data->dev, "DAC: [%2.2x] [%2.2x] [%2.2x] [%2.2x]\n",
		 registers[0], registers[1], registers[2], registers[3]);

	dev_info(dev_data->dev, "%s: successfully probed\n", __FUNCTION__);

	return 0;
}

static int
tps65132b2_remove(struct i2c_client *client)
{
	/* nothing to do here since ENP and ENN are fixed */
	return 0;
}

#define TPS65132B2_PM_OPS		NULL

static const struct i2c_device_id tps65132b2_i2c_ids[] = {
	{ "tps65132b2", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, tps65132b2_i2c_ids);

static const struct of_device_id tps65132b2_of_match[] = {
	{ .compatible = "ti,tps65132b2", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, tps65132b2_of_match);

static struct i2c_driver tps65132b2_driver = {
	.driver = {
		.name = "tps65132b2",
		.owner = THIS_MODULE,
		.pm = TPS65132B2_PM_OPS,
		.of_match_table = tps65132b2_of_match,
	},
	.probe  = tps65132b2_probe,
	.remove = tps65132b2_remove,
	.id_table = tps65132b2_i2c_ids,
};

module_i2c_driver(tps65132b2_driver);

MODULE_DESCRIPTION("TPS65132B2 dual voltage driver");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
