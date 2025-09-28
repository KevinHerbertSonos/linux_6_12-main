// SPDX-License-Identifier: GPL-2.0
/*
 * heap extra function support
 *
 * Copyright (C) 2022 Synaptics, Inc.
 */

#include <linux/err.h>
#include <linux/module.h>
#include "heap_extra.h"

static LIST_HEAD(heap_extra_list);
static DEFINE_SPINLOCK(heap_extra_lock);

static int (*free_cb_all)(struct dma_buf *dmabuf);

int heap_extra_add_heap(struct heap_extra *heap_extra)
{
	spin_lock(&heap_extra_lock);

	heap_extra->free_cb = free_cb_all;
	list_add(&heap_extra->list, &heap_extra_list);

	spin_unlock(&heap_extra_lock);

	return 0;
}
EXPORT_SYMBOL(heap_extra_add_heap);

int heap_extra_rm_heap(struct heap_extra *heap_extra)
{
	spin_lock(&heap_extra_lock);

	list_del(&heap_extra->list);

	spin_unlock(&heap_extra_lock);

	return 0;
}
EXPORT_SYMBOL(heap_extra_rm_heap);

int heap_extra_reg_free_cb(int (*free_cb)(struct dma_buf *dmabuf))
{
	struct heap_extra *heap_extra;

	spin_lock(&heap_extra_lock);

	free_cb_all = free_cb;
	list_for_each_entry(heap_extra, &heap_extra_list, list) {
		heap_extra->free_cb = free_cb;
	}
	spin_unlock(&heap_extra_lock);

	return 0;
}
EXPORT_SYMBOL(heap_extra_reg_free_cb);

static int __init heap_extra_init(void)
{
	return 0;
}

static void __exit heap_extra_exit(void)
{
	struct heap_extra *heap_extra, *n;

	spin_lock(&heap_extra_lock);

	list_for_each_entry_safe(heap_extra, n, &heap_extra_list, list) {
		list_del(&heap_extra->list);
	}

	spin_unlock(&heap_extra_lock);
}

module_init(heap_extra_init);
module_exit(heap_extra_exit);
MODULE_LICENSE("GPL v2");
