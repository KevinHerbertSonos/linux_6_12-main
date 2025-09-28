/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SCALER_DBG_H__
#define __SCALER_DBG_H__

#define MAX_QUEUE_ITEM          120
#define MAX_SCALER_INSTANCE     4

extern int scaler_debug_on;
#define scaler_print(arg...)		\
do {						\
	if (scaler_debug_on)	\
		pr_info(arg);	\
} while (0);

extern int scaler_dump_debugfs;
extern int m2m_scaler_max_instance;

struct scaler_dbg_time {
	ktime_t                 job_entry_time;
	ktime_t                 buf_entry_time;
	s64                     job_duration;
	s64                     idle_duration;
	s64                     buf_live_duration;
	u32                     client_id;
	u32                     num_ready_bufs;
	//struct queue_info       queue_info_data;
};

struct scaler_dbg {
	struct dentry           *debugfs_entry;
	struct scaler_dbg_time  buf_time[MAX_QUEUE_ITEM];
	u32                     job_reader;
	u32                     job_writer;
	u32                     reader;
	u32                     writer;
};

struct m2m_scaler_dev;

void scaler_dbg_create(struct m2m_scaler_dev *scaler);
void scaler_dbg_remove(struct m2m_scaler_dev *scaler);
void scaler_dbg_clear(struct m2m_scaler_dev *scaler, int id);
void scaler_dbg_perf_job_begin(struct m2m_scaler_dev *scaler, int id);
void scaler_dbg_perf_job_end(struct m2m_scaler_dev *scaler, int id);
void scaler_dbg_perf_queue_begin(struct m2m_scaler_dev *scaler, u32 rdy_bufs, int id);
void scaler_dbg_perf_queue_end(struct m2m_scaler_dev *scaler, int id);


#endif
