/*
 * drivers/rtc/rtc-dspg.c - driver for the RTC block of the DP52 chip
 *
 * Copyright (C) 2011 DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/clk.h>

static const char dspg_rtc_name[] = "dspg-rtc";

#define DSPG_RTC_CFG		0x00
#define DSPG_RTC_ALARMVAL	0x08
#define DSPG_RTC_STATUS		0x0C
#define DSPG_RTC_INTCLR		0x10
#define DSPG_RTC_TIME		0x14

#define RTC_CFG_ALARMEN		(3 << 0)
#define RTC_STATUS_WRITEVALID	(1 << 1)
#define RTC_STATUS_ALARM	(1 << 0)
#define RTC_INTCLR_ALARM	(1 << 0)

struct dspg_rtc {
	struct rtc_device *dev;
	void *base;
	struct clk *clk;
	unsigned long rate;
	int irq;

	/*
	 * Those two variables build the foundation of this driver. Since the
	 * rtc only features a counter which cannot be reset and which value is
	 * unknown at system boot, we need a fixed point in time and a
	 * corresponding rtc counter sample to be able to calculate the current
	 * time. This pair is set via the settime call.
	 *
	 * rtc_reference: the rtc counter sample (one tick is one second)
	 * rtc_time:      the time at the rtc counter sample in seconds (mktime)
	 */
	unsigned long rtc_reference;
	unsigned long rtc_time;
};
struct dspg_rtc *dspg_rtc;

static unsigned long
dspg_rtc_read_sample(struct dspg_rtc *rtc)
{
	return readl(rtc->base + DSPG_RTC_TIME) / rtc->rate;
}

static irqreturn_t
dspg_rtc_irq(int irq, void *priv)
{
	struct dspg_rtc *rtc = priv;

	/* clear interrupt */
	writel(RTC_INTCLR_ALARM, rtc->base + DSPG_RTC_INTCLR);

	/* update status to rtc layer */
	rtc_update_irq(rtc->dev, 1, RTC_AF);

	return IRQ_HANDLED;
}

static int
dspg_rtc_readtime(struct device *dev, struct rtc_time *tm)
{
	struct dspg_rtc *rtc = platform_get_drvdata(to_platform_device(dev));
	unsigned long sample;

	/*
	 * get the sample first, this will also retrieve the valid bit
	 * used below
	 */
	sample = dspg_rtc_read_sample(rtc);

	rtc_time64_to_tm(rtc->rtc_time + sample - rtc->rtc_reference, tm);

	return 0;
}

static int
dspg_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	struct dspg_rtc *rtc = platform_get_drvdata(to_platform_device(dev));

	rtc->rtc_reference = dspg_rtc_read_sample(rtc);
	rtc->rtc_time = mktime64(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
				 tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;
}

static int
dspg_rtc_readalarm(struct device *dev, struct rtc_wkalrm *wkalrm)
{
	struct dspg_rtc *rtc = platform_get_drvdata(to_platform_device(dev));
	unsigned long alarm;
	struct rtc_time *tm = &wkalrm->time;

	alarm  = readl(rtc->base + DSPG_RTC_ALARMVAL);

	wkalrm->enabled = !!(readl(rtc->base + DSPG_RTC_CFG) & RTC_CFG_ALARMEN);

	alarm = alarm - rtc->rtc_reference + rtc->rtc_time;

	rtc_time64_to_tm(alarm, tm);

	return 0;
}

static int
dspg_rtc_setalarm(struct device *dev, struct rtc_wkalrm *wkalrm)
{
	struct dspg_rtc *rtc = platform_get_drvdata(to_platform_device(dev));
	unsigned long alarm;
	struct rtc_time *tm = &wkalrm->time;
	unsigned long cfg;

	alarm = mktime64(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			 tm->tm_hour, tm->tm_min, tm->tm_sec);
	alarm = alarm - rtc->rtc_time + rtc->rtc_reference;

	while (!(readl(rtc->base + DSPG_RTC_STATUS) & RTC_STATUS_WRITEVALID))
		;

	writel(alarm * rtc->rate, rtc->base + DSPG_RTC_ALARMVAL);
	cfg = readl(rtc->base + DSPG_RTC_CFG);
	writel(cfg | RTC_CFG_ALARMEN, rtc->base + DSPG_RTC_CFG);

	return 0;
}

static int
dspg_rtc_proc(struct device *dev, struct seq_file *seq)
{
	return 0;
}

static const struct rtc_class_ops dspg_rtc_ops = {
	.read_time	= dspg_rtc_readtime,
	.set_time	= dspg_rtc_settime,
	.read_alarm	= dspg_rtc_readalarm,
	.set_alarm	= dspg_rtc_setalarm,
	.proc		= dspg_rtc_proc,
};

static int
dspg_rtc_probe(struct platform_device *pdev)
{
	struct dspg_rtc *rtc;
	struct resource *res;
	int ret;
	unsigned long cfg;

	rtc = devm_kzalloc(&pdev->dev, sizeof(*rtc), GFP_KERNEL);
	if (!rtc)
		return -ENOMEM;

	dspg_rtc = rtc;

	rtc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(rtc->clk))
		return PTR_ERR(rtc->clk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource defined");
		return -ENODEV;
	}

	rtc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rtc->base))
		return PTR_ERR(rtc->base);

	rtc->irq = platform_get_irq(pdev, 0);
	if (rtc->irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource defined");
		return -ENODEV;
	}

	platform_set_drvdata(pdev, rtc);

	device_init_wakeup(&pdev->dev, 1);

	clk_prepare_enable(rtc->clk);
	rtc->rate = clk_get_rate(rtc->clk);

	/* reset alarm */
	cfg = readl(rtc->base + DSPG_RTC_CFG);
	writel(cfg & ~RTC_CFG_ALARMEN, rtc->base + DSPG_RTC_CFG);

	ret = devm_request_irq(&pdev->dev, rtc->irq, dspg_rtc_irq, 0,
			       dspg_rtc_name, rtc);
	if (ret)
		goto err_clk;

	rtc->dev = devm_rtc_device_register(&pdev->dev, dspg_rtc_name,
					    &dspg_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc->dev)) {
		ret = PTR_ERR(rtc->dev);
		goto err_clk;
	}

	dev_info(&pdev->dev, "initialized\n");

	return 0;

err_clk:
	clk_disable_unprepare(rtc->clk);

	return ret;
}

static int
dspg_rtc_remove(struct platform_device *pdev)
{
	struct dspg_rtc *rtc = platform_get_drvdata(pdev);

	clk_disable_unprepare(rtc->clk);

	return 0;
}

#ifdef CONFIG_PM
static int
dspg_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dspg_rtc *rtc = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev))
		enable_irq_wake(rtc->irq);

	return 0;
}

static int
dspg_rtc_resume(struct platform_device *pdev)
{
	struct dspg_rtc *rtc = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev))
		disable_irq_wake(rtc->irq);

	return 0;
}
#else
#define dspg_rtc_suspend NULL
#define dspg_rtc_resume NULL
#endif

static const struct of_device_id dspg_rtc_of_match[] = {
	{.compatible = "dspg,rtc", },
	{ },
};

static struct platform_driver dspg_rtc_driver = {
	.remove  = dspg_rtc_remove,
	.probe   = dspg_rtc_probe,
	.suspend = dspg_rtc_suspend,
	.resume  = dspg_rtc_resume,
	.driver  = {
		.name = dspg_rtc_name,
		.owner = THIS_MODULE,
		.of_match_table = dspg_rtc_of_match,
	},
};

static int __init dspg_rtc_init(void)
{
	return platform_driver_register(&dspg_rtc_driver);
}

static void __exit dspg_rtc_exit(void)
{
	platform_driver_unregister(&dspg_rtc_driver);
}

module_init(dspg_rtc_init);
module_exit(dspg_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("Driver for the RTC block of the DSPG-DVF99");
