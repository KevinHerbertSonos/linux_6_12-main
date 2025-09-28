// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-drv.h"
#include "hrx-audio.h"

static void *AIPFifoGetKernelPreRdDMAInfo(
	AIP_DMA_CMD_FIFO * p_aip_cmd_fifo, int pair)
{
	void *pHandle;
	int rd_offset = p_aip_cmd_fifo->nKernelPreRdOffset;

	if (rd_offset > p_aip_cmd_fifo->nCmdSize || rd_offset < 0) {
		int i = 0, fifo_cmd_size = sizeof(AIP_DMA_CMD_FIFO) >> 2;
		int *temp = (int *) p_aip_cmd_fifo;
		(void)temp;
		HRX_LOG(AIP_INFO, "rd_offset = %d fifo_cmd_size = %d :\n",
			rd_offset, fifo_cmd_size);

		HRX_LOG(AIP_INFO, "memory %p is corrupted! corrupted data :\n",
			  p_aip_cmd_fifo);
		for (i = 0; i < fifo_cmd_size; i++)
			HRX_LOG(AIP_INFO, "0x%x\n", *temp++);

		rd_offset = 0;
	}
	pHandle = &(p_aip_cmd_fifo->sAIPDmaCmd[pair][rd_offset]);
	return pHandle;
}

static void AIPFifoKernelPreRdUpdate(
	AIP_DMA_CMD_FIFO * p_aip_cmd_fifo, int adv)
{
	p_aip_cmd_fifo->nKernelPreRdOffset += adv;
	p_aip_cmd_fifo->nKernelPreRdOffset %= p_aip_cmd_fifo->nCmdSize;
}

static void AIPFifoKernelRdUpdate(
	AIP_DMA_CMD_FIFO * p_aip_cmd_fifo, int adv)
{
	p_aip_cmd_fifo->nKernelRdOffset += adv;
	p_aip_cmd_fifo->nKernelRdOffset %= p_aip_cmd_fifo->nCmdSize;
}

static int AIPFifoCheckKernelFullness(AIP_DMA_CMD_FIFO *p_aip_cmd_fifo)
{
	int full;

	full = p_aip_cmd_fifo->nCmdWrOffset - p_aip_cmd_fifo->nKernelPreRdOffset;
	if (full < 0)
		full += p_aip_cmd_fifo->nCmdSize;
	return full;
}

static void aip_start_cmd(struct syna_hrx_v4l2_dev *hrx_dev)
{
	//int *p = aip_info;
	int chanId;
	HDL_dhub *dhub = NULL;
	AIP_DMA_CMD *p_dma_cmd;
	AIP_DMA_CMD_FIFO *pCmdFifo = NULL;
	u32 rc;

	if (!hrx_dev) {
		HRX_LOG(AIP_ERROR, "%s: null handler!\n", __func__);
		return;
	}

	pCmdFifo = hrx_dev->aip_fifo.pDmaCmdFifo;
	if (!pCmdFifo) {
		HRX_LOG(AIP_ERROR, "%s: p_aip_fifo is NULL\n", __func__);
		return;
	}

	dhub = hrx_dev->dhub;
	if (hrx_dev->aip_i2s_pair == 1) {
		p_dma_cmd = (AIP_DMA_CMD *)
			AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, 0);

			chanId = avioDhubChMap_aio64b_MIC3_CH_W;
			rc = dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->nAddr0,
				p_dma_cmd->nSize0, 0, 0, 0, 1, 0, 0);

			AIPFifoKernelPreRdUpdate(pCmdFifo, 1);

			/* push 2nd dHub command */

			p_dma_cmd =	(AIP_DMA_CMD *)
				AIPFifoGetKernelPreRdDMAInfo(
							pCmdFifo, 0);
			rc = dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->nAddr0,
				p_dma_cmd->nSize0, 0, 0, 0, 1, 0, 0);
			AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	} else if (hrx_dev->aip_i2s_pair == 4) {
		unsigned int pair;

		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			chanId = avioDhubChMap_aio64b_MIC3_CH_W +
			pair;
			dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->nAddr0,
				p_dma_cmd->nSize0, 0, 0, 0, 1, 0,
				0);
		}

		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);

		for (pair = 0; pair < 4; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);
			chanId = avioDhubChMap_aio64b_MIC3_CH_W +
						pair;
			dhub_channel_write_cmd(dhub, chanId,
				p_dma_cmd->nAddr0,
				p_dma_cmd->nSize0, 0, 0, 0, 1, 0,
				0);
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	}
}

static void aip_stop_cmd(struct syna_hrx_v4l2_dev *hrx_dev)
{
	if (!hrx_dev) {
		HRX_LOG(AIP_ERROR, "%s: null handler!\n", __func__);
		return;
	}
}

void aip_resume_cmd(struct syna_hrx_v4l2_dev *hrx_dev)
{
	AIP_DMA_CMD *p_dma_cmd;
	HDL_dhub *dhub = NULL;
	unsigned int chanId;
	int pair;
	AIP_DMA_CMD_FIFO *pCmdFifo;

	if (!hrx_dev) {
		HRX_LOG(AIP_ERROR, "%s: null handler!\n", __func__);
		return;
	}

	if (hrx_dev->aip_status != AIP_STATUS_START)
		return;

	pCmdFifo = hrx_dev->aip_fifo.pDmaCmdFifo;
	if (!pCmdFifo) {
		HRX_LOG(AIP_ERROR, "%s::p_aip_fifo is NULL\n", __func__);
		return;
	}

	spin_lock(&hrx_dev->aip_spinlock);

	if (!pCmdFifo->nFifoOverflow)
		AIPFifoKernelRdUpdate(pCmdFifo, 1);

	dhub = hrx_dev->dhub;

	if (AIPFifoCheckKernelFullness(pCmdFifo)) {
		pCmdFifo->nFifoOverflow = 0;

		for (pair = 0; pair < hrx_dev->aip_i2s_pair; pair++) {
			p_dma_cmd = (AIP_DMA_CMD *)
				AIPFifoGetKernelPreRdDMAInfo(pCmdFifo, pair);

				chanId = avioDhubChMap_aio64b_MIC3_CH_W	+ pair;
				dhub_channel_write_cmd(dhub, chanId,
					p_dma_cmd->nAddr0,
					p_dma_cmd->nSize0, 0, 0,
					0,
					p_dma_cmd->nAddr1 ? 0 : 1,
					0, 0);
				if (p_dma_cmd->nAddr1) {
					dhub_channel_write_cmd(dhub,
						chanId,
						p_dma_cmd->nAddr1,
						p_dma_cmd->nSize1,
						0, 0, 0, 1, 0,
						0);
				}
		}
		AIPFifoKernelPreRdUpdate(pCmdFifo, 1);
	} else {
		pCmdFifo->nFifoOverflow = 1;
		pCmdFifo->nFifoOverflowCnt++;
		HRX_LOG(AIP_DEBUG, "%s: nFifoOverflow = %d nFifoOverflowCnt = %d\n",
			__func__, pCmdFifo->nFifoOverflow,
			pCmdFifo->nFifoOverflowCnt);
		for (pair = 0; pair < hrx_dev->aip_i2s_pair; pair++) {
			/* FIXME:
			 * chanid should be changed if 4 pair is supported
			 */
			chanId = avioDhubChMap_aio64b_MIC3_CH_W	+ pair;
			dhub_channel_write_cmd(dhub, chanId,
				(uintptr_t)pCmdFifo->nOverflowBuffer,
				pCmdFifo->nOverflowBufferSize, 0,
				0, 0, 1, 0, 0);
		}
	}

	spin_unlock(&hrx_dev->aip_spinlock);
}

int aip_call_start_cmd(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned long aip_spinlock_flags;

	spin_lock_irqsave(&hrx_dev->aip_spinlock, aip_spinlock_flags);
	aip_start_cmd(hrx_dev);
	spin_unlock_irqrestore(&hrx_dev->aip_spinlock, aip_spinlock_flags);
	return 0;

}
void aip_call_stop_cmd(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned long aip_spinlock_flags;

	spin_lock_irqsave(&hrx_dev->aip_spinlock, aip_spinlock_flags);
	aip_stop_cmd(hrx_dev);
	spin_unlock_irqrestore(&hrx_dev->aip_spinlock, aip_spinlock_flags);

}
