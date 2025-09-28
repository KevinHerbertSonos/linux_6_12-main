#if defined(SUPPORT_TRUSTED_DEVICE)
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>

#include "tee_client_api.h"
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

static TEEC_Context             tee_context;
static TEEC_Session             tee_session;
const TEEC_UUID GPU_TA_UUID =   {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x36}};

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

static TEEC_Result getValueFromTA(int cmdId, int *size)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = 0xdeadbeef;

	result = TEEC_InvokeCommand(&tee_session, cmdId, &operation, NULL);

	*size = operation.params[0].value.a;

	return result;
}

static TEEC_Result setFwParams(int cmdId, IMG_UINT64 a, IMG_UINT64 b, IMG_UINT32 c, IMG_UINT32 d)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT);
	operation.params[0].value.a = (IMG_UINT32)(a>>32);
	operation.params[0].value.b = (IMG_UINT32)(a&0xFFFFFFFF);
	operation.params[1].value.a = (IMG_UINT32)(b>>32);
	operation.params[1].value.b = (IMG_UINT32)(b&0xFFFFFFFF);
	operation.params[2].value.a = c;
	operation.params[2].value.b = d;

	//printk(KERN_ERR "[Kernel] sGPURegAddr=%x:%x\n", a>>32, a&0xFFFFFFFF);
	//printk(KERN_ERR "[Kernel] sFWStackAddr=%x:%x\n", b>>32, b&0xFFFFFFFF);
	//printk(KERN_ERR "[Kernel] ui32FWPageTableLog2PageSize=%d\n", c);
	//printk(KERN_ERR "[Kernel] ui32FWPageTableNumPages=%d\n", d);

	result = TEEC_InvokeCommand(&tee_session, cmdId, &operation, NULL);

	return result;
}

static TEEC_Result setFwPageTableAddr(int cmdId, IMG_UINT64 a, IMG_UINT64 b, IMG_UINT64 c, IMG_UINT64 d)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT);
	operation.params[0].value.a = (IMG_UINT32)(a>>32);
	operation.params[0].value.b = (IMG_UINT32)(a&0xFFFFFFFF);
	operation.params[1].value.a = (IMG_UINT32)(b>>32);
	operation.params[1].value.b = (IMG_UINT32)(b&0xFFFFFFFF);
	operation.params[2].value.a = (IMG_UINT32)(c>>32);
	operation.params[2].value.b = (IMG_UINT32)(c&0xFFFFFFFF);
	operation.params[3].value.a = (IMG_UINT32)(d>>32);
	operation.params[3].value.b = (IMG_UINT32)(d&0xFFFFFFFF);

	result = TEEC_InvokeCommand(&tee_session, cmdId, &operation, NULL);

	return result;
}


PVRSRV_ERROR init_tz(void *pvOSDevice)
{
	TEEC_Result result;
	TEEC_Parameter parameter;
	TEEC_SharedMemory shm;
	TEEC_Property *property = NULL;
	const struct firmware *psFW = NULL;
	PVRSRV_ERROR res = PVRSRV_OK;

	result = TEEC_InitializeContext(NULL, &tee_context);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_InitializeContext fail, result=%x\n", result);
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	res = request_firmware(&psFW, "ta/libgfx_img_linux.ta", pvOSDevice);
	if (res != 0) {
		printk(KERN_ERR "%s: request_firmware(libgfx_img.ta) failed (%d)", __func__, res);
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanupContext;
	}

	shm.size = psFW->size;
	shm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	result = TEEC_AllocateSharedMemory(&tee_context, &shm);
	if (result != TEEC_SUCCESS || NULL == shm.buffer) {
		printk(KERN_ERR "AllocateShareMemory error, ret=0x%08x\n", result);
		res = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto cleanupFW;
	}

	memcpy(shm.buffer, psFW->data, psFW->size);

	parameter.memref.parent = &shm;
	parameter.memref.size = shm.size;
	parameter.memref.offset = 0;

	result = TEEC_RegisterTAExt(&tee_context, &parameter, TEEC_MEMREF_PARTIAL_INPUT, property);
	if (result != TEEC_SUCCESS) {
		if (result == TEEC_ERROR_ACCESS_CONFLICT)
			printk(KERN_ERR "This TA has been registed already\n");
		else
			printk(KERN_ERR "RegisterTAExt error, ret=0x%08x\n", result);

		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanSharedBuffer;
	}

	result = TEEC_OpenSession(&tee_context, &tee_session, &GPU_TA_UUID, TEEC_LOGIN_USER, NULL, NULL, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_OpenSession fail, result=%x\n", result);
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanSharedBuffer;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	res = allocateFWAddress(pvOSDevice);
#else
	res = allocateFWAddress();
#endif

cleanSharedBuffer:
	TEEC_ReleaseSharedMemory(&shm);
cleanupFW:
	release_firmware(psFW);
cleanupContext:
	TEEC_FinalizeContext(&tee_context);
	return res;
}

void deinit_tz(void)
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
	TEEC_Operation operation;
	TEEC_SharedMemory srcShm;
	TEEC_SharedMemory dstShm;
	TEEC_Result result;
	int codeSize;
	int dataSize;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	struct dma_buf_map fw_src_data;
#else
	void *fw_src_data;
#endif

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
	if (dma_buf_map_is_null(&fw_src_data)) {
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

	srcShm.buffer = NULL;
	srcShm.phyAddr = (void *) fw_src_phy_addr;
	srcShm.size = psTDFWParams->ui32FirmwareSize;
	srcShm.flags = TEEC_MEM_INPUT;
	result = TEEC_RegisterSharedMemory(&tee_context, &srcShm);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_RegisterSharedMemory fail as:%d  (phyAddr=%llx  size=%d)\n",
				result, (unsigned long long)srcShm.phyAddr, (int)srcShm.size);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	dstShm.buffer  = NULL;
	dstShm.phyAddr = (void *)getSecureHeapPaddr();
	dstShm.size    = FW_CODE_SIZE+FW_DATA_SIZE;
	dstShm.flags   = TEEC_MEM_INPUT;
	result = TEEC_RegisterSharedMemory(&tee_context, &dstShm);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_RegisterSharedMemory fail as:%d  (phyAddr=%llx  size=%d)\n",
				 result, (unsigned long long)dstShm.phyAddr, (int)dstShm.size);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
											TEEC_MEMREF_PARTIAL_INPUT,
											TEEC_NONE,
											TEEC_NONE);

	operation.params[0].memref.parent = &srcShm;
	operation.params[0].memref.offset = 0;
	operation.params[0].memref.size   = psTDFWParams->ui32FirmwareSize;

	operation.params[1].memref.parent = &dstShm;
	operation.params[1].memref.offset = 0;
	operation.params[1].memref.size   = FW_CODE_SIZE+FW_DATA_SIZE;

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SEND_IMAGE, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SEND_IMAGE fail as:%d \n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
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
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_GET_CODE_SIZE fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*FW code size must be the same as FW_CODE_SIZE, otherwise MIPS can not bootup*/
	if (codeSize != FW_CODE_SIZE) {
		printk(KERN_ERR "Get wrong FW_CODE_SIZE from TA, expect:%d actual:%d", FW_CODE_SIZE, codeSize);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*get FW code size*/
	result = getValueFromTA(GPU_CMD_GET_DATA_SIZE, &dataSize);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_GET_DATA_SIZE fail as:%d\n",  result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*FW code size must be the same as FW_DATA_SIZE, otherwise MIPS can not bootup*/
	if (dataSize != FW_DATA_SIZE) {
		printk(KERN_ERR "Get wrong FW_DATA_SIZE from TA, expect:%d actual:%d", FW_DATA_SIZE, dataSize);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_SET_POWER_PARAMS(IMG_HANDLE hSysData, PVRSRV_TD_POWER_PARAMS *psTDPowerParams)
{
	int i = 0;
	TEEC_Result result;
	TEEC_Operation operation = {0};
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT);

	if (psTDPowerParams->sPCAddr.uiAddr>>32        != 0 ||
		psTDPowerParams->sGPURegAddr.uiAddr>>32    != 0 ||
		psTDPowerParams->sBootRemapAddr.uiAddr>>32 != 0 ||
		psTDPowerParams->sCodeRemapAddr.uiAddr>>32 != 0 ||
		psTDPowerParams->sDataRemapAddr.uiAddr>>32 != 0) {
		printk(KERN_ERR "GPU PC/Reg/Boot/Code/Data address must within 32bits!");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.params[0].value.a = (IMG_UINT32)(psTDPowerParams->sPCAddr.uiAddr & 0xFFFFFFFF);
	operation.params[0].value.b = (IMG_UINT32)(psTDPowerParams->sGPURegAddr.uiAddr & 0xFFFFFFFF);
	operation.params[1].value.a = (IMG_UINT32)(psTDPowerParams->sBootRemapAddr.uiAddr & 0xFFFFFFFF);
	operation.params[1].value.b = (IMG_UINT32)(psTDPowerParams->sCodeRemapAddr.uiAddr & 0xFFFFFFFF);
	operation.params[2].value.a = (IMG_UINT32)(psTDPowerParams->sDataRemapAddr.uiAddr & 0xFFFFFFFF);

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_POWER_PARAMS, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_POWER_PARAMS fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = (IMG_UINT32)(psTDPowerParams->ui32B);
	operation.params[0].value.b = (IMG_UINT32)(psTDPowerParams->ui32V);
	operation.params[1].value.a = (IMG_UINT32)(psTDPowerParams->ui32N);
	operation.params[1].value.b = (IMG_UINT32)(psTDPowerParams->ui32C);

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_BVNC, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_BNVC fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = psTDPowerParams->bDevicePA0IsValid;
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_bDevicePA0IsValid, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_bDevicePA0IsValid fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = (unsigned int)(psTDPowerParams->ui64DevErnsBrns>>32);
	operation.params[0].value.b = (unsigned int)(psTDPowerParams->ui64DevErnsBrns&0xFFFFFFFF);
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_ui64DevErnsBrns, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_ui64DevErnsBrns fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = (unsigned int)(psTDPowerParams->ui64DevFeatures>>32);
	operation.params[0].value.b = (unsigned int)(psTDPowerParams->ui64DevFeatures&0xFFFFFFFF);
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_ui64DevFeatures, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_ui64DevFeatures fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	for(i =0; i<RGX_FEATURE_WITH_VALUES_MAX_IDX; i++) {
		operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
		operation.params[0].value.a = i;
		operation.params[0].value.b = psTDPowerParams->ui32FeaturesValues[i];
		result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_ui64DevFeaturesValue, &operation, NULL);
		if (result != TEEC_SUCCESS) {
			printk(KERN_ERR "GPU_CMD_SET_ui64DevFeaturesValue fail as:%d\n", result);
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_RGXSTART(IMG_HANDLE hSysData)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_RGX_START, &operation, NULL);

	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_RGX_START fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_RGXSTOP(IMG_HANDLE hSysData)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_RGX_STOP, &operation, NULL);

	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_RGX_STOP fail as:%d\n",  result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_TD_SET_GFX_CORE_CLOCK(unsigned int value)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = value;

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_RGX_GFX_CORE_CLOCK, &operation, NULL);

	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_RGX_GFX_CORE_CLOCK fail as:%d\n",  result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}
#endif
