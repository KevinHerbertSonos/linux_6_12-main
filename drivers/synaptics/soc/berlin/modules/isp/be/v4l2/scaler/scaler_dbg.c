// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "common.h"
#include "m2m_scaler.h"
#include "scaler_dbg.h"

int scaler_debug_on;
module_param(scaler_debug_on, int, 0644);

int scaler_dump_debugfs;
module_param(scaler_dump_debugfs, int, 0644);

module_param(m2m_scaler_max_instance, int, 0644);

void scaler_dbg_perf_job_begin(struct m2m_scaler_dev *scaler, int id)
{
	int x =  scaler->dbg[id].job_writer;
	ktime_t time_now = ktime_get();

	scaler->dbg[id].buf_time[x].job_entry_time = time_now;
	scaler->dbg[id].buf_time[x].idle_duration =
		ktime_us_delta(time_now,
				scaler->dbg[id].buf_time[x].buf_entry_time);

	x = (x + 1) % MAX_QUEUE_ITEM;
	scaler->dbg[id].job_writer = x;
}

void scaler_dbg_perf_job_end(struct m2m_scaler_dev *scaler, int id)
{
	int counter = scaler->dbg[id].job_reader;

	scaler->dbg[id].buf_time[counter].job_duration =
		ktime_us_delta(ktime_get(),
				scaler->dbg[id].buf_time[counter].job_entry_time);

	counter = (counter + 1) % MAX_QUEUE_ITEM;
	scaler->dbg[id].job_reader = counter;
}

void scaler_dbg_perf_queue_begin(struct m2m_scaler_dev *scaler, u32 ready_bufs, int id)
{
	int counter =  scaler->dbg[id].writer;

	scaler->dbg[id].buf_time[counter].buf_entry_time = ktime_get();
	scaler->dbg[id].buf_time[counter].num_ready_bufs = ready_bufs;

	counter = (counter + 1) % MAX_QUEUE_ITEM;
	scaler->dbg[id].writer = counter;
}

void scaler_dbg_perf_queue_end(struct m2m_scaler_dev *scaler, int id)
{
	int counter =  scaler->dbg[id].reader;

	scaler->dbg[id].buf_time[counter].buf_live_duration =
		ktime_us_delta(ktime_get(),
				scaler->dbg[id].buf_time[counter].buf_entry_time);

	counter = (counter + 1) % MAX_QUEUE_ITEM;
	scaler->dbg[id].reader = counter;
}

void scaler_dbg_clear(struct m2m_scaler_dev *scaler, int id)
{
	memset(scaler->dbg[id].buf_time, 0,
			sizeof(struct scaler_dbg_time) * MAX_QUEUE_ITEM);
	scaler->dbg[id].job_reader = 0;
	scaler->dbg[id].job_writer = 0;
	scaler->dbg[id].reader = 0;
	scaler->dbg[id].writer = 0;
}

static int perf_show(struct seq_file *s, void *data)
{
	struct scaler_dbg *dbg = s->private;
	int i = 0;

	seq_puts(s, "Num Rdy Bufs\t Idle Time\t Job Time\t Buf live Time\n");
	if (dbg != NULL) {
		for (i = 0; i < MAX_QUEUE_ITEM; i++) {
			seq_printf(s, "%d %lld %lld %lld\n", dbg->buf_time[i].num_ready_bufs,
					dbg->buf_time[i].idle_duration,
					dbg->buf_time[i].job_duration,
					dbg->buf_time[i].buf_live_duration);
		}
	}
	return 0;
}

#define scaler_dbg_create_entry(name, i) \
	debugfs_create_file(#name, 0444, scaler->dbg[i].debugfs_entry, \
			&scaler->dbg[i], &name##_fops)

DEFINE_SHOW_ATTRIBUTE(perf);

void scaler_dbg_create(struct m2m_scaler_dev *scaler)
{
	char dirname[16];
	int i = 0;
	struct dentry *ret;

	for (i = 0; i < MAX_SCALER_INSTANCE; i++) {
		memset(dirname, 0, 16);
		snprintf(dirname, sizeof(dirname), "%s%d", SCALER_NAME, i);
		scaler->dbg[i].debugfs_entry = debugfs_create_dir(dirname, NULL);

		ret = scaler_dbg_create_entry(perf, i);
		if (ret == NULL)
			pr_err("perf create failed");
	}
}

void scaler_dbg_remove(struct m2m_scaler_dev *scaler)
{
	int i = 0;

	for (i = 0; i < MAX_SCALER_INSTANCE; i++) {
		debugfs_remove_recursive(scaler->dbg[i].debugfs_entry);
		scaler->dbg[i].debugfs_entry = NULL;
	}
}
