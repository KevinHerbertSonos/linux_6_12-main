/**
 *  Copyright (C) 2016 DSP Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mmc/host.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <linux/sysfs.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"

#define DW_MCI_DSPG_MAX_TUNE_VALS	20
/*#define DW_MCI_DSPG_DEBUG_TUNING*/

/* Comma separated phase shifts for DDR52 mode:
 *     ddr52tune=<drive shift>,<sample shift>
 *
 * These values have precedence over values parsed from the device tree.
 */
static char *ddr52tune;
static int
ddr52tune_setup(char *str)
{
	ddr52tune = str;
	return 1;
}
__setup("ddr52tune=", ddr52tune_setup);

struct dw_mci_dspg_priv_data {
	struct clk *clk_d;
	struct clk *clk_s;
	struct clk *main_in;

	/* MMC_TIMING_LEGACY
	 * MMC_TIMING_MMC_HS
	 * MMC_TIMING_SD_HS
	 * MMC_TIMING_UHS_SDR12
	 * MMC_TIMING_UHS_SDR25
	 * MMC_TIMING_UHS_SDR50
	 * MMC_TIMING_UHS_SDR104
	 * MMC_TIMING_UHS_DDR50
	 * MMC_TIMING_MMC_DDR52
	 * MMC_TIMING_MMC_HS200
	 * MMC_TIMING_MMC_HS400
	 */
	int shift_d[MMC_TIMING_MMC_HS400 + 1];
	int shift_s[MMC_TIMING_MMC_HS400 + 1];
	int tune_drive_clk;
};

static inline int
find_best_value(int results[], int values, int interval_start)
{
	int best_value;
	int i = interval_start;
	int longest = 0;
	int cur_length = 0;
	int cur_sum = 0;

	do {
		if (results[i % values] == 1) {
			cur_length++;
			cur_sum += i;
		} else if (results[i % values] == 0) {
			if (cur_length > longest) {
				longest = cur_length;
				best_value = (cur_sum / cur_length) % values;
			}
			cur_length = 0;
			cur_sum = 0;
		} else {
			return -EINVAL;
		}

		i++;
	} while ((i % values) != interval_start);

	return best_value;
}

static int
dw_mci_dspg_tune_clk_in(struct dw_mci_slot *slot, u32 opcode,
			int *best_phase)
{
	struct dw_mci *host = slot->host;
	struct dw_mci_dspg_priv_data *priv = host->priv;
	struct mmc_host *mmc = slot->mmc;
	int results[DW_MCI_DSPG_MAX_TUNE_VALS];
	unsigned int tried_phases;
	int i;
	int interval_start = -1;
	int good_phases = 0;

	for (tried_phases = 0; tried_phases < DW_MCI_DSPG_MAX_TUNE_VALS;
	     tried_phases++) {
		if (clk_set_phase(priv->clk_s, tried_phases))
			break;
		udelay(100);
#ifdef DW_MCI_DSPG_DEBUG_TUNING
		dev_info(host->dev, "    sample phase %d:", tried_phases);
#endif

		if (mmc_send_tuning(mmc, opcode, NULL)) {
#ifdef DW_MCI_DSPG_DEBUG_TUNING
			dev_info(host->dev, "    ERR");
#endif
			results[tried_phases] = 0;
		} else {
#ifdef DW_MCI_DSPG_DEBUG_TUNING
			dev_info(host->dev, "    OK");
#endif
			results[tried_phases] = 1;
			good_phases++;
		}
	}

	if (!tried_phases) {
		dev_err(host->dev, "could not set drive clock phase");
		return -EIO;
	}

	for (i = 0; i < tried_phases; i++)
		if (results[i] == 0 && results[(i + 1) % tried_phases] == 1)
			interval_start = (i + 1) % tried_phases;

	if (interval_start < 0) {
		/* all phases are valid or invalid */
		*best_phase = 0;
		return good_phases;
	}

	*best_phase = find_best_value(results, tried_phases, interval_start);

	return good_phases;
}

static int
dw_mci_dspg_phase_index(struct mmc_ios *ios)
{
	return ios->timing;
}

static int
dw_mci_dspg_execute_tuning(struct dw_mci_slot *slot, u32 opcode)
{
	struct dw_mci *host = slot->host;
	struct dw_mci_dspg_priv_data *priv = host->priv;
	int in_phase, phase_index;

	phase_index = dw_mci_dspg_phase_index(&slot->mmc->ios);

	if (priv->tune_drive_clk) {
		int best_out_phase = -1;
		int best_in_phase = -1;
		int best = 0;
		int out_phase, cur;

		for (out_phase = 0; out_phase < DW_MCI_DSPG_MAX_TUNE_VALS;
		     out_phase++) {
			if (clk_set_phase(priv->clk_d, out_phase))
				break;
			udelay(100);

			cur = dw_mci_dspg_tune_clk_in(slot, opcode, &in_phase);
#ifdef DW_MCI_DSPG_DEBUG_TUNING
			dev_info(host->dev, "drive phase %d score: %d",
				 out_phase, cur);
#endif
			if (cur > best) {
				best = cur;
				best_out_phase = out_phase;
				best_in_phase = in_phase;
			}
		}

		if (best_out_phase < 0 || best_in_phase < 0)
			return -EIO;

		clk_set_phase(priv->clk_d, best_out_phase);
		clk_set_phase(priv->clk_s, best_in_phase);
		udelay(100);

		priv->shift_d[phase_index] = best_out_phase;
		priv->shift_s[phase_index] = best_in_phase;
		priv->tune_drive_clk = 0;
	} else {
		dw_mci_dspg_tune_clk_in(slot, opcode, &in_phase);

		if (in_phase < 0)
			return -EIO;

		clk_set_phase(priv->clk_s, in_phase);
		udelay(100);

		priv->shift_s[phase_index] = in_phase;
	}

	return 0;
}

static int
dw_mci_dspg_parse_dt99(struct dw_mci *host)
{
	struct dw_mci_dspg_priv_data *priv;
	const struct dw_mci_drv_data *drv_data = host->drv_data;

	priv = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	if (of_property_read_bool(host->dev->of_node, "dspg,non-removable"))
		*drv_data->caps |= MMC_CAP_NONREMOVABLE;

	host->priv = priv;
	dev_set_drvdata(host->dev, host);

	return 0;
}

static ssize_t
tune_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t size)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_dspg_priv_data *priv = host->priv;

	priv->tune_drive_clk = 1;

	return size;
}

static ssize_t
drive_shift_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_dspg_priv_data *data = host->priv;

	return sprintf(buf, "[%d %d %d %d %d]\n",
		       data->shift_d[0], data->shift_d[1], data->shift_d[2],
		       data->shift_d[3], data->shift_d[4]);
}

static ssize_t
sample_shift_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_dspg_priv_data *data = host->priv;

	return sprintf(buf, "[%d %d %d %d %d]\n",
		       data->shift_s[0], data->shift_s[1], data->shift_s[2],
		       data->shift_s[3], data->shift_s[4]);
}

static DEVICE_ATTR_WO(tune);
static DEVICE_ATTR_RO(drive_shift);
static DEVICE_ATTR_RO(sample_shift);

static struct attribute *dw_mci_dspg_attrs[] = {
	&dev_attr_tune.attr,
	&dev_attr_drive_shift.attr,
	&dev_attr_sample_shift.attr,
	NULL
};
ATTRIBUTE_GROUPS(dw_mci_dspg);

static void
get_shifts_from_bootargs(int *shift_d, int *shift_s)
{
	char **p = &ddr52tune;
	char *shift_d_str, *shift_s_str;
	unsigned long shift_d_tmp, shift_s_tmp;

	if (!*p)
		return;

	shift_d_str = ddr52tune;
	if (strsep(p, ",") && *p) {
		shift_s_str = *p;

		if (!kstrtoul(shift_d_str, 0, &shift_d_tmp) &&
		    !kstrtoul(shift_s_str, 0, &shift_s_tmp)) {
			*shift_d = shift_d_tmp;
			*shift_s = shift_s_tmp;
		}
	}
}

static int
dw_mci_dspg_parse_dt(struct dw_mci *host)
{
	struct dw_mci_dspg_priv_data *priv;
	const struct dw_mci_drv_data *drv_data = host->drv_data;
	int num_elems;

	priv = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->clk_d = devm_clk_get(host->dev, "clk-d");
	if (IS_ERR(priv->clk_d)) {
		dev_err(host->dev,
			"Could not parse 'clk-d' property.  Tuning will be disabled.\n");
		return -EIO;
	}
	priv->clk_s = devm_clk_get(host->dev, "clk-s");
	if (IS_ERR(priv->clk_s)) {
		dev_err(host->dev,
			"Could not parse 'clk-s' property.  Tuning will be disabled.\n");
		return -EIO;
	}
	priv->main_in = devm_clk_get(host->dev, "main-in");
	if (IS_ERR(priv->main_in))
		dev_info(host->dev,
			"Could not parse 'main-in' property.\n");

	priv->shift_d[0] = priv->shift_d[1] = priv->shift_d[2] =
		priv->shift_d[3] = priv->shift_d[4] = -1;
	priv->shift_s[0] = priv->shift_s[1] = priv->shift_s[2] =
		priv->shift_s[3] = priv->shift_s[4] = -1;

	num_elems = of_property_count_u32_elems(host->dev->of_node,
						"dspg,shift-d");
	if (num_elems > 0)
		of_property_read_u32_array(host->dev->of_node, "dspg,shift-d",
					   priv->shift_d,
					   min(ARRAY_SIZE(priv->shift_d),
					       (size_t)num_elems));
	num_elems = of_property_count_u32_elems(host->dev->of_node,
						"dspg,shift-s");
	if (num_elems > 0)
		of_property_read_u32_array(host->dev->of_node, "dspg,shift-s",
					   priv->shift_s,
					   min(ARRAY_SIZE(priv->shift_s),
					       (size_t)num_elems));

	get_shifts_from_bootargs(&priv->shift_d[2], &priv->shift_s[2]);

	if (of_property_read_bool(host->dev->of_node, "dspg,non-removable"))
		*drv_data->caps |= MMC_CAP_NONREMOVABLE;

	host->priv = priv;
	dev_set_drvdata(host->dev, host);

	if (sysfs_create_group(&host->dev->kobj, dw_mci_dspg_groups[0]))
		dev_info(host->dev,
			"Could not create sysfs group.  Drive clock tuning will not be accessible.");

	return 0;
}

static int
dw_mci_dspg_setup_clocks(struct dw_mci *host)
{
	struct dw_mci_dspg_priv_data *priv = host->priv;

	clk_prepare(priv->clk_d);
	clk_prepare(priv->clk_s);
	if (!IS_ERR_OR_NULL(priv->main_in))
		clk_prepare_enable(priv->main_in);

	if (priv->shift_d[0] >= 0 && priv->shift_d[0] < 32)
		clk_set_phase(priv->clk_d, priv->shift_d[0]);
	if (priv->shift_s[0] >= 0 && priv->shift_s[0] < 32)
		clk_set_phase(priv->clk_s, priv->shift_s[0]);

	clk_enable(priv->clk_d);
	clk_enable(priv->clk_s);

	return 0;
}

static void
dw_mci_dspg_set_phases(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_dspg_priv_data *priv = host->priv;
	int phase_index;

	phase_index = dw_mci_dspg_phase_index(ios);

	if (phase_index >= 0 && priv->shift_d[phase_index] >= 0 &&
	    priv->shift_d[phase_index] < 32) {
		clk_disable(priv->clk_d);
		udelay(100);
		clk_set_phase(priv->clk_d,  priv->shift_d[phase_index]);
		udelay(100);
		clk_enable(priv->clk_d);
		/* bypass HOLD register when drive clock is not shifted */
		if (host->slot) {
			if (host->verid == 0x290a &&
			    ios->timing == MMC_TIMING_MMC_DDR52 &&
			    ios->bus_width == MMC_BUS_WIDTH_8) {
				/* handle erratum for v2.90a */
				clear_bit(DW_MMC_CARD_NO_USE_HOLD,
					  &host->slot->flags);
			} else {
				if (priv->shift_d[phase_index])
					clear_bit(DW_MMC_CARD_NO_USE_HOLD,
						  &host->slot->flags);
				else
					set_bit(DW_MMC_CARD_NO_USE_HOLD,
						&host->slot->flags);
			}
		}
	}
	if (phase_index >= 0 && priv->shift_s[phase_index] >= 0 &&
	    priv->shift_s[phase_index] < 32) {
		clk_disable(priv->clk_s);
		udelay(100);
		clk_set_phase(priv->clk_s,  priv->shift_s[phase_index]);
		udelay(100);
		clk_enable(priv->clk_s);
	}
}

static void
dw_mci_dspg_set_ios(struct dw_mci *host, struct mmc_ios *ios)
{
	struct clk *main_in =
		((struct dw_mci_dspg_priv_data *)host->priv)->main_in;

	if (ios->clock == 0)
		return;

	if (!IS_ERR_OR_NULL(main_in)) {
		if (ios->clock <= 400000)
			clk_set_rate(main_in,    4000000);
		else if (ios->clock <=  25000000)
			clk_set_rate(main_in,  250000000);
		else if (ios->clock <=  52000000)
			clk_set_rate(main_in,  500000000);
		else if (ios->clock <= 100000000)
			clk_set_rate(main_in, 1000000000);
		else if (ios->clock <= 152000000)
			clk_set_rate(main_in, 1400000000);
		else
			dev_err(host->dev,
				"%u exceeds maximum clock rate 152 MHz",
				ios->clock);
	}

	/* the block internal divider should always be set to 1 (bypass) */
	clk_set_rate(host->ciu_clk, ios->clock);

	if (host->bus_hz != clk_get_rate(host->ciu_clk))
		/* force bus setup for internal divider (SDMMC_CLKDIV) */
		host->current_speed = 0;
	host->bus_hz = clk_get_rate(host->ciu_clk);

	dw_mci_dspg_set_phases(host, ios);
}

static unsigned long dw_mci_dspg_caps_mmc_evb99[1] = {
	MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
};

static unsigned long dw_mci_dspg_caps_mmc_evb101[1] = {
	/* EVB101 does not support automatic I/O voltage switching yet */
	/*MMC_CAP_1_8V_DDR | */MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED /*|
	MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 |
	MMC_CAP_UHS_SDR104 | MMC_CAP_UHS_DDR50*/,
};

static unsigned long dw_mci_dspg_caps_mmc_iot_revb[1] = {
	MMC_CAP_1_8V_DDR | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED |
	MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 |
	MMC_CAP_UHS_SDR104,
};

static unsigned long dw_mci_dspg_caps_emmc_evb101[1] = {
	/* UHS is not supported as we cannot switch to 1.8V; DDR52 mode at 3.3V
	 * and 4Bit mode are implicitly assumed to be supported
	 */
	MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
};

static const struct dw_mci_drv_data dspg_drv_data_mmc_evb99 = {
	.parse_dt		= dw_mci_dspg_parse_dt99,
	.caps			= dw_mci_dspg_caps_mmc_evb99,
	.num_caps		= ARRAY_SIZE(dw_mci_dspg_caps_mmc_evb99),
};

static const struct dw_mci_drv_data dspg_drv_data_mmc_evb101 = {
	.execute_tuning		= dw_mci_dspg_execute_tuning,
	.set_ios		= dw_mci_dspg_set_ios,
	.parse_dt		= dw_mci_dspg_parse_dt,
	.setup_clock		= dw_mci_dspg_setup_clocks,
	.caps			= dw_mci_dspg_caps_mmc_evb101,
	.num_caps		= ARRAY_SIZE(dw_mci_dspg_caps_mmc_evb101),
};

static const struct dw_mci_drv_data dspg_drv_data_mmc_iot_revb = {
	.execute_tuning		= dw_mci_dspg_execute_tuning,
	.set_ios		= dw_mci_dspg_set_ios,
	.parse_dt		= dw_mci_dspg_parse_dt,
	.setup_clock		= dw_mci_dspg_setup_clocks,
	.caps			= dw_mci_dspg_caps_mmc_iot_revb,
	.num_caps		= ARRAY_SIZE(dw_mci_dspg_caps_mmc_iot_revb),
};

static const struct dw_mci_drv_data dspg_drv_data_emmc_evb101 = {
	.execute_tuning		= dw_mci_dspg_execute_tuning,
	.set_ios		= dw_mci_dspg_set_ios,
	.parse_dt		= dw_mci_dspg_parse_dt,
	.setup_clock		= dw_mci_dspg_setup_clocks,
	.caps			= dw_mci_dspg_caps_emmc_evb101,
	.num_caps		= ARRAY_SIZE(dw_mci_dspg_caps_emmc_evb101),
};

static const struct of_device_id dw_mci_dspg_match[] = {
	{
		.compatible = "dspg,dw-mmc-evb99",
		.data = &dspg_drv_data_mmc_evb99
	},
	{
		.compatible = "dspg,dw-mmc-evb101",
		.data = &dspg_drv_data_mmc_evb101
	},
	{
		.compatible = "dspg,dw-mmc-iot-revb",
		.data = &dspg_drv_data_mmc_iot_revb
	},
	{
		.compatible = "dspg,dw-emmc-evb101",
		.data = &dspg_drv_data_emmc_evb101
	},
	{ /* sentinel */ },
};

static int
dw_mci_dspg_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data;
	const struct of_device_id *match;

	if (!pdev->dev.of_node)
		return -ENODEV;

	match = of_match_node(dw_mci_dspg_match, pdev->dev.of_node);
	drv_data = match->data;

	return dw_mci_pltfm_register(pdev, drv_data);
}

static int
dw_mci_dspg_remove(struct platform_device *pdev)
{
	struct dw_mci *host = dev_get_drvdata(&pdev->dev);
	struct dw_mci_dspg_priv_data *priv = host->priv;

	if (host->drv_data->execute_tuning)
		sysfs_remove_group(&host->dev->kobj, dw_mci_dspg_groups[0]);

	if (!IS_ERR_OR_NULL(priv->main_in))
		clk_disable_unprepare(priv->main_in);
	clk_disable_unprepare(priv->clk_s);
	clk_disable_unprepare(priv->clk_d);

	return dw_mci_pltfm_remove(pdev);
}


static const struct dev_pm_ops dw_mci_dspg_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(dw_mci_runtime_suspend,
			   dw_mci_runtime_resume,
			   NULL)
};

static struct platform_driver dw_mci_dspg_pltfm_driver = {
	.probe	= dw_mci_dspg_probe,
	.remove	= dw_mci_dspg_remove,
	.driver	= {
		.name		= "dwmmc_dspg",
		.of_match_table	= dw_mci_dspg_match,
		.pm		= &dw_mci_dspg_pmops,
	},
};

module_platform_driver(dw_mci_dspg_pltfm_driver);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("DSPG Specific DW-MSHC Driver Extension");
MODULE_LICENSE("GPL");
