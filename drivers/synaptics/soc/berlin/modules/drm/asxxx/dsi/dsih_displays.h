/* SPDX-License-Identifier: GPL-2.0 */
/**
 * Copyright (C) 2018 Synopsys, Inc.
 *
 * @file dsih_displays.h
 * @brief Synopsys MIPI DSI Host display support
 * included as a part of Synopsys MIPI DSI Host controller driver
 *
 * @author Luis Oliveira <luis.oliveira@synopsys.com>
 */

#ifndef MIPI_DISPLAYS_H_
#define MIPI_DISPLAYS_H_

#include "includes.h"

#define WAKE_UP
#define WAKE_UP_SEND_VIDEO

void copy_dpi_param_changes(dsih_dpi_video_t * from_param,
				dsih_dpi_video_t * to_param);

void copy_edpi_param_changes(dsih_cmd_mode_video_t * from_param,
				 dsih_cmd_mode_video_t * to_param);

int dsi_screen_init(struct mipi_dsi_dev *dev, int screen, int video_mode);

int pre_video_mode(struct mipi_dsi_dev *dev, unsigned screen, int lanes);

int pre_command_mode(struct mipi_dsi_dev *dev, unsigned screen, int lanes);

int dsi_panel_send_cmd (unsigned int cmdsize, unsigned char *pcmd);

void dsi_register_device (void *mipi);

int dsi_host_shutdown(int shutdown);

#endif	//MIPI_DISPLAYS_H_
