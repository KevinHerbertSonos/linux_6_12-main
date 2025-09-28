/*
 * This file is part of DSPG Technologies' Guardian driver.
 *
 * The Guardian driver is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 *
 * The Guardian driver is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Guardian driver.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_gpio.h>

struct piog_data {
	struct device	*dev;
	int		protectee;
	bool		high;
	bool		low;
	bool		dir;
};

static int
piog_parse_dt(struct piog_data *data)
{
	struct device_node *pnode = data->dev->of_node;

	data->protectee = of_get_named_gpio(pnode, "protectee", 0);
	if (data->protectee >= 0 && gpio_is_valid(data->protectee)) {
		data->high = of_property_read_bool(pnode, "high");
		data->low = of_property_read_bool(pnode, "low");
		data->dir = of_property_read_bool(pnode, "out");
	} else {
		dev_err(data->dev, "could not parse 'protectee' property\n");
		return -EIO;
	}

	return 0;
}

static int
piog_probe(struct platform_device *pdev)
{
	struct piog_data *data = pdev->dev.platform_data;
	int ret;
	unsigned long flags = 0;

	dev_info(&pdev->dev, "probing DSPG PIO Guardian");

	if (!data) {
		data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
		if (!data) {
			dev_err(&pdev->dev, "no memory for device data\n");
			return -ENOMEM;
		}
	}

	data->dev = &pdev->dev;
	ret = piog_parse_dt(data);
	if (ret < 0) {
		dev_err(&pdev->dev, "could not parse device tree\n");
		return ret;
	}

	if (data->high)
		flags |= GPIOF_INIT_HIGH;
	else if (data->low)
		flags |= GPIOF_INIT_LOW;

	if (data->dir)
		flags |= GPIOF_DIR_OUT;
	else
		flags |= GPIOF_DIR_IN;

	ret = devm_gpio_request_one(data->dev, data->protectee, flags,
				    "dspg-piog");
	if (ret) {
		dev_err(data->dev,
			"failed to request protected gpio %d\n",
			data->protectee);
		return -EIO;
	}

	return ret;
}

static int
piog_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id piog_of_match[] = {
	{ .compatible = "dspg,pio-guardian", },
	{ /* guardian */ },
};
MODULE_DEVICE_TABLE(of, piog_of_match);

static struct platform_driver piog_driver = {
	.driver = {
		.name		= "dspg-pio-guardian",
		.owner		= THIS_MODULE,
		.pm		= NULL,
		.of_match_table	= piog_of_match,
	},
	.probe	= piog_probe,
	.remove	= piog_remove,
};

static int __init piog_init(void)
{
	return platform_driver_register(&piog_driver);
}
fs_initcall(piog_init);

static void __exit piog_exit(void)
{
	platform_driver_unregister(&piog_driver);
}
module_exit(piog_exit);

MODULE_DESCRIPTION("PIO guardian");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
