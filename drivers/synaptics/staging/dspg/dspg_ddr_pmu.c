// SPDX-License-Identifier: GPL-2.0
/*
 * dspg_ddr_pmu.c - PMU Framework for DDR on DVF101
 *
 * Copyright (C) 2017 DSP Group
 *
 */

#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/perf_event.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#define CNTR_EN_REG		0x00
#define CNTR_INPUT_SEL1		0x04
#define CNTR_INPUT_SEL2		0x08
#define CNTR_INPUT_SEL3		0x0C
#define CNTR_VALUES_BASE	0x10

static void __iomem *virtbase;

struct ddr_perf {
	struct device *dev;
	void __iomem *base;
	unsigned int irq;
	int id;

	cpumask_t cpu;

	struct perf_event *event;
	struct pmu pmu;
	raw_spinlock_t pmu_lock;
};

static struct ddr_perf *perf;

//#define to_ddr_perf(p) (container_of(p, struct ddr_perf, pmu))
//#define pmu_to_ddr_perf(_pmu) container_of(_pmu, struct ddr_perf, pmu)

static inline void writel_perf(u32 val, u32 offset)
{
	writel(val, virtbase + offset);
}

static inline u32 readl_perf(u32 offset)
{
	return readl(virtbase + offset);
}

struct ddr_perf_pmu_event {
	struct device_attribute attr;
	unsigned int config;
};

#define DDR_PERF_EVENT_ATTR(_name, _config) \
		{.attr = __ATTR(_name, 0444, ddr_perf_pmu_event_show, NULL),\
		 .config = _config, }

static ssize_t ddr_perf_pmu_event_show(
					struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct ddr_perf_pmu_event *event = container_of(attr,
					struct ddr_perf_pmu_event, attr);

	return snprintf(buf, PAGE_SIZE, "config=%d\n", event->config);
}

static struct ddr_perf_pmu_event ddr_perf_pmu_events[] = {
	DDR_PERF_EVENT_ATTR(hif_rd_or_wr,		0),
	DDR_PERF_EVENT_ATTR(hif_wr,			1),
	DDR_PERF_EVENT_ATTR(hif_rd,			2),
	DDR_PERF_EVENT_ATTR(hif_rmw,			3),
	DDR_PERF_EVENT_ATTR(hif_hi_pri_rd,		4),
	DDR_PERF_EVENT_ATTR(dfi_wr_data_cycles,		5),
	DDR_PERF_EVENT_ATTR(dfi_rd_data_cycles,		6),
	DDR_PERF_EVENT_ATTR(hpr_xact_when_crit,		7),
	DDR_PERF_EVENT_ATTR(lpr_xact_when_crit,		8),
	DDR_PERF_EVENT_ATTR(wr_xact_when_crit,		9),
	DDR_PERF_EVENT_ATTR(op_is_activate,		10),
	DDR_PERF_EVENT_ATTR(op_is_rd_or_wr,		11),
	DDR_PERF_EVENT_ATTR(op_is_rd_activate,		12),
	DDR_PERF_EVENT_ATTR(op_is_rd,			13),
	DDR_PERF_EVENT_ATTR(op_is_wr,			14),
	DDR_PERF_EVENT_ATTR(op_is_precharge,		15),
	DDR_PERF_EVENT_ATTR(precharge_for_rdwr,		16),
	DDR_PERF_EVENT_ATTR(precharge_for_other,	17),
	DDR_PERF_EVENT_ATTR(rdwr_transitions,		18),
	DDR_PERF_EVENT_ATTR(write_combine,		19),
	DDR_PERF_EVENT_ATTR(war_hazard,			20),
	DDR_PERF_EVENT_ATTR(raw_hazard,			21),
	DDR_PERF_EVENT_ATTR(waw_hazard,			22),
	DDR_PERF_EVENT_ATTR(op_is_enter_selfref,	23),
	DDR_PERF_EVENT_ATTR(op_is_enter_pwdown,		24),
	DDR_PERF_EVENT_ATTR(selfref_mode,		25),
	DDR_PERF_EVENT_ATTR(op_is_refresh,		26),
	DDR_PERF_EVENT_ATTR(op_is_load_mode,		27),
	DDR_PERF_EVENT_ATTR(op_is_zqcl,			28),
	DDR_PERF_EVENT_ATTR(op_is_zqcs,			29),
	DDR_PERF_EVENT_ATTR(op_is_rd_or_wr_bank,	30),
	DDR_PERF_EVENT_ATTR(nop,			31),
};

static struct attribute
		*ddr_perf_pmu_events_attrs[ARRAY_SIZE(ddr_perf_pmu_events) + 1];

static struct attribute_group ddr_perf_pmu_events_attr_group = {
	.name = "events",
	.attrs = ddr_perf_pmu_events_attrs,
};

static const struct attribute_group *ddr_perf_attr_groups[] = {
	&ddr_perf_pmu_events_attr_group,
	NULL
};

static int ddr_pmu_event_init(struct perf_event *event)
{
	u32 val;
	struct hw_perf_event *hw = &event->hw;

	if (event->attr.type != perf->pmu.type)
		return -ENOENT;

	//perf = pmu_to_ddr_perf(event->pmu);
	hw->sample_period  = ((1ULL << 32) - 1) >> 1;
	hw->last_period    = hw->sample_period;
	local64_set(&hw->period_left, hw->sample_period);

	if (hw->config_base < 6) {
		val = readl_perf(CNTR_INPUT_SEL1);
		val &= ~(0x1F << (hw->config_base * 5));
		val |= (0x1F & hw->config_base) << (hw->config_base * 5);
		writel_perf(val, CNTR_INPUT_SEL1);
	} else if (hw->config_base < 12) {
		val = readl_perf(CNTR_INPUT_SEL2);
		val &= ~(0x1F << ((hw->config_base - 6) * 5));
		val |= (hw->config_base & 0x1F) << ((hw->config_base - 6) * 5);
		writel_perf(val, CNTR_INPUT_SEL2);
	} else if (hw->config_base < 16) {
		val = readl_perf(CNTR_INPUT_SEL3);
		val &= ~(0x1F << ((hw->config_base - 12) * 5));
		val |= (hw->config_base & 0x1F) << ((hw->config_base - 12) * 5);
		writel_perf(val, CNTR_INPUT_SEL3);
	} else {
		pr_info("Event not implemented yet.!!\n");
	}

	return 0;
}

static u64 ddr_pmu_event_update(struct perf_event *event)
{
	struct hw_perf_event *hw = &event->hw;
	u64 prev_count, new_count, delta;

	hw->config_base = event->attr.config;

	do {
		prev_count = local64_read(&hw->prev_count);
		new_count = readl_perf(CNTR_VALUES_BASE + hw->config_base * 4);
	} while (local64_cmpxchg(&hw->prev_count, prev_count,
				new_count) != prev_count);

	delta = (new_count - prev_count) & ((1LLU << 32) - 1);

	//printk("prev_value = %llu, mask = %llu", prev_count, mask);

	local64_add(delta, &event->count);

	//printk(KERN_INFO "ddr_pmu_event_update idx %d, config %lu\n", hw->idx,
	//	hw->config_base);

	return new_count;
}

static void ddr_pmu_event_set_period(struct perf_event *event)
{
	u64 val = 1ULL << 31;
	struct hw_perf_event *hw = &event->hw;

	/*
	 * XXX This part is implemented with reference to arm-cci.XXX
	 */

	local64_set(&hw->prev_count, val);

	hw->state |= PERF_HES_ARCH;
}

static void ddr_pmu_event_start(struct perf_event *event, int pmu_flags)
{
	u64 prev_value;
	struct hw_perf_event *hw = &event->hw;

	hw->config_base = event->attr.config;

	if (pmu_flags & PERF_EF_RELOAD)
		WARN_ON_ONCE(!(hw->state & PERF_HES_UPTODATE));

	hw->state = 0;

	ddr_pmu_event_set_period(event);
	prev_value = readl_perf(CNTR_VALUES_BASE + hw->config_base * 4);
	local64_set(&event->hw.prev_count, prev_value);
	//ddr_pmu_enable(pmu);

	//printk(KERN_INFO "ddr_pmu_event_start idx %d, config %lu\n", hw->idx,
	//		hw->config_base);
}

static void ddr_pmu_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hw = &event->hw;
	//int idx = hw->idx;

	hw->config_base = event->attr.config;

	if (hw->state & PERF_HES_STOPPED)
		return;

	ddr_pmu_event_update(event);
	hw->state |= PERF_HES_STOPPED | PERF_HES_UPTODATE;

	//printk(KERN_INFO "ddr_pmu_event_stop idx %d, config %lu\n", hw->idx,
	//		hw->config_base);
}

static int ddr_idx;

static int ddr_pmu_event_add(struct perf_event *event, int flags)
{
	struct hw_perf_event *hw = &event->hw;

	hw->config_base = event->attr.config;
	hw->idx = ddr_idx++;

	perf_pmu_disable(event->pmu);

	hw->state = PERF_HES_STOPPED | PERF_HES_UPTODATE;
	if (flags & PERF_EF_START)
		ddr_pmu_event_start(event, PERF_EF_RELOAD);

	/* Propagate our changes to the userspace mapping. */
	perf_event_update_userpage(event);
	perf_pmu_enable(event->pmu);

	//printk(KERN_INFO "ddr_pmu_event_add idx %d, config %lu\n", hw->idx,
	//	hw->config_base);

	return 0;
}

static void ddr_pmu_event_del(struct perf_event *event, int flags)
{
	//struct hw_perf_event *hw = &event->hw;

	ddr_pmu_event_stop(event, PERF_EF_UPDATE);
	perf_event_update_userpage(event);

	//printk(KERN_INFO "ddr_pmu_event_del idx %d, config %lu\n", hw->idx,
	//		hw->config_base);
}

static void ddr_pmu_event_read(struct perf_event *event)
{
	//struct hw_perf_event *hw = &event->hw;

	//printk("count = %u\n", readl_perf(CNTR_VALUES_BASE +
	//	hw->config_base * 4));
	ddr_pmu_event_update(event);
	//printk(KERN_INFO "ddr_pmu_event_read idx %d, config %lu\n", hw->idx,
	//	hw->config_base);
}

static void ddr_pmu_enable(struct pmu *pmu)
{
	u32 val;

	val = readl_perf(CNTR_EN_REG);
	val |= 0x0F;
	writel_perf(val, CNTR_EN_REG);

	//printk(KERN_INFO "ddr_pmu_enable\n");
}

static void ddr_pmu_disable(struct pmu *pmu)
{
	u32 val;

	val = readl_perf(CNTR_EN_REG);
	val &= 0;
	writel_perf(val, CNTR_EN_REG);

	//printk(KERN_INFO "ddr_pmu_disable\n");
}

static DEFINE_IDA(ddr_pmu_ida);

static int ddr_pmu_init(struct ddr_perf *perf)
{
	char *name;
	int err;

	dev_info(perf->dev, "Initializing PMU for DDR... ");

	/* Get a convenient /sys/bus/event_source/devices/ name */
	perf->id = ida_simple_get(&ddr_pmu_ida, 0, 0, GFP_KERNEL);
	if (perf->id == 0) {
	name = "ddr";
	} else {
		int len = snprintf(NULL, 0, "ddr_%d", perf->id);

		name = devm_kzalloc(perf->dev, len + 1, GFP_KERNEL);
		snprintf(name, len + 1, "ddr_%d", perf->id);
	}

	/* Perf Driver Registration*/
	perf->pmu = (struct pmu) {
		.attr_groups = ddr_perf_attr_groups,
		.task_ctx_nr = perf_invalid_context,
		.event_init = ddr_pmu_event_init,
		.add = ddr_pmu_event_add,
		.del = ddr_pmu_event_del,
		.start = ddr_pmu_event_start,
		.stop = ddr_pmu_event_stop,
		.read = ddr_pmu_event_read,
		.pmu_enable = ddr_pmu_enable,
		.pmu_disable = ddr_pmu_disable,
	};

	err = perf_pmu_register(&perf->pmu, name, 0);
	if (err)
		goto error_pmu_register;

	return 0;

error_pmu_register:
	ida_simple_remove(&ddr_pmu_ida, perf->id);
	return err;
}

static int ddr_pmu_probe(struct platform_device *pdev)
{
	struct resource *res;

	dev_info(&pdev->dev, "Registering PMU Framework for DDR on DVF101.\n");

	perf = devm_kzalloc(&pdev->dev, sizeof(*perf), GFP_KERNEL);
	if (!perf)
		return -ENOMEM;

	perf->dev = &pdev->dev;
	platform_set_drvdata(pdev, perf);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	if (!devm_request_mem_region(
					perf->dev, res->start,
					resource_size(res), pdev->name))
		return -EBUSY;

	perf->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	virtbase = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(virtbase))
		return PTR_ERR(virtbase);

	return ddr_pmu_init(perf);
}

static int ddr_pmu_remove(struct platform_device *pdev)
{
	struct ddr_perf *perf = platform_get_drvdata(pdev);

	perf_pmu_unregister(&perf->pmu);
	ida_simple_remove(&ddr_pmu_ida, perf->id);

	return 0;
}

static const struct of_device_id ddr_pmu_of_match[] = {
	{ .compatible = "dspg,ddr-pmu" },
	{ /* guardian */ },
};

static struct platform_driver ddr_pmu_driver = {
	.driver = {
		.name		= "dspg-ddr-pmu",
		.owner		= THIS_MODULE,
		.pm		= NULL,
		.of_match_table	= ddr_pmu_of_match,
	},
	.probe	= ddr_pmu_probe,
	.remove	= ddr_pmu_remove,
};

static int __init ddr_pmu__init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ddr_perf_pmu_events); i++)
		ddr_perf_pmu_events_attrs[i] =
			&ddr_perf_pmu_events[i].attr.attr;

	return platform_driver_register(&ddr_pmu_driver);
}

static void __exit ddr_pmu__exit(void)
{
	platform_driver_unregister(&ddr_pmu_driver);
}

module_init(ddr_pmu__init);
module_exit(ddr_pmu__exit);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
