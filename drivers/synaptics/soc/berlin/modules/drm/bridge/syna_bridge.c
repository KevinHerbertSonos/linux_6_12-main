// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Synaptics Incorporated
 *
 */
#include <linux/platform_device.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>

#include "syna_bridge.h"

SYNA_BRIDGE_FUNC_TABLE syna_bridge_funcs;

int __weak syna_bridge_probe(struct platform_device *client, SYNA_BRIDGE_FUNC_TABLE *pSynaBridgeFuncs)
{
	return 0;
}

int syna_bridge_init(struct platform_device *client)
{
	return syna_bridge_probe(client, &syna_bridge_funcs);
}

void syna_bridge_modeset(struct display_timing *dsi_timing)
{
	if (syna_bridge_funcs.modeset)
		syna_bridge_funcs.modeset(dsi_timing);

	return;
}

void syna_bridge_deinit(void)
{
	if (syna_bridge_funcs.deinit)
		syna_bridge_funcs.deinit();
}