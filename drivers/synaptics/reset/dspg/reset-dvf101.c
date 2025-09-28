// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2024 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset-controller.h>
#include <linux/mfd/syscon.h>
#include <linux/sysfs.h>
#include <mach/hardware.h>
#include <dt-bindings/reset-controller/dvf101-resets.h>

struct dvf_reset_channel_data {
	char *name;
	struct reg_field reg_field;
};

struct dvf_reset_controller_data {
	const struct dvf_reset_channel_data *channels;
	unsigned int nr_of_channels;
};

struct dvf_reset_channel {
	struct regmap_field *regmap_field;
};

struct dvf_reset_controller {
	spinlock_t lock;
	struct reset_controller_dev rst;
	struct dvf_reset_channel *channels;
};

#define to_dvf_reset_controller(_rst) \
	container_of(_rst, struct dvf_reset_controller, rst)

static int dvf_set_reset(struct reset_controller_dev *rcdev, unsigned long idx,
			 unsigned int val)
{
	struct dvf_reset_controller *rst = to_dvf_reset_controller(rcdev);
	const struct dvf_reset_channel *ch;

	if (idx >= rcdev->nr_resets)
		return -EINVAL;

	ch = &rst->channels[idx];

	return regmap_field_write(ch->regmap_field, val);
}

static int dvf_reset_assert(struct reset_controller_dev *rcdev,
			    unsigned long idx)
{
	return dvf_set_reset(rcdev, idx, 1);
}

static int dvf_reset_deassert(struct reset_controller_dev *rcdev,
			      unsigned long idx)
{
	return dvf_set_reset(rcdev, idx, 0);
}

static int dvf_reset_status(struct reset_controller_dev *rcdev,
			    unsigned long id)
{
	struct dvf_reset_controller *rst = to_dvf_reset_controller(rcdev);
	const struct dvf_reset_channel *ch;
	unsigned int val;
	int ret;

	if (id >= rcdev->nr_resets)
		return -EINVAL;

	ch = &rst->channels[id];

	ret = regmap_field_read(ch->regmap_field, &val);
	if (ret)
		return ret;

	return !!val;
}

/* reset without clock */
#define _RESET(_idx, _reg, _bit)					\
	[DVF_101_ ## _idx ## _RESET] = {				\
		.name = __stringify(_idx),                              \
		.reg_field = REG_FIELD(DVF_CMU_ ## _reg, _bit, _bit),	\
	}

static const struct dvf_reset_channel_data dvf101_resets[] = {
	_RESET(CSS_SYS, CSS_CTRL, 14),
	_RESET(CSS_ETM, CSS_CTRL, 13),
	_RESET(CSS_CPU, CSS_CTRL,  0),
	_RESET(TDM1, TDM1_CTRL, 0),
	_RESET(TDM2, TDM2_CTRL, 0),
	_RESET(TDM3, TDM3_CTRL, 0),
	_RESET(UART1, UART1_CTRL, 0),
	_RESET(UART2, UART2_CTRL, 0),
	_RESET(UART3, UART3_CTRL, 0),
	_RESET(UART4, UART4_CTRL, 0),
	_RESET(NFC, NFC_CTRL, 0),
	_RESET(BMP, BMP_CTRL, 0),
	_RESET(CS, CS_CTRL, 0),
	_RESET(DCLS, DCLS_CTRL, 0),
	_RESET(DAIF, DAIF_CTRL, 0),
	_RESET(LCDC, LCDC_CTRL, 0),
	_RESET(PWM_GEN, PWM_GEN_CTRL, 0),
	_RESET(CRYPTO, CRYPTO_CTRL, 0),
	_RESET(GDMAC, MCU_GDMAC_CTRL, 0),
	_RESET(CSS_GDMAC, CSS_GDMAC_CTRL, 0),
	_RESET(SEC, SEC_CTRL, 0),
	_RESET(SPI1, SPI1_CTRL, 0),
	_RESET(SPI2, SPI2_CTRL, 0),
	_RESET(SPI3, SPI3_CTRL, 0),
	_RESET(I2C1, I2C1_CTRL, 0),
	_RESET(I2C2, I2C2_CTRL, 0),
	_RESET(GPIO, GPIO_CTRL, 0),
	_RESET(RTC, RTC_CTRL, 0),
	_RESET(WD, WD_CTRL, 0),
	_RESET(DRT, DRT_CTRL, 0),
	_RESET(ADPCM, ADPCM_CTRL, 0),
	_RESET(TIMER12, MCU_TIMER12_CTRL, 0),
	_RESET(TIMER34, MCU_TIMER34_CTRL, 0),
	_RESET(CSS_TIMER12, CSS_TIMER12_CTRL, 0),
	_RESET(CCU, CCU_CTRL, 0),
	_RESET(GPU, GPU_CTRL, 0),
	_RESET(ETH, ETH_CTRL, 0),
	/* TODO DDR */
	_RESET(SDIO, SDIO_CTRL, 0),
	_RESET(EMMC, EMMC_CTRL, 0),
	_RESET(DSI, DSI_CTRL, 0),
	_RESET(USB_PHY2, USB_PHY_CTRL, 1),
	_RESET(USB_PHY1, USB_PHY_CTRL, 0),
	_RESET(OTG_P, USB_OTG_CTRL, 1),
	_RESET(OTG_H, USB_OTG_CTRL, 0),
	_RESET(USB_HOST_UTMI1, USB_HOST_CTRL, 5),
	_RESET(USB_HOST_UTMI0, USB_HOST_CTRL, 4),
	_RESET(USB_HOST_OHCI_H, USB_HOST_CTRL, 3),
	_RESET(USB_HOST_EHCI_H, USB_HOST_CTRL, 2),
	_RESET(USB_HOST_AUX_WELL, USB_HOST_CTRL, 1),
	_RESET(USB_HOST_PHY, USB_HOST_CTRL, 0),
};

static const struct dvf_reset_controller_data dvf101_reset_controller_data = {
	.channels = dvf101_resets,
	.nr_of_channels = ARRAY_SIZE(dvf101_resets)
};

static struct of_device_id dvf101_reset_match[] = {
	{
		.compatible = "dspg,dvf101-resets",
		.data = &dvf101_reset_controller_data,
	},
	{ /* sentinel */ },
};

static struct reset_control_ops dvf_reset_ops = {
	.assert   = dvf_reset_assert,
	.deassert = dvf_reset_deassert,
	.status   = dvf_reset_status,
};

static ssize_t status_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct dvf_reset_controller *rc = dev_get_drvdata(dev);
	int ret;
	unsigned long id = 0;
	ssize_t size = 0;

	size += sprintf(buf, "\n");
	do {
		ret = dvf_reset_status(&rc->rst, id);
		if (ret >= 0)
			size += sprintf(buf + size, "%s: %sasserted\n",
					dvf101_resets[id].name,
					ret? "  ": "de");
		id++;
	} while (ret != -EINVAL);

	return size;
}

static DEVICE_ATTR_RO(status);

static const struct attribute *dvf101_reset_sysfs[] = {
	&dev_attr_status.attr,
	NULL,
};

static int dvf_reset_controller_register(struct device *dev,
				const struct dvf_reset_controller_data *data)
{
	struct dvf_reset_controller *rc;
	struct regmap *map;
	size_t size;
	int i, ret;

	rc = devm_kzalloc(dev, sizeof(*rc), GFP_KERNEL);
	if (!rc)
		return -ENOMEM;

	dev_set_drvdata(dev, rc);

	spin_lock_init(&rc->lock);

	size = sizeof(struct dvf_reset_channel) * data->nr_of_channels;

	rc->channels = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!rc->channels)
		return -ENOMEM;

	rc->rst.ops = &dvf_reset_ops,
	rc->rst.of_node = dev->of_node;
	rc->rst.nr_resets = data->nr_of_channels;

	map = syscon_regmap_lookup_by_phandle(dev->of_node, "dvf,syscfg");
	if (IS_ERR(map))
		return PTR_ERR(map);

	for (i = 0; i < data->nr_of_channels; i++) {
		struct regmap_field *f;

		f = devm_regmap_field_alloc(dev, map,
					    data->channels[i].reg_field);
		if (IS_ERR(f))
			return PTR_ERR(f);

		rc->channels[i].regmap_field = f;
	}

	ret = sysfs_create_files(&dev->kobj, dvf101_reset_sysfs);
	if (ret)
		return ret;

	ret = reset_controller_register(&rc->rst);
	if (!ret)
		dev_info(dev, "registered\n");

	return ret;
}

static int
dvf_reset_probe(struct platform_device *pdev)
{
	struct device *dev = pdev ? &pdev->dev : NULL;
	const struct of_device_id *match;

	if (!dev || !dev->driver)
		return -ENODEV;

	match = of_match_device(dev->driver->of_match_table, dev);
	if (!match || !match->data)
		return -EINVAL;

	return dvf_reset_controller_register(dev, match->data);
}

static struct platform_driver dvf101_reset_driver = {
	.probe = dvf_reset_probe,
	.driver = {
		.name = "reset-dvf101",
		.of_match_table = dvf101_reset_match,
	},
};

static int __init dvf101_reset_init(void)
{
	return platform_driver_register(&dvf101_reset_driver);
}
/* TODO: need to call earlier for RTC, timer (clockevent/clocksource),
 * and CCU */
arch_initcall(dvf101_reset_init);
