// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/tee_drv.h>

#include "tee_ca_vpp.h"
#include "tee_ca_common.h"
#include "vpp_mem.h"
#include "vbuf.h"
#include "vpp_vbuf.h"


#define bTST(x, b) (((x) >> (b)) & 1)

#define DEFAULT_SESSION_INDEX 0

#undef VPP_CA_ENABLE_DIAG_PRT
//#define VPP_CA_ENABLE_DIAG_PRT
#ifdef VPP_CA_ENABLE_DIAG_PRT
#define VPP_CA_DBG_PRINT(...) pr_info(__VA_ARGS__)
#else
#define VPP_CA_DBG_PRINT(fmt, ...) do {} while (0)
#endif

#define VPP_CA_UUID_TO_STR(uuid) \
	((uuid == TA_UUID_VPP) ? "vpp.ta" : "fastlogo.ta")

typedef struct __VPP_CA_CONTEXT__ {
	ENUM_TA_UUID_TYPE uuidType;
	int initialized;
	struct tee_context *context;
} VPP_CA_CONTEXT;

typedef struct __VPP_CA_SESSION__ {
	int initialized;
	u32 session;
	struct tee_shm *Shm;
	struct mutex shm_mutex;
} VPP_CA_SESSION;

static VPP_CA_CONTEXT g_vppCaContext;
static VPP_CA_SESSION TAVPPInstance[MAX_TAVPP_INSTANCE_NUM];

static const uuid_t vpp_ta_uuid = UUID_INIT(0x1316a183, 0x894d, 0x43fe,
				0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf1);
static const uuid_t fastlogo_ta_uuid = UUID_INIT(0x1316a183, 0x894d, 0x43fe,
				0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x03, 0xf5);

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	return (ver->impl_id == TEE_IMPL_ID_OPTEE);
}

static void VPP_CA_getVppTaUUID(ENUM_TA_UUID_TYPE uuidType, uuid_t *pUUID)
{
	//Get the mode of bootup recovery/normal and select the ta accordingly
	g_vppCaContext.uuidType = uuidType;
	pr_err("%s:%d: loading recovery ta:%x(%s)\n", __func__, __LINE__,
		g_vppCaContext.uuidType, VPP_CA_UUID_TO_STR(g_vppCaContext.uuidType));
	memcpy(pUUID, (g_vppCaContext.uuidType == TA_UUID_VPP) ?
		&vpp_ta_uuid : &fastlogo_ta_uuid, sizeof(uuid_t));
}

static int VPP_CA_alloc(unsigned int len, void **handle)
{
	struct tee_shm *shm;

	shm = tee_shm_alloc_kernel_buf(g_vppCaContext.context, len);
	if (IS_ERR(shm)) {
		pr_err("fail to allocate share memory: size %x\n", len);
		kfree(shm);
		return -ENOMEM;
	}

	*handle = shm;
	return 0;
}

static void VPP_CA_free(void *handle)
{
	if (handle)
		tee_shm_free(handle);
}

static int VPP_CA_RegisterMemory(VPP_MEM *vppMem)
{
	int fd;
	struct tee_shm *shm;

	fd = dma_buf_fd(vppMem->handle, O_RDWR);
	if (fd < 0) {
		pr_err("failed to get dma-buf fd (err=%d/%x)\n", fd, fd);
		return -ENOMEM;
	}

	shm = tee_shm_register_fd(g_vppCaContext.context, fd);
	if (IS_ERR(shm)) {
		pr_err("failed to register fd %x\n", fd);
		return -ENOMEM;
	}

	vppMem->teeShm = shm;

	return 0;
}

int VPP_CA_Initialize(ENUM_TA_UUID_TYPE uuidType, struct device *dev)
{
	struct tee_ioctl_open_session_arg sess_arg;
	int index;
	int ret;
	uuid_t TAVPP_CA_UUID;

	if (g_vppCaContext.initialized)
		return 0;

	g_vppCaContext.initialized = true;

	VPP_CA_getVppTaUUID(uuidType, &TAVPP_CA_UUID);
	/* ========================================================================
	 *  [1] Connect to TEE
	 * ========================================================================
	 */
	g_vppCaContext.context = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(g_vppCaContext.context)) {
		pr_err("fail to initialize optee context\n");
		ret = -ENODEV;
		goto cleanup1;
	}

	/* ========================================================================
	 *  [2] Load TA, If required during ampless boot.
	 * ========================================================================
	 */

	/* ========================================================================
	 * [3] Open session with TEE application
	 * ========================================================================
	 */
	for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++) {

		memset(&sess_arg, 0, sizeof(sess_arg));
		memcpy(sess_arg.uuid, TAVPP_CA_UUID.b, TEE_IOCTL_UUID_LEN);
		sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
		sess_arg.num_params = 0;

		ret = tee_client_open_session(g_vppCaContext.context, &sess_arg, NULL);
		if (ret < 0 || sess_arg.ret != 0) {
			pr_err("tee_client_open_session failed with code 0x%x origin 0x%x",
				ret, sess_arg.ret);
			ret = -EINVAL;
			goto cleanup2;
		}
		TAVPPInstance[index].session = sess_arg.session;

		ret = VPP_CA_alloc(TAVPP_PASS_SHMSIZE, (void **)&(TAVPPInstance[index].Shm));
		if (ret) {
			pr_err("fail to alloc shm for param passing %x", TAVPP_PASS_SHMSIZE);
			goto cleanup3;
		}

		mutex_init(&(TAVPPInstance[index].shm_mutex));
		TAVPPInstance[index].initialized = true;
	}

	return 0;
cleanup3:
	for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++) {
		if (TAVPPInstance[index].initialized)
			VPP_CA_free(TAVPPInstance[index].Shm);
	}
cleanup2:
	for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++) {
		if (TAVPPInstance[index].initialized)
			tee_client_close_session(g_vppCaContext.context,
						TAVPPInstance[index].session);
	}
	tee_client_close_context(g_vppCaContext.context);
cleanup1:
	return ret;
}

void VPP_CA_Finalize(void)
{
	int index;

	if (!g_vppCaContext.initialized)
		return;

	g_vppCaContext.initialized = 0;

	for (index = 0; index < MAX_TAVPP_INSTANCE_NUM; index++) {
		if (TAVPPInstance[index].initialized) {
			tee_client_close_session(g_vppCaContext.context,
						TAVPPInstance[index].session);
			VPP_CA_free(TAVPPInstance[index].Shm);
			memset(&TAVPPInstance[index], 0, sizeof(TAVPPInstance[index]));
			TAVPPInstance[index].initialized = false;
		}
	}
	tee_client_close_context(g_vppCaContext.context);
}

static inline int VPP_CA_GetInstanceID(void)
{
	int InstanceID = TAVPP_API_INSTANCE;

	return InstanceID;
}

static int InvokeCommandHelper(int session_index, u32 *session_id,
	VPP_CMD_ID commandID, struct tee_param *param, int nParam)
{
	int result = 0;
	int cmdID = commandID;
	int instID = session_index;

	commandID = CREATE_CMD_ID(cmdID, instID);
	VPP_CA_DBG_PRINT("%s:%d: session_index:%d, pSession:%x commandID:%d, pOperation:%p, initialsized:%d\n",
		__func__, __LINE__, session_index, *session_id, commandID, param, g_vppCaContext.initialized);

	if (g_vppCaContext.initialized) {
		struct tee_ioctl_invoke_arg arg;

		memset(&arg, 0, sizeof(arg));
		arg.func = commandID;
		arg.num_params = nParam;
		arg.session = *session_id;

		result = tee_client_invoke_func(g_vppCaContext.context, &arg, param);
		if (result < 0 || arg.ret != 0) {
			pr_err("fail to invoke command(%x) to %s : ret = %x,"
					"TEE err: 0x%x\n", commandID,
					VPP_CA_UUID_TO_STR(g_vppCaContext.uuidType),
					result, arg.ret);
			result = -EINVAL;
		}
	}

	return result;
}

int VPP_CA_InitVPPS(UINT32 vpp_addr, UINT32 ta_heapHandle)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = vpp_addr;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	param[0].u.value.b = ta_heapHandle;
#endif
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_INITIATE_VPPS, param, 2);
	if (!ret)
		ret = param[1].u.value.a;

	return ret;
}

int VPP_CA_GetCPCBOutputResolution(int cpcbID, int *pResID)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = cpcbID;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;
	param[1].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_GETCPCBOUTRES, param, 2);
	if (!ret) {
		ret = param[1].u.value.a;
		*pResID = param[1].u.value.b;
	}

	return ret;
}

int VPP_CA_GetResDescription(void *pOutBuffer, VPP_SHM_ID shmCmdId,
						unsigned int sOutBufferSize, unsigned int ResId)
{
	int ret;
	struct tee_param param[4];
	struct tee_shm *pShm;
	int index;
	u32 *pSession;

	if (!g_vppCaContext.initialized)
		return 0;

	if (!pOutBuffer)
		return MV_VPP_EBADPARAM;

	pShm = TAVPPInstance[index].Shm;
	if (pShm && TAVPP_PASS_SHMSIZE < sOutBufferSize) {
		pr_err("%s:%d: pShm->size %x, sOutBufferSize %d\n",
			__func__, __LINE__, TAVPP_PASS_SHMSIZE, sOutBufferSize);
		return -EINVAL;
	}

	mutex_lock(&(TAVPPInstance[index].shm_mutex));

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = shmCmdId;
	param[1].u.memref.shm = pShm;
	param[1].u.memref.size = sOutBufferSize;
	param[1].u.memref.shm_offs = 0;
	param[2].u.value.a = ResId;

	ret = InvokeCommandHelper(index, pSession, VPP_PASSSHM, param, 3);
	if (!ret)
		ret = param[2].u.value.a;

	if (pOutBuffer)
		memcpy(pOutBuffer, pShm->kaddr, sOutBufferSize);

	mutex_unlock(&(TAVPPInstance[index].shm_mutex));

	return ret;
}

int VPP_CA_GetCurrentHDCPVersion(int *pHDCPVersion)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = 0;
	/* clear result */
	param[0].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_GETCURHDCPVERSION, param, 1);
	if (!ret) {
		ret = param[0].u.value.b;
		*pHDCPVersion = (param[0].u.value.a & 0xFF);
	}

	return ret;
}

int VPP_CA_PassVbufInfo_Phy(int is_vpp_ta, void *Vbuf, unsigned int VbufSize,
						 void *Clut, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
	return VPP_CA_PassVbufInfo(is_vpp_ta, Vbuf, VbufSize, Clut, ClutSize, PlaneID, ClutValid, ShmID);
}

int VPP_CA_PassVbufInfo(int is_vpp_ta, void *Vbuf, unsigned int VbufSize,
						 void *Clut_phyAddr, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;
	VBUF_INFO *pVppDesc = Vbuf;
	VPP_MEM *vppMem = pVppDesc->hShm_vbuf;

	if (!vppMem->teeShm) {
		//Register the VBUF_INFO Only once
		ret = VPP_CA_RegisterMemory(vppMem);
	}

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = ShmID;
	param[1].u.memref.shm = vppMem->teeShm;
	param[1].u.memref.size = vppMem->size;
	param[1].u.memref.shm_offs = 0;
	param[3].u.value.a = PlaneID;

	/* clear result */
	param[3].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_PASSSHM, param, 4);
	if (!ret)
		ret = param[3].u.value.b;

    return ret;
}

int VPP_CA_Init(VPP_INIT_PARM *vpp_init_parm)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;
	int k;
	UINT32 vpp_addr;
	UINT32 ta_heapHandle;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;

	param[0].u.value.a = vpp_init_parm->iHDMIEnable;
	param[0].u.value.b = vpp_init_parm->iVdacEnable;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	param[2].u.value.a = vpp_init_parm->uiShmPA;
	param[2].u.value.b = vpp_init_parm->uiShmSize;
#endif
	/* clear result */
	param[1].u.value.a = 0x0;
	param[1].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_INIT, param, 3);
	if (!ret) {
		ret = param[1].u.value.b;
		return ret;
	}

	vpp_init_parm->g_vpp = param[1].u.value.b;
	vpp_addr = vpp_init_parm->g_vpp;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	vpp_init_parm->gMemhandle = param[0].u.value.b;
	ta_heapHandle = vpp_init_parm->gMemhandle;
#endif

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[1].u.value.a = 0xdeadbeef;
	param[0].u.value.a = vpp_addr;
#if defined(VPP_ENABLE_INTERNAL_MEM_MGR)
	param[0].u.value.b = ta_heapHandle;
#endif

	for (k = TAVPP_API_INSTANCE; k < MAX_TAVPP_INSTANCE_NUM; k++) {
		pSession = &(TAVPPInstance[k].session);
		ret = InvokeCommandHelper(index, pSession, VPP_INITIATE_VPPS, param, 2);
		if (!ret) {
			ret = param[1].u.value.a;
			break;
		}
	}

	return ret;
}

int VPP_CA_Create(void)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	/* clear result */
	param[0].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_CREATE, param, 1);
	if (!ret) {
		ret = param[0].u.value.a;
	}

	return ret;
}


int VPP_CA_Reset(void)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	/* clear result */
	param[0].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_RESET, param, 1);
	if (!ret) {
		ret = param[0].u.value.a;
	}

	return ret;
}


int VPP_CA_ObjConfig(const int *pvinport_cfg, const int *pdv_cfg,
		const int *pzorder_cfg, const int *pvoutport_cfg, const int *pfeature_cfg)
{
	int Ret;
	static UINT8 cfg_mem[256];
	int plane_size = sizeof(INT32)* (MAX_NUM_PLANES_ALL);
	int vout_size = sizeof(INT32)* (MAX_NUM_VOUTS);
	int feature_size = sizeof(INT32)* (MAX_NUM_FEATURE_CFG);

	memcpy(&cfg_mem[0], pvinport_cfg, plane_size);
	memcpy(&cfg_mem[plane_size], pdv_cfg, plane_size);
	memcpy(&cfg_mem[(plane_size*2)], pzorder_cfg, plane_size);
	memcpy(&cfg_mem[(plane_size*3)], pvoutport_cfg, vout_size);
	memcpy(&cfg_mem[(plane_size*3)+vout_size], pfeature_cfg, feature_size);

	Ret = VPP_CA_PassShm_InBuffer(&cfg_mem[0], VPP_OBJCONFIG, 256);

	return Ret;
}

int VPP_CA_Config(void)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	/* clear result */
	param[0].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_CONFIG, param, 1);
	if (!ret) {
		ret = param[0].u.value.a;
	}

	return ret;
}

int VPP_CA_IsrHandler(unsigned int MsgId, unsigned int IntSts)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = MsgId;
	param[0].u.value.b = IntSts;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_HANDLEINT, param, 2);
	if (!ret) {
		ret = param[1].u.value.a;
	}

	return ret;
}


int VPP_CA_SetOutRes(int CpcbId, int ResId, int BitDepth)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = CpcbId;
	param[0].u.value.b = ResId;
	param[1].u.value.a = BitDepth;
	/* clear result */
	param[2].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_SETRES, param, 3);
	if (!ret) {
		ret = param[2].u.value.a;
	}

	return ret;
}

int VPP_CA_OpenDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = PlaneId;
	param[0].u.value.b = WinX;
	param[1].u.value.a = WinY;
	param[1].u.value.b = WinW;
	param[2].u.value.a = WinH;
	param[2].u.value.b = BgClr;
	param[3].u.value.a = Alpha;
	/* clear result */
	param[3].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_OPENDISPWIN, param, 4);
	if (!ret) {
		ret = param[3].u.value.b;
	}

	return ret;
}

int VPP_CA_RecycleFrame(int PlaneId)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = PlaneId;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_RECYCLEFRAME, param, 2);
	if (!ret) {
		ret = param[1].u.value.a;
	}

	return ret;
}

int VPP_CA_SetDispMode(int PlaneId, int Mode)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = PlaneId;
	param[0].u.value.b = Mode;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_SETDISPMODE, param, 2);
	if (!ret) {
		ret = param[1].u.value.a;
	}

	return ret;
}

int VPP_CA_HdmiSetVidFmt(int ColorFmt, int BitDepth, int PixelRep)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = ColorFmt;
	param[0].u.value.b = BitDepth;
	param[1].u.value.a = PixelRep;
	/* clear result */
	param[2].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_HDMISETVIDFMT, param, 3);
	if (!ret) {
		ret = param[2].u.value.a;
	}

	return ret;
}


int VPP_CA_SetRefWin(int PlaneId, int WinX, int WinY, int WinW, int WinH)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = PlaneId;
	param[0].u.value.b = WinX;
	param[1].u.value.a = WinY;
	param[1].u.value.b = WinW;
	param[2].u.value.a = WinH;
	/* clear result */
	param[3].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_SETREFWIN, param, 4);
	if (!ret) {
		ret = param[3].u.value.a;
	}

	return ret;
}

int VPP_CA_SetRefWinFromISR(int PlaneId, int WinX, int WinY, int WinW, int WinH)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = PlaneId;
	param[0].u.value.b = WinX;
	param[1].u.value.a = WinY;
	param[1].u.value.b = WinW;
	param[2].u.value.a = WinH;
	/* clear result */
	param[3].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_SETREFWINFROMISR, param, 4);
	if (!ret) {
		ret = param[3].u.value.a;
	}

	return ret;
}

int VPP_CA_ChangeDispWin(int PlaneId, int WinX, int WinY, int WinW, int WinH, int BgClr, int Alpha, ENUM_GLOBAL_ALPHA_FLAG globalAlphaFlag, bool isFromISR)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = 0;
	VPP_ATTR_PARAM_SET_GLOBALPHA(param[0].u.value.a, globalAlphaFlag);
	VPP_ATTR_PARAM_SET_PLANEID(param[0].u.value.a, PlaneId);
	param[0].u.value.b = WinX;
	param[1].u.value.a = WinY;
	param[1].u.value.b = WinW;
	param[2].u.value.a = WinH;
	param[2].u.value.b = BgClr;
	param[3].u.value.a = Alpha;
	/* clear result */
	param[3].u.value.b = 0xdeadbeef;

	if (isFromISR)
		ret = InvokeCommandHelper(index, pSession, VPP_CHANGEDISPWINFROMISR, param, 4);
	else
		ret = InvokeCommandHelper(index, pSession, VPP_CHANGEDISPWIN, param, 4);
	if (!ret) {
		ret = param[3].u.value.b;
	}

	return ret;
}


int VPP_CA_SetPlaneMute(int PlaneId, int mute)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = PlaneId;
	param[0].u.value.b = mute;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_SETPLANEMUTE, param, 2);
	if (!ret) {
		ret = param[1].u.value.a;
	}

	return ret;
}

int VPP_CA_Suspend(int enable)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;
	VPP_CMD_ID cmd;

	cmd = enable ? VPP_SUSPEND : VPP_RESUME;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	/* clear result */
	param[0].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, cmd, param, 1);
	if (!ret) {
		ret = param[0].u.value.a;
	}

	return ret;
}

int VPP_CA_SetHdmiTxControl(int enable)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = enable;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_HDMISETTXCTRL, param, 2);
	if (!ret) {
		ret = param[1].u.value.a;
	}

	return ret;
}

int VPP_CA_stop(void)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	/* clear result */
	param[0].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_STOP, param, 1);
	if (!ret) {
		ret = param[0].u.value.a;
	}

	return ret;
}

int VPP_CA_Destroy(void)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	/* clear result */
	param[0].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_DESTROY, param, 1);
	if (!ret) {
		ret = param[0].u.value.a;
	}

	return ret;
}

#ifdef VPP_CA_ENABLE_PASS_VBUFINFO_BY_PARAMETER
int VPP_CA_PassVbufInfoPar(unsigned int *Vbuf, unsigned int VbufSize,
						 unsigned int *Clut, unsigned int ClutSize,
						 int PlaneID, int ClutValid, VPP_SHM_ID ShmID)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;
	int index, i, flag;
	VBUF_INFO *pVBufInfo = (VBUF_INFO *) Vbuf;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	for (i = 0; i < 5; i++) {
		flag = 0;
		switch (i) {
		case 0:
			param[1].u.value.a = (unsigned int)((intptr_t)pVBufInfo->m_pbuf_start);
			param[1].u.value.b = pVBufInfo->m_bytes_per_pixel;
			param[2].u.value.a = pVBufInfo->m_bits_per_pixel;
			param[2].u.value.b = pVBufInfo->m_srcfmt;
			param[3].u.value.a = pVBufInfo->m_order;
			flag = 1;
			break;
		case 1:
			param[1].u.value.a = pVBufInfo->m_content_width;
			param[1].u.value.b = pVBufInfo->m_content_height;
			param[2].u.value.a = pVBufInfo->m_buf_stride;
			param[2].u.value.b = pVBufInfo->m_buf_pbuf_start_UV;
			param[3].u.value.a = pVBufInfo->m_buf_size;
			flag = 1;
			break;
		case 2:
			param[1].u.value.a = pVBufInfo->m_allocate_type;
			param[1].u.value.b = pVBufInfo->m_buf_type;
			param[2].u.value.a = pVBufInfo->m_buf_use_state;
			param[2].u.value.b = pVBufInfo->m_flags;
			param[3].u.value.a = pVBufInfo->m_is_frame_seq;
			flag = 1;
			break;
		case 3:
			param[1].u.value.a = pVBufInfo->m_frame_rate_num;
			param[1].u.value.b = pVBufInfo->m_frame_rate_den;
			param[2].u.value.a = pVBufInfo->m_active_width;
			param[2].u.value.b = pVBufInfo->m_active_height;
			param[3].u.value.a = pVBufInfo->m_active_left;
			flag = 1;
			break;
		case 4:
			param[1].u.value.a = pVBufInfo->m_active_top;
			param[1].u.value.b = pVBufInfo->m_content_offset;
			param[2].u.value.a = pVBufInfo->m_is_progressive_pic;
			param[2].u.value.b = pVBufInfo->m_hDesc;
			flag = 1;
			break;
		}

		if (flag) {
			param[0].u.value.a = (ShmID | (PlaneID << 8));
			param[0].u.value.b = i;

			/* clear result */
			param[3].u.value.b = 0xdeadbeef;

			ret = InvokeCommandHelper(index, pSession, VPP_PASSPAR, param, 4);
			if (!ret) {
				ret = param[3].u.value.b;
			}

		}
	}

	return ret;
}
#endif //VPP_CA_ENABLE_PASS_VBUFINFO_BY_PARAMETER


int VPP_CA_SemOper(int cmd_id, int sem_id, int *pParam)
{
	int ret = 0;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = cmd_id;
	param[0].u.value.b = sem_id;
	if (pParam)
		param[1].u.value.a = *pParam;
	/* clear result */
	param[1].u.value.b = 0xdeadbeef;

#ifdef VPP_CA_ENABLE_CMD_VPP_SEMOPER
	ret = InvokeCommandHelper(index, pSession, VPP_SEMOPER, param, 2);
	if (!ret) {
		ret = param[1].u.value.b;
		*pParam = operation.params[1].value.a;
	}
#endif

	return ret;
}

int VPP_CA_EnableHdmiAudioFmt(int enable)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	param[0].u.value.a = enable;
	/* clear result */
	param[0].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_HDMIENABLEAUDIOFMT, param, 1);
	if (!ret) {
		ret = param[0].u.value.b;
	}

	return ret;

}

static int VPP_CA_PassShm(struct tee_shm *pShm, unsigned int shmCmdId, unsigned int sBufferSize)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	if (pShm && TAVPP_PASS_SHMSIZE < sBufferSize) {
		pr_err("%s:%d: pShm->size %x, sOutBufferSize %d\n",
			__func__, __LINE__, TAVPP_PASS_SHMSIZE, sBufferSize);
		return -EINVAL;
	}

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = shmCmdId;
	param[1].u.memref.shm = pShm;
	param[1].u.memref.size = sBufferSize;
	param[1].u.memref.shm_offs = 0;

	/* clear result */
	param[2].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_PASSSHM, param, 3);
	if (!ret)
		ret = param[2].u.value.a;

	return ret;
}

int VPP_CA_PassShm_InBuffer(void *pBuffer, unsigned int shmCmdId, unsigned int sInBufferSize)
{
	int index;
	int Ret;
	struct tee_shm *pShm;

	index = VPP_CA_GetInstanceID();

	pShm = (TAVPPInstance[index].Shm);
	if (pShm && pShm->size < sInBufferSize) {
		pr_err("%s:%d: pShm->size %zu, sInBufferSize %d\n",
			__func__, __LINE__, pShm->size, sInBufferSize);
		return -EINVAL;
	}

	mutex_lock(&(TAVPPInstance[index].shm_mutex));
	memcpy(pShm->kaddr, pBuffer, sInBufferSize);
	Ret = VPP_CA_PassShm(pShm, shmCmdId, sInBufferSize);

	mutex_unlock(&(TAVPPInstance[index].shm_mutex));
	return Ret;
}

int VPP_CA_PassShm_OutBuffer(void *pOutBuffer, unsigned int shmCmdId, unsigned int sOutBufferSize)
{
	int index;
	int Ret;
	struct tee_shm *pShm;

	index = VPP_CA_GetInstanceID();

	pShm = (TAVPPInstance[index].Shm);
	if (pShm && pShm->size < sOutBufferSize) {
		pr_err("%s:%d: pShm->size %zu, sOutBufferSize %d\n",
			__func__, __LINE__, pShm->size, sOutBufferSize);
		return -EINVAL;
	}

	mutex_lock(&(TAVPPInstance[index].shm_mutex));

	Ret = VPP_CA_PassShm(pShm, shmCmdId, sOutBufferSize);
	memcpy(pOutBuffer, pShm->kaddr, sOutBufferSize);

	mutex_unlock(&(TAVPPInstance[index].shm_mutex));

	return Ret;
}

int VPP_CA_PassShm_InOutBuffer(void *pInBuffer, void *pOutBuffer,
				VPP_SHM_ID shmCmdId, UINT32 sInBufferSize, UINT32 sOutBufferSize)
{
	int index;
	int Ret;
	struct tee_shm *pShm;

	if (!pOutBuffer)
		return MV_VPP_EBADPARAM;

	index = VPP_CA_GetInstanceID();

	pShm = (TAVPPInstance[index].Shm);
	if (pShm && pShm->size < sInBufferSize) {
		pr_err("%s:%d: pShm->size %zu, sInBufferSize %d\n",
			__func__, __LINE__, pShm->size, sInBufferSize);
		return -EINVAL;
	}

	mutex_lock(&(TAVPPInstance[index].shm_mutex));

	if (pInBuffer)
		memcpy(pShm->kaddr, pInBuffer, sInBufferSize);
	else
		memset(pShm->kaddr, 0, sInBufferSize);

	Ret = VPP_CA_PassShm(pShm, shmCmdId, sOutBufferSize);
	if (pOutBuffer)
		memcpy(pOutBuffer, pShm->kaddr, sOutBufferSize);

	mutex_unlock(&(TAVPPInstance[index].shm_mutex));

	return Ret;
}

int VPP_CA_GetCPCBOutputPixelClock(int resID,  int *pixel_clock)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = resID;
	/* clear result */
	param[1].u.value.a = 0xdeadbeef;
	param[1].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_GETCPCBOUTPIXELCLOCK, param, 2);
	if (!ret) {
		*pixel_clock = param[1].u.value.a;
		ret = param[1].u.value.b;
	}

	return ret;

}

int VPP_CA_AVIOReset(void)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	/* clear result */
	param[0].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_AVIORESET, param, 1);
	if (!ret) {
		ret = param[0].u.value.a;
	}

	return ret;
}

int VPP_CA_GetHPDStatus(unsigned char *pHpdStatus)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;

	/* clear result */
	param[0].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_GETHPDSTATUS, param, 1);
	if (!ret) {
		ret = param[0].u.value.b;
		*pHpdStatus = (param[0].u.value.a & 0x01) ? true : false;
	}

	return ret;
}

int VPP_CA_GetBlockStatus(ENUM_VPP_BLOCK BlkId, int BlkSubId, int *status)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = BlkSubId;
	param[0].u.value.b = BlkId;

	/* clear result */
	param[1].u.value.b = 0xdeadbeef;
	param[1].u.value.a = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_GET_BLOCK_STATUS, param, 2);
	if (!ret) {
		ret = param[1].u.value.b;
		*status = param[1].u.value.a;
	}

	return ret;
}

int VPP_CA_GetDispWindow(int PlaneId, int *WinX, int *WinY, int *WinW, int *WinH,
			int *BgClr, int *Alpha, ENUM_GLOBAL_ALPHA_FLAG *globalAlphaFlag)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = PlaneId;

	/* clear result */
	param[0].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_GETDISPWINDOW, param, 4);
	if (!ret) {
		ret = param[0].u.value.b;
		*WinX = param[1].u.value.a;
		*WinY = param[1].u.value.b;
		*WinW = param[2].u.value.a;
		*WinH = param[2].u.value.b;
		*BgClr = param[3].u.value.a;
		*Alpha = param[3].u.value.b;
		*globalAlphaFlag = param[0].u.value.a;
	}

	return ret;
}

int VPP_CA_GetRefWindow(int PlaneId, int *WinX, int *WinY, int *WinW, int *WinH)
{
	int ret;
	struct tee_param param[4];
	int index;
	u32 *pSession;

	index = VPP_CA_GetInstanceID();
	pSession = &(TAVPPInstance[index].session);

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	param[0].u.value.a = PlaneId;

	/* clear result */
	param[0].u.value.b = 0xdeadbeef;

	ret = InvokeCommandHelper(index, pSession, VPP_GETREFWINDOW, param, 3);
	if (!ret) {
		ret = param[0].u.value.b;
		*WinX = param[1].u.value.a;
		*WinY = param[1].u.value.b;
		*WinW = param[2].u.value.a;
		*WinH = param[2].u.value.b;
	}

	return ret;
}
