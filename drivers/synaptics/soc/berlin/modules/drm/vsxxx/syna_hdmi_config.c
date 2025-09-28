// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Synaptics Incorporated
 *
 */

#include "syna_hdmi_config.h"

int syna_hdmi_tx_read_config(struct syna_conn_hdmi *syna_hdmi)
{
        struct device_node *hdmi_node;

        hdmi_node = of_find_compatible_node(NULL, NULL, "syna,drm-hdmi");
        if (!hdmi_node) {
                DRM_INFO("syna,drm-hdmi node missing, not setting hdmi connector\n");
                return -ENODEV;
        }

        of_property_read_u8_array(hdmi_node, "hdtx-core-config",
                                  &syna_hdmi->syna_hdmi_conf.hdmiTxConfig[0], HDMITX_CORE_CONFIG_ENTRIES);
        of_property_read_u8_array(hdmi_node, "hdtx-supported-formats",
                                  &syna_hdmi->count_format_supported, 1);

        if ((syna_hdmi->count_format_supported < 1) ||
            (syna_hdmi->count_format_supported > MAX_NUM_RESS)) {
                DRM_ERROR("invalid format support passed in via dts:%d\n",
                          syna_hdmi->count_format_supported);
		of_node_put(hdmi_node);
                return -EINVAL;
        }

        syna_hdmi->supported_formats = kzalloc(syna_hdmi->count_format_supported, GFP_KERNEL);
        if (!syna_hdmi->supported_formats) {
                DRM_ERROR("error allocating memory for storing formats\n");
		of_node_put(hdmi_node);
                return -ENOMEM;
        }

        of_property_read_u8_array(hdmi_node, "hdtx-supported-formats",
                                        syna_hdmi->supported_formats, syna_hdmi->count_format_supported);

        of_node_put(hdmi_node);
        return 0;
}
