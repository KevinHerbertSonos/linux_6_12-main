// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Synaptics Incorporated
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/delay.h>
#include <linux/hwspinlock.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "../../hwspinlock/hwspinlock_internal.h"

#define IPC_INT_STATUS		0x00
#define IPC_INT_MASK		0x04
#define IPC_MASK_INT		0x08
#define IPC_LOCK_INT_ENABLE	0x0c
#define IPC_LOCK_STATUS		0x10
#define IPC_LOCK_SET		0x14
#define IPC_LOCK_CLR		0x18
#define IPC_MBX_INT_ENABLE	0x1c
#define IPC_MBX0		0x20
#define IPC_MBX1		0x24
#define IPC_MBX2		0x28
#define IPC_MBX3		0x2c

#define IPC_CSR_P0		0x00
#define IPC_CSR_P1		0x30

#define NUM_CHANS		4
#define NUM_LOCKS		32

struct syna_ipc_priv {
	struct mbox_controller mbox;
	void __iomem *reg;
	int irq[NUM_CHANS];
	spinlock_t lock;
};

static inline int channel_number(struct mbox_chan *chan)
{
	return chan - chan->mbox->chans;
}

static inline struct syna_ipc_priv *to_syna_ipc_priv(struct mbox_controller *mbox)
{
	return container_of(mbox, struct syna_ipc_priv, mbox);
}

static irqreturn_t syna_ipc_mbox_irq(int irq, void *data)
{
	struct mbox_chan *chan = data;
	struct syna_ipc_priv *priv = to_syna_ipc_priv(chan->mbox);
	int n = channel_number(chan);
	u32 val;

	writel_relaxed(BIT(n), priv->reg + IPC_INT_STATUS);

	val = readl_relaxed(priv->reg + IPC_CSR_P0 + IPC_MBX0 + n * 4);
	mbox_chan_received_data(chan, (void *)&val);

	return IRQ_HANDLED;
}

static int syna_ipc_mbox_startup(struct mbox_chan *chan)
{
	struct syna_ipc_priv *priv = to_syna_ipc_priv(chan->mbox);
	int ret, n = channel_number(chan);
	u32 val;

	ret = request_threaded_irq(priv->irq[n], NULL, syna_ipc_mbox_irq,
				   IRQF_ONESHOT, "syna-ipc-mbox", chan);
	if (ret) {
		dev_err(chan->mbox->dev, "Unable to acquire IRQ %d\n", priv->irq[n]);
		return ret;
	}

	spin_lock(&priv->lock);

	writel_relaxed(BIT(n), priv->reg + IPC_INT_STATUS);

	val = readl_relaxed(priv->reg + IPC_INT_MASK);
	val |= BIT(n);
	writel_relaxed(val, priv->reg + IPC_INT_MASK);

	val = readl_relaxed(priv->reg + IPC_MBX_INT_ENABLE);
	val |= BIT(n);
	writel_relaxed(val, priv->reg + IPC_MBX_INT_ENABLE);

	spin_unlock(&priv->lock);

	return 0;
}

static void syna_ipc_mbox_shutdown(struct mbox_chan *chan)
{
	struct syna_ipc_priv *priv = to_syna_ipc_priv(chan->mbox);
	int n = channel_number(chan);
	u32 val;

	spin_lock(&priv->lock);

	val = readl_relaxed(priv->reg + IPC_INT_MASK);
	val &= ~BIT(n);
	writel_relaxed(val, priv->reg + IPC_INT_MASK);

	val = readl_relaxed(priv->reg + IPC_MBX_INT_ENABLE);
	val &= ~BIT(n);
	writel_relaxed(val, priv->reg + IPC_MBX_INT_ENABLE);

	spin_unlock(&priv->lock);

	free_irq(priv->irq[n], chan);
}

static int syna_ipc_mbox_send_data(struct mbox_chan *chan, void *data)
{
	struct syna_ipc_priv *priv = to_syna_ipc_priv(chan->mbox);
	int n = channel_number(chan);
	u32 msg = data ? *(u32 *)data : 0;

	writel_relaxed(msg, priv->reg + IPC_CSR_P1 + IPC_MBX0 + n * 4);

	return 0;
}

static const struct mbox_chan_ops syna_ipc_mbox_chan_ops = {
	.send_data	= syna_ipc_mbox_send_data,
	.startup	= syna_ipc_mbox_startup,
	.shutdown	= syna_ipc_mbox_shutdown,
};

static int syna_ipc_hwspinlock_trylock(struct hwspinlock *lock)
{
	struct syna_ipc_priv *priv = dev_get_drvdata(lock->bank->dev);
	int lock_id = hwlock_to_id(lock);

	if (readl_relaxed(priv->reg + IPC_LOCK_STATUS) & BIT(lock_id))
		return 1;

	writel_relaxed(BIT(lock_id), priv->reg + IPC_LOCK_SET);

	return readl_relaxed(priv->reg + IPC_LOCK_STATUS) & BIT(lock_id);
}

static void syna_ipc_hwspinlock_unlock(struct hwspinlock *lock)
{
	struct syna_ipc_priv *priv = dev_get_drvdata(lock->bank->dev);
	int lock_id = hwlock_to_id(lock);

	writel_relaxed(BIT(lock_id), priv->reg + IPC_LOCK_CLR);
}

static void syna_ipc_hwspinlock_relax(struct hwspinlock *lock)
{
	ndelay(50);
}

static const struct hwspinlock_ops syna_ipc_hwspinlock_ops = {
	.trylock = syna_ipc_hwspinlock_trylock,
	.unlock = syna_ipc_hwspinlock_unlock,
	.relax = syna_ipc_hwspinlock_relax,
};

static int syna_ipc_hwspinlock_probe(struct device *dev, struct syna_ipc_priv *priv)
{
#if IS_ENABLED(CONFIG_HWSPINLOCK)
	struct hwspinlock_device *bank;
	int num_locks = NUM_LOCKS;

	bank = devm_kzalloc(dev, struct_size(bank, lock, num_locks), GFP_KERNEL);
	if (!bank)
		return -ENOMEM;

	return devm_hwspin_lock_register(dev, bank, &syna_ipc_hwspinlock_ops, 0, num_locks);
#else
	return 0;
#endif
}

static int syna_ipc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct syna_ipc_priv *priv;
	struct mbox_controller *mbox;
	int ret;
	int i;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	spin_lock_init(&priv->lock);

	mbox = &priv->mbox;
	mbox->dev = dev;
	mbox->ops = &syna_ipc_mbox_chan_ops;
	mbox->num_chans = NUM_CHANS;
	mbox->chans = devm_kcalloc(dev, mbox->num_chans, sizeof(*mbox->chans), GFP_KERNEL);
	if (!mbox->chans)
		return -ENOMEM;

	priv->reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->reg))
		return PTR_ERR(priv->reg);

	for (i = 0; i < NUM_CHANS; i++) {
		priv->irq[i] = platform_get_irq(pdev, i);
		if (priv->irq[i] < 0)
			return priv->irq[i];
	}

	platform_set_drvdata(pdev, priv);

	ret = syna_ipc_hwspinlock_probe(dev, priv);
	if (ret < 0)
		return ret;

	return devm_mbox_controller_register(dev, &priv->mbox);
}

static const struct of_device_id syna_ipc_of_match[] = {
	{ .compatible = "syna,sl261x-ipc" },
	{},
};
MODULE_DEVICE_TABLE(of, syna_ipc_of_match);

static struct platform_driver syna_ipc_driver = {
	.probe		= syna_ipc_probe,
	.driver = {
		.name	= "syna-ipc",
		.of_match_table = syna_ipc_of_match,
	},
};
module_platform_driver(syna_ipc_driver);

MODULE_DESCRIPTION("Synaptics IPC driver");
MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_LICENSE("GPL v2");
