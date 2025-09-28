// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Synaptics Incorporated
 *
 */
#include <video/display_timing.h>
#include <video/of_display_timing.h>

typedef struct SYNA_BRIDGE_FUNC_TABLE_T {
    void (*modeset)(struct display_timing *pDispTiming);
    void (*deinit)(void);
} SYNA_BRIDGE_FUNC_TABLE;

int syna_bridge_init(struct platform_device *client);
int syna_bridge_probe(struct platform_device *client, SYNA_BRIDGE_FUNC_TABLE *pSynaBridgeFuncs);
void syna_bridge_modeset(struct display_timing *dsi_timing);
void syna_bridge_deinit(void);
