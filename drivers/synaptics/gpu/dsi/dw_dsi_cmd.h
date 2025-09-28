#ifndef __DW_DSI_CMD_H
#define __DW_DSI_CMD_H

#include <dt-bindings/dspg/mipi_dsih.h>
#include "dw_drm_dsi.h"

struct dsi_cmd {
	uint8_t dcs_cmd;
	unsigned num_params;
	uint8_t *params;
};

int
dw_dsi_send(struct dw_dsi *dev, struct dsi_cmd *cmd);

#endif	/* __DW_DSI_CMD_H */
