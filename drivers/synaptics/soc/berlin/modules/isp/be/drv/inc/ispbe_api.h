/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ISPBE_API_H_
#define _ISPBE_API_H_

#include <linux/semaphore.h>
#include "ispBE_module_common.h"

#define ISPBE_CLIENT_MAX 10

enum ISPSS_CLK_STATE {
	ISPSS_CLK_STATE_DISABLE,
	ISPSS_CLK_STATE_ENABLE
};

enum ISPBE_MODULES {
	ISPBE_MODULE_MIN = 0,
	ISPBE_MODULE_TILER = 0,
	ISPBE_MODULE_DEWARP,
	ISPBE_MODULE_DNSCL3,
	ISPBE_MODULE_ROT,
	ISPBE_MODULE_MAX
};

enum ISPMISC_MODULES {
	ISPMISC_MODULE_MIN = 0,
	ISPMISC_MODULE_BCM = ISPMISC_MODULE_MIN,
	ISPMISC_MODULE_DHUB_TSB,
	ISPMISC_MODULE_DHUB_FWR,
	ISPMISC_MODULE_MAX
};

enum ISP_DHUBSEM_TSB {
	ISP_DHUBSEM_TSB_MIN = 0x10,
	ISP_DHUBSEM_TSB_ispIntr = 0x10,
	ISP_DHUBSEM_TSB_miIntr,
	ISP_DHUBSEM_TSB_mipi0Intr,
	ISP_DHUBSEM_TSB_mipi1Intr,
	ISP_DHUBSEM_TSB_Fdet_intr0,
	ISP_DHUBSEM_TSB_Fdet_intr1,
	ISP_DHUBSEM_TSB_Scldn_intr0,
	ISP_DHUBSEM_TSB_Scldn_intr1,
	ISP_DHUBSEM_TSB_Scldn_intr2,
	ISP_DHUBSEM_TSB_dwrp_intr,
	ISP_DHUBSEM_TSB_rot_intr,
	ISP_DHUBSEM_TSB_tile_intr,
	ISP_DHUBSEM_TSB_bcmInvalidIntr,
	ISP_DHUBSEM_TSB_usr_inr13,
	ISP_DHUBSEM_TSB_usr_inr14,
	ISP_DHUBSEM_TSB_usr_inr15,
	ISP_DHUBSEM_TSB_MAX
};


/* If you have any miscellaneous APIs in your driver
 * create a command and add it here
 */
enum ISP_BE_IOCTL_CMD {
	ISP_BE_IOCTL_CMD_MIN = 0,
	ISP_BE_IOCTL_CMD_CONFIG,
	ISP_BE_IOCTL_CMD_REG_DUMP,
	ISP_BE_IOCTL_CMD_TILE_RESTART,
	ISP_BE_IOCTL_CMD_TILE_FIFO_STATUS,
	ISP_BE_IOCTL_CMD_READ_SCL_COEFF,
	ISP_BE_IOCTL_CMD_WRITE_SCL_COEFF,
	ISP_BE_IOCTL_CMD_GET_ALL_FRAMEQ_WAIT_CNT,
	ISP_BE_IOCTL_CMD_GET_FB_SIZE,
	ISPBE_IOCTL_CMD_MAX
};

struct ISPBE_GET_FRAMEQ_WAIT_CNT {
	int iClientId;
	int noOfFramesWaitingInputQ;
	int noOfFramesWaitingProcessQ;
	int noOfFramesWaitingOutputQ;
};

typedef void (*ispbe_client_cb_t)(struct ISP_BE_RQST_MSG *rqst_msg);

typedef int(*ispbe_module_init)(void);
typedef int(*ispbe_module_destroy)(void);
typedef int(*ispbe_module_open)(int *clientID, int priority);
typedef int(*ispbe_module_close)(int clientID);
typedef int(*ispbe_module_ioctl)(enum ISP_BE_IOCTL_CMD ioctl_cmd, void *param);
typedef int(*ispbe_module_ProcessISR)(UINT32 intrNum, void *pArgs);
typedef int(*ispbe_module_PushRequest)(int iClientId, struct ISP_BE_RQST_MSG *pstRqstMsg);
typedef int(*ispbe_module_PopRequest)(struct ISP_BE_RQST_MSG **pstRqstMsg);
typedef int(*ispbe_module_ReleaseRequest)(int iClientId, struct ISP_BE_RQST_MSG **pstRqstMsg);
typedef int(*ispbe_module_GetNoOfFramesWaiting)(int iClientId, UINT32 *puiFramesWaiting);

struct ISPBE_DRIVER_OPS {
	ispbe_module_init module_init;
	ispbe_module_destroy module_destroy;
	ispbe_module_open module_open;
	ispbe_module_close module_close;
	ispbe_module_ioctl module_ioctl;
	ispbe_module_ProcessISR module_ProcessISR;
	ispbe_module_PushRequest module_PushRequest;
	ispbe_module_PopRequest module_PopRequest;
	ispbe_module_ReleaseRequest module_ReleaseRequest;
	ispbe_module_GetNoOfFramesWaiting module_GetNoOfFramesWaiting;
};

/* App callback */
typedef void (*ispbe_cb_t)(int intr_num, void *param);

struct ISPBE_INTR_CB {
	UINT intrStatus;
	void *user_param;
};

/* Application Callbacks */
struct ISPBE_DRIVER_CB {
	ispbe_cb_t pre_IntrCB; /* Callback to application */
	ispbe_cb_t post_IntrCB; /* Callback to application */
	void *pre_param, *post_param;
};

struct ISPBE_CA_DRV_CTX {
	struct ISPBE_DRIVER_OPS fops;
	enum ISP_DHUBSEM_TSB sem_id;
	ispbe_client_cb_t client_cb[ISPBE_CLIENT_MAX];
};

struct ISPBE_CA_CLKGT_CTX {
	UINT ispbe_clkgate_module_refcnt[ISPBE_MODULE_MAX];
	UINT ispbe_clkgate_shared_refcnt;
	struct mutex ispbe_clkgate_mutex;
};

struct ISPBE_CA_CTX {
	UINT is_created;
	UINT pollingTaskExitFlag, isrTaskExitFlag, isrCbTaskExitFlag, processBypassReqExitFlag;
	struct task_struct *ispbe_polling_isr_task;
	struct task_struct *ispbe_isr_handler_task;
	struct task_struct *ispbe_isr_cb_task;
	struct task_struct *ispbe_process_bypassreq_task;
	struct ISPBE_DRIVER_CB intr_cb[ISP_DHUBSEM_TSB_MAX];
	struct ISPBE_CA_DRV_CTX drv_ctx[ISPBE_MODULE_MAX];
	struct ISPBE_CA_CLKGT_CTX clkgate_ctx;
};

INT ISPBE_CA_Initialize(void);
INT ISPBE_CA_DeInitialize(void);
INT ISPSS_CA_RegisterInterruptCallback(enum ISP_DHUBSEM_TSB DhubSem_Id,
			struct ISPBE_DRIVER_CB *intr_cb);
INT ISPSS_CA_UnRegisterInterruptCallback(enum ISP_DHUBSEM_TSB DhubSem_Id);
INT ISPBE_MODULE_Init(enum ISPBE_MODULES module, struct ISPBE_CA_DRV_CTX module_ctx,
		ispbe_client_cb_t client_cb);
INT ISPBE_MODULE_Destroy(enum ISPBE_MODULES module);
INT ISPBE_MODULE_Open(enum ISPBE_MODULES module, INT *clientID,
		INT priority, ispbe_client_cb_t client_cb);
INT ISPBE_MODULE_Close(enum ISPBE_MODULES module, INT clientID);
INT ISPBE_MODULE_Ioctl(enum ISPBE_MODULES module, enum ISP_BE_IOCTL_CMD ioctl_cmd, void *param);
INT ISPBE_MODULE_PushRequest(enum ISPBE_MODULES module, INT clientID,
			struct ISP_BE_RQST_MSG *pstRqstMsg);
INT ISPBE_MODULE_PopRequest(enum ISPBE_MODULES module, INT clientID,
			struct ISP_BE_RQST_MSG **pstRqstMsg);
INT ISPBE_MODULE_ReleaseRequest(enum ISPBE_MODULES module,
		INT iClientId, struct ISP_BE_RQST_MSG **pstRqstMsg);
INT ISPBE_MODULE_GetNoOfFramesWaiting(enum ISPBE_MODULES module,
		INT iClientId, UINT32 *puiFramesWaiting);
void ISPSS_CA_ClockGateSharedResources(BOOL state);

#endif
