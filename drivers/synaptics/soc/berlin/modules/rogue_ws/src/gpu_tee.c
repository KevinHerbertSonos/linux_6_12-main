#if defined(SUPPORT_TRUSTED_DEVICE)
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/tee_drv.h>

#include "gpu_tz.h"
//#include "vz_support.h"
#include "interrupt_support.h"
#include "rgx_bvnc_defs_km.h"
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/dma-buf.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include "kernel_compatibility.h"
#ifdef CONFIG_DMABUF_HEAPS
#include <linux/dma-heap.h>
#endif
#ifdef CONFIG_ION
#include "ion_support.h"
#include <linux/ion.h>
#endif

enum {
	GPU_CMD_SEND_IMAGE			= 0,
	GPU_CMD_GET_CODE_SIZE			= 1,
	GPU_CMD_GET_DATA_SIZE			= 2,
	GPU_CMD_SET_FW_PARAMS			= 3,
	GPU_CMD_SET_FW_PageTableAddr		= 4,
	GPU_CMD_SET_POWER_PARAMS		= 5,
	GPU_CMD_SET_bDevicePA0IsValid		= 6,
	GPU_CMD_SET_ui64DevErnsBrns		= 7,
	GPU_CMD_SET_ui64DevFeatures		= 8,
	GPU_CMD_SET_ui64DevFeaturesValue	= 9,
	GPU_CMD_RGX_START			= 10,
	GPU_CMD_RGX_STOP			= 11,
	GPU_CMD_RGX_GFX_CORE_CLOCK		= 12,
	GPU_CMD_SET_BVNC			= 13,
};

#define ALLOC_SIZE              1048576
#define PVR_ALLOC_ALIGN         SZ_64K
/* Add 0xFFFF to size since this memory need to be 64K alinged and dma_buf
   will give only 4K aligned buf */
#define PVR_ALIGN_OFFSET        (SZ_64K - 1)

static struct tee_context * tee_gpu_ctx;
static u32             tee_session;
static const uuid_t gpu_ta_uuid = UUID_INIT(0x1316a183, 0x894d, 0x43fe, 0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x36);

static struct dma_buf                  *fw_src_dma_buf;
static struct dma_buf                  *fw_dma_buf;
static phys_addr_t                     fw_src_phy_addr;
static phys_addr_t                     fw_phy_addr;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static struct dma_heap *fw_nonsecure_dma_heap;
static struct dma_heap *fw_secure_dma_heap;
static struct dma_buf_attachment       *fw_src_buf_attach;
static struct dma_buf_attachment       *fw_buf_attach;
static struct sg_table                 *fw_src_table;
static struct sg_table                 *fw_table;

extern int close_fd(unsigned int fd);

static phys_addr_t getFWSecureHeapPaddr(struct sg_table *fw_table)
{
	unsigned long ori = (unsigned long)PFN_PHYS(page_to_pfn(sg_page(fw_table->sgl)));

	return ALIGN(ori, PVR_ALLOC_ALIGN);
}

static PVRSRV_ERROR allocateFWAddress(void *pvOSDevice)
{
	struct device *dev;

	dev = (struct device *)pvOSDevice;
	fw_nonsecure_dma_heap = dma_heap_find("reserved");
	if (fw_nonsecure_dma_heap == NULL) {
		dev_err(dev, "reserved heap is unavailable\n");
		goto free0;
	}

	fw_secure_dma_heap = dma_heap_find("Secure");
	if (fw_secure_dma_heap == NULL) {
		dev_err(dev, "Secure heap is unavailable\n");
		goto free1;
	}

	fw_src_dma_buf = dma_heap_buffer_alloc(fw_nonsecure_dma_heap, ALLOC_SIZE, 0, 0);
	if (fw_src_dma_buf == NULL) {
		dev_err(dev, "non-secure dma buffer alloc failed, error %ld", PTR_ERR(fw_dma_buf));
		goto free2;
	}
	fw_dma_buf = dma_heap_buffer_alloc(fw_secure_dma_heap,
					(FW_CODE_SIZE+FW_DATA_SIZE)*2 + PVR_ALIGN_OFFSET, 0, 0);
	if (fw_dma_buf == NULL) {
		dev_err(dev, "secure buffer alloc failed, error %ld", PTR_ERR(fw_dma_buf));
		goto free3;
	}

	fw_src_buf_attach = dma_buf_attach(fw_src_dma_buf, dev);
	if (fw_src_buf_attach == NULL) {
		dev_err(dev, "fw src dma buf attach failed, error %ld", PTR_ERR(fw_src_buf_attach));
		goto free4;
	}
	fw_buf_attach = dma_buf_attach(fw_dma_buf, dev);
	if (fw_buf_attach == NULL) {
		dev_err(dev, "fw dma buf attach failed, error %ld", PTR_ERR(fw_buf_attach));
		goto free5;
	}

	fw_src_table = dma_buf_map_attachment(fw_src_buf_attach, DMA_FROM_DEVICE);
	if (fw_src_table == NULL) {
		dev_err(dev, "fw src dma bufmap attach failed, error %ld", PTR_ERR(fw_src_table));
		goto free6;
	}
	fw_table = dma_buf_map_attachment(fw_buf_attach, DMA_BIDIRECTIONAL);
	if (fw_table == NULL) {
		dev_err(dev, "fw dma bufmap attach failed, error %ld", PTR_ERR(fw_table));
		goto free7;
	}

	fw_src_phy_addr = sg_dma_address(fw_src_table->sgl);
	fw_phy_addr = getFWSecureHeapPaddr(fw_table);

	return PVRSRV_OK;
free7:
	dma_buf_unmap_attachment(fw_src_buf_attach, fw_src_table, DMA_FROM_DEVICE);
free6:
	dma_buf_detach(fw_dma_buf, fw_buf_attach);
free5:
	dma_buf_detach(fw_src_dma_buf, fw_src_buf_attach);
free4:
	dma_heap_buffer_free(fw_dma_buf);
free3:
	dma_heap_buffer_free(fw_src_dma_buf);
free2:
	dma_heap_put(fw_secure_dma_heap);
free1:
	dma_heap_put(fw_nonsecure_dma_heap);
free0:
	return PVRSRV_ERROR_INIT_FAILURE;
}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static unsigned long ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (unsigned long)PFN_PHYS(page_to_pfn(page));
}

static phys_addr_t getFWSecureHeapPaddr(void)
{
	unsigned long ori = ion_dmabuf_get_phy(fw_dma_buf);

	return ALIGN(ori, PVR_ALLOC_ALIGN);
}

static PVRSRV_ERROR allocateFWAddress(void)
{
	int nonSecurePoolID = -1;
	int securePoolID = -1;
	struct ion_heap_data *hdata;
	int heap_num, i;
	PVRSRV_ERROR res = PVRSRV_OK;

	hdata = kmalloc(sizeof(*hdata) * ION_NUM_MAX_HEAPS, GFP_KERNEL);
	if (!hdata) {
		printk(KERN_ERR "tzd_ion_shm_init alloc mem failed\n");
		res = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto allocateFWAddrReturn;
	}

	heap_num = ion_query_heaps_kernel(hdata, ION_NUM_MAX_HEAPS);

	for (i = 0; i < heap_num; i++) {
		if (!strcmp(hdata[i].name, "CMA-reserved"))
			nonSecurePoolID = 1 << hdata[i].heap_id;
	}

	for (i = 0; i < heap_num; i++) {
		if (!strcmp(hdata[i].name, "Secure"))
			securePoolID = 1 << hdata[i].heap_id;
	}

	kfree(hdata);

	if (nonSecurePoolID == -1 || securePoolID == -1) {
		printk(KERN_ERR "Can't find CMA-reserved or Secure Pool! %d %d",
					nonSecurePoolID, securePoolID);
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto allocateFWAddrReturn;
	}

	fw_src_dma_buf = ion_alloc(ALLOC_SIZE, nonSecurePoolID, ION_NONCACHED);
	if (IS_ERR(fw_src_dma_buf)) {
		printk(KERN_ERR "non-secure ion alloc failed");
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto allocateFWAddrReturn;
	}
	fw_dma_buf = ion_alloc((FW_CODE_SIZE+FW_DATA_SIZE)*2 + PVR_ALIGN_OFFSET,
							securePoolID, ION_NONCACHED);
	if (IS_ERR(fw_src_dma_buf)) {
		printk(KERN_ERR "secure ion alloc failed");
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto allocateFWAddrReturn;
	}

	fw_src_phy_addr = ion_dmabuf_get_phy(fw_src_dma_buf);
	fw_phy_addr = getFWSecureHeapPaddr();

allocateFWAddrReturn:
	return res;
}
#else
static PVRSRV_ERROR allocateFWAddress(void)
{
	printk(KERN_ERR "Kernel not supported\n");
	return PVRSRV_ERROR_INIT_FAILURE;
}

#endif

phys_addr_t getSecureHeapPaddr(void)
{
	return fw_phy_addr;
}

static int getValueFromTA(int cmdId, int *size)
{
	int result;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT;
	param[0].u.value.a = 0xdeadbeef;

	arg.func = cmdId;
	arg.num_params = 1;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0)
		pr_err("fail to invoke command to PTM TA: ret = %x, TEE err: 0x%x\n", result, arg.ret);
	else
		*size = param[0].u.value.a;

	return result;
}

static int setFwParams(int cmdId, IMG_UINT64 a, IMG_UINT64 b, IMG_UINT32 c, IMG_UINT32 d)
{
	int result;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = (IMG_UINT32)(a>>32);
	param[0].u.value.b = (IMG_UINT32)(a&0xFFFFFFFF);
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].u.value.a = (IMG_UINT32)(b>>32);
	param[1].u.value.b = (IMG_UINT32)(b&0xFFFFFFFF);
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].u.value.a = c;
	param[2].u.value.b = d;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;

	arg.func = cmdId;
	arg.num_params = 4;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0)
		pr_err("fail to invoke command to PTM TA: ret = %x, TEE err: 0x%x\n", result, arg.ret);

	//printk(KERN_ERR "[Kernel] sGPURegAddr=%x:%x\n", a>>32, a&0xFFFFFFFF);
	//printk(KERN_ERR "[Kernel] sFWStackAddr=%x:%x\n", b>>32, b&0xFFFFFFFF);
	//printk(KERN_ERR "[Kernel] ui32FWPageTableLog2PageSize=%d\n", c);
	//printk(KERN_ERR "[Kernel] ui32FWPageTableNumPages=%d\n", d);

	return result;
}

static int setFwPageTableAddr(int cmdId, IMG_UINT64 a, IMG_UINT64 b, IMG_UINT64 c, IMG_UINT64 d)
{
	int result;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = (IMG_UINT32)(a>>32);
	param[0].u.value.b = (IMG_UINT32)(a&0xFFFFFFFF);
	param[1].u.value.a = (IMG_UINT32)(b>>32);
	param[1].u.value.b = (IMG_UINT32)(b&0xFFFFFFFF);
	param[2].u.value.a = (IMG_UINT32)(c>>32);
	param[2].u.value.b = (IMG_UINT32)(c&0xFFFFFFFF);
	param[3].u.value.a = (IMG_UINT32)(d>>32);
	param[3].u.value.b = (IMG_UINT32)(d&0xFFFFFFFF);

	arg.func = cmdId;
	arg.num_params = 4;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0)
		pr_err("fail to invoke command to PTM TA: ret = %x, TEE err: 0x%x\n", result, arg.ret);


	return result;
}


static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	return (ver->impl_id == TEE_IMPL_ID_OPTEE);
}

PVRSRV_ERROR init_tz(void *pvOSDevice)
{
	int result;
	PVRSRV_ERROR res = PVRSRV_OK;
	struct tee_ioctl_open_session_arg sess_arg;

	tee_gpu_ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(tee_gpu_ctx)) {
		pr_err("fail to initialize optee context\n");
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	/* Open session with PTM Trusted App */

	memset(&sess_arg, 0, sizeof(sess_arg));
	memcpy(sess_arg.uuid, gpu_ta_uuid.b, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;
	tee_gpu_ctx->supp_nowait = false;

	result = tee_client_open_session(tee_gpu_ctx, &sess_arg, NULL);
	if (result < 0 || sess_arg.ret != 0) {
		pr_err("tee_client_open_session failed with code 0x%x origin 0x%x",
			result, sess_arg.ret);
		tee_client_close_context(tee_gpu_ctx);
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	tee_session = sess_arg.session;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	res = allocateFWAddress(pvOSDevice);
#else
	res = allocateFWAddress();
#endif

	return res;
}

void deinit_tz()
{
	if(fw_dma_buf != NULL) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		dma_buf_unmap_attachment(fw_buf_attach, fw_table, DMA_BIDIRECTIONAL);
		dma_buf_detach(fw_dma_buf, fw_buf_attach);
#endif
		dma_heap_buffer_free(fw_dma_buf);
		dma_heap_put(fw_secure_dma_heap);
		fw_dma_buf = NULL;
	}
	if(fw_src_dma_buf != NULL) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		dma_buf_unmap_attachment(fw_src_buf_attach, fw_src_table, DMA_FROM_DEVICE);
		dma_buf_detach(fw_src_dma_buf, fw_src_buf_attach);
#endif
		dma_heap_buffer_free(fw_src_dma_buf);
		dma_heap_put(fw_nonsecure_dma_heap);
		fw_src_dma_buf = NULL;
	}
}

PVRSRV_ERROR syna_PFN_TD_SEND_FW_IMAGE(IMG_HANDLE hSysData, PVRSRV_FW_PARAMS *psTDFWParams)
{
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];
	struct tee_shm* srcShm;
	struct tee_shm* dstShm;
	int result = 0;
	int codeSize;
	int dataSize;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	struct iosys_map fw_src_data;
#else
	void *fw_src_data;
#endif
	PVRSRV_ERROR res = PVRSRV_OK;
	int fw_src_fd, fw_dst_fd;

	setFwParams(GPU_CMD_SET_FW_PARAMS,
				psTDFWParams->uFWP.sMips.sGPURegAddr.uiAddr,
				psTDFWParams->uFWP.sMips.sFWStackAddr.uiAddr,
				psTDFWParams->uFWP.sMips.ui32FWPageTableLog2PageSize,
				psTDFWParams->uFWP.sMips.ui32FWPageTableNumPages);

	setFwPageTableAddr(GPU_CMD_SET_FW_PageTableAddr,
				psTDFWParams->uFWP.sMips.asFWPageTableAddr[0].uiAddr,
				psTDFWParams->uFWP.sMips.asFWPageTableAddr[1].uiAddr,
				psTDFWParams->uFWP.sMips.asFWPageTableAddr[2].uiAddr,
				psTDFWParams->uFWP.sMips.asFWPageTableAddr[3].uiAddr);


	dma_buf_begin_cpu_access(fw_src_dma_buf, DMA_FROM_DEVICE);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	dma_buf_vmap(fw_src_dma_buf, &fw_src_data);
	if (iosys_map_is_null(&fw_src_data)) {
		printk(KERN_ERR "dma heap buffer map failed\n");
		dma_buf_end_cpu_access(fw_src_dma_buf, DMA_FROM_DEVICE);
		dma_buf_unmap_attachment(fw_src_buf_attach, fw_src_table, DMA_FROM_DEVICE);
		dma_buf_detach(fw_src_dma_buf, fw_src_buf_attach);
		dma_heap_buffer_free(fw_src_dma_buf);
		dma_heap_put(fw_nonsecure_dma_heap);
		return PVRSRV_ERROR_DEVICEMEM_MAP_FAILED;
	}
	memcpy(fw_src_data.vaddr, psTDFWParams->pvFirmware, psTDFWParams->ui32FirmwareSize);
	dma_buf_vunmap(fw_src_dma_buf, &fw_src_data);
#else
	fw_src_data = dma_buf_kmap(fw_src_dma_buf, 0);
	memcpy(fw_src_data, psTDFWParams->pvFirmware, psTDFWParams->ui32FirmwareSize);
	dma_buf_kunmap(fw_src_dma_buf, 0, fw_src_data);
#endif
	dma_buf_end_cpu_access(fw_src_dma_buf, DMA_FROM_DEVICE);
	get_dma_buf(fw_src_dma_buf); //take additional ref to account for fd close
	fw_src_fd = dma_buf_fd(fw_src_dma_buf, O_RDWR);
	if (fw_src_fd < 0) {
		dma_buf_put(fw_src_dma_buf);
		res = PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	get_dma_buf(fw_dma_buf); //take additional ref to account for fd close
	srcShm = tee_shm_register_fd(tee_gpu_ctx, fw_src_fd);
	if (IS_ERR(srcShm)) {
		printk(KERN_ERR "tee_shm_register fail as:%d  (phyAddr=%llx  size=%d)\n",
				result, (unsigned long long)fw_src_phy_addr, psTDFWParams->ui32FirmwareSize);
		res = PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	fw_dst_fd = dma_buf_fd(fw_dma_buf, O_RDWR);
	if (fw_dst_fd < 0) {
		dma_buf_put(fw_dma_buf);
		res = PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	dstShm = tee_shm_register_fd(tee_gpu_ctx, fw_dst_fd);
	if (IS_ERR(dstShm)) {
		printk(KERN_ERR "tee_shm_register fail as:%d  (phyAddr=%llx  size=0x%x)\n",
				 result, (unsigned long long) getSecureHeapPaddr(), psTDFWParams->ui32FirmwareSize);
		res = PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = srcShm;
	param[0].u.memref.size = psTDFWParams->ui32FirmwareSize;
	param[0].u.memref.shm_offs = 0;
	param[1].u.memref.shm = dstShm;
	param[1].u.memref.size = FW_CODE_SIZE+FW_DATA_SIZE;
	param[1].u.memref.shm_offs = 0;


	arg.func = GPU_CMD_SEND_IMAGE;
	arg.num_params = 2;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_SEND_IMAGE fail as:ret = %x, TEE err: 0x%x\n", result, arg.ret);
		res = PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	if(fw_src_dma_buf != NULL) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		dma_buf_unmap_attachment(fw_src_buf_attach, fw_src_table, DMA_FROM_DEVICE);
		dma_buf_detach(fw_src_dma_buf, fw_src_buf_attach);
#endif
		dma_heap_buffer_free(fw_src_dma_buf);
		dma_heap_put(fw_nonsecure_dma_heap);
		fw_src_dma_buf = NULL;
	}

	/*get FW code size*/
	result = getValueFromTA(GPU_CMD_GET_CODE_SIZE, &codeSize);
	if (result != 0) {
		printk(KERN_ERR "GPU_CMD_GET_CODE_SIZE fail as:%d\n", result);
		res =  PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	/*FW code size must be the same as FW_CODE_SIZE, otherwise MIPS can not bootup*/
	if (codeSize != FW_CODE_SIZE) {
		printk(KERN_ERR "Get wrong FW_CODE_SIZE from TA, expect:%d actual:%d", FW_CODE_SIZE, codeSize);
		res = PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	/*get FW code size*/
	result = getValueFromTA(GPU_CMD_GET_DATA_SIZE, &dataSize);
	if (result != 0) {
		printk(KERN_ERR "GPU_CMD_GET_DATA_SIZE fail as:%d\n",  result);
		res =  PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

	/*FW code size must be the same as FW_DATA_SIZE, otherwise MIPS can not bootup*/
	if (dataSize != FW_DATA_SIZE) {
		printk(KERN_ERR "Get wrong FW_DATA_SIZE from TA, expect:%d actual:%d", FW_DATA_SIZE, dataSize);
		res = PVRSRV_ERROR_INVALID_PARAMS;
		goto out;
	}

out:
	if (dstShm) {
		tee_shm_free(dstShm);
		if (fw_dst_fd > 0)
			close_fd(fw_dst_fd);
	}

	if (srcShm) {
		tee_shm_free(srcShm);
		if (fw_src_fd > 0)
			close_fd(fw_src_fd);
	}

	return res;
}

PVRSRV_ERROR syna_PFN_TD_SET_POWER_PARAMS(IMG_HANDLE hSysData, PVRSRV_TD_POWER_PARAMS *psTDPowerParams)
{
	int i = 0;
	int result = 0;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	if (psTDPowerParams->sPCAddr.uiAddr>>32        != 0 ||
		psTDPowerParams->sGPURegAddr.uiAddr>>32    != 0 ||
		psTDPowerParams->sBootRemapAddr.uiAddr>>32 != 0 ||
		psTDPowerParams->sCodeRemapAddr.uiAddr>>32 != 0 ||
		psTDPowerParams->sDataRemapAddr.uiAddr>>32 != 0) {
		printk(KERN_ERR "GPU PC/Reg/Boot/Code/Data address must within 32bits!");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = (IMG_UINT32)(psTDPowerParams->sPCAddr.uiAddr & 0xFFFFFFFF);
	param[0].u.value.b = (IMG_UINT32)(psTDPowerParams->sGPURegAddr.uiAddr & 0xFFFFFFFF);
	param[1].u.value.a = (IMG_UINT32)(psTDPowerParams->sBootRemapAddr.uiAddr & 0xFFFFFFFF);
	param[1].u.value.b = (IMG_UINT32)(psTDPowerParams->sCodeRemapAddr.uiAddr & 0xFFFFFFFF);
	param[2].u.value.a = (IMG_UINT32)(psTDPowerParams->sDataRemapAddr.uiAddr & 0xFFFFFFFF);
	arg.func = GPU_CMD_SET_POWER_PARAMS;
	arg.num_params = 4;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_SET_POWER_PARAMS fail as: ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = (IMG_UINT32)(psTDPowerParams->ui32B);
	param[0].u.value.b = (IMG_UINT32)(psTDPowerParams->ui32V);
	param[1].u.value.a = (IMG_UINT32)(psTDPowerParams->ui32N);
	param[1].u.value.b = (IMG_UINT32)(psTDPowerParams->ui32C);

	arg.func = GPU_CMD_SET_BVNC;
	arg.num_params = 2;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_SET_BNVC fail as:ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = psTDPowerParams->bDevicePA0IsValid;
	param[0].u.value.b = (IMG_UINT32)(psTDPowerParams->ui32V);

	arg.func = GPU_CMD_SET_bDevicePA0IsValid;
	arg.num_params = 1;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_SET_bDevicePA0IsValid fail as:ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = (unsigned int)(psTDPowerParams->ui64DevErnsBrns>>32);
	param[0].u.value.b = (unsigned int)(psTDPowerParams->ui64DevErnsBrns&0xFFFFFFFF);

	arg.func = GPU_CMD_SET_ui64DevErnsBrns;
	arg.num_params = 1;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_SET_ui64DevErnsBrns fail as:ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = (unsigned int)(psTDPowerParams->ui64DevFeatures>>32);
	param[0].u.value.b = (unsigned int)(psTDPowerParams->ui64DevFeatures&0xFFFFFFFF);

	arg.func = GPU_CMD_SET_ui64DevFeatures;
	arg.num_params = 1;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_SET_ui64DevFeatures fail as:ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));
	for(i = 0; i < RGX_FEATURE_WITH_VALUES_MAX_IDX; i++) {
		param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
		param[0].u.value.a = i;
		param[0].u.value.b = psTDPowerParams->ui32FeaturesValues[i];

		arg.func = GPU_CMD_SET_ui64DevFeaturesValue;
		arg.num_params = 1;
		arg.session = tee_session;

		result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
		if (result < 0 || arg.ret != 0) {
			printk(KERN_ERR "GPU_CMD_SET_ui64DevFeaturesValue fail as: i = %d, ret = %d, TEE err: 0x%x\n", i, result, arg.ret);
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_RGXSTART(IMG_HANDLE hSysData)
{
	int result = 0;
	struct tee_ioctl_invoke_arg arg = {0};
	arg.func = GPU_CMD_RGX_START;
	arg.num_params = 0;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, NULL);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_RGX_START fail as:ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_RGXSTOP(IMG_HANDLE hSysData)
{
	int result = 0;
	struct tee_ioctl_invoke_arg arg = {0};

	arg.func = GPU_CMD_RGX_STOP;
	arg.num_params = 0;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, NULL);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_RGX_STOP fail as:ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_TD_SET_GFX_CORE_CLOCK(unsigned int value)
{
	int result = 0;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param param[4];

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));
	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = value;

	arg.func = GPU_CMD_RGX_GFX_CORE_CLOCK;
	arg.num_params = 1;
	arg.session = tee_session;

	result = tee_client_invoke_func(tee_gpu_ctx, &arg, param);
	if (result < 0 || arg.ret != 0) {
		printk(KERN_ERR "GPU_CMD_RGX_GFX_CORE_CLOCK fail as:ret = %d, TEE err: 0x%x\n", result, arg.ret);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}
#endif
