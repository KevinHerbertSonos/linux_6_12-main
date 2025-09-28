// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>

#define MSG		"pingpong!"

static int rpmsg_pingpong_cb(struct rpmsg_device *rpdev, void *data, int len, void *priv, u32 src)
{
	int ret;
	unsigned int pingpong;

	pingpong = *(int *)data;
	dev_info(&rpdev->dev, "get %d (src: 0x%x)\n", pingpong, src);

	if (pingpong++ > 100) {
		dev_info(&rpdev->dev, "goodbye!\n");
		return 0;
	}

	ret = rpmsg_sendto(rpdev->ept, &pingpong, sizeof(pingpong), src);
	if (ret)
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);

	return ret;
}

static int rpmsg_pingpong_probe(struct rpmsg_device *rpdev)
{
	int ret;
	unsigned int pingpong = 0;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n", rpdev->src, rpdev->dst);

	ret = rpmsg_send(rpdev->ept, MSG, strlen(MSG));
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		return ret;
	}

	ret = rpmsg_sendto(rpdev->ept, &pingpong, sizeof(pingpong), rpdev->dst);
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct rpmsg_device_id rpmsg_pingpong_id_table[] = {
	{ .name	= "rpmsg-client-pingpong" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_pingpong_id_table);

static struct rpmsg_driver rpmsg_client_pingpong = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_pingpong_id_table,
	.probe		= rpmsg_pingpong_probe,
	.callback	= rpmsg_pingpong_cb,
};
module_rpmsg_driver(rpmsg_client_pingpong);

MODULE_DESCRIPTION("Remote processor messaging pingpong client driver");
MODULE_AUTHOR("Jisheng Zhang <jszhang@kernel.org>");
MODULE_LICENSE("GPL");
