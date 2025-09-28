// SPDX-License-Identifier: GPL-2.0
// Copyright 2021 Synaptics Incorporated

#ifndef HRX_SIG_STAT_NOTIFY_H
#define HRX_SIG_STAT_NOTIFY_H

#include <linux/kobject.h>
#include <linux/sysfs.h>

#define SYSFS_DIR_NAME "hrx_v4l2"
#define ATTRIBUTE_NAME "signal_status"

struct hdmi_rx_input_change_event {
    int hrx_stable_state;
    int width;
    int height;
    int fi_num;
    int fi_den;
};

int hrx_sig_stat_create(void);
void hrx_sig_stat_remove(void);
void hrx_sig_stat_set_attr(struct hdmi_rx_input_change_event *in_event);

#endif //HRX_SIG_STAT_NOTIFY_H
