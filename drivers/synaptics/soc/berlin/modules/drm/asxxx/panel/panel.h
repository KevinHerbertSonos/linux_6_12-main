/*
// SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2023 Synaptics Incorporated
 *
 *
 * Author: Prem Anand N <prem.anand@synaptics.com>
 *
 */
#ifndef __PANEL_SYNA
#define __PANEL_SYNA

#define MAX_PANELS           2
#define PANEL_MAX_WIDTH     1920
#define PANEL_MAX_HEIGHT    1080

int syna_panel_lcdc_init(struct platform_device *pdev);
void syna_panel_lcdc_deinit(void);
void syna_panel_dsi_deinit(void);
int syna_dsi_panel_send_cmd (unsigned int cmdsize, unsigned char *pcmd);
int syna_panel_dsi_init(struct platform_device *pdev);
#endif