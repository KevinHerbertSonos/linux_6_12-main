/*
 * linux/drivers/staging/dspg/sec.c
 *
 *  Copyright (C) 2012, 2016 DSP Group
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
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/sec.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/reset.h>

#define CPU2COM_MEM	0x000 /* R/W */
#define COM2CPU_MEM	0x100 /* RO  */

#define CPU2COM_STAT1	0x600 /* RO  */
#define CPU2COM_STAT2	0x604 /* RO  */
#define CPU2COM_OVR1	0x610 /* RO  */
#define CPU2COM_OVR2	0x614 /* RO  */
#define COM2CPU_STAT1	0x640 /* RO/CBW */
#define COM2CPU_STAT2	0x644 /* RO/CBW */
#define COM2CPU_MASK1	0x648 /* R/W */
#define COM2CPU_MASK2	0x64C /* R/W */
#define COM2CPU_OVR1	0x650 /* R/W */
#define COM2CPU_OVR2	0x654 /* R/W */
#define COM2CPU_CAUS1	0x658 /* RO  */
#define COM2CPU_CAUS2	0x65C /* RO  */
#define COM_CAUSE	0x660 /* RO  */

#define SEC_SEM_COUNT	64

#define SEC_BUSY	0x1

struct sec_msg {
	int id;
	int flags;
	sec_callback cb;
	void *context;
};

struct sec_sem {
	struct sec_msg *enter;
	struct sec_msg *number;
};

struct sec_dev {
	void __iomem *regs;
	struct clk   *clk;
	struct reset_control *reset;

	spinlock_t lock;

	struct sec_msg messages[SEC_SEM_COUNT];
};

static struct sec_dev *sec = NULL;


static unsigned long sec_read(int id)
{
	return readl(sec->regs + COM2CPU_MEM + id*4);
}

static void sec_write(int id, unsigned long data)
{
	writel(data, sec->regs + CPU2COM_MEM + id*4);
}

static void sec_int_clear(struct sec_msg *msg)
{
	writel(1 << (msg->id%32), sec->regs + COM2CPU_STAT1 + (msg->id>>5)*4);
}

static void sec_int_enable(struct sec_msg *msg)
{
	unsigned long tmp;

	tmp = readl(sec->regs + COM2CPU_MASK1 + (msg->id>>5)*4);
	tmp |=  1 << (msg->id%32);
	writel(tmp, sec->regs + COM2CPU_MASK1 + (msg->id>>5)*4);
}

static void sec_int_disable(struct sec_msg *msg)
{
	unsigned long tmp;

	tmp = readl(sec->regs + COM2CPU_MASK1 + (msg->id>>5)*4);
	tmp &= ~(1 << (msg->id%32));
	writel(tmp, sec->regs + COM2CPU_MASK1 + (msg->id>>5)*4);
}

static void
sec_overwrite(struct sec_msg *msg, int enable)
{
	unsigned long tmp;

	tmp = readl(sec->regs + COM2CPU_OVR1 + (msg->id>>5)*4);
	if (enable)
		tmp |=   1 << (msg->id%32);
	else
		tmp &= ~(1 << (msg->id%32));
	writel(tmp, sec->regs + COM2CPU_OVR1 + (msg->id>>5)*4);
}

static irqreturn_t sec_interrupt_handler(int irq, void *data)
{
	int i;
	unsigned long stat[2];

	spin_lock(&sec->lock);

	stat[0] = readl_relaxed(sec->regs + COM2CPU_CAUS1);
	stat[1] = readl(sec->regs + COM2CPU_CAUS2);

	i = find_first_bit(stat, 64);
	while (i < 64) {
		struct sec_msg *msg = &sec->messages[i];

		if (msg->flags & SEC_DONT_CLEAR) {
			sec_int_disable(msg);
			msg->cb(i, sec_read(i), msg->context);
		} else {
			msg->cb(i, sec_read(i), msg->context);
			sec_int_clear(msg);
		}

		i = find_next_bit(stat, 64, i+1);
	}

	spin_unlock(&sec->lock);

	return IRQ_HANDLED;
}

int sec_msg_trigger(struct sec_msg *msg, unsigned long data)
{
	sec_write(msg->id, data);

	return 0;
}
EXPORT_SYMBOL(sec_msg_trigger);

void sec_msg_clear(struct sec_msg *msg)
{
	unsigned long flags;

	sec_int_clear(msg);

	spin_lock_irqsave(&sec->lock, flags);
	sec_int_enable(msg);
	spin_unlock_irqrestore(&sec->lock, flags);
}
EXPORT_SYMBOL(sec_msg_clear);

unsigned long sec_msg_read(struct sec_msg *msg)
{
	return sec_read(msg->id);
}
EXPORT_SYMBOL(sec_msg_read);

struct sec_msg * sec_msg_register(sec_callback cb, int id, int sec_flags,
				  void *context)
{
	struct sec_msg *msg = ERR_PTR(-EBUSY);
	unsigned long flags;

	if (!sec)
		return ERR_PTR(-ENODEV);

	if ((id < 0) || (id >= SEC_SEM_COUNT))
		return ERR_PTR(-EINVAL);

	spin_lock_irqsave(&sec->lock, flags);

	clk_enable(sec->clk);

	if (!(sec->messages[id].flags & SEC_BUSY)) {
		msg = &sec->messages[id];
		msg->cb = cb;
		msg->context = context;
		msg->flags = sec_flags;
		msg->flags |= SEC_BUSY;
		sec_overwrite(msg, (sec_flags & SEC_OVERWRITE));
		if (cb)
			sec_int_enable(msg);
	}

	spin_unlock_irqrestore(&sec->lock, flags);

	return msg;
}
EXPORT_SYMBOL(sec_msg_register);

void sec_msg_deregister(struct sec_msg *msg)
{
	unsigned long flags;

	if (!msg)
		return;

	spin_lock_irqsave(&sec->lock, flags);

	if (msg->cb)
		sec_int_disable(msg);
	msg->flags = 0;
	msg->cb = NULL;

	clk_disable(sec->clk);

	spin_unlock_irqrestore(&sec->lock, flags);
}
EXPORT_SYMBOL(sec_msg_deregister);

static int __init sec_probe(struct platform_device *pdev)
{
	struct resource *res;
	int i, ret, irq;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	sec = devm_kzalloc(&pdev->dev, sizeof(struct sec_dev), GFP_KERNEL);
	if (!sec)
		return -ENOMEM;

	for (i = 0; i < SEC_SEM_COUNT; i++)
		sec->messages[i].id = i;

	spin_lock_init(&sec->lock);

	sec->regs = devm_ioremap_resource(&pdev->dev, res);
	if (!sec->regs) {
		dev_err(&pdev->dev, "failed to map registers\n");
		return -ENOMEM;
	}

	sec->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(sec->clk)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		return PTR_ERR(sec->clk);
	}
	clk_prepare(sec->clk);

	sec->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(sec->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		return PTR_ERR(sec->reset);
	}

	ret = reset_control_deassert(sec->reset);
	if (ret)
		return ret;

	ret = devm_request_irq(&pdev->dev, irq, sec_interrupt_handler, 0, "sec",
			       sec);
	if (ret) {
		dev_err(&pdev->dev, "cannot register interrupt\n");
		return ret;
	}

	enable_irq_wake(irq);

	return 0;
}

static struct of_device_id sec_of_device_ids[] = {
	{ .compatible = "dspg,sec" },
	{ },
};

static struct platform_driver sec_driver = {
	.driver = {
		.name = "sec",
		.owner = THIS_MODULE,
		.of_match_table = sec_of_device_ids,
	},
};

static int __init sec_init(void)
{
	return platform_driver_probe(&sec_driver, sec_probe);
}
module_init(sec_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Semaphore Exchange Center driver");
MODULE_AUTHOR("DSP Group, Inc.");
