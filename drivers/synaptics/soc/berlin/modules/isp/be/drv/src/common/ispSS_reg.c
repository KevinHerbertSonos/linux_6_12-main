// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "ispSS_api_dhub_config.h"
#include "ispSS_reg.h"
#include "drv_ispdhub_prv.h"
#include "drv_ispdhub.h"
#include "ispbe_api.h"
#include "drv_msg.h"

UNSG32 ispss_reg_base_virt_addr;
isp_module_ctx *gModeCtx;

#define CONFIG_DOLPHIN_ISPSS_REGAREA_BASE          0xf9100000
#define CONFIG_DOLPHIN_ISPSS_REGAREA_SIZE          0x100000
#define IRQ_dmaIntrTSB_0                         0x74
#define ISP_DHUB_AFFINITY                        0x1

SIGN32 ispss_create_iomap(void)
{
	gModeCtx = isp_dhub_get_mode_ctx();

	if (gModeCtx)
		return isp_dhub_mod_open(NULL, gModeCtx);
	else
		return -EINVAL;
}

UNSG32 ispss_destroy_iomap(void)
{
	gModeCtx = isp_dhub_get_mode_ctx();
	return isp_dhub_mod_close(NULL, gModeCtx);
}

UNSG64 ispss_devmem_phy_to_virt(UNSG32 uPhyAddr)
{
	if (ispss_reg_base_virt_addr &&
			(uPhyAddr >= CONFIG_DOLPHIN_ISPSS_REGAREA_BASE) &&
			(uPhyAddr <
			 (CONFIG_DOLPHIN_ISPSS_REGAREA_BASE + CONFIG_DOLPHIN_ISPSS_REGAREA_SIZE))) {
		return (uPhyAddr - CONFIG_DOLPHIN_ISPSS_REGAREA_BASE +
				ispss_reg_base_virt_addr);
	}
	pr_err("invalid physical address:%x, ispss_reg_base_virt_addr = %x\n",
			uPhyAddr, ispss_reg_base_virt_addr);
	return 0;
}
EXPORT_SYMBOL(ispss_devmem_phy_to_virt);

void ispss_write_phys_reg(UNSG32 uPhyAddr, UNSG32 val)
{
	if ((uPhyAddr >= CONFIG_DOLPHIN_ISPSS_REGAREA_BASE) &&
			(uPhyAddr <
			 (CONFIG_DOLPHIN_ISPSS_REGAREA_BASE + CONFIG_DOLPHIN_ISPSS_REGAREA_SIZE))) {
		isp_dhub_write_reg((uPhyAddr - CONFIG_DOLPHIN_ISPSS_REGAREA_BASE), val);
		return;
	}
	pr_err("invalid physical address:%x\n", uPhyAddr);
}
EXPORT_SYMBOL(ispss_write_phys_reg);

UNSG32 ispss_read_phys_reg(UNSG32 uPhyAddr)
{
	if ((uPhyAddr >= CONFIG_DOLPHIN_ISPSS_REGAREA_BASE) &&
			(uPhyAddr <
			 (CONFIG_DOLPHIN_ISPSS_REGAREA_BASE + CONFIG_DOLPHIN_ISPSS_REGAREA_SIZE))) {
		return isp_dhub_read_reg((uPhyAddr - CONFIG_DOLPHIN_ISPSS_REGAREA_BASE));
	}
	pr_err("invalid physical address:%x\n", uPhyAddr);
	return 0;
}
EXPORT_SYMBOL(ispss_read_phys_reg);

static UNSG32 ispss_ioctl(int request, void *msg)
{
	return isp_dhub_mod_func(gModeCtx, request, msg);
}

/* Interrupt Handling */
HRESULT ispss_dhub_init(void)
{
	ISP_DHUB_IOCTL dhub_ioctl = ISP_DHUB_IOCTL_DHUB_INIT;
	unsigned char isp_cfg_dhub_mod = ISP_MODULE_DHUB;
	HRESULT rc;

	//initialize Dhub here;DhubInitialization() in user space
	ISPSS_DhubInit(0);

	//Configure Dhub module in kernel
	rc = ispss_ioctl(ISP_MODULE_CONFIG, &isp_cfg_dhub_mod);

	//initialize Dhub in kernel
	rc = ispss_ioctl(dhub_ioctl, 0);

	return rc;
}

HRESULT ispss_enable_dhub_sem_intr(enum ISP_DHUBSEM_TSB dhub_semid, int enable)
{
	ISP_DHUB_IOCTL dhub_ioctl = enable ? ISP_DHUB_IOCTL_ENABLE_INT : ISP_DHUB_IOCTL_DISABLE_INT;
	HRESULT rc;

	pr_debug("%s: enable: %u %u\n", __func__, dhub_ioctl, ISP_DHUB_IOCTL_ENABLE_INT);
	rc = ispss_ioctl(dhub_ioctl, &dhub_semid);
	return rc;
}

HRESULT ispss_wait_dhub_sem_intr(CC_MSG_t *msg)
{
	HRESULT rc;

	rc = ispss_ioctl(ISP_DHUB_IOCTL_WAIT_FOR_INT, msg);
	return rc;
}
