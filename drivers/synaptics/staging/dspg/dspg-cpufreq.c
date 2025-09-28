// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2024 Synaptics Incorporated */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>

#define MODULE_NAME		"dspg-cpufreq"

static int
dspg_cpufreq_init(void)
{
	struct device *cpu_dev;
	struct clk *cpu_clk;
	struct opp_table *cpufreq_opp_table;
	unsigned int version[1];
	int ret = 0;

	if (!of_machine_is_compatible("dspg,dvf101"))
		return -ENODEV;

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		pr_err("%s: no CPU device\n", MODULE_NAME);
		return -ENODEV;
	}

	cpu_clk = devm_clk_get(cpu_dev, "cpu");
	if (!cpu_clk) {
		dev_err(cpu_dev, "could not get cpufreq clock\n");
		return -ENODEV;
	}

	/* check if CPU is already running higher than 1GHz from bootloader */
	if (clk_get_rate(cpu_clk) > 1000000000)
		version[0] = BIT(15);
	else
		version[0] = BIT(31);
	cpufreq_opp_table = dev_pm_opp_set_supported_hw(cpu_dev, version, 1);
	if (IS_ERR(cpufreq_opp_table)) {
		ret = PTR_ERR(cpufreq_opp_table);
		dev_err(cpu_dev, "failed to set supported hardware\n");
		return ret;
	}

	platform_device_register_simple("cpufreq-dt", -1, NULL, 0);

	return ret;
}
module_init(dspg_cpufreq_init);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL v2");
