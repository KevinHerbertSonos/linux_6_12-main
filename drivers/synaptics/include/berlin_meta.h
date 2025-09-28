/* SPDX-License-Identifier: GPL-2.0 */
/*
 * berlin meta header
 *
 * Copyright (C) 2022 Synaptics, Inc.
 */

#ifndef _BERLIN_META_H
#define _BERLIN_META_H

struct berlin_pte_node {
	struct list_head list;
	struct dma_buf *dmabuf;
	unsigned int try_count;
	phys_addr_t phy_addr_pt;
	size_t len_pt;
	u64 mem_id;
	u32 flags;
	bool is_pte;
	unsigned long dmabuf_flags;
	char task_comm[TASK_COMM_LEN];
	pid_t pid;
	char thread_name[TASK_COMM_LEN];
	pid_t tid;
};

struct mtr_meta {
	u32 mtrcfg[7];
	u32 mtr_flags;
};

typedef struct berlin_meta {
	struct berlin_pte_node pte;
	struct page *pages;
	struct mtr_meta mtrr;
} berlin_meta_t;

#define DMA_BUF_FLAG_CUST_HEAP   BIT(2)

#endif /* _BERLIN_META_H */
