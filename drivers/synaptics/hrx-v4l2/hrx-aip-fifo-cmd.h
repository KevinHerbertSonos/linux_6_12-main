// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef _HRX_AIP_FIFO_CMD_H_
#define _HRX_AIP_FIFO_CMD_H_

typedef struct AIP_DMA_CMD_T {
	u32 nAddr0;
	u32 nSize0;
	u32 nAddr1;
	u32 nSize1;
} AIP_DMA_CMD;

typedef struct AIP_DMA_CMD_FIFO_T {
	AIP_DMA_CMD sAIPDmaCmd[AIP_MAX_IN_PAIR_NR][AIP_MAX_DMA_CMD_NR];
	u32 nDmaUpdate[AIP_MAX_DMA_CMD_NR];
	u32 nDmaCmdDataSize[AIP_MAX_DMA_CMD_NR];
	u32 nCmdSize;
	u32 nCmdWrOffset;
	u32 nCmdRdOffset;
	u32 nKernelRdOffset;
	u32 nPreFifoOverflowCnt;
	/* used by kernel */
	u32 nKernelPreRdOffset;
	int *nOverflowBuffer;
	u32 nOverflowBufferSize;
	u32 nFifoOverflow;
	u32 nFifoOverflowCnt;
} AIP_DMA_CMD_FIFO;

typedef struct AIP_FIFO_T {
	VPP_MEM hNonCacheShm;
	VPP_MEM hOverflowShm;
	AIP_DMA_CMD_FIFO *pDmaCmdFifo;
} AIP_FIFO;

int aip_dma_cmd_create_fifo(AIP_FIFO *pFifo, VPP_MEM_LIST *memlist);
int aip_dma_cmd_fifo_free(AIP_FIFO *pFifo, VPP_MEM_LIST *memlist);
int aip_dma_cmd_fifo_reset(AIP_DMA_CMD_FIFO *pCmdFifo);
int aip_dma_cmd_fifo_get_fullness(AIP_DMA_CMD_FIFO *pCmdFifo, int *pFullness);
int aip_dma_cmd_fifo_get_finished_fullness(AIP_DMA_CMD_FIFO *pCmdFifo, int *pFullness);
int aip_dma_cmd_fifo_get_space(AIP_DMA_CMD_FIFO *pCmdFifo, int *pSpace);
int aip_dma_cmd_fifo_rd_update(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nAdv);
int aip_dma_cmd_fifo_wr_update(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nAdv);
int aip_dma_cmd_fifo_get_update_flag(AIP_DMA_CMD_FIFO *pCmdFifo, u32 *pFlag);
int aip_dma_cmd_fifo_get_update_size(AIP_DMA_CMD_FIFO *pCmdFifo, u32 *pSize);
int aip_dma_cmd_fifo_set_update_flag(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nFlag);
int aip_dma_cmd_fifo_set_update_size(AIP_DMA_CMD_FIFO *pCmdFifo, u32 nFlag);
int aip_dma_cmd_fifo_get_wr_dma_cmd(AIP_DMA_CMD_FIFO *pCmdFifo, unsigned int nPair, void **pCmd);
int aip_dma_cmd_set(AIP_DMA_CMD *pCmd, u32 nAddr0, u32 nSize0, u32 nAddr1, u32 nSize1);
int aip_dma_cmd_fifo_get_overflow_delta_cnt(AIP_DMA_CMD_FIFO *pCmdFifo, unsigned int *pDeltaCnt);

#endif //_HRX_AIP_FIFO_CMD_H_
