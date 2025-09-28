// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Synaptics Incorporated
 *
 */

#include <linux/debugfs.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <drm/drm_file.h>
#include "drm_syna_drv.h"

#define VPP_LOG_BUFF_SIZE   1024

/* debug nodes */
static struct dentry *debugfs_refwin_node;
static struct dentry *debugfs_dispwin_node;

static ssize_t dispwin_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	ssize_t bytes = 0;
	char buf[VPP_LOG_BUFF_SIZE];
	ENUM_PLANE_ID plane_id;

	bytes += scnprintf(buf, VPP_LOG_BUFF_SIZE,
			"plane : ( dispwin - x,y,w,h )\n");

	for (plane_id = FIRST_PLANE; plane_id < MAX_NUM_PLANES; plane_id++) {
		VPP_WIN dispwin;
		VPP_WIN_ATTR win_attr;

#ifdef USE_PLATYPUS
		if (plane_id == PLANE_PIP)
			continue;
#endif

		if (!wrap_MV_VPPOBJ_GetDispWindow(plane_id, &dispwin, &win_attr))
			bytes += scnprintf(buf + bytes, VPP_LOG_BUFF_SIZE - bytes,
				"  %d  : (%4d, %4d, %4d, %4d)\n", plane_id,
				dispwin.x, dispwin.y, dispwin.width, dispwin.height);

	}

	return simple_read_from_buffer(user_buf, count, ppos, buf, bytes);
}

static ssize_t refwin_read(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	ssize_t bytes = 0;
	char buf[VPP_LOG_BUFF_SIZE];
	ENUM_PLANE_ID plane_id;

	bytes += scnprintf(buf, VPP_LOG_BUFF_SIZE,
			"plane : ( refwin - x,y,w,h )\n");

	for (plane_id = FIRST_PLANE; plane_id < MAX_NUM_PLANES; plane_id++) {
		VPP_WIN refwin;

#ifdef USE_PLATYPUS
		if (plane_id == PLANE_PIP)
			continue;
#endif

		if (!wrap_MV_VPPOBJ_GetRefWindow(plane_id, &refwin))
			bytes += scnprintf(buf + bytes, VPP_LOG_BUFF_SIZE - bytes,
				"  %d  : (%4d, %4d, %4d, %4d)\n", plane_id,
				refwin.x, refwin.y, refwin.width, refwin.height);

	}

	return simple_read_from_buffer(user_buf, count, ppos, buf, bytes);
}

static const struct file_operations dispwin_fops = {
	.open		= simple_open,
	.read		= dispwin_read,
	.llseek		= default_llseek,
};

static const struct file_operations refwin_fops = {
	.open		= simple_open,
	.read		= refwin_read,
	.llseek		= default_llseek,
};

void syna_vpp_add_debugfs_entry(struct syna_drm_private *dev_priv)
{
	struct drm_minor *minor = dev_priv->dev->primary;
	struct dentry *root = minor ? minor->debugfs_root : NULL;

	if (!root)
		return;

	/* show refwin */
	debugfs_refwin_node = debugfs_create_file("refwin", S_IRUGO | S_IWUSR,
						root, dev_priv, &refwin_fops);

	/* show dispwin */
	debugfs_dispwin_node = debugfs_create_file("dispwin", S_IRUGO | S_IWUSR,
						root, dev_priv, &dispwin_fops);
}

void syna_vpp_remove_debugfs_entry(struct syna_drm_private *dev_priv)
{
	debugfs_remove(debugfs_refwin_node);
	debugfs_remove(debugfs_dispwin_node);
}
