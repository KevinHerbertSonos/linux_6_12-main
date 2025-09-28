// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-drv.h"
#include "hrx-aip-fifo-cmd.h"

#define WRAPAROUND(a, b)    (((a) >= (b))?(a-b):(a))

int aip_dma_cmd_create_fifo(AIP_FIFO *pAipFIFO, VPP_MEM_LIST *memlist)
{
	int rc;
	int result;
	//i;
	//  AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;

	pAipFIFO->hNonCacheShm.size = sizeof(AIP_DMA_CMD_FIFO);
	result = VPP_MEM_AllocateMemory(memlist, VPP_MEM_TYPE_DMA,
			&pAipFIFO->hNonCacheShm, 0);
	if (result != VPP_MEM_ERROR_TYPE_OK)
		HRX_LOG(AIP_ERROR, "share memory allocation failed result:%d\n", result);

	pAipFIFO->pDmaCmdFifo = pAipFIFO->hNonCacheShm.k_addr;

	pAipFIFO->hOverflowShm.size = AIP_DEFAULT_CHUNK_SIZE;
	VPP_MEM_AllocateMemory(memlist, VPP_MEM_TYPE_DMA,
			&pAipFIFO->hOverflowShm, 0);
	if (result != VPP_MEM_ERROR_TYPE_OK)
		HRX_LOG(AIP_ERROR, "share memory allocation failed result:%d\n", result);

	pAipFIFO->pDmaCmdFifo->nCmdSize = AIP_MAX_DMA_CMD_NR;
	pAipFIFO->pDmaCmdFifo->nOverflowBuffer = (void *)pAipFIFO->hOverflowShm.p_addr;
	pAipFIFO->pDmaCmdFifo->nOverflowBufferSize = AIP_DEFAULT_CHUNK_SIZE;
	pAipFIFO->pDmaCmdFifo->nFifoOverflowCnt = 0;

	return rc;
}

int aip_dma_cmd_fifo_free(AIP_FIFO *pFifo, VPP_MEM_LIST *memlist)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pFifo == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	VPP_MEM_FreeMemory(memlist,
			VPP_MEM_TYPE_DMA,
			&pFifo->hOverflowShm);

	VPP_MEM_FreeMemory(memlist,
			VPP_MEM_TYPE_DMA,
			&pFifo->hNonCacheShm);

	pFifo->pDmaCmdFifo = NULL;

EXIT:
	return rc;
}

int aip_dma_cmd_fifo_reset(AIP_DMA_CMD_FIFO *pCmdFifo)
{
	int rc, i;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL) {
		rc = HRX_AIP_EBADPARAM;
		HRX_LOG(AIP_INFO, "pCmdFifo is NULL\n");
		goto EXIT;
	}

	for (i = 0; i < AIP_MAX_DMA_CMD_NR; i++) {
		pCmdFifo->nDmaUpdate[i] = 0;
		pCmdFifo->nDmaCmdDataSize[i] = 0;
	}
	pCmdFifo->nCmdWrOffset = 0;
	pCmdFifo->nCmdRdOffset = 0;
	pCmdFifo->nKernelRdOffset = 0;
	pCmdFifo->nKernelPreRdOffset = 0;
	pCmdFifo->nPreFifoOverflowCnt = 0;
	pCmdFifo->nFifoOverflow = 0;
	pCmdFifo->nFifoOverflowCnt = 0;
EXIT:
	return rc;

}

int aip_dma_cmd_fifo_get_fullness(AIP_DMA_CMD_FIFO *pCmdFifo, int *pFullness)
{
	int rc, nFullness;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || pFullness == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nFullness = pCmdFifo->nCmdWrOffset - pCmdFifo->nCmdRdOffset;
	if (nFullness < 0)
		nFullness += pCmdFifo->nCmdSize;

	*pFullness = nFullness;
EXIT:
	return rc;

}

int aip_dma_cmd_fifo_get_finished_fullness(AIP_DMA_CMD_FIFO *pCmdFifo, int *pFullness)
{
	int rc, nFullness;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || pFullness == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nFullness = pCmdFifo->nKernelRdOffset - pCmdFifo->nCmdRdOffset;
	if (nFullness < 0)
		nFullness += pCmdFifo->nCmdSize;
	*pFullness = nFullness;
EXIT:
	return rc;
}

int aip_dma_cmd_fifo_get_space(AIP_DMA_CMD_FIFO *pCmdFifo, int *pSpace)
{
	int rc, nSpace;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || pSpace == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nSpace = pCmdFifo->nCmdRdOffset - pCmdFifo->nCmdWrOffset;
	if (nSpace <= 0)
		nSpace += pCmdFifo->nCmdSize;
	*pSpace = nSpace - 1;
EXIT:
	return rc;
}

int aip_dma_cmd_fifo_rd_update(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nAdv)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || nAdv >= pCmdFifo->nCmdSize) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pCmdFifo->nCmdRdOffset = WRAPAROUND(pCmdFifo->nCmdRdOffset + nAdv,
			pCmdFifo->nCmdSize);

EXIT:
	return rc;
}

int aip_dma_cmd_fifo_wr_update(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nAdv)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || nAdv >= pCmdFifo->nCmdSize) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	pCmdFifo->nCmdWrOffset = WRAPAROUND(pCmdFifo->nCmdWrOffset + nAdv,
			pCmdFifo->nCmdSize);

EXIT:
	return rc;
}

int aip_dma_cmd_fifo_get_update_flag(AIP_DMA_CMD_FIFO *pCmdFifo, u32 *pFlag)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || pFlag == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	*pFlag = pCmdFifo->nDmaUpdate[pCmdFifo->nCmdRdOffset];
EXIT:
	return rc;
}

int aip_dma_cmd_fifo_get_update_size(AIP_DMA_CMD_FIFO *pCmdFifo, u32 *pSize)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || pSize == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	*pSize = pCmdFifo->nDmaCmdDataSize[pCmdFifo->nCmdRdOffset];
EXIT:
	return rc;
}

int aip_dma_cmd_fifo_set_update_flag(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nFlag)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	pCmdFifo->nDmaUpdate[pCmdFifo->nCmdWrOffset] = nFlag;
EXIT:
	return rc;
}

int aip_dma_cmd_fifo_set_update_size(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nSize)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	pCmdFifo->nDmaCmdDataSize[pCmdFifo->nCmdWrOffset] = nSize;
EXIT:
	return rc;
}

int aip_dma_cmd_fifo_get_wr_dma_cmd(AIP_DMA_CMD_FIFO *pCmdFifo, unsigned int nPair, void **pCmd)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || pCmd == NULL || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	*pCmd = &pCmdFifo->sAIPDmaCmd[nPair][pCmdFifo->nCmdWrOffset];
EXIT:
	return rc;

}
int aip_dma_cmd_set(AIP_DMA_CMD *pCmd, u32 nAddr0, u32 nSize0, u32 nAddr1, u32 nSize1)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmd == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	pCmd->nAddr0 = nAddr0;
	pCmd->nSize0 = nSize0;
	pCmd->nAddr1 = nAddr1;
	pCmd->nSize1 = nSize1;
EXIT:
	return rc;
}

int aip_dma_cmd_fifo_get_overflow_delta_cnt(AIP_DMA_CMD_FIFO *pCmdFifo, unsigned int *pDeltaCnt)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pCmdFifo == NULL || pDeltaCnt == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	*pDeltaCnt = pCmdFifo->nFifoOverflowCnt - pCmdFifo->nPreFifoOverflowCnt;
	pCmdFifo->nPreFifoOverflowCnt = pCmdFifo->nFifoOverflowCnt;
EXIT:
	return rc;
}
