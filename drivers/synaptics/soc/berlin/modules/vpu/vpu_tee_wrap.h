#ifndef __VPU_TEE_WRAP_H
#define __VPU_TEE_WRAP_H

#if IS_ENABLED(CONFIG_OPTEE)
#include <linux/tee_drv.h>
typedef struct tee_context		VPU_TEEC_Context;
typedef int		                VPU_TEEC_Session;
#else
#include "tee_client_api.h"
typedef struct TEEC_Context		VPU_TEEC_Context;
typedef struct TEEC_Session		VPU_TEEC_Session;
#endif

#endif //__VPU_TEE_WRAP_H