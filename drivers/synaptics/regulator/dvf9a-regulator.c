/*
 * DSPG DVF9A pmu driver
 *
 * Copyright (c) 2012, DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/dvf9a.h>
#include <linux/of.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>

#define DVF9A_PMU "dvf9a-pmu"

#define DVF9A_DCDC_MAX_UV	1380000
#define DVF9A_DCDC_MIN_UV	1070000
#define DVF9A_DCDC_STEP_UV	10000
#define DVF9A_DCDC_SUSPEND_STEP	20000
#define DVF9A_DCDC_MAX_SEL \
		((DVF9A_DCDC_MAX_UV - DVF9A_DCDC_MIN_UV) / DVF9A_DCDC_STEP_UV)

struct dvf9a_pmu {
	struct dvf9a *dvf9a;
	struct platform_device *pdev;

	unsigned short dpuctl1;
	unsigned short dpuctl6;
	unsigned short pmuen;
	unsigned short auxamp;
	unsigned short afecfg1;
};

struct dvf9a_dcdc {
	char name[6];
	struct regulator_desc desc;
	struct dvf9a_pmu *dvf9a_pmu;
	struct regulator_dev *regulator;
};

static int
dvf9a_dcdc_list_voltage(struct regulator_dev *rdev, unsigned int selector)
{
	if (selector <= DVF9A_DCDC_MAX_SEL)
		return DVF9A_DCDC_MIN_UV + (selector * DVF9A_DCDC_STEP_UV);

	return -EINVAL;
}

static int
dvf9a_dcdc_set_voltage(struct regulator_dev *rdev,
		       int min_uV, int max_uV, unsigned int *selector)
{
	struct dvf9a_pmu *dvf9a_pmu = rdev_get_drvdata(rdev);
	int vsel, uV;

	if (min_uV <= DVF9A_DCDC_MAX_UV)
		vsel = (min_uV - DVF9A_DCDC_MIN_UV) / DVF9A_DCDC_STEP_UV;
	else
		return -EINVAL;

	uV = dvf9a_dcdc_list_voltage(rdev, vsel);
	if (uV > max_uV)
		return -EINVAL;

	*selector = vsel;

	daif_write(dvf9a_pmu->dvf9a, DAIF_DPUCTL1, vsel & DVF9A_DCDC_MAX_SEL);
	rdev->constraints->state_mem.uV = uV - DVF9A_DCDC_SUSPEND_STEP;

	return 0;
}

static int
dvf9a_dcdc_get_voltage_sel(struct regulator_dev *rdev)
{
	struct dvf9a_pmu *dvf9a_pmu = rdev_get_drvdata(rdev);

	return daif_read(dvf9a_pmu->dvf9a, DAIF_DPUCTL1) & DVF9A_DCDC_MAX_SEL;
}

static int
dvf9a_dcdc_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
	unsigned int selector;

	return dvf9a_dcdc_set_voltage(rdev, uV, uV, &selector);
}

static struct regulator_ops dvf9a_dcdc_ops = {
	.set_voltage = dvf9a_dcdc_set_voltage,
	.get_voltage_sel = dvf9a_dcdc_get_voltage_sel,
	.list_voltage = dvf9a_dcdc_list_voltage,
	.set_suspend_voltage = dvf9a_dcdc_set_suspend_voltage,
};

#ifdef CONFIG_PM
static int
dvf9a_pmu_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	struct dvf9a_pmu *dvf9a_pmu = platform_get_drvdata(pdev);

	dvf9a_pmu->afecfg1 = daif_read(dvf9a_pmu->dvf9a, DAIF_AFECFG1);
	dvf9a_pmu->dpuctl6 = daif_read(dvf9a_pmu->dvf9a, DAIF_DPUCTL6);
	dvf9a_pmu->dpuctl1 = daif_read(dvf9a_pmu->dvf9a, DAIF_DPUCTL1);
	dvf9a_pmu->auxamp  = daif_read(dvf9a_pmu->dvf9a, DAIF_AUXAMP);
	dvf9a_pmu->pmuen   = daif_read(dvf9a_pmu->dvf9a, DAIF_PMUEN);

	daif_write(dvf9a_pmu->dvf9a, DAIF_AFECFG1, 0);

	/* optimize 1.2V DCDC for low current efficiency:
	 * constant duty-cycle mode instead of PFM mode
	 */
	daif_write(dvf9a_pmu->dvf9a, DAIF_DPUCTL6, dvf9a_pmu->dpuctl6 | 0x10);

	/* reduce core DCDC by 20mV */
	daif_write(dvf9a_pmu->dvf9a, DAIF_DPUCTL1, dvf9a_pmu->dpuctl1 - 2);

	/* configure AUXAMP to minimum */
	daif_write(dvf9a_pmu->dvf9a, DAIF_AUXAMP,  0);

	/* disable BCLK and XTAL */
	if ((dvf9a_pmu->pmuen & (1<<5)) /*|| !(dvf9a_pmu->pmuen | (1<<7))*/)
		daif_write(dvf9a_pmu->dvf9a, DAIF_PMUEN,
			   (dvf9a_pmu->pmuen & ~(1<<5)) /*| (1<<7)*/);

	return 0;
}

static int
dvf9a_pmu_resume(struct platform_device *pdev)
{
	struct dvf9a_pmu *dvf9a_pmu = platform_get_drvdata(pdev);

	/* Restore PMU settings */
	if (dvf9a_pmu->pmuen & (1<<5))
		daif_write(dvf9a_pmu->dvf9a, DAIF_PMUEN, dvf9a_pmu->pmuen);

	daif_write(dvf9a_pmu->dvf9a, DAIF_AUXAMP,  dvf9a_pmu->auxamp);
	daif_write(dvf9a_pmu->dvf9a, DAIF_DPUCTL6, dvf9a_pmu->dpuctl6);
	daif_write(dvf9a_pmu->dvf9a, DAIF_DPUCTL1, dvf9a_pmu->dpuctl1);
	daif_write(dvf9a_pmu->dvf9a, DAIF_AFECFG1, dvf9a_pmu->afecfg1);

	return 0;
}
#else
#define dvf9a_pmu_suspend	NULL
#define dvf9a_pmu_resume	NULL
#endif /* CONFIG_PM */

static int
dvf9a_pmu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct dvf9a *dvf9a = dev_get_drvdata(pdev->dev.parent);
	struct dvf9a_pmu *dvf9a_pmu;
	struct dvf9a_dcdc *dcdc;
	struct regulator_init_data *initdata;
	struct regulator_config config = { };
	int ret;

	dvf9a_pmu = devm_kzalloc(dev, sizeof(*dvf9a_pmu), GFP_KERNEL);
	if (!dvf9a_pmu)
		return -ENOMEM;

	dvf9a_pmu->pdev = pdev;
	dvf9a_pmu->dvf9a = dvf9a;

	dcdc = kzalloc(sizeof(*dcdc), GFP_KERNEL);
	if (!dcdc)
		return -ENOMEM;

	sprintf(dcdc->name, "DVF9A-DCDC");
	dcdc->desc.name = dcdc->name;
	dcdc->desc.id = -1;
	dcdc->desc.type = REGULATOR_VOLTAGE;
	dcdc->desc.n_voltages = 32;
	dcdc->desc.ops = &dvf9a_dcdc_ops;
	dcdc->desc.owner = THIS_MODULE;
	initdata = of_get_regulator_init_data(&pdev->dev, np, &dcdc->desc);

	config.dev = &pdev->dev;
	config.init_data = initdata;
	config.driver_data = dvf9a_pmu;
	config.of_node = np;

	dcdc->regulator = regulator_register(&dcdc->desc, &config);
	if (IS_ERR(dcdc->regulator)) {
		ret = PTR_ERR(dcdc->regulator);
		dev_err(&pdev->dev, "failed to register DCDC regulator: %d\n",
			ret);
		return ret;
	}

	dcdc->regulator->constraints->state_mem.uV =
			dvf9a_dcdc_list_voltage(dcdc->regulator,
			dvf9a_dcdc_get_voltage_sel(dcdc->regulator)) -
			DVF9A_DCDC_SUSPEND_STEP;

	platform_set_drvdata(pdev, dvf9a_pmu);

	dev_info(&pdev->dev, "successfully registered\n");

	return 0;
}

static int
dvf9a_pmu_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id dvf9a_pmu_of_ids[] = {
	{ .compatible = "dspg,dvf9a-pmu" },
	{ },
};

static struct platform_driver dvf9a_pmu_platform_driver = {
	.driver = {
		.name	= DVF9A_PMU,
		.owner	= THIS_MODULE,
		.of_match_table = dvf9a_pmu_of_ids,
	},
	.probe		= dvf9a_pmu_probe,
	.remove		= dvf9a_pmu_remove,
	.suspend	= dvf9a_pmu_suspend,
	.resume		= dvf9a_pmu_resume,
};

static int __init dvf9a_pmu_init(void)
{
	return platform_driver_register(&dvf9a_pmu_platform_driver);
}

static void __exit dvf9a_pmu_exit(void)
{
	platform_driver_unregister(&dvf9a_pmu_platform_driver);
}

module_init(dvf9a_pmu_init);
module_exit(dvf9a_pmu_exit);

MODULE_DESCRIPTION("DSPG DVF9A-PMU driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_ALIAS("platform:dvf9a-pmu");
