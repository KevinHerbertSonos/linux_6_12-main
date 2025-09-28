/*
 * mipi_displays.h
 *
 *  Created on: Aug 31, 2011
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef MIPI_DISPLAYS_H_
#define MIPI_DISPLAYS_H_

#include "includes.h"
#include <dt-bindings/dspg/mipi_dsih.h>

enum mipi_displays {
	TREMOLO_S = DSPG_DISPLAY_TREMOLO_S,
	TREMOLO_M = DSPG_DISPLAY_TREMOLO_M,
	HH060IA = DSPG_DISPLAY_HH060IA,
	TM060JVZ = DSPG_DISPLAY_TM060JVZ,
	OTM8009A = DSPG_DISPLAY_OTM8009A,
	TM050RDHG17 = DSPG_DISPLAY_TM050RDHG17,
	TM050RSPA01 = DSPG_DISPLAY_TM050RSPA01,
	TM055JVZG08 = DSPG_DISPLAY_TM055JVZG08,
	EAG_TG7201280C12A = DSPG_DISPLAY_EAG_TG7201280C12A,
	TESTING,
};

#define WAKE_UP
#define WAKE_UP_SEND_VIDEO

#define COMMAND_MODE	DSPG_DISPLAY_MODE_COMMAND
#define VIDEO_MODE	DSPG_DISPLAY_MODE_VIDEO

void
copy_dpi_param_changes(dsih_dpi_video_t *from_param,
		       dsih_dpi_video_t *to_param);

void
copy_edpi_param_changes(dsih_cmd_mode_video_t *from_param,
			dsih_cmd_mode_video_t *to_param);

int
dsi_screen_init(struct mipi_dsi_dev *dev, int screen);

int
pre_video_mode(struct mipi_dsi_dev *dev, unsigned screen, int lanes);

int
pre_command_mode(struct mipi_dsi_dev *dev, unsigned screen, int lanes);

#endif	/* MIPI_DISPLAYS_H_ */
