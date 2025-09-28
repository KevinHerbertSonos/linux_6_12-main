// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2024 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2014 - 2024 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/


#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_graph.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mc.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-fwnode.h>
#include <linux/version.h>

#include "vvcam_video_driver.h"
#include "vvcam_video_register.h"
#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
#include "vvcam_video_platform.h"
#include "vvcam_pipeline_link.h"
#endif
#ifdef DOLPHIN
#include "isp_dma_heap.h"
#include "vvcam_v4l2_common.h"
#endif

#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
static int mcm_mask = 0x00;
MODULE_PARM_DESC(mcm_mask, "vvcam video mcm mask");
#endif

static int vvcam_video_register_ports(struct vvcam_media_dev *vvcam_mdev)
{
    int i = 0;
    int ret = 0;
    for (i = 0; i < vvcam_mdev->ports; i++) {
        if (vvcam_mdev->video_params[i].m2m == false) {
            ret = vvcam_video_register(vvcam_mdev, i);
            if (ret) {
                goto err_register_video;
            }
        }
    }
    return 0;

err_register_video:
    for (i = 0; i < vvcam_mdev->ports; i++) {
        if (vvcam_mdev->video_params[i].m2m == false)
            vvcam_video_unregister(vvcam_mdev, i);
    }

    return ret;
}

static int vvcam_video_unregister_ports(struct vvcam_media_dev *vvcam_mdev)
{
    int i;

    for (i = 0; i < vvcam_mdev->ports; i++) {
        if (vvcam_mdev->video_params[i].m2m == false)
            vvcam_video_unregister(vvcam_mdev, i);
    }

    return 0;
}

#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int vvcam_video_notifier_bound(struct v4l2_async_notifier *notifier,
                struct v4l2_subdev *sd, struct v4l2_async_connection *asc)
#else
static int vvcam_video_notifier_bound(struct v4l2_async_notifier *notifier,
                struct v4l2_subdev *sd, struct v4l2_async_subdev *asd)
#endif
{
    struct vvcam_media_dev *vvcam_mdev = container_of(notifier,
                            struct vvcam_media_dev, notifier);
    struct device *dev = vvcam_mdev->dev;
    struct v4l2_subdev *remote_subdev;
    struct v4l2_subdev *local_subdev;
    struct video_device *vdev;
    uint32_t port;
    struct media_entity *source, *sink;
    unsigned int source_pad, sink_pad;
    int i = 0;
    int ret;

    for (i = 0; i < vvcam_mdev->pipeline_link_size; i++) {
        if (vvcam_mdev->pipeline_link[i].remote_subdev) {
            remote_subdev = *vvcam_mdev->pipeline_link[i].remote_subdev;
            if (remote_subdev) {
                if (sd == remote_subdev) {
                    if (vvcam_mdev->pipeline_link[i].local_is_video) {
                        port = vvcam_mdev->pipeline_link[i].video_index;
                        vdev = vvcam_mdev->video_devs[port]->video;
                        source = &sd->entity;
                        source_pad = vvcam_mdev->pipeline_link[i].remote_pad;
                        sink =&vdev->entity;
                        sink_pad = 0;

                    } else {
                        local_subdev = *vvcam_mdev->pipeline_link[i].local_subdev;
                        source = &sd->entity;
                        source_pad = vvcam_mdev->pipeline_link[i].remote_pad;
                        sink =&local_subdev->entity;
                        sink_pad = vvcam_mdev->pipeline_link[i].local_pad;
                    }

                    ret = media_create_pad_link(source, source_pad, sink, sink_pad, 
                                                MEDIA_LNK_FL_ENABLED);
                    if (ret) {
                        dev_err(dev, "failed to create %s:%u -> %s:%u link\n",
                            source->name, source_pad, sink->name, sink_pad);
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int vvcam_video_notifier_bound(struct v4l2_async_notifier *notifier,
                struct v4l2_subdev *sd, struct v4l2_async_connection *asc)
#else
static int vvcam_video_notifier_bound(struct v4l2_async_notifier *notifier,
                struct v4l2_subdev *sd, struct v4l2_async_subdev *asd)
#endif
{
    struct vvcam_media_dev *vvcam_mdev = container_of(notifier,
                            struct vvcam_media_dev, notifier);
    struct device *dev = vvcam_mdev->dev;
    struct fwnode_handle *ep = NULL;
    struct v4l2_fwnode_link link;
    struct media_entity *source, *sink;
    unsigned int source_pad, sink_pad;
    struct vvcam_video_dev *vvcam_vdev;
    struct video_device *vdev;
    int ret;

    while (1) {
        ep = fwnode_graph_get_next_endpoint(sd->fwnode, ep);
        if (!ep)
            break;
        ret = v4l2_fwnode_parse_link(ep, &link);
        if (ret < 0) {
            dev_err(dev, "failed to parse link for %pOF: %d\n", to_of_node(ep), ret);
            continue;
        }

        if (sd->entity.pads[link.local_port].flags == MEDIA_PAD_FL_SINK)
            continue;

        vvcam_vdev = vvcam_mdev->video_devs[link.remote_port];
        vdev = vvcam_vdev->video;
        source = &sd->entity;
        source_pad = link.local_port;
        sink = &vdev->entity;
        sink_pad = 0;
        v4l2_fwnode_put_link(&link);
        ret = media_create_pad_link(source, source_pad, sink, sink_pad, MEDIA_LNK_FL_ENABLED);
        if (ret) {
            dev_err(dev, "failed to create %s:%u -> %s:%u link\n", source->name, source_pad,
            sink->name, sink_pad);
            break;
        }
        ++vvcam_mdev->active_device_cnt;

    }
    fwnode_handle_put(ep);

    return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static void vvcam_video_notifier_unbound(struct v4l2_async_notifier *notifier,
                    struct v4l2_subdev *sd, struct v4l2_async_connection *asc)
{
    return;
}
#else
static void vvcam_video_notifier_unbound(struct v4l2_async_notifier *notifier,
                    struct v4l2_subdev *sd, struct v4l2_async_subdev *asd)
{
    return;
}
#endif

static int vvcam_video_notifier_complete(struct v4l2_async_notifier *notifier)
{
    struct vvcam_media_dev *vvcam_mdev = container_of(notifier,
                            struct vvcam_media_dev, notifier);
#ifdef DOLPHIN
    struct v4l2_subdev *subdev;
    struct media_pad *pad;
    struct vvcam_video_dev *vvcam_vdev;
    struct vvcam_mcm_mode mcm;

    vvcam_vdev = vvcam_mdev->video_devs[0];
    if (vvcam_vdev == NULL)
        return -EINVAL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
    pad = media_pad_remote_pad_first(&vvcam_vdev->pad);
#else
    pad = media_entity_remote_pad(&vvcam_vdev->pad);
#endif
    if (!pad || !is_media_entity_v4l2_subdev(pad->entity))
        return  -EINVAL;;

    subdev = media_entity_to_v4l2_subdev(pad->entity);

    if (!subdev)
        return -EINVAL;

    if (vvcam_mdev-> active_device_cnt > VVCAM_VIDEO_PORT_PAD_NR)
        vvcam_mdev->mcm_mode = ISP_MCM_MODE_ENABLED;

    mcm.mcm_mode = vvcam_mdev->mcm_mode;
    v4l2_subdev_call(subdev, core, ioctl, VVCAM_PAD_SET_MCM_MODE, &mcm);
#endif
    return v4l2_device_register_subdev_nodes(&vvcam_mdev->v4l2_dev);
}

static const struct v4l2_async_notifier_operations vvcam_video_async_nf_ops = {
    .bound    = vvcam_video_notifier_bound,
    .unbind   = vvcam_video_notifier_unbound,
    .complete = vvcam_video_notifier_complete,
};

#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int vvcam_video_async_register_subdev(struct vvcam_media_dev *vvcam_mdev)
{
    int ret;
    int i = 0;
    bool repeat = false;
    struct v4l2_subdev *remote_subdev = NULL;
    struct v4l2_subdev *last_subdev = NULL;
    struct v4l2_async_connection *asc;

    v4l2_async_nf_init(&vvcam_mdev->notifier, &vvcam_mdev->v4l2_dev);

    vvcam_mdev->notifier.ops = &vvcam_video_async_nf_ops;

    for (i = 0; i < vvcam_mdev->pipeline_link_size; i++) {
        if (vvcam_mdev->pipeline_link[i].remote_subdev) {
            remote_subdev = *vvcam_mdev->pipeline_link[i].remote_subdev;
            /* not allow repeat add same fwnode */
            for (int j = i - 1; j >= 0; j--) {
                last_subdev = *vvcam_mdev->pipeline_link[j].remote_subdev;
                if (remote_subdev && last_subdev &&
                    remote_subdev == last_subdev) {
                    repeat = true;
                    break;
                }
            }

            if (remote_subdev && !repeat) {
                asc = v4l2_async_nf_add_fwnode(&vvcam_mdev->notifier,
                                        remote_subdev->fwnode,
                                        struct v4l2_async_connection);
                if (IS_ERR(asc)) {
                    ret = PTR_ERR(asc);
                    if (ret != -EEXIST) {
                        v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
                        return ret;
                    }
                }
            }
            repeat = false;
        }
    }

    ret = v4l2_async_nf_register(&vvcam_mdev->notifier);
    if (ret) {
        v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
        return ret;
    }

    return 0;
}

#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0) */
static int vvcam_video_async_register_subdev(struct vvcam_media_dev *vvcam_mdev)
{
    int ret;
    int i = 0;
    struct v4l2_subdev *remote_subdev;
    struct v4l2_async_subdev *asd;

    vvcam_mdev->notifier.ops = &vvcam_video_async_nf_ops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
    v4l2_async_nf_init(&vvcam_mdev->notifier);
#else
    v4l2_async_notifier_init(&vvcam_mdev->notifier);
#endif

    for (i = 0; i < vvcam_mdev->pipeline_link_size; i++) {
        if (vvcam_mdev->pipeline_link[i].remote_subdev) {
            remote_subdev = *vvcam_mdev->pipeline_link[i].remote_subdev;
            if (remote_subdev) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
                asd = v4l2_async_nf_add_fwnode(&vvcam_mdev->notifier,
                                        remote_subdev->fwnode,
                                        struct v4l2_async_subdev);
#else
                asd = v4l2_async_notifier_add_fwnode_subdev(
                                        &vvcam_mdev->notifier,
                                        remote_subdev->fwnode,
                                        struct v4l2_async_subdev);
#endif
                 if (IS_ERR(asd)) {
                        ret = PTR_ERR(asd);
                        if (ret != -EEXIST) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
                        v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
#else
                        v4l2_async_notifier_cleanup(&vvcam_mdev->notifier);
#endif
                        return ret;
                    }
                }
            }
        }
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
    ret = v4l2_async_nf_register(&vvcam_mdev->v4l2_dev, &vvcam_mdev->notifier);
#else
    ret = v4l2_async_notifier_register(&vvcam_mdev->v4l2_dev, &vvcam_mdev->notifier);
#endif
    if (ret) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
        v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
#else
        v4l2_async_notifier_cleanup(&vvcam_mdev->notifier);
#endif
        dev_err(vvcam_mdev->dev, "v4l2 async notifier register error\n");
        return ret;
    }

    return 0;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0) */

#else /* VVCAM_SUBDEV_PLATFORM_REGISTER */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
static struct v4l2_async_connection *
vvcam_video_async_nf_add_fwnode_remote(struct list_head *endpoint_list, struct v4l2_async_notifier *notif,
                struct fwnode_handle *endpoint,
                unsigned int asc_struct_size)
{
    struct v4l2_async_connection *asc;
    struct fwnode_handle *remote;
    struct list_head *cur;
    unsigned int add_remote_node = 0;
    struct remote_node_handle *entry;
    struct remote_node_handle *new_node;

    remote = fwnode_graph_get_remote_port_parent(endpoint);
    if (!remote) {
        return ERR_PTR(-ENOTCONN);
    }

    /* Add the remote nodes to the list and call __v4l2_async_nf_add_fwnode
     * for the remote handle. If the remote node is already present return
     * without adding.
     */
    if (!list_empty(endpoint_list)) {
        list_for_each(cur, endpoint_list) {
            entry = list_entry(cur, struct remote_node_handle, link);
            if (entry->remote == remote)
                return ERR_PTR(-EEXIST);
            else
                add_remote_node = 1;
        }
    } else {
        add_remote_node = 1;
    }

    if (add_remote_node) {
        new_node = kzalloc(sizeof(struct remote_node_handle), GFP_KERNEL);
        new_node->remote = remote;
        list_add_tail(&new_node->link, endpoint_list);
        asc = __v4l2_async_nf_add_fwnode(notif, remote, asc_struct_size);
    } else {
		fwnode_handle_put(remote);
	}

    return asc;
}

static int vvcam_video_async_register_subdev(struct vvcam_media_dev *vvcam_mdev)
{
    int ret = 0;
    struct fwnode_handle *ep;
    struct v4l2_async_connection *asc;
    unsigned int port_id = 0;
    struct list_head endpoint_list;
    struct remote_node_handle *pos = NULL, *next = NULL;

    INIT_LIST_HEAD(&endpoint_list);

    vvcam_mdev->notifier.ops = &vvcam_video_async_nf_ops;

    v4l2_async_nf_init(&vvcam_mdev->notifier, &vvcam_mdev->v4l2_dev);

    while (1) {
        ep = fwnode_graph_get_endpoint_by_id(dev_fwnode(vvcam_mdev->dev),
                port_id, 0, FWNODE_GRAPH_ENDPOINT_NEXT);
        if (!ep)
            break;

        asc = vvcam_video_async_nf_add_fwnode_remote(&endpoint_list, &vvcam_mdev->notifier,
                ep, sizeof(struct v4l2_async_connection));

        fwnode_handle_put(ep);

        if (IS_ERR(asc)) {
            ret = PTR_ERR(asc);
            if (ret != -EEXIST) {
                v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
                goto clean_list;
            }
        }
        port_id++;
    }

    ret = v4l2_async_nf_register(&vvcam_mdev->notifier);
    if (ret) {
        v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
        dev_err(vvcam_mdev->dev, "v4l2 async notifier register error %d\n", ret);
    }

clean_list:
    // Delete the list
    if (!list_empty(&endpoint_list)) {
        list_for_each_entry_safe(pos, next, &endpoint_list, link) {
            if (pos) {
				fwnode_handle_put(pos->remote);
                list_del(&pos->link);
                kfree(pos);
            }
        }
    }
    return ret;

}

#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0) */
#if 0
static struct v4l2_async_subdev *
vvcam_video_async_nf_add_fwnode_remote(struct v4l2_async_notifier *notif,
                struct fwnode_handle *endpoint,
                unsigned int asd_struct_size)
{
    struct v4l2_async_subdev *asd;
    struct fwnode_handle *remote;

    remote = fwnode_graph_get_remote_port_parent(endpoint);
    if (!remote)
        return ERR_PTR(-ENOTCONN);

    asd = __v4l2_async_nf_add_fwnode(notif, remote, asd_struct_size);
    fwnode_handle_put(remote);

    return asd;
}
#endif

static int vvcam_video_async_register_subdev(struct vvcam_media_dev *vvcam_mdev)
{
    int ret = 0;
    struct fwnode_handle *ep;
    struct v4l2_async_subdev *asd;
    unsigned int port_id = 0;

    vvcam_mdev->notifier.ops = &vvcam_video_async_nf_ops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
    v4l2_async_nf_init(&vvcam_mdev->notifier);
#else
    v4l2_async_notifier_init(&vvcam_mdev->notifier);
#endif

    while (1) {
        ep = fwnode_graph_get_endpoint_by_id(dev_fwnode(vvcam_mdev->dev),
                port_id, 0, FWNODE_GRAPH_ENDPOINT_NEXT);
        if (!ep)
            break;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
        asd = vvcam_video_async_nf_add_fwnode_remote(&vvcam_mdev->notifier,
                ep, sizeof(struct v4l2_async_subdev));
#else
        asd = v4l2_async_notifier_add_fwnode_remote_subdev(&vvcam_mdev->notifier,
                ep, struct v4l2_async_subdev);
#endif
        fwnode_handle_put(ep);

        if (IS_ERR(asd)) {
            ret = PTR_ERR(asd);
            if (ret != -EEXIST) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
                v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
#else
                v4l2_async_notifier_cleanup(&vvcam_mdev->notifier);
#endif
                return ret;
            }
        }
        port_id++;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
    ret = v4l2_async_nf_register(&vvcam_mdev->v4l2_dev, &vvcam_mdev->notifier);
#else
    ret = v4l2_async_notifier_register(&vvcam_mdev->v4l2_dev, &vvcam_mdev->notifier);
#endif
    if (ret) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
        v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
#else
        v4l2_async_notifier_cleanup(&vvcam_mdev->notifier);
#endif
        dev_err(vvcam_mdev->dev, "v4l2 async notifier register error\n");
        return ret;
    }

    return 0;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0) */
#endif /* VVCAM_SUBDEV_PLATFORM_REGISTER */

static int vvcam_video_async_unregister_subdev(struct vvcam_media_dev *vvcam_mdev)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
    v4l2_async_nf_unregister(&vvcam_mdev->notifier);
    v4l2_async_nf_cleanup(&vvcam_mdev->notifier);
#else
    v4l2_async_notifier_unregister(&isp_dev->notifier);
    v4l2_async_notifier_cleanup(&isp_dev->notifier);
#endif

    return 0;
}

static const struct media_device_ops vvcam_video_media_ops = {
    .link_notify = v4l2_pipeline_link_notify,
};

static int vvcam_video_parse_params(struct vvcam_media_dev *vvcam_mdev,
                                struct platform_device *pdev)
{
#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
    int ports = 0;
    int i;
    vvcam_mdev->id = pdev->id;
    switch(vvcam_mdev->id) {
        case 0:
            if (!mcm_mask) {
                vvcam_mdev->pipeline_link = pipeline0;
                vvcam_mdev->pipeline_link_size = ARRAY_SIZE(pipeline0);
            } else {
                vvcam_mdev->pipeline_link = mcm_pipeline0;
                vvcam_mdev->pipeline_link_size = ARRAY_SIZE(mcm_pipeline0);
            }
            break;
        case 1:
            if (!mcm_mask) {
                vvcam_mdev->pipeline_link = pipeline1;
                vvcam_mdev->pipeline_link_size = ARRAY_SIZE(pipeline1);
            } else {
                vvcam_mdev->pipeline_link = mcm_pipeline1;
                vvcam_mdev->pipeline_link_size = ARRAY_SIZE(mcm_pipeline1);
            }
            break;
        default:
            break;
    }

    for (i = 0; i < vvcam_mdev->pipeline_link_size; i++) {
        if (vvcam_mdev->pipeline_link[i].local_is_video) {
            if ((vvcam_mdev->pipeline_link[i].video_index + 1) > ports) {
                ports = vvcam_mdev->pipeline_link[i].video_index + 1;
            }
        }
    }
    vvcam_mdev->ports = ports;

#else
    unsigned int port_id = 0;
    struct fwnode_handle *ep;
    fwnode_property_read_u32(of_fwnode_handle(pdev->dev.of_node),
            "id", &vvcam_mdev->id);

     while (1) {
        ep = fwnode_graph_get_endpoint_by_id(dev_fwnode(vvcam_mdev->dev),
                port_id, 0, FWNODE_GRAPH_ENDPOINT_NEXT);
        if (!ep)
            break;

        port_id++;
     }

    vvcam_mdev->ports = port_id;
#endif
    return 0;
}

static int vvcam_video_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct media_device *mdev;
    struct device *dev = &pdev->dev;
    struct vvcam_media_dev *vvcam_mdev;

    vvcam_mdev = devm_kzalloc(dev, sizeof(struct vvcam_media_dev), GFP_KERNEL);
    if (!vvcam_mdev)
        return -ENOMEM;

    vvcam_mdev->dev = dev;
    platform_set_drvdata(pdev, vvcam_mdev);

    ret = vvcam_video_parse_params(vvcam_mdev, pdev);
    if (ret) {
        dev_err(dev, "parse device params error\n");
        return ret;
    }

    mdev = &vvcam_mdev->mdev;
    mdev->dev = dev;
#ifdef DOLPHIN
    vvcam_mdev->active_device_cnt = 0;
    vvcam_mdev->mcm_mode = ISP_MCM_MODE_DISABLED;
#endif
    mdev->ops = &vvcam_video_media_ops;
    strscpy(mdev->model, "verisilicon_media", sizeof(mdev->model));
    media_device_init(mdev);

    vvcam_mdev->v4l2_dev.mdev = mdev;
#ifdef DOLPHIN
    /* Create alloc device for creating DMA memory */
    ret = isp_dma_heap_dev_alloc((vvcam_mdev->alloc_dev));
    if (ret) {
        pr_err("%s(): failed to create allocate evice\n", __func__);
        return ret;
    }
#endif
    ret = v4l2_device_register(dev, &vvcam_mdev->v4l2_dev);
    if (ret) {
        dev_err(dev, "register v4l2 device error\n");
#ifdef DOLPHIN
        goto err_release_dma_heap;
#else
        return ret;
#endif
    }
    ret = vvcam_video_register_ports(vvcam_mdev);
    if (ret) {
        dev_err(dev, "register video device nodes error\n");
        goto err_unregister_v4l2_device;
    }

    ret = vvcam_video_async_register_subdev(vvcam_mdev);
    if (ret) {
        dev_err(dev, "register v4l2 async notifier error\n");
        goto err_unregister_video_ports;
    }

    ret = media_device_register(mdev);
    if (ret) {
        dev_err(dev, "register media device error\n");
        goto err_unregister_subdev;
    }
    dev_info(&pdev->dev, "vvcam video driver probe success\n");
    return 0;

err_unregister_subdev:
    vvcam_video_async_unregister_subdev(vvcam_mdev);
err_unregister_video_ports:
    vvcam_video_unregister_ports(vvcam_mdev);
err_unregister_v4l2_device:
    v4l2_device_unregister(&vvcam_mdev->v4l2_dev);
#ifdef DOLPHIN
err_release_dma_heap:
    isp_dma_heap_dev_release();
#endif
    return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
static void vvcam_video_remove(struct platform_device *pdev)
#else
static int vvcam_video_remove(struct platform_device *pdev)
#endif
{
    struct vvcam_media_dev *vvcam_mdev;

    vvcam_mdev = platform_get_drvdata(pdev);

    media_device_unregister(&vvcam_mdev->mdev);
    vvcam_video_async_unregister_subdev(vvcam_mdev);
    vvcam_video_unregister_ports(vvcam_mdev);
    v4l2_device_unregister(&vvcam_mdev->v4l2_dev);
    isp_dma_heap_dev_release();
    dev_info(&pdev->dev, "vvcam video driver remove\n");

#if (LINUX_VERSION_CODE <KERNEL_VERSION(6, 0, 0))
    return 0;
#endif
}

static const struct of_device_id vvcam_video_of_match[] = {
#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
    {.compatible = "verisilicon,nocompat",},
#else
    {.compatible = "verisilicon,video",},
#endif
    { /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, vvcam_video_of_match);

static struct platform_driver vvcam_video_driver = {
    .probe  = vvcam_video_probe,
    .remove = vvcam_video_remove,
    .driver = {
        .name  = VVCAM_VIDEO_NAME,
        .owner = THIS_MODULE,
        .of_match_table = vvcam_video_of_match,
    },
};

static int __init vvcam_video_init_module(void)
{
    int ret;
    ret = platform_driver_register(&vvcam_video_driver);
    if (ret) {
        printk(KERN_ERR "Failed to register video driver\n");
        return ret;
    }

#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
    ret = vvcam_video_platform_device_register();
    if (ret) {
        platform_driver_unregister(&vvcam_video_driver);
        printk(KERN_ERR "Failed to register vvcam video platform devices\n");
        return ret;
    }
#endif

    return ret;
}

static void __exit vvcam_video_exit_module(void)
{
    platform_driver_unregister(&vvcam_video_driver);
#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
    vvcam_video_platform_device_unregister();
#endif

    return;
}

#ifdef VVCAM_SUBDEV_PLATFORM_REGISTER
module_param(mcm_mask, int, 0644);
#endif

module_init(vvcam_video_init_module);
module_exit(vvcam_video_exit_module);

MODULE_DESCRIPTION("Verisilicon isp driver");
MODULE_AUTHOR("Verisilicon ISP SW Team");
MODULE_LICENSE("GPL");

