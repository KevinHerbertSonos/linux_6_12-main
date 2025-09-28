#!/bin/bash

# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2025 Synaptics Incorporated

source build/header.rc
source build/install.rc

mod_dir=${CONFIG_SYNA_SDK_PATH}/linux_5_15/modules/drivers/syna-gpio-mux
dst_dir=${CONFIG_SYNA_SDK_REL_PATH}/linux_5_15/modules/drivers/syna-gpio-mux

# install the sources of the kernel module
mkdir -p ${dst_dir}
rsync -az --exclude=release.sh ${mod_dir} ${dst_dir}
