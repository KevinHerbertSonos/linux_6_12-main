#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#define CNTR_EN			0x00
#define CNTR_INPUT_SEL1		0x04
#define CNTR_INPUT_SEL2		0x08
#define CNTR_INPUT_SEL3		0x0C
#define CNTR_VALUES_BASE	0x10

static unsigned chosen_counter;
static void __iomem *virtbase;

static inline void
writel_perf(u32 val, u32 offset)
{
	writel(val, virtbase + offset);
}

static inline u32
readl_perf(u32 offset)
{
	return readl(virtbase + offset);
}

static ssize_t
ddr_perf_enable_store(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t size)
{
	u32 reg;

	if (chosen_counter > 15) {
		dev_err(dev, "invalid counter chosen!\n");
		return -EINVAL;
	}

	reg = readl_perf(CNTR_EN);
	reg |= 1 << chosen_counter;
	writel_perf(reg, CNTR_EN);

	return size;
}

static ssize_t
ddr_perf_enable_show(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	return sprintf(buf, "0x%4.4X\n", readl_perf(CNTR_EN) & 0xFFFF);
}

static ssize_t
ddr_perf_counter_show(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	return sprintf(buf, "%u\n", readl_perf(CNTR_VALUES_BASE +
		       chosen_counter * 4));
}

static ssize_t
ddr_perf_input_store(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t size)
{
	u32 input, reg;

	if (kstrtoul(buf, 0, (unsigned long *)&input)) {
		dev_err(dev, "enter a number!\n");
		return -EINVAL;
	}

	if (chosen_counter < 6) {
		reg = readl_perf(CNTR_INPUT_SEL1);
		reg &= ~(0x1F << (chosen_counter * 5));
		reg |= (input & 0x1F) << (chosen_counter * 5);
		writel_perf(reg, CNTR_INPUT_SEL1);
	} else if (chosen_counter < 12) {
		reg = readl_perf(CNTR_INPUT_SEL2);
		reg &= ~(0x1F << ((chosen_counter - 6) * 5));
		reg |= (input & 0x1F) << ((chosen_counter - 6) * 5);
		writel_perf(reg, CNTR_INPUT_SEL2);
	} else {
		reg = readl_perf(CNTR_INPUT_SEL3);
		reg &= ~(0x1F << ((chosen_counter - 12) * 5));
		reg |= (input & 0x1F) << ((chosen_counter - 12) * 5);
		writel_perf(reg, CNTR_INPUT_SEL3);
	}

	return size;
}

static ssize_t
ddr_perf_input_show(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	if (chosen_counter < 6)
		return sprintf(buf, "%u\n",
			       (readl_perf(CNTR_INPUT_SEL1) >>
			       (chosen_counter * 5)) & 0x1F);
	else if (chosen_counter < 12)
		return sprintf(buf, "%u\n",
			       (readl_perf(CNTR_INPUT_SEL2) >>
			       ((chosen_counter - 6) * 5)) & 0x1F);
	else
		return sprintf(buf, "%u\n",
			       (readl_perf(CNTR_INPUT_SEL3) >>
			       ((chosen_counter - 12) * 5)) & 0x1F);
}

static ssize_t
ddr_perf_select_store(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t size)
{
	u32 input;

	if (kstrtoul(buf, 0, (unsigned long *)&input)) {
		dev_err(dev, "enter a positive number!\n");
		return -EINVAL;
	}

	chosen_counter = (unsigned)input;

	return size;
}

static ssize_t
ddr_perf_select_show(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	return sprintf(buf, "%u\n", chosen_counter);
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR,
		   ddr_perf_enable_show, ddr_perf_enable_store);
static DEVICE_ATTR(count, S_IRUGO, ddr_perf_counter_show, NULL);
static DEVICE_ATTR(select, S_IRUGO | S_IWUSR,
		   ddr_perf_select_show, ddr_perf_select_store);
static DEVICE_ATTR(target, S_IRUGO | S_IWUSR,
		   ddr_perf_input_show, ddr_perf_input_store);

static struct attribute *ddr_perf_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_count.attr,
	&dev_attr_select.attr,
	&dev_attr_target.attr,
	NULL,
};

static struct attribute_group ddr_perf_group = {
	.attrs = ddr_perf_attributes,
	.name = "ddr_perf_cnt",
};

static int
ddr_perf_probe(struct platform_device *pdev)
{
	struct resource *res;

	dev_info(&pdev->dev, "probing DSPG DDR performance counter driver\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;

	virtbase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(virtbase))
		return PTR_ERR(virtbase);

	return sysfs_create_group(&pdev->dev.kobj, &ddr_perf_group);
}

static struct of_device_id ddr_perf_of_match[] = {
	{ .compatible = "dspg,ddr-perf" },
	{ /* guardian */ },
};

static struct platform_driver ddr_perf_driver = {
	.driver = {
		.name		= "dspg-perf-counters",
		.owner		= THIS_MODULE,
		.pm		= NULL,
		.of_match_table	= ddr_perf_of_match,
	},
	.probe	= ddr_perf_probe,
	.remove	= NULL,
};
module_platform_driver(ddr_perf_driver);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
