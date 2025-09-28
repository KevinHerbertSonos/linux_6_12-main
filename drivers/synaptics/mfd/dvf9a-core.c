// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * DVF9A core driver including access to the DAIF interface
 *
 * Copyright (c) 2016, DSP Group
 * Copyright (c) 2023, Synaptics Incorporated
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/dvf9a.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/gpio.h>

#include <asm/unaligned.h>

#ifdef CONFIG_AVIO_MYNA2
#include "avio_common.h"
#endif

struct dvf9a;

struct anaint {
	u16 hwirq;
	struct dvf9a *dvf9a;
	struct irq_domain *irq_domain;
};

struct dvf9a {
	void __iomem *regs;
	void __iomem *clk_regs;
	struct clk *daif_clk;
	struct clk *apu_clk;
	struct clk *parent_clk;
	spinlock_t lock;
	int irq;
	int owner;
	void (*handle)(struct dvf9a *dvf9a);

	struct device *dev;
	struct irq_domain *anaint_domain;
	struct anaint anaints[DVF9A_NUM_IRQS];
	int user_irq[DVF9A_NUM_IRQS];

	struct miscdevice miscdev;

	int irq_enable_cur; /* currently active value */
	int daif_indirect_end;
	int num_irqs;
	int version;
	int filterless;
	struct gpio_desc *gpio;
};

struct dvf9a *the_dvf9a;

#define DVF9A				"dvf9a"

#define DAIF_WAPU_TRAN_MASK		(1<<15)
#define DAIF_RAPU_NEW_VALUE		(1<<15)

#define DAIF_DIRECT_MIRRORED_START	0
#define DAIF_DIRECT_MIRRORED_END	64
#define DAIF_DIRECT_RAPU_WAPU_START	65
#define DAIF_DIRECT_RAPU_WAPU_END	127

#define DAIF_RAPU			41  // 0x00a4
#define DAIF_WAPU			64  // 0x0100
#define DAIF_MODE			117 // 0x01d4
#define DAIF_PEND			118 // 0x01d8
#define DAIF_PENDB0			119 // 0x01dc
#define DAIF_PENDB1			120 // 0x01e0
#define DAIF_PENDB2			121 // 0x01e4
#define DAIF_PENDB3			122 // 0x01e8

#define DAIF_ANA_DATA_WR_LSB		65
#define DAIF_ANA_DATA_WR_MSB		66
#define DAIF_ANA_ADDR_WR		67
#define DAIF_ANA_ADDR_RD		68
#define DAIF_ANA_DATA_RD_LSB		69
#define DAIF_ANA_DATA_RD_MSB		70

#define DAIF_MODE_RAPU_PRIO		(1<<2)
#define DAIF_MODE_WAPU_PRIO		(1<<3)

static DECLARE_BITMAP(reserved_regs, DAIF_INDIRECT_END);

static const int irq_to_ana_stat_en[DVF9A_NUM_IRQS] = {
	0, 1, 2, 3, 4, -1, 6, -1, -1, -1, -1, 7, -1
};

void
dvf9a_apu_clk(struct dvf9a *dvf9a, int enable)
{
	if (enable)
		clk_enable(dvf9a->apu_clk);
	else
		clk_disable(dvf9a->apu_clk);
}
EXPORT_SYMBOL(dvf9a_apu_clk);

void
dvf9a_pmu_clk(struct dvf9a *dvf9a, int enable)
{
	if (enable)
		clk_enable(dvf9a->daif_clk);
	else
		clk_disable(dvf9a->daif_clk);
}
EXPORT_SYMBOL(dvf9a_pmu_clk);

void
dvf9a_handle_ownership(struct dvf9a *dvf9a, void (*handle)(struct dvf9a *))
{
	dvf9a->handle = handle;
}
EXPORT_SYMBOL(dvf9a_handle_ownership);

void
dvf9a_acquire(struct dvf9a *dvf9a)
{
	if (dvf9a->owner)
		return;

	if (dvf9a->handle)
		dvf9a->handle(dvf9a);

	dvf9a->owner = 1;
}
EXPORT_SYMBOL(dvf9a_acquire);

void
dvf9a_set_owner(struct dvf9a *dvf9a, int owner)
{
	dvf9a->owner = !!owner;
}
EXPORT_SYMBOL(dvf9a_set_owner);


static int
daif_is_direct_mirrored(unsigned short reg)
{
	if (reg <= DAIF_DIRECT_MIRRORED_END)
		return 1;
	return 0;
}

static int
daif_is_direct_rapu_wapu(unsigned short reg)
{
	if ((reg > DAIF_DIRECT_MIRRORED_END) &&
	    (reg <= DAIF_DIRECT_RAPU_WAPU_END))
		return 1;
	return 0;
}

static int
daif_is_indirect_analog(unsigned short reg)
{
	if (reg >= DAIF_INDIRECT_START)
		return 1;
	return 0;
}

static unsigned short
daif_readw(struct dvf9a *dvf9a, unsigned short reg)
{
	return readw(dvf9a->regs + (reg * 4));
}

static void
daif_writew(struct dvf9a *dvf9a, unsigned short val, unsigned short reg)
{
	writew(val, dvf9a->regs + (reg * 4));
}

static void
daif_wait_wapu_tran(struct dvf9a *dvf9a)
{
	while (daif_readw(dvf9a, DAIF_PEND))
		;
}

static int
daif_read_via_rapu(struct dvf9a *dvf9a, unsigned short reg)
{
	unsigned short val;
	unsigned long start_time;

	daif_writew(dvf9a, reg, DAIF_RAPU);

	start_time = jiffies;

	while (!((val = daif_readw(dvf9a, DAIF_RAPU)) & DAIF_RAPU_NEW_VALUE)) {
		udelay(1);
		if ((jiffies - start_time) > 100)
			return -ENODEV;
	}

	/* Check if indicated register address (RAPU[14:8]) matches */
	if (((val & 0x7f00) >> 8) == reg)
		return val & 0xff;
	else
		return daif_read_via_rapu(dvf9a, reg); /* repeat request */
}

static void
daif_write_via_wapu(struct dvf9a *dvf9a, unsigned short reg,
		    unsigned short value)
{
	daif_writew(dvf9a, ((reg & 0x7f) << 8) | (value & 0xff), DAIF_WAPU);
	daif_wait_wapu_tran(dvf9a);
}

static int
daif_direct_mirrored_init(struct dvf9a *dvf9a)
{
	int val, reg;

	/* enable "prev_replica" */
	daif_writew(dvf9a, 2, DAIF_MODE);

	for (reg = 0; reg < DAIF_DIRECT_MIRRORED_END; reg++) {
		if (reg == DAIF_RAPU)
			continue;
		val = daif_read_via_rapu(dvf9a, reg);
		if (val == -ENODEV)
			return val;
		udelay(10);
		daif_writew(dvf9a, val & 0xff, reg);
	}

	/* disable "prev_replica" */
	daif_writew(dvf9a, 0, DAIF_MODE);

	return 0;
}

unsigned short
daif_read(struct dvf9a *dvf9a, unsigned short reg)
{
	unsigned short ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&dvf9a->lock, flags);

	clk_enable(dvf9a->daif_clk);
	clk_enable(dvf9a->apu_clk);
	udelay(1);

	dvf9a_acquire(dvf9a);

	if (daif_is_direct_rapu_wapu(reg) ||
	    (reg == DAIF_RAADOMSB) || (reg == DAIF_LQSD_CFG1) ||
	    (reg == DAIF_PORCTL) || (reg == DAIF_BASECTRL) ||
	    (reg == DAIF_AFE_MIC_MUTE) || (reg == DAIF_ICU_ANA_STAT3) ||
	    (reg == DAIF_ICU_ANA_EN3)) {
		ret = daif_read_via_rapu(dvf9a, reg);
	} else if (daif_is_direct_mirrored(reg)) {
		ret = daif_readw(dvf9a, reg); /* immediate access */
	} else if (daif_is_indirect_analog(reg)) {
		/* set required address */
		reg -= DAIF_INDIRECT_START;
		daif_write_via_wapu(dvf9a, DAIF_ANA_ADDR_RD, reg >> 1);
		if (daif_readw(dvf9a, DAIF_MODE) & DAIF_MODE_RAPU_PRIO)
			daif_wait_wapu_tran(dvf9a);

		/* read value */
		ret  = daif_read_via_rapu(dvf9a, DAIF_ANA_DATA_RD_MSB) << 8;
		udelay(10);
		ret |= daif_read_via_rapu(dvf9a, DAIF_ANA_DATA_RD_LSB) & 0xff;
	}

	spin_unlock_irqrestore(&dvf9a->lock, flags);

	udelay(1);
	clk_disable(dvf9a->apu_clk);
	clk_disable(dvf9a->daif_clk);

	return ret;
}
EXPORT_SYMBOL(daif_read);

void
daif_write(struct dvf9a *dvf9a, unsigned short reg, unsigned short value)
{
	unsigned long flags;

	spin_lock_irqsave(&dvf9a->lock, flags);

	clk_enable(dvf9a->daif_clk);
	clk_enable(dvf9a->apu_clk);
	udelay(1);

	dvf9a_acquire(dvf9a);

	if (dvf9a->filterless && (reg == DAIF_DCLS_ANA_CTRL))
		value &= ~0x40; /* disable compatibility mode */

	if (daif_is_direct_mirrored(reg)) {
		unsigned short pend_reg = reg >> 4; /* register number 0..3 */
		unsigned short pend_mask = reg - (pend_reg << 4);   /* 0..15 */

		pend_mask = 1 << pend_mask; /* bit mask */

		/* immediate access */
		while (daif_readw(dvf9a, DAIF_PENDB0 + pend_reg) & pend_mask)
			;
		daif_writew(dvf9a, value, reg);
		while (daif_readw(dvf9a, DAIF_PENDB0 + pend_reg) & pend_mask)
			;
	} else if (daif_is_direct_rapu_wapu(reg)) {
		daif_write_via_wapu(dvf9a, reg, value);
	} else if (daif_is_indirect_analog(reg)) {
		/* write value msb, lsb */
		daif_write_via_wapu(dvf9a, DAIF_ANA_DATA_WR_MSB, value >> 8);
		daif_write_via_wapu(dvf9a, DAIF_ANA_DATA_WR_LSB, value & 0xff);

		/* set required address */
		reg -= DAIF_INDIRECT_START;
		daif_write_via_wapu(dvf9a, DAIF_ANA_ADDR_WR, reg >> 1);
	}

	udelay(1);
	clk_disable(dvf9a->apu_clk);
	clk_disable(dvf9a->daif_clk);

	spin_unlock_irqrestore(&dvf9a->lock, flags);
}
EXPORT_SYMBOL(daif_write);

int
daif_translate_irq(struct dvf9a *dvf9a, int hw_irq)
{
	return irq_create_mapping(dvf9a->anaint_domain, hw_irq);
}
EXPORT_SYMBOL(daif_translate_irq);

int
daif_has_dvfa10(struct dvf9a *dvf9a)
{
	if (dvf9a->version >= 0xa1)
		return 1;
	return 0;
}
EXPORT_SYMBOL(daif_has_dvfa10);

static void
dvf9a_irq_mask(struct irq_data *data)
{
	struct anaint *anaint = irq_data_get_irq_chip_data(data);
	struct dvf9a *dvf9a = anaint->dvf9a;
	unsigned short val;

	dvf9a->irq_enable_cur &= ~(1 << anaint->hwirq);

	daif_write(dvf9a, DAIF_ICU_ANA_EN1, dvf9a->irq_enable_cur & 0x7f);
	if (irq_to_ana_stat_en[anaint->hwirq] != -1) {
		val = daif_read(dvf9a, DAIF_ICU_ANA_STAT_EN1);
		val &= ~(1 << irq_to_ana_stat_en[anaint->hwirq]);
		daif_write(dvf9a, DAIF_ICU_ANA_STAT_EN1, val);
	}
	daif_write(dvf9a, DAIF_ICU_ANA_EN2, dvf9a->irq_enable_cur >> 7);

	udelay(1);
	clk_disable(dvf9a->apu_clk);
	clk_disable(dvf9a->daif_clk);
}

static void
dvf9a_irq_unmask(struct irq_data *data)
{
	struct anaint *anaint = irq_data_get_irq_chip_data(data);
	struct dvf9a *dvf9a = anaint->dvf9a;
	unsigned short val;

	clk_enable(dvf9a->daif_clk);
	clk_enable(dvf9a->apu_clk);
	udelay(1);

	dvf9a->irq_enable_cur |= 1 << anaint->hwirq;

	daif_write(dvf9a, DAIF_ICU_ANA_EN1, dvf9a->irq_enable_cur & 0x7f);
	if (irq_to_ana_stat_en[anaint->hwirq] != -1) {
		val = daif_read(dvf9a, DAIF_ICU_ANA_STAT_EN1);
		val |= 1 << irq_to_ana_stat_en[anaint->hwirq];
		daif_write(dvf9a, DAIF_ICU_ANA_STAT_EN1, val);
	}
	daif_write(dvf9a, DAIF_ICU_ANA_EN2, dvf9a->irq_enable_cur >> 7);
}

static struct irq_chip dvf9a_irq_chip = {
	.name = DVF9A,
	.irq_mask = dvf9a_irq_mask,
	.irq_disable = dvf9a_irq_mask,
	.irq_unmask = dvf9a_irq_unmask,
};

static irqreturn_t
dvf9a_irq_thread(int irq, void *priv)
{
	struct dvf9a *dvf9a = priv;
	int i, pending, pend_misc = 0;

	pending = daif_read(dvf9a, DAIF_ICU_ANA_SRC1);
	if (!pending)
		/* if ANA_MISC_INT is not set, assume dclass short or
		 * high-input
		 */
		pending |= 0x80;

	if (pending & 0x80) {
		pend_misc = daif_read(dvf9a, DAIF_ICU_ANA_STAT2);
		daif_write(dvf9a, DAIF_ICU_ANA_STAT2, pend_misc);
	}
	pending &= 0x7f;

	if (pending)
		daif_write(dvf9a, DAIF_ICU_ANA_STAT1, pending);

	pending |= pend_misc << 7;

	if (!pending)
		return IRQ_NONE;

	for (i = 0; pending != 0; i++, pending >>= 1) {
		if (!(pending & 1))
			continue;

		/* Check the mask again. Another handler/thread might have
		 * changed it in between.
		 */
		if (dvf9a->irq_enable_cur & (1 << i))
			handle_nested_irq(
				irq_linear_revmap(dvf9a->anaint_domain, i));
	}

	return IRQ_HANDLED;
}

static int
dvf9a_irq_map(struct irq_domain *dom, unsigned int irq,
	     irq_hw_number_t hw_irq)
{
	struct dvf9a *dvf9a = dom->host_data;

	irq_set_chip_data(irq, &(dvf9a->anaints[hw_irq]));
	irq_set_chip(irq, &dvf9a_irq_chip);
	irq_set_nested_thread(irq, 1);
	irq_set_probe(irq);

	return 0;
}

static const struct irq_domain_ops dvf9a_irq_ops = {
	.map	= dvf9a_irq_map,
	.xlate	= irq_domain_xlate_onecell,
};

#define DAIF_BYTES DAIF_INDIRECT_END

static int
dvf9a_rw_validate(loff_t pos, size_t *len)
{
	if (pos >= the_dvf9a->daif_indirect_end)
		return 0;	/* Past EOF */

	if (*len > the_dvf9a->daif_indirect_end - pos)
		*len = the_dvf9a->daif_indirect_end - pos;

	/* make sure start is aligned */
	if (pos >= DAIF_INDIRECT_START && (pos & 1))
		return -EINVAL;

	/* make sure end is aligned */
	if ((pos + *len) >= DAIF_INDIRECT_START && ((pos + *len) & 1))
		return -EINVAL;

	return 1;
}

static loff_t
dvf9a_llseek(struct file *file, loff_t offset, int origin)
{
	switch (origin) {
	case 0:
		/* nothing to do */
		break;
	case 1:
		offset += file->f_pos;
		break;
	case 2:
		offset += the_dvf9a->daif_indirect_end;
		break;
	default:
		return -EINVAL;
	}

	/* be a bit more strict than POSIX because seek'ing beyond the end
	 * doesn't make sense in our case
	 */
	if (offset < 0 || offset >= the_dvf9a->daif_indirect_end)
		return -EINVAL;

	/* ensure correct word alignment on indirect registers */
	if (offset >= DAIF_INDIRECT_START && (offset & 1))
		return -EINVAL;

	file->f_pos = offset;
	return offset;
}

static ssize_t
dvf9a_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned char contents[DAIF_BYTES];
	unsigned int i = *ppos;
	unsigned char *tmp;
	ssize_t ret;

	ret = dvf9a_rw_validate(*ppos, &count);
	if (ret <= 0)
		return ret;

	tmp = contents;
	while (count) {
		unsigned short val;

		if (test_bit(i, reserved_regs))
			val = 0;
		else
			val = daif_read(the_dvf9a, i);

		if (i >= DAIF_INDIRECT_START) {
			put_unaligned(val, (u16 *)tmp);
			tmp += 2;
			i += 2;
			count -= 2;
		} else {
			*tmp = val;
			tmp++;
			i++;
			count--;
		}
	}

	if (copy_to_user(buf, contents, tmp - contents))
		return -EFAULT;

	*ppos = i;

	return tmp - contents;
}

static ssize_t
dvf9a_write(struct file *file, const char __user *buf, size_t count,
	    loff_t *ppos)
{
	unsigned char contents[DAIF_BYTES];
	unsigned int i = *ppos;
	unsigned char *tmp;
	ssize_t ret;

	ret = dvf9a_rw_validate(*ppos, &count);
	if (ret <= 0)
		return ret;

	if (copy_from_user(contents, buf, count))
		return -EFAULT;

	tmp = contents;
	while (count) {
		if (i >= DAIF_INDIRECT_START) {
			if (!test_bit(i, reserved_regs))
				daif_write(the_dvf9a, i,
					   get_unaligned((u16 *)tmp));
			i += 2;
			tmp += 2;
			count -= 2;
		} else {
			if (!test_bit(i, reserved_regs))
				daif_write(the_dvf9a, i, *tmp);
			i++;
			tmp++;
			count--;
		}
	}

	*ppos = i;

	return tmp - contents;
}

static irqreturn_t
dvf9a_user_irq_thread(int irq, void *priv)
{
	int hw_irq;
	struct dvf9a *dvf9a = priv;
	char irq_num[8], hw_num[10];
	char const *envp[] = { irq_num, hw_num, NULL };

	for (hw_irq = 0; hw_irq < dvf9a->num_irqs; hw_irq++)
		if (dvf9a->user_irq[hw_irq] == irq)
			break;

	snprintf(irq_num, 8, "IRQ=%d", irq);
	snprintf(hw_num, 10, "HWIRQ=%d", hw_irq);
	kobject_uevent_env(&dvf9a->dev->kobj, KOBJ_CHANGE, (char **)envp);

	return IRQ_HANDLED;
}

static long
dvf9a_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct dvf9a *dvf9a = the_dvf9a;
	int irq;
	int ret = 0;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != DVF9A_IOC_MAGIC)
		return -ENOIOCTLCMD;

	/* Check access direction */
	ret = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	if (ret)
		return -EFAULT;

	switch (cmd) {
	case DVF9A_IOC_REQ_IRQ:
		if (__get_user(irq, (int __user *)arg))
			return -EFAULT;
		if ((irq < 0) || (irq >= dvf9a->num_irqs) ||
		     dvf9a->user_irq[irq])
			ret = -EBUSY;

		dvf9a->user_irq[irq] = daif_translate_irq(dvf9a, irq);
		if (!ret)
			ret = devm_request_irq(dvf9a->dev, dvf9a->user_irq[irq],
					       dvf9a_user_irq_thread, 0,
					       "dvf9a-user", dvf9a);
		if (ret)
			dvf9a->user_irq[irq] = 0;

		if (__put_user(ret, (int __user *)arg))
			return -EFAULT;

		return 0;
	case DVF9A_IOC_REL_IRQ:
		if (__get_user(irq, (int __user *)arg))
			return -EFAULT;
		if ((irq < 0) || (irq >= dvf9a->num_irqs) ||
		     !dvf9a->user_irq[irq])
			ret = -EINVAL;

		if (!ret)
			devm_free_irq(dvf9a->dev,
				      daif_translate_irq(dvf9a, irq), dvf9a);
		if (!ret)
			dvf9a->user_irq[irq] = 0;

		return 0;
	}

	return -ENOIOCTLCMD;
}

static const struct file_operations dvf9a_fops = {
	.owner		= THIS_MODULE,
	.llseek		= dvf9a_llseek,
	.read		= dvf9a_read,
	.write		= dvf9a_write,
	.unlocked_ioctl	= dvf9a_ioctl,
};

static int
dvf9a_probe(struct platform_device *pdev)
{
	struct reset_control *rc;
	struct dvf9a *dvf9a;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	int i, ret = 0;
	unsigned long rate;

	if (!np) {
		dev_err(&pdev->dev, "missing devicetree node");
		return -EINVAL;
	}

	rc = of_reset_control_get(np, NULL);
	if (!IS_ERR(rc)) {
		if (reset_control_deassert(rc)) {
			dev_err(&pdev->dev, "unable to release reset");
			return -EIO;
		}
	} else {
		dev_dbg(&pdev->dev, "no reset control available");
		rc = NULL;
	}

	dvf9a = devm_kzalloc(&pdev->dev, sizeof(*dvf9a), GFP_KERNEL);
	if (!dvf9a)
		return -ENOMEM;
	dvf9a->dev = &pdev->dev;

	the_dvf9a = dvf9a;

	platform_set_drvdata(pdev, dvf9a);

	spin_lock_init(&dvf9a->lock);

	dvf9a->daif_clk = devm_clk_get_optional(&pdev->dev, "daif_clk");
	if (IS_ERR(dvf9a->daif_clk))
		return PTR_ERR(dvf9a->daif_clk);
	clk_prepare_enable(dvf9a->daif_clk);

	dvf9a->apu_clk = devm_clk_get_optional(&pdev->dev, "apu_clk");
	if (IS_ERR(dvf9a->apu_clk)) {
		ret = PTR_ERR(dvf9a->apu_clk);
		goto err_apu_clk;
	}
	if (dvf9a->apu_clk) {
		rate = clk_round_rate(dvf9a->apu_clk, 13824000);
		clk_set_rate(dvf9a->apu_clk, rate);
		clk_prepare_enable(dvf9a->apu_clk);
	}

	dvf9a->irq = platform_get_irq(pdev, 0);
	if (dvf9a->irq < 0) {
		dev_err(&pdev->dev, "need an irq resource\n");
		ret = dvf9a->irq;
		goto err_apu_clk;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	dvf9a->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dvf9a->regs)) {
		dev_err(&pdev->dev, "failed to map registers\n");
		ret = PTR_ERR(dvf9a->regs);
		goto err_apu_clk;
	}

	dvf9a->gpio = devm_gpiod_get_optional(&pdev->dev, "reset",
					      GPIOD_OUT_HIGH);
	if (IS_ERR(dvf9a->gpio)) {
		ret = PTR_ERR(dvf9a->gpio);
		goto err_apu_clk;
	}

	if (dvf9a->gpio) /* reset-release requires some delay */
		udelay(1000);

#ifdef CONFIG_AVIO_MYNA2
	if (!is_avio_driver_initialized())
		return -EPROBE_DEFER;

	dvf9a->parent_clk = devm_clk_get_optional(&pdev->dev, "parent_clk");
	if (IS_ERR(dvf9a->parent_clk)) {
		ret = PTR_ERR(dvf9a->parent_clk);
		goto err_apu_clk;
	}
	if (!dvf9a->parent_clk)
		dev_dbg(&pdev->dev, "no parent clock available");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res)
		dvf9a->clk_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dvf9a->clk_regs)) {
		ret = PTR_ERR(dvf9a->clk_regs);
		goto err_apu_clk;
	}

	if (dvf9a->clk_regs && dvf9a->parent_clk) {
		uint32_t rate, div1, div2 = 0;

		rate = clk_get_rate(dvf9a->parent_clk);
		div1 = DIV_ROUND_CLOSEST(rate, 13824000);

		if (div1 >= 31) {
			div1 /= 2;
			div2++;
		}

		writel((0x1f << 15) | (1 << 13) | (div2 << 9) | (div1 << 3),
		       dvf9a->clk_regs);

		udelay(1000);
	}
#endif

	ret = daif_direct_mirrored_init(dvf9a);
	if (ret < 0) {
		if (!rc || !dvf9a->daif_clk) {
			dev_err(&pdev->dev, "failed to initialize DAIF\n");
			goto err_apu_clk;
		}

		dev_err(&pdev->dev,
			"failed to initialize DAIF - trying reset\n");

		clk_disable(dvf9a->daif_clk);
		udelay(1000);
		reset_control_assert(rc);
		udelay(1000);
		clk_enable(dvf9a->daif_clk);
		udelay(1000);
		clk_disable(dvf9a->daif_clk);
		udelay(1000);
		reset_control_deassert(rc);
		udelay(1000);
		clk_enable(dvf9a->daif_clk);
		udelay(1000);

		ret = daif_direct_mirrored_init(dvf9a);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"failed to initialize DAIF (again)\n");
			goto err_apu_clk;
		}
	}

	dvf9a->daif_indirect_end = DAIF_INDIRECT_END;
	dvf9a->num_irqs = DVF9A_NUM_IRQS;
	dvf9a->version = daif_read(dvf9a, DAIF_HW_REVISION);

	bitmap_set(reserved_regs,   9, 1);
	bitmap_set(reserved_regs,  12, 2);
	bitmap_set(reserved_regs,  41, 1);
	bitmap_set(reserved_regs,  45, 1);
	bitmap_set(reserved_regs,  64, 1);
	bitmap_set(reserved_regs,  90, 1);
	bitmap_set(reserved_regs, 105, 1);

	if (dvf9a->version < 0xa1) {
		dvf9a->daif_indirect_end = DAIF_DCLS_SHRT_TIMOUT + 2;
		dvf9a->num_irqs = 13;
		if (dvf9a->version < 2) {
			dvf9a->daif_indirect_end = DAIF_AFEICTRL2 + 2;
			dvf9a->num_irqs = 11;
		}
		bitmap_set(reserved_regs,  27,  1);
		bitmap_set(reserved_regs,  50,  1);
		bitmap_set(reserved_regs,  52, 12);
		bitmap_set(reserved_regs,  80,  1);
		bitmap_set(reserved_regs,  90,  1);
		bitmap_set(reserved_regs, 115, 13);
		bitmap_set(reserved_regs, 164, 28);
	} else {
		bitmap_set(reserved_regs,  27,  2);
		bitmap_set(reserved_regs,  30,  1);
		bitmap_set(reserved_regs,  35,  3);
		bitmap_set(reserved_regs,  43,  1);
		bitmap_set(reserved_regs,  56,  8);
		bitmap_set(reserved_regs,  82,  1);
		bitmap_set(reserved_regs,  91,  7);
		bitmap_set(reserved_regs, 119,  9);
		bitmap_set(reserved_regs, 128,  2);
		bitmap_set(reserved_regs, 134, 14);
		bitmap_set(reserved_regs, 152,  2);
		bitmap_set(reserved_regs, 156,  2);
		bitmap_set(reserved_regs, 168,  2);
		bitmap_set(reserved_regs, 174, 18);
	}

	dvf9a->filterless = of_property_read_bool(np, "filterless-mode");
	if (dvf9a->filterless) {
		if (daif_has_dvfa10(dvf9a)) { /* DVFA10 */
			daif_write(dvf9a, DAIF_DCLASS_ANA_CFG, 0xbb1f);

			daif_write(dvf9a, DAIF_DCLS_SHRT_TIMOUT, 0x05ff);

			daif_write(dvf9a, DAIF_DCLS_ANA_PROT, 0x588f);
		} else {
			/* New overlap mode 5 (ENN), 5 (ENP).
			 * Limit current to 740mA (0x4) (spare for diff PVT)
			 */
			daif_write(dvf9a, DAIF_DCLASS_ANA_CFG, 0x5da4);

			/* Set HI_INP_DELAY_DET to 0xf and DCLS_SHRT_DET_THRS
			 * to 0x0
			 */
			daif_write(dvf9a, DAIF_DCLS_SWCH_CFG, 0x0f30);

			/* Set HI_INP_ON_THRS to 0x1 */
			daif_write(dvf9a, DAIF_DCLS_HI_INP_THRS, 0x0427);
		}
	}

	/* Setup IRQs */
	daif_write(dvf9a, DAIF_ICU_INT_POL,      0x0000);
	daif_write(dvf9a, DAIF_ICU_ANA_EN1,      0x0000);
	daif_write(dvf9a, DAIF_ICU_ANA_EN2,      0x0000);
	daif_write(dvf9a, DAIF_ICU_ANA_STAT_EN1, 0x0000);
	daif_write(dvf9a, DAIF_ICU_ANA_STAT1,    0x00ff);
	daif_write(dvf9a, DAIF_ICU_ANA_STAT2,    0x00ff);

	/* PWR interrupts are not (yet) supported */
	daif_write(dvf9a, DAIF_ICU_PWR_EN,       0x0000);
	daif_write(dvf9a, DAIF_ICU_PWR_STAT_EN,  0x0000);
	daif_write(dvf9a, DAIF_ICU_PWR_STAT,     0x00ff);

	udelay(1);
	clk_disable(dvf9a->apu_clk);
	clk_disable(dvf9a->daif_clk);

	dvf9a->anaint_domain = irq_domain_add_linear(np, dvf9a->num_irqs,
						    &dvf9a_irq_ops, dvf9a);
	if (!dvf9a->anaint_domain) {
		dev_err(&pdev->dev, "cannot register IRQ domain!\n");
		ret = -ENOMEM;
		goto err_apu_clk;
	}

	for (i = 0; i < dvf9a->num_irqs; i++) {
		dvf9a->anaints[i].dvf9a = dvf9a;
		dvf9a->anaints[i].irq_domain = dvf9a->anaint_domain;
		dvf9a->anaints[i].hwirq = i;
	}

	ret = devm_request_irq(&pdev->dev, dvf9a->irq, dvf9a_irq_thread,
			       IRQF_TRIGGER_HIGH | IRQF_ONESHOT, DVF9A, dvf9a);
	if (ret < 0) {
		dev_err(&pdev->dev, "request irq %d failed\n", dvf9a->irq);
		goto err_irq_domain;
	}

	ret = of_platform_populate(np, NULL, NULL, &pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "error populating subdevices\n");
		goto err_irq_domain;
	}

	dvf9a->miscdev.minor = MISC_DYNAMIC_MINOR;
	dvf9a->miscdev.name = DVF9A;
	dvf9a->miscdev.fops = &dvf9a_fops;
	dvf9a->miscdev.parent = &pdev->dev;
	ret = misc_register(&dvf9a->miscdev);
	if (ret) {
		dev_err(&pdev->dev, "can't misc_register\n");
		goto err_irq_domain;
	}

	dev_info(&pdev->dev, "successfully registered (DVF9A version %d)\n",
		 dvf9a->version);

	return 0;

err_irq_domain:
	irq_domain_remove(dvf9a->anaint_domain);
err_apu_clk:
	clk_disable_unprepare(dvf9a->apu_clk);
	clk_disable_unprepare(dvf9a->daif_clk);

	return ret;
}

static int
dvf9a_remove(struct platform_device *dev)
{
	struct dvf9a *dvf9a = platform_get_drvdata(dev);

	irq_domain_remove(dvf9a->anaint_domain);

	misc_deregister(&dvf9a->miscdev);
	clk_disable_unprepare(dvf9a->daif_clk);
	clk_disable_unprepare(dvf9a->apu_clk);

	return 0;
}

static const struct of_device_id dvf9a_of_ids[] = {
	{ .compatible = "dspg,dvf9a" },
	{ },
};

static struct platform_driver dvf9a_platform_driver = {
	.driver = {
		.name	= DVF9A,
		.owner	= THIS_MODULE,
		.of_match_table = dvf9a_of_ids,
	},
	.probe		= dvf9a_probe,
	.remove		= dvf9a_remove,
};
module_platform_driver(dvf9a_platform_driver);

MODULE_DESCRIPTION("Synaptics DVF9A driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_ALIAS("platform:dvf9a-core");
