// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 *
 * Based on imx_rpmsg.c
 *
 * Copyright 2019 NXP
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mailbox_client.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/virtio_config.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_ring.h>

#include "../../rpmsg/rpmsg_internal.h"

#define REMOTE_IS_READY			BIT(0)
#define SPECIFIC_DMA_POOL		BIT(1)

#define RPMSG_NUM_BUFS			(512)
#define RPMSG_BUF_SIZE			(512)
#define RPMSG_BUFS_SPACE		(RPMSG_NUM_BUFS * RPMSG_BUF_SIZE)
#define RPMSG_VRING_ALIGN		(4096)
#define RPMSG_VRING_SIZE		0x8000UL

struct syna_virtio_dev {
	struct virtio_device vdev;
	unsigned int vring[2];
	struct virtqueue *vq[2];
	int base_vq_id;
	int num_of_vqs;
	struct syna_rpmsg_dev *rpdev;
};

#define to_syna_rpvdev(vd)		container_of(vd, struct syna_virtio_dev, vdev)

struct syna_rpmsg_dev {
	struct mbox_client cl;
	struct mbox_chan *chan;
	int vdev_nums;
	int first_notify;
	u32 flags;
#define MAX_VDEV_NUMS	8
	struct syna_virtio_dev *rpvdev[MAX_VDEV_NUMS];
	struct platform_device *pdev;
};

struct syna_rpmsg_vq_info {
	__u16 num;		/* number of entries in the virtio_ring */
	__u16 vq_id;		/* a globaly unique index of this virtqueue */
	__u32 mmsg;		/* the mailbox msg transferred on the virtqueue */
	void *vring_addr;	/* address where we mapped the virtio ring */
	struct syna_rpmsg_dev *rpdev;
};

static u8 syna_rpmsg_get_status(struct virtio_device *vdev)
{
	return 0;
}

static void syna_rpmsg_set_status(struct virtio_device *vdev, u8 status)
{
	dev_dbg(&vdev->dev, "%s: %d\n", __func__, status);
}

static void syna_rpmsg_reset(struct virtio_device *vdev)
{
	dev_dbg(&vdev->dev, "reset !\n");
}

static bool syna_rpmsg_notify(struct virtqueue *vq)
{
	struct syna_rpmsg_vq_info *rpvq = vq->priv;
	struct syna_rpmsg_dev *rpdev = rpvq->rpdev;
	int ret;
	u32 mmsg;

	mmsg = rpvq->vq_id << 16;

	if (unlikely(rpdev->first_notify > 0)) {
		rpdev->first_notify--;
		rpdev->cl.tx_tout = 20;
	} else {
		rpdev->cl.tx_tout = 1000;
	}

	ret = mbox_send_message(rpdev->chan, &mmsg);
	if (ret < 0)
		return false;

	mbox_client_txdone(rpdev->chan, ret);

	return true;
}

static struct virtqueue *syna_rpmsg_find_vq(struct virtio_device *vdev,
					    unsigned int index,
					    void (*callback)(struct virtqueue *vq),
					    const char *name,
					    bool ctx)
{
	struct syna_virtio_dev *rpvdev = to_syna_rpvdev(vdev);
	struct syna_rpmsg_dev *rpdev = rpvdev->rpdev;
	struct platform_device *pdev = rpdev->pdev;
	struct device *dev = &pdev->dev;
	struct syna_rpmsg_vq_info *rpvq;
	struct virtqueue *vq;
	int ret;

	rpvq = kmalloc(sizeof(*rpvq), GFP_KERNEL);
	if (!rpvq)
		return ERR_PTR(-ENOMEM);

	/* ioremap'ing normal memory, so we cast away sparse's complaints */
	rpvq->vring_addr = (__force void *)ioremap(rpvdev->vring[index], RPMSG_VRING_SIZE);
	if (!rpvq->vring_addr) {
		ret = -ENOMEM;
		goto free_rpvq;
	}
	dev_dbg(dev, "vring%d: phys 0x%x, virt 0x%p\n", index, rpvdev->vring[index], rpvq->vring_addr);

	memset_io(rpvq->vring_addr, 0, RPMSG_VRING_SIZE);

	vq = vring_new_virtqueue(index, RPMSG_NUM_BUFS / 2, RPMSG_VRING_ALIGN, vdev, true, ctx,
				 rpvq->vring_addr, syna_rpmsg_notify, callback, name);
	if (!vq) {
		dev_err(dev, "vring_new_virtqueue failed\n");
		ret = -ENOMEM;
		goto unmap_vring;
	}

	rpvdev->vq[index] = vq;
	vq->priv = rpvq;
	rpvq->vq_id = rpvdev->base_vq_id + index;
	rpvq->rpdev = rpdev;

	return vq;

unmap_vring:
	iounmap((__force void __iomem *)rpvq->vring_addr);
free_rpvq:
	kfree(rpvq);
	return ERR_PTR(ret);
}

static void syna_rpmsg_del_vqs(struct virtio_device *vdev)
{
	struct virtqueue *vq, *n;

	list_for_each_entry_safe(vq, n, &vdev->vqs, list) {
		struct syna_rpmsg_vq_info *rpvq = vq->priv;

		iounmap(rpvq->vring_addr);
		vring_del_virtqueue(vq);
		kfree(rpvq);
	}
}

static int syna_rpmsg_find_vqs(struct virtio_device *vdev, unsigned int nvqs,
			     struct virtqueue *vqs[],
			     struct virtqueue_info vqs_info[],
			     struct irq_affinity *desc)
{
	struct syna_virtio_dev *rpvdev = to_syna_rpvdev(vdev);
	int i, ret;

	/* we maintain two virtqueues per remote processor (for RX and TX) */
	if (nvqs != 2)
		return -EINVAL;

	for (i = 0; i < nvqs; ++i) {
		struct virtqueue_info *vqi = &vqs_info[i];

		vqs[i] = syna_rpmsg_find_vq(vdev, i, vqi->callback, vqi->name, vqi->ctx);
		if (IS_ERR(vqs[i])) {
			ret = PTR_ERR(vqs[i]);
			goto error;
		}
	}

	rpvdev->num_of_vqs = nvqs;

	return 0;

error:
	syna_rpmsg_del_vqs(vdev);

	return ret;
}

static u64 syna_rpmsg_get_features(struct virtio_device *vdev)
{
	/* VIRTIO_RPMSG_F_NS has been made private */
	return 1 << 0;
}

static int syna_rpmsg_finalize_features(struct virtio_device *vdev)
{
	/* Give virtio_ring a chance to accept features */
	vring_transport_features(vdev);

	return 0;
}

static struct virtio_config_ops syna_rpmsg_config_ops = {
	.get_status		= syna_rpmsg_get_status,
	.set_status		= syna_rpmsg_set_status,
	.reset			= syna_rpmsg_reset,
	.find_vqs		= syna_rpmsg_find_vqs,
	.del_vqs		= syna_rpmsg_del_vqs,
	.get_features		= syna_rpmsg_get_features,
	.finalize_features	= syna_rpmsg_finalize_features,
};

static void syna_rpmsg_vdev_release(struct device *dev)
{
	/* this handler is provided so driver core doesn't yell at us */
}

static int set_vring_phy_buf(struct platform_device *pdev,
			     struct syna_rpmsg_dev *rpdev, int vdev_nums)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	resource_size_t size;
	unsigned int start, end;
	int i, ret = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	size = resource_size(res);
	start = res->start;
	end = res->start + size;
	for (i = 0; i < vdev_nums; i++) {
		rpdev->rpvdev[i] = devm_kzalloc(dev, sizeof(struct syna_virtio_dev), GFP_KERNEL);
		if (!rpdev->rpvdev[i])
			return -ENOMEM;

		rpdev->rpvdev[i]->vring[0] = start;
		rpdev->rpvdev[i]->vring[1] = start + RPMSG_VRING_SIZE;
		start += 2 * RPMSG_VRING_SIZE;
		if (start > end) {
			dev_err(dev, "Too small memory size %x!\n", (u32)size);
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}

static void syna_rpmsg_rx_callback(struct mbox_client *client, void *msg)
{
	struct syna_virtio_dev *rpvdev;
	struct syna_rpmsg_dev *rpdev = container_of(client, struct syna_rpmsg_dev, cl);
	struct platform_device *pdev = rpdev->pdev;
	struct device *dev = &pdev->dev;
	u32 data = *(u32 *)msg;
	int i;

	dev_dbg(dev, "%s msg: 0x%x\n", __func__, data);

	i = (data >> 16) / 2;
	if (i >= MAX_VDEV_NUMS)
		return;

	rpvdev = rpdev->rpvdev[i];

	data = data >> 16;
	data -= rpvdev->base_vq_id;

	if (data < rpvdev->num_of_vqs)
		vring_interrupt(data, rpvdev->vq[data]);
}

static int syna_rpmsg_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct syna_rpmsg_dev *rpdev = NULL;
	struct mbox_client *cl;
	int i, ret = 0;

	rpdev = devm_kzalloc(dev, sizeof(*rpdev), GFP_KERNEL);
	if (!rpdev)
		return -ENOMEM;

	dev_info(dev, "syna rpmsg platform probe.\n");
	rpdev->pdev = pdev;
	rpdev->first_notify = 0;

	cl = &rpdev->cl;
	cl->dev = dev;
	cl->rx_callback = syna_rpmsg_rx_callback;

	rpdev->chan = mbox_request_channel_byname(cl, "rpmsg");
	if (IS_ERR(rpdev->chan)) {
		ret = PTR_ERR(rpdev->chan);
		return dev_err_probe(dev, ret, "failed to request mbox rx/tx chan");
	}

	ret = device_property_read_u32(dev, "syna,vdev-nums", &rpdev->vdev_nums);
	if (ret)
		rpdev->vdev_nums = 1;
	if (rpdev->vdev_nums > MAX_VDEV_NUMS) {
		dev_err(dev, "vdev-nums exceed the max %d\n", MAX_VDEV_NUMS);
		ret = -EINVAL;
		goto free_channel;
	}

	ret = set_vring_phy_buf(pdev, rpdev, rpdev->vdev_nums);
	if (ret) {
		dev_err(dev, "No vring buffer.\n");
		ret = -ENOMEM;
		goto free_channel;
	}

	if (of_reserved_mem_device_init(dev)) {
		dev_dbg(dev, "dev doesn't have specific DMA pool.\n");
		rpdev->flags &= (~SPECIFIC_DMA_POOL);
	} else {
		rpdev->flags |= SPECIFIC_DMA_POOL;
	}

	for (i = 0; i < rpdev->vdev_nums; i++) {
		dev_info(dev, "rpdev vdev%d: vring0 0x%x, vring1 0x%x\n",
			 i, rpdev->rpvdev[i]->vring[0], rpdev->rpvdev[i]->vring[1]);
		rpdev->rpvdev[i]->vdev.id.device = VIRTIO_ID_RPMSG;
		rpdev->rpvdev[i]->vdev.config = &syna_rpmsg_config_ops;
		rpdev->rpvdev[i]->vdev.dev.parent = dev;
		rpdev->rpvdev[i]->vdev.dev.release = syna_rpmsg_vdev_release;
		rpdev->rpvdev[i]->base_vq_id = i * 2;
		rpdev->rpvdev[i]->rpdev = rpdev;

		ret = register_virtio_device(&rpdev->rpvdev[i]->vdev);
		if (ret) {
			dev_err(dev, "fail to register rpvdev: %d\n", ret);
			goto free_reserved_mem;
		}
	}

	platform_set_drvdata(pdev, rpdev);

	return ret;

free_reserved_mem:
	if (rpdev->flags & SPECIFIC_DMA_POOL)
		of_reserved_mem_device_release(dev);

free_channel:
	mbox_free_channel(rpdev->chan);

	return ret;
}

static void syna_rpmsg_remove(struct platform_device *pdev)
{
	int i;
	struct device *dev = &pdev->dev;
	struct syna_rpmsg_dev *rpdev = platform_get_drvdata(pdev);

	for (i = 0; i < rpdev->vdev_nums; i++)
		unregister_virtio_device(&rpdev->rpvdev[i]->vdev);

	of_reserved_mem_device_release(dev);

	mbox_free_channel(rpdev->chan);
}

static const struct of_device_id syna_rpmsg_dt_ids[] = {
	{ .compatible = "syna,rpmsg", },
	{ },
};
MODULE_DEVICE_TABLE(of, syna_rpmsg_dt_ids);

static struct platform_driver syna_rpmsg_driver = {
	.probe = syna_rpmsg_probe,
	.remove = syna_rpmsg_remove,
	.driver = {
		.name = "syna-rpmsg",
		.of_match_table = syna_rpmsg_dt_ids,
	},
};
module_platform_driver(syna_rpmsg_driver);

MODULE_DESCRIPTION("Synaptics Remote Processors Messaging Platform Support");
MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_LICENSE("GPL");
