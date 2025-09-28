/* SPDX-License-Identifier: GPL-2.0
 *
 * heap extra function header
 *
 * Copyright (C) 2022 Synaptics, Inc.
 */

#ifndef _HEAP_EXTRA_H
#define _HEAP_EXTRA_H

#include <linux/list.h>
#include <linux/dma-buf.h>

struct heap_extra {
	struct list_head list;
	int (*free_cb)(struct dma_buf *dmabuf);
};

int heap_extra_add_heap(struct heap_extra *heap_extra);

int heap_extra_rm_heap(struct heap_extra *heap_extra);

int heap_extra_reg_free_cb(int (*free_cb)(struct dma_buf *dmabuf));

#endif /* _HEAP_EXTRA_H */
