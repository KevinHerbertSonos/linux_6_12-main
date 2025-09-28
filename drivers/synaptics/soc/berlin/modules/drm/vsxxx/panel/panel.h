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

#define MAX_PANELS           1
#define PANEL_MAX_WIDTH     3840
#define PANEL_MAX_HEIGHT    2160

int syna_panel_lcdc_init(struct platform_device *pdev);
void __weak syna_panel_lcdc_deinit(void);
int syna_panel_dsi_init(struct platform_device *pdev);
void syna_panel_dsi_deinit(void);
int syna_dsi_panel_send_cmd (unsigned int cmdsize, unsigned char *pcmd);
#endif
