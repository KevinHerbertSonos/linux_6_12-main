// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-drv.h"
#include "hrx-aip-isr.h"
#include "hrx-aip.h"
#include "hrx-audio.h"
#include "hdmi_in_if.h"

#define AIP_UNSTABLE_SIGNAL_COUNT_MAX      25
#define SYNC_WORD_PA 0xF872
#define SYNC_WORD_PB 0x4E1F
#define SYNC_WORD_AC3 0x0B77
#define SYNC_WORD_DTS 0x7FFE
#define SYNC_WORD_DTS_HD 0x100
#define SYNC_WORD_MAT 0x079E


typedef struct{
	unsigned int nReserved        : 16;
	unsigned int nDataType        : 7;
	unsigned int nErrFlag         : 1;
	unsigned int nDataTypeInfo    : 5;
	unsigned int nStreamNo        : 3;
} AIP_BURST_INFO;

int aip_isr_handler(struct syna_hrx_v4l2_dev *hrx_dev, u32 intr_id)
{
	int rc;
	CC_MSG_t msg;

	msg.m_MsgID = 1 << intr_id;
	msg.m_Param1 = 0;
	msg.m_Param2 = 0;

	rc = AMPMsgQ_Add(&hrx_dev->aip_msg_queue, &msg);
	if (rc == S_OK)
		up(&hrx_dev->aip_sem);

	return 0;
}

static void aip_data_isr(struct syna_hrx_v4l2_dev *hrx_dev, int nChanId)
{
	int rc;
	AIP_BUFFER *pDataBuffer;
	AIP_DMA_CMD_FIFO *pCmdFifo;
	AIP_DMA_CMD *pDmaCmd;
	int nCmdFullness, nCmdSpace;
	unsigned int nBufferSpace;
	void *pDataAddr[AIP_MAX_IN_PAIR_NR] = {0};
	void *pStartAddr[AIP_MAX_IN_PAIR_NR] = {0};
	unsigned int nPhyAddr0, nPhyAddr1, nSize0, nSize1;
	unsigned int nShmOffset;
	unsigned int nUpdateFlag, nUpdateSize;
	unsigned int nPair, nPairsNr;
	unsigned int nTakeInSize;
	unsigned int nDeltaOverflowCnt;
	bool bNeedNewTimeStamp;
	//static int intrnum=0;
	//u32 addr, size, i, buf;
	//AIP_DMA_CMD *p_dma_cmd;

	pDataBuffer = &hrx_dev->aip_buf;
	pCmdFifo = hrx_dev->aip_fifo.pDmaCmdFifo;

	rc = aip_buffer_get_pairs_nr(pDataBuffer, &nPairsNr);
	AIP_CHKRC(rc);

	nTakeInSize = hrx_dev->nDmaChunkSize;
    /*
     * update cmd fifo and data buffer
     */
	while (1) {
		rc = aip_dma_cmd_fifo_get_finished_fullness(pCmdFifo, &nCmdFullness);
		AIP_CHKRC(rc);
		if (nCmdFullness <= 0)
			break;

		rc = aip_dma_cmd_fifo_get_update_flag(pCmdFifo, &nUpdateFlag);
		AIP_CHKRC(rc);

		if (nUpdateFlag) {
			rc = aip_dma_cmd_fifo_get_update_size(pCmdFifo, &nUpdateSize);
			AIP_CHKRC(rc);
			if (nUpdateSize) {
		/* transfer overflow count for time stamp update */
				rc = aip_dma_cmd_fifo_get_overflow_delta_cnt(pCmdFifo, &nDeltaOverflowCnt);
				AIP_CHKRC(rc);
		/*
		 * get time stamp when wr offset is 0
		 * chunk size should be divided exactly by AIP buffer size
		 */
				rc = aip_buffer_neednew_timestamp(pDataBuffer, &bNeedNewTimeStamp);
				AIP_CHKRC(rc);
				if (bNeedNewTimeStamp) {
/*					AIP_EVENT_INFO sEventInfo;
 *					void *pCallbackCtx;
 *					unsigned int nPtsHi, nPtsLo;
 *					pCallbackCtx = hrx_dev->pEventCtx;
 *					rc = hrx_dev->pEventCB(AIP_EVENT_GET_TIMESTAMP, (void*)&sEventInfo, pCallbackCtx);
 *					AIP_CHKRC(rc);
 *					if (rc == S_OK)
 *					{
 *						nPtsHi = sEventInfo.sTimeStampInfo.sTimeStamp.nPtsHi;
 *						nPtsLo = sEventInfo.sTimeStampInfo.sTimeStamp.nPtsLo;
 *						aip_buffer_set_timestamp(pDataBuffer, nPtsHi, nPtsLo);
 *					}
 */
				}

				for (nPair = 0; nPair < nPairsNr; nPair++) {
					rc = aip_buffer_wr_update(pDataBuffer, nPair, nUpdateSize);
					AIP_CHKRC(rc);
				}
			}
		}
		rc = aip_dma_cmd_fifo_rd_update(pCmdFifo, 1);
		AIP_CHKRC(rc);
	}

	while (1) {
		rc = aip_dma_cmd_fifo_get_space(pCmdFifo, &nCmdSpace);
		AIP_CHKRC(rc);

		if (nCmdSpace <= 0)
			break;

		rc = aip_buffer_get_prespace(pDataBuffer, (int *) &nBufferSpace);
		AIP_CHKRC(rc);

		if (nBufferSpace < nTakeInSize)
			break;

		rc = aip_buffer_get_prespace_nowrap(pDataBuffer, (int *) &nBufferSpace);
		AIP_CHKRC(rc);
		for (nPair = 0; nPair < nPairsNr; nPair++) {
			rc = aip_buffer_get_pre_wr_addr(pDataBuffer, nPair, &pDataAddr[nPair]);
			AIP_CHKRC(rc);
			rc = aip_buffer_get_start_addr(pDataBuffer, nPair, &pStartAddr[nPair]);
			AIP_CHKRC(rc);
			rc = aip_dma_cmd_fifo_get_wr_dma_cmd(pCmdFifo, nPair, (void **)&pDmaCmd);
			AIP_CHKRC(rc);

			nShmOffset = pDataBuffer->nStartOffset[nPair] + (u8 *)pDataAddr[nPair] - (u8 *)pStartAddr[nPair];
			nPhyAddr0 = (uintptr_t)pDataBuffer->vpp_aip_buf_mem_handle.p_addr + nShmOffset;
			HRX_LOG(AIP_DEBUG, "AIP isr: write dma cmd, addr0[%x] size0[%x] nShmOffset %u\n", nPhyAddr0, nSize0, nShmOffset);

			if (nBufferSpace >= nTakeInSize) {
				nSize0 = nTakeInSize;
				nPhyAddr1 = 0;
				nSize1 = 0;
			} else {
				nSize0 = nBufferSpace;
				nShmOffset = pDataBuffer->nStartOffset[nPair];
				nPhyAddr1 = (uintptr_t)pDataBuffer->vpp_aip_buf_mem_handle.p_addr + nShmOffset;
				nSize1 = nTakeInSize - nBufferSpace;
				HRX_LOG(AIP_DEBUG, "AIP isr: write dma cmd, addr1[%x] size1[%x] nShmOffset %u\n", nPhyAddr1, nSize1, nShmOffset);
			}
			rc = aip_dma_cmd_set(pDmaCmd, nPhyAddr0, nSize0, nPhyAddr1, nSize1);
			AIP_CHKRC(rc);

			rc = aip_buffer_pre_wr_update(pDataBuffer, nPair, nTakeInSize);
			AIP_CHKRC(rc);
		}

		rc = aip_dma_cmd_fifo_set_update_flag(pCmdFifo, 1);
		AIP_CHKRC(rc);
		rc = aip_dma_cmd_fifo_set_update_size(pCmdFifo, nTakeInSize);
		AIP_CHKRC(rc);
		rc = aip_dma_cmd_fifo_wr_update(pCmdFifo, 1);
		AIP_CHKRC(rc);
	}
	return;
}

int aip_create_isr_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;

	sema_init(&hrx_dev->aip_sem, 0);

	ret = AMPMsgQ_Init(&hrx_dev->aip_msg_queue, HRX_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		HRX_LOG(AIP_ERROR, "hrx_msg_queue init: failed, err:%8x\n", ret);
		return -1;
	}

	hrx_dev->aip_isr_task = kthread_run(aip_isr_task, hrx_dev, "AIP ISR Thread");
	if (IS_ERR(hrx_dev->aip_isr_task))
		return -1;

	return 0;
}

void aip_stop_isr_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;
	CC_MSG_t msg;

	up(&hrx_dev->aip_sem);
	kthread_stop(hrx_dev->aip_isr_task);
	do {
		ret = AMPMsgQ_DequeueRead(&hrx_dev->aip_msg_queue, &msg);
	} while (likely(ret == 1));
	sema_init(&hrx_dev->aip_sem, 0);
	ret = AMPMsgQ_Destroy(&hrx_dev->aip_msg_queue);
	if (unlikely(ret != S_OK))
		HRX_LOG(AIP_ERROR, "%s:%d: HRX MsgQ Destroy FAILED, err:%8x\n", __func__, __LINE__, ret);
}

int aip_isr_enable(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	AIP_BUFFER *pDataBuffer;
	int nSpace, nSize, nPairNum, nPair;
	void *pDataAddr;
	AIP_FIFO *pFifo;
	AIP_DMA_CMD_FIFO *pCmdFifo = NULL;
	AIP_DMA_CMD *pDmaCmd;
	unsigned int nPhyAddr0;
	int nShmOffset;

	rc = HRX_AIP_OK;

	aip_dhub_intr_enable(hrx_dev, 1);
		//check pointer
	pDataBuffer = &hrx_dev->aip_buf;
	if (pDataBuffer == NULL) {
		rc = HRX_AIP_EBADCALL;
		HRX_LOG(AIP_ERROR, "pDataBuffer is NULL RC=0x%x", rc);
		goto EXIT;
	}

	rc = aip_buffer_get_prespace_nowrap(pDataBuffer, &nSpace);
	if (rc != HRX_AIP_OK || nSpace < hrx_dev->nDmaChunkSize) {
		rc = HRX_AIP_EBADCALL;
		goto EXIT;
	}
	nSize = hrx_dev->nDmaChunkSize;

	pFifo = &hrx_dev->aip_fifo;
	if (pFifo) {
		pCmdFifo = pFifo->pDmaCmdFifo;
		rc = aip_dma_cmd_fifo_reset(pCmdFifo);
		if (rc != HRX_AIP_OK) {
			HRX_LOG(AIP_ERROR, "%s : RC=0x%x :%d", __func__, rc, __LINE__);
			goto EXIT;
		}
	}


	rc = aip_buffer_get_pairs_nr(pDataBuffer, (unsigned int *)&nPairNum);
	if (rc != HRX_AIP_OK) {
		HRX_LOG(AIP_ERROR, "%s : RC=0x%x :%d", __func__, rc, __LINE__);

		goto EXIT;
	}
	for (nPair = 0; nPair < nPairNum; nPair++) {
		rc = aip_dma_cmd_fifo_get_wr_dma_cmd(pCmdFifo, nPair, (void **)&pDmaCmd);
		AIP_CHKRC(rc);
		nShmOffset = pDataBuffer->nStartOffset[nPair] + pDataBuffer->nPreWrOffset[nPair];
		nPhyAddr0 = (uintptr_t)pDataBuffer->vpp_aip_buf_mem_handle.p_addr;

		rc = aip_dma_cmd_set(pDmaCmd, nPhyAddr0, nSize, 0, 0);
		AIP_CHKRC(rc);

		rc = aip_buffer_pre_wr_update(pDataBuffer, nPair, nSize);
		AIP_CHKRC(rc);
	}

	aip_dma_cmd_fifo_set_update_flag(pCmdFifo, 1);
	aip_dma_cmd_fifo_set_update_size(pCmdFifo, nSize);
	aip_dma_cmd_fifo_wr_update(pCmdFifo, 1);

    /* Launch two DMA to avoid hardware FIFO underflow */
	for (nPair = 0; nPair < nPairNum; nPair++) {
		rc = aip_dma_cmd_fifo_get_wr_dma_cmd(pCmdFifo, nPair, (void **)&pDmaCmd);
		AIP_CHKRC(rc);
		nShmOffset = pDataBuffer->nStartOffset[nPair] + pDataBuffer->nPreWrOffset[nPair];
		nPhyAddr0 = (uintptr_t)pDataBuffer->vpp_aip_buf_mem_handle.p_addr;

		rc = aip_dma_cmd_set(pDmaCmd, nPhyAddr0, nSize, 0, 0);
		AIP_CHKRC(rc);

		rc = aip_buffer_pre_wr_update(pDataBuffer, nPair, nSize);
		AIP_CHKRC(rc);
	}

	aip_dma_cmd_fifo_set_update_flag(pCmdFifo, 1);
	aip_dma_cmd_fifo_set_update_size(pCmdFifo, nSize);
	aip_dma_cmd_fifo_wr_update(pCmdFifo, 1);


    /* Launch another DMA to avoid software FIFO underflow */
	for (nPair = 0; nPair < nPairNum; nPair++) {
		rc = aip_dma_cmd_fifo_get_wr_dma_cmd(pCmdFifo, nPair, (void **)&pDmaCmd);
		AIP_CHKRC(rc);

		rc = aip_buffer_get_pre_wr_addr(pDataBuffer, nPair, &pDataAddr);
		AIP_CHKRC(rc);
		nShmOffset = pDataBuffer->nStartOffset[nPair] + pDataBuffer->nPreWrOffset[nPair];
		nPhyAddr0 = (uintptr_t) pDataBuffer->vpp_aip_buf_mem_handle.p_addr;

		rc = aip_dma_cmd_set(pDmaCmd, nPhyAddr0, nSize, 0, 0);
		HRX_LOG(AIP_DEBUG, "%s %d nPhyAddr0= 0x%x nSize =0x%x nShmOffset = %d\n",
			__func__, __LINE__, nPhyAddr0, nSize, nShmOffset);
		AIP_CHKRC(rc);

		rc = aip_buffer_pre_wr_update(pDataBuffer, nPair, nSize);
		AIP_CHKRC(rc);
	}

	aip_dma_cmd_fifo_set_update_flag(pCmdFifo, 1);
	aip_dma_cmd_fifo_set_update_size(pCmdFifo, nSize);
	aip_dma_cmd_fifo_wr_update(pCmdFifo, 1);

	hrx_dev->aip_i2s_pair = nPairNum;

	aip_call_start_cmd(hrx_dev);

EXIT:
	return rc;
}

int aip_isr_disable(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	int nPairNum;
	AIP_BUFFER *pDataBuffer;
	AIP_FIFO *pFifo;

	rc = HRX_AIP_OK;

	pDataBuffer = &hrx_dev->aip_buf;
	rc = aip_buffer_get_pairs_nr(pDataBuffer, (unsigned int *)&nPairNum);
	AIP_CHKRC(rc);

	aip_dhub_intr_enable(hrx_dev, 0);
	mdelay(1);

	aip_dhub_dataclear(hrx_dev);

	pFifo = &hrx_dev->aip_fifo;

	aip_call_stop_cmd(hrx_dev);
	mdelay(1);
	return rc;
}

int aip_isr_task(void *pParam)
{
	int rc;
	CC_MSG_t msg;
	u32 intsts;
	int chanId;
	HDL_semaphore *pSemHandle = NULL;
	static u32 intrNum;
	struct syna_hrx_v4l2_dev *hrx_dev = (struct syna_hrx_v4l2_dev *)pParam;

	pSemHandle = dhub_semaphore(hrx_dev->dhub);
	rc = -1; //S_OK;

	while (!kthread_should_stop()) {
		rc = down_interruptible(&hrx_dev->aip_sem);
		if (unlikely(rc < 0))
			return rc;

		memset(&msg, 0, sizeof(CC_MSG_t));
		rc = AMPMsgQ_ReadTry(&hrx_dev->aip_msg_queue, &msg);
		if (unlikely(rc != S_OK)) {
			HRX_LOG(AIP_DEBUG, "%s:%d Failed to read from aip msgQ\n", __func__, __LINE__);
			return -EFAULT;
		}
		AMPMsgQ_ReadFinish(&hrx_dev->aip_msg_queue);

		if (rc == S_OK) {
			intsts = msg.m_MsgID;

			chanId = avioDhubChMap_aio64b_MIC3_CH_W;
			if (bTST(intsts, chanId)) {
				intrNum++;
				HRX_LOG(AIP_DEBUG, "%s Received intr :%u intrNum = %d\n", __func__, intsts, intrNum);
				aip_data_isr(hrx_dev, chanId);
			}
			up(&hrx_dev->aip_main_sem);
		}
	}

	return 0;
}

/* process control tag from interrupt service */
static int aip_main_task_process_tag(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	AIP_BUFFER *pDataBuffer;
	AIP_AUDIO_TYPE *pAudioType;
	AIP_CTRL_TAG sCtrlTag;
	unsigned int nSamples, nFs;

	pDataBuffer = &hrx_dev->aip_buf;
	pAudioType = &pDataBuffer->sAudioType;

	while (1) {
		rc = aip_buffer_read_ctrl_tag(pDataBuffer, &sCtrlTag);
		if (rc == HRX_AIP_ENOTREADY || rc == HRX_AIP_EEMPTY)
			break;

		AIP_CHKRC(rc);

		switch (sCtrlTag.nCtrlTagType) {
		case AIP_TAG_OVERFLOW:
			nSamples = sCtrlTag.sCtrlTagUnion.sDataOverflow.nOverflowSize >> 2;
			nFs = pAudioType->nFs;
			rc = aip_buffer_update_timestamp(pDataBuffer, nSamples, nFs);
			AIP_CHKRC(rc);
			break;
		default:
			break;
		}
	}

	return rc;
}

static int aip_main_task_process(struct syna_hrx_v4l2_dev *hrx_dev)
{
	int rc;
	AIP_AUDIO_TYPE *pAudioType;
	AIP_BUFFER *pDataBuffer;
	int nFullness, nFullnessNoWrap;
	AIP_FRAME_DES *pFrameDes;

	unsigned int nFrameSize, nPairNum, nPair, nChanNr, nDmasize, nCpysize;
	void *pCallbackCtx;
	void *pSrc, *pDst;
	unsigned int *pSrc0, *pSrc1, *pSrc2, *pSrc3;
	unsigned int nPtsHi, nPtsLo;
	int  nShmOffset, nBurstSize, i;
	unsigned int *pA, *pB, *pC, *pD, *pData;
	unsigned int nA,  nB,  nData;
	unsigned int nDataType, nStreamID, nPayloadSize;
	bool bSync;

	void *tmp_data;
	void *ptmp_data;

	u8 bHbr;
	AIP_EVENT_INFO sEventInfo;

	pDataBuffer = &hrx_dev->aip_buf;
	pAudioType = &pDataBuffer->sAudioType;
	ptmp_data = (unsigned int *)pDataBuffer->tmp_buffer;

	while (1) {
		rc = aip_buffer_get_fullness(pDataBuffer, &nFullness);
		AIP_CHKRC(rc);
		/* no data left in buffer */
		if (nFullness < hrx_dev->nInputThres)
			break;

		rc = aip_main_task_process_tag(hrx_dev);

		rc = aip_buffer_copy_to_pad(pDataBuffer, hrx_dev->nInputThres, hrx_dev->aip_mem_list);
		AIP_CHKRC(rc);
		bSync = 0;
		nBurstSize = hrx_dev->nInputThres;

		/* Invalidate Cache */
		rc = aip_buffer_get_pairs_nr(pDataBuffer, &nPairNum);
		AIP_CHKRC(rc);
		rc = aip_buffer_get_audio_hbr(pDataBuffer, &bHbr);
		AIP_CHKRC(rc);
		rc = aip_buffer_get_fullness_nowrap(pDataBuffer, &nFullnessNoWrap);
		AIP_CHKRC(rc);
		HRX_LOG(AIP_DEBUG, " %s nFullness = %d nFullnessNoWrap =%d nInputThres = %d nPairNum = %d\n",
				__func__, nFullness, nFullnessNoWrap, hrx_dev->nInputThres, nPairNum);

		for (nPair = 0; nPair < nPairNum; nPair++) {
			rc = aip_buffer_get_rd_addr(pDataBuffer, nPair, &pSrc);
			AIP_CHKRC(rc);
			nShmOffset = pDataBuffer->nStartOffset[nPair]
				+ (u8 *)pSrc - (u8 *)pDataBuffer->pStart[nPair];
			HRX_LOG(AIP_DEBUG, " %s nShmOffset = 0x%x nStartOffset = 0x%x pSrc = 0x%p pStart = 0x%p\n",
					__func__, nShmOffset, pDataBuffer->nStartOffset[nPair],
					pSrc, pDataBuffer->pStart[nPair]);
			VPP_MEM_FlushCache(hrx_dev->aip_mem_list, &pDataBuffer->vpp_aip_buf_mem_handle, nFullnessNoWrap);
		}

		if (nPairNum == AIP_MAX_IN_PAIR_NR) {
			rc = aip_buffer_get_rd_addr(pDataBuffer, 0, (void **)&pSrc0);
			AIP_CHKRC(rc);
			rc = aip_buffer_get_rd_addr(pDataBuffer, 1, (void **)&pSrc1);
			AIP_CHKRC(rc);
			rc = aip_buffer_get_rd_addr(pDataBuffer, 2, (void **)&pSrc2);
			AIP_CHKRC(rc);
			rc = aip_buffer_get_rd_addr(pDataBuffer, 3, (void **)&pSrc3);
			AIP_CHKRC(rc);

			pA = (unsigned int *)pSrc0;
			pB = pA + 1;

			if (hrx_dev->eSignalStatus == AIP_SIGNAL_STATUS_UNSTABLE) {
				for (i = 0; i < (nBurstSize>>2)-4; i++) {
					nA = *pA >> 16;
					nB = *pB >> 16;
					if (nA == SYNC_WORD_PA && nB == SYNC_WORD_PB) {
						pC = pB + 1;
						nDataType = ((AIP_BURST_INFO *)pC)->nDataType;
						pData = pC + 2;
						nData = *pData >> 16;

						bSync = 1;
						break;
					}
					pA++;
					pB++;
				}
				if (!bSync) {
					hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_STABLE;
					hrx_dev->nInputThres   = PCM_BURST_SIZE;
					pAudioType->eDataFmt   = AIP_FORMAT_PCM;
					rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
					HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), detect as PCM format\n",
							__func__, __LINE__);
				}
			} else if (hrx_dev->eSignalStatus == AIP_SIGNAL_STATUS_STABLE && pAudioType->eDataFmt == AIP_FORMAT_PCM) {
				for (i = 0; i < (nBurstSize>>2)-1; i++) {
					nA = *pA >> 16;
					nB = *pB >> 16;
					if (nA == SYNC_WORD_PA && nB == SYNC_WORD_PB) {
						pC = pB + 2;
						pD = pB + 1;
						if ((*pC>>16) == 0xE000 && (*pD>>16) == 0x0) {//NULL PACKET
							dev_warn(hrx_dev->dev, "[AIP]:%s(%d), NULL packet detected\n", __func__, __LINE__);
							continue;
						} else {
							hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
							hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
							hrx_dev->nInputThres   = DEFAULT_BURST_SIZE;
							pAudioType->eDataFmt   = AIP_FORMAT_UNDEF;
							rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
							bSync = 1;
							dev_warn(hrx_dev->dev, "[AIP]:%s(%d), signal unstable, drop %d bytes data\n", __func__, __LINE__, nBurstSize);
							break;
						}
					}
					pA++;
					pB++;
				}
				if (!bSync) {
					if (pAudioType->eDecFmt == AIP_FORMAT_PCM) {
						if (hrx_dev->ePreSignalStatus == AIP_SIGNAL_STATUS_UNSTABLE) {
							sEventInfo.sFormatInfo.eDataFmt = AIP_FORMAT_PCM;
							pCallbackCtx = hrx_dev->pEventCtx;
							rc = hrx_dev->pEventCB(AIP_EVENT_FORMAT_CHANGE, (void *)&sEventInfo, pCallbackCtx);
							AIP_CHKRC(rc);
							HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), input audio format changed to PCM\n", __func__, __LINE__);
						}
						hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_STABLE;

						//request frame buffer
						aip_buffer_get_chan_num(pDataBuffer, &nChanNr);
						if (nChanNr == 6)
							nFrameSize = (nBurstSize*3) >> 1;
						else
							nFrameSize = nBurstSize << 1;
						if (pAudioType->nBitDepth == 24)
							nFrameSize = (nFrameSize * 3) >> 1;
						pCallbackCtx = hrx_dev->pFrameAllocCtx;
						rc = hrx_dev->pFrameAllocCB((void **)&pFrameDes, (int *)&nFrameSize, pCallbackCtx);
						if (rc != S_OK) {
							dev_warn(hrx_dev->dev, "[AIP]:%s(%d), allocate frame failed size %d\n", __func__, __LINE__, nFrameSize);
							return rc;
						}
						//process PCM data
						pDst = pFrameDes->pBuffer;
						for (i = 0; i < nBurstSize>>3; i++) {
							/* Do a channel re-org here because
							 * channel order in HDMI SPEC is
							 * L, R, LFE, C, Ls, Rs, Lb, Rb
							 * in Raw PCM Decoder is
							 * L, C, R, Ls, Rs, Lb, Rb, LEF.*/
							if (pAudioType->nBitDepth == 16) {
								// L
								*(u16 *)pDst = (*pSrc0 >> 16);
								pDst = (u16 *)pDst + 1;

								// R
								*(u16 *)pDst = (*(pSrc0+1) >> 16);
								pDst = (u16 *)pDst + 1;

								// C
								*(u16 *)pDst = (*(pSrc1+1) >> 16);
								pDst = (u16 *)pDst + 1;

								// LFE
								*(u16 *)pDst = (*(pSrc1) >> 16);
								pDst = (u16 *)pDst + 1;

								// Ls
								*(u16 *)pDst = (*pSrc2 >> 16);
								pDst = (u16 *)pDst + 1;

								// Rs
								*(u16 *)pDst = (*(pSrc2+1) >> 16);
								pDst = (u16 *)pDst + 1;

								if (nChanNr == 8) {
									// Lb
									*(u16 *)pDst = (*pSrc3 >> 16);
									pDst = (u16 *)pDst + 1;

									// Rb
									*(u16 *)pDst = (*(pSrc3+1) >> 16);
									pDst = (u16 *)pDst + 1;
								}
							} else if (pAudioType->nBitDepth == 24) {
								// L + R
								*(u16 *)pDst = (*pSrc0 >> 8);
								pDst = (u16 *)pDst + 1;
								*(u32 *)pDst = (u32)((*(pSrc0+1) &
											0xffffff00) +
										(u8)(*pSrc0>>24));
								pDst = (u32 *)pDst + 1;

								// C + LFE
								*(u16 *)pDst = (*(pSrc1+1) >> 8);
								pDst = (u16 *)pDst + 1;
								*(u32 *)pDst = (u32)((*pSrc1 &
											0xffffff00) +
										(u8)(*(pSrc1+1)>>24));
								pDst = (u32 *)pDst + 1;

								//Ls + Rs
								*(u16 *)pDst = (*(pSrc2) >> 8);
								pDst = (u16 *)pDst + 1;
								*(u32 *)pDst = (u32)((*(pSrc2+1) &
											0xffffff00) +
										(u8)(*(pSrc2)>>24));
								pDst = (u32 *)pDst + 1;

								// 8 ch
								if (nChanNr == 8) {
									// Lb + Rb
									*(u16 *)pDst = (*(pSrc3) >> 8);
									pDst = (u16 *)pDst + 1;
									*(u32 *)pDst = (u32)((*(pSrc3+1) &
												0xffffff00) +
											(u8)(*(pSrc3)>>24));
									pDst = (u32 *)pDst + 1;
								}
							}
							pSrc0 = pSrc0 + 2;
							pSrc1 = pSrc1 + 2;
							pSrc2 = pSrc2 + 2;
							pSrc3 = pSrc3 + 2;
						}
						//insert frm des
						pFrameDes->nFillLen = nFrameSize;
						pFrameDes->nOffset = 0;
						rc = aip_buffer_get_timestamp(pDataBuffer, &nPtsHi, &nPtsLo);
						AIP_CHKRC(rc);
						pFrameDes->sTimeStamp.nPtsHi = nPtsHi;
						pFrameDes->sTimeStamp.nPtsLo = nPtsLo;

						//update input buffer read pointer
						rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
						AIP_CHKRC(rc);
						//notify upper level user
						pCallbackCtx = hrx_dev->pEventCtx;
						rc = hrx_dev->pEventCB(AIP_EVENT_FRAME_READY, NULL, pCallbackCtx);
						AIP_CHKRC(rc);
						HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), produce a frame, size %d, pts [0x%x][0x%x]\n", __func__, __LINE__, nFrameSize, nPtsHi, nPtsLo);
					} else {
						sEventInfo.sFormatInfo.eDataFmt = AIP_FORMAT_PCM;
						pCallbackCtx = hrx_dev->pEventCtx;
						rc = hrx_dev->pEventCB(AIP_EVENT_FORMAT_CHANGE, (void *)&sEventInfo, pCallbackCtx);
						AIP_CHKRC(rc);
						HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), input audio format changed to PCM\n",
								__func__, __LINE__);

						hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_STABLE;
						// update input buffer read pointer
						rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
						AIP_CHKRC(rc);
					}
				}
			} else {
					hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
					hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
					hrx_dev->nInputThres   = DEFAULT_BURST_SIZE;
					pAudioType->eDataFmt   = AIP_FORMAT_UNDEF;
					rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);

					dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unstable data\n",
							__func__, __LINE__);
			}
		} else if (bHbr) {
			rc = aip_buffer_get_rd_addr(pDataBuffer, 0, &pSrc);
			AIP_CHKRC(rc);
			pA = (unsigned int *)pSrc;
			pB = pA;

			if (hrx_dev->eSignalStatus == AIP_SIGNAL_STATUS_UNSTABLE) {
				for (i = 0; i < (nBurstSize>>3)-4; i++) {
					nA = *pA & 0xFFFF;
					nB = *pA >> 16;
					if (nA == SYNC_WORD_PA && nB == SYNC_WORD_PB) {
						pC = (pB + 1);
						nData = *pC << 16;
						nDataType = ((AIP_BURST_INFO *)&nData)->nDataType;
						pData = pC + 1;
						nData = *pData & 0xFFFF;
						HRX_LOG(AIP_ERROR, "[AIP]:%s(%d), no format matched...END Type:%d, SyncWord: %x\n",
							__func__, __LINE__, nDataType, nData);
						bSync = 1;
						break;
					}

					pA++;
					pB++;
				}
				if (!bSync) {
					//hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_STABLE;
					//hrx_dev->nInputThres   = DOLBY_MAT_BURST_SIZE;
					//pAudioType->eDataFmt   = AIP_FORMAT_PCM;
					//pAudioType->eDecFmt = AIP_FORMAT_PCM;
					rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
					HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), detect as PCM format : %d\n",
						__func__, __LINE__, nBurstSize);
				}
			} else if (hrx_dev->eSignalStatus == AIP_SIGNAL_STATUS_STABLE) {
				nA = *pA & 0xFFFF;
				nB = *pA >> 16;
				if (nA == SYNC_WORD_PA && nB == SYNC_WORD_PB) {
					pC = (pB + 1);
					pD = pC;
					nData = *pC << 16;
					nDataType = ((AIP_BURST_INFO *)&nData)->nDataType;
					nStreamID = ((AIP_BURST_INFO *)&nData)->nStreamNo;
					nPayloadSize = *pD >> 16;
					if (nDataType == 0) {// NULL
						rc = aip_buffer_rd_update(pDataBuffer, 16);
						dev_warn(hrx_dev->dev, "[AIP]:%s(%d), NULL packect\n",
								__func__, __LINE__);
					} else if (nDataType == 3) {// PAUSE
						rc = aip_buffer_rd_update(pDataBuffer, ((nPayloadSize>>3)+4)<<2);
						dev_warn(hrx_dev->dev, "[AIP]:%s(%d), pause burst\n",
								__func__, __LINE__);
					} else {// NOT SUPPORTED
						rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
						dev_warn(hrx_dev->dev, "[AIP]:%s(%d), Not supported data format\n",
								__func__, __LINE__);
					}
				} else {
					hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
					hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
					hrx_dev->nInputThres   = DEFAULT_BURST_SIZE;
					pAudioType->eDataFmt   = AIP_FORMAT_UNDEF;
					rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
					dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unstable data\n",
							__func__, __LINE__);
				}
			}
		} else {
			rc = aip_buffer_get_rd_addr(pDataBuffer, 0, &pSrc);
			AIP_CHKRC(rc);

			pA = (unsigned int *)pSrc;
			pB = pA + 1;
			if (hrx_dev->eSignalStatus == AIP_SIGNAL_STATUS_UNSTABLE) {
				if (!bSync) {
					hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_STABLE;
					hrx_dev->nInputThres   = hrx_dev->in_data.period_bytes * 2;
					pAudioType->eDataFmt   = AIP_FORMAT_PCM;
					rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
					HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), detect as PCM format\n", __func__, __LINE__);
				}
			} else if (hrx_dev->eSignalStatus == AIP_SIGNAL_STATUS_STABLE && pAudioType->eDataFmt == AIP_FORMAT_PCM) {
				for (i = 0; i < (nBurstSize>>2)-1; i++) {
					nA = *pA >> 16;
					nB = *pB >> 16;
					if (nA == SYNC_WORD_PA && nB == SYNC_WORD_PB) {
						pC = pB + 1;
						pD = pC + 1;
						if ((*pC>>16) == 0xE000 && (*pD>>16) == 0x0) {// NULL PACKET
							HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), NULL packet detected\n",
									__func__, __LINE__);
							continue;
						} else {
							hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
							hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
							hrx_dev->nInputThres   = DEFAULT_BURST_SIZE;
							pAudioType->eDataFmt   = AIP_FORMAT_UNDEF;
							rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
							bSync = 1;
							HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), signal unstable, drop %d bytes data\n",
									__func__, __LINE__, nBurstSize);
							break;
						}
					}
					pA++;
					pB++;
				}
				if (!bSync) {
					if (pAudioType->eDecFmt == AIP_FORMAT_PCM) {
						if (hrx_dev->ePreSignalStatus == AIP_SIGNAL_STATUS_UNSTABLE) {
							sEventInfo.sFormatInfo.eDataFmt = AIP_FORMAT_PCM;
							pCallbackCtx = hrx_dev->pEventCtx;
							rc = hrx_dev->pEventCB(AIP_EVENT_FORMAT_CHANGE, (void *)&sEventInfo, pCallbackCtx);
							AIP_CHKRC(rc);
							HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), input audio format changed to PCM\n", __func__, __LINE__);
						}
						hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_STABLE;

						if (pAudioType->nBitDepth == 32) {
							nFrameSize = nBurstSize;
						} else if (pAudioType->nBitDepth == 16) {
							nFrameSize = nBurstSize >> 1;
							HRX_LOG(AIP_DEBUG, "16bit set for AIP data process : %d\n", nFrameSize);

						} else if (pAudioType->nBitDepth == 24) {
							nFrameSize = (nBurstSize * 3) >> 2;
							HRX_LOG(AIP_DEBUG, "24bit set for AIP data process  : %d\n", nFrameSize);
						}
						//request frame buffer
						pCallbackCtx = hrx_dev->pFrameAllocCtx;
						nCpysize = 0;
						//process PCM data
						/* notify upper level user */
						pData = (unsigned int *)pSrc;
						tmp_data = (unsigned int *)ptmp_data;
						memset(tmp_data, 0, nBurstSize);


						switch (hrx_dev->eMonoMode) {
						case AIP_MONO_LEFT:
							if (pAudioType->nBitDepth == 16) {
								for (i = 0; i < nBurstSize>>2; i++) {
									*(u16 *)tmp_data = (*pData >> 16);
									tmp_data = (u16 *)tmp_data + 1;
									*(u16 *)tmp_data = (*pData >> 16);
									tmp_data = (u16 *)tmp_data + 1;
									pData += 2;
									i++;
								}
							} else if (pAudioType->nBitDepth == 24) {
									//two samples one time
								*(u16 *)tmp_data = (u16) (*pData >> 8);
								tmp_data = (u16 *)tmp_data + 1;
								*(u32 *)tmp_data = (u32)((u8)(*pData >> 24)+
									(*(pData)&0xffffff00));
								tmp_data = (u32 *)tmp_data + 1;
								pData += 2;
								i++;
							} else if (pAudioType->nBitDepth == 32) {
								//No need to process
							} else {
								dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unsupported bit depth\n", __func__, __LINE__);
							}
							break;
						case AIP_MONO_RGHT:
							if (pAudioType->nBitDepth == 16) {
								for (i = 0; i < nBurstSize>>2; i++) {
									pData++;
									*(u16 *)tmp_data = (*pData >> 16);
									tmp_data = (u16 *)tmp_data + 1;

									*(u16 *)tmp_data = (*pData >> 16);
									tmp_data = (u16 *)tmp_data + 1;
									pData++;
									i++;
								}
							} else if (pAudioType->nBitDepth == 24) {
									//two samples one time
								pData++;
								*(u16 *)tmp_data = (u16) (*pData >> 8);
								tmp_data = (u16 *)tmp_data + 1;
								*(u32 *)tmp_data = (u32)((u8)(*pData >> 24)+
									(*(pData)&0xffffff00));
								tmp_data = (u32 *)tmp_data + 1;
								pData++;
								i++;
							} else if (pAudioType->nBitDepth == 32) {
								//No need to process
							} else {
								dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unsupported bit depth\n", __func__, __LINE__);
							}
							break;
						default:
							if (pAudioType->nBitDepth == 16) {
								for (i = 0; i < nBurstSize>>2; i++) {
									*(u16 *)tmp_data = (*pData >> 16);
									tmp_data = (u16 *)tmp_data + 1;
									pData++;

									*(u16 *)tmp_data = (*pData >> 16);
									tmp_data = (u16 *)tmp_data + 1;
									pData++;

									i++;
								}
							} else if (pAudioType->nBitDepth == 24) {
									//two samples one time
								unsigned int *pData_bkp;

								*(u16 *)tmp_data = (u16) (*pData >> 8);
								tmp_data = (u16 *)tmp_data + 1;
									/*TBD*/
								pData_bkp = ++pData;
								*(u32 *)tmp_data = (u32)((u8)(*pData >> 24) + (*(pData_bkp)&0xffffff00));
								pData++;
								tmp_data = (u32 *)tmp_data + 1;
								i++;
							} else if (pAudioType->nBitDepth == 32) {
								//No need to process
							} else {
								dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unsupported bit depth\n", __func__, __LINE__);
							}
							break;
						}
						while (nCpysize < nFrameSize) {

							rc = hrx_dev->pFrameAllocCB((void **)&pFrameDes,
								(int *)&nDmasize,
								pCallbackCtx);
							if (pFrameDes == NULL) {
								HRX_LOG(AIP_ERROR, "[AIP]:%s(%d), allocate frame desc failed\n", __func__, __LINE__);
								return rc;
							}

							pDst = pFrameDes->pBuffer;

							if (pDst == NULL) {
								HRX_LOG(AIP_ERROR, "[AIP]:%s(%d), allocate frame failed size %d\n", __func__, __LINE__, nDmasize);
								return rc;
							}
							//TBC::Workaround. Refine the code
							if (hrx_dev->aip_hold_enable == 0) {
								if ((pAudioType->nBitDepth == 16) || (pAudioType->nBitDepth == 24))
									memcpy((void *)pDst, (void *)ptmp_data+nCpysize, nDmasize);
								else if (pAudioType->nBitDepth == 32)
									memcpy((void *)pDst, (void *)pSrc+nCpysize, nDmasize);
								else
									dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unsupported bit depth\n", __func__, __LINE__);
							} else {
								if ((pAudioType->nBitDepth == 16) || (pAudioType->nBitDepth == 24) || (pAudioType->nBitDepth == 32))
									memset((void *)pDst, 0, nDmasize);
								else
									dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unsupported bit depth\n", __func__, __LINE__);
							}
							pFrameDes->nFillLen = nDmasize;
							pFrameDes->nOffset = 0;
							rc = aip_buffer_get_timestamp(pDataBuffer, &nPtsHi, &nPtsLo);
							AIP_CHKRC(rc);
							pFrameDes->sTimeStamp.nPtsHi = nPtsHi;
							pFrameDes->sTimeStamp.nPtsLo = nPtsLo;

							pCallbackCtx = hrx_dev->pEventCtx;
							rc = hrx_dev->pEventCB(AIP_EVENT_FRAME_READY, NULL, pCallbackCtx);
							AIP_CHKRC(rc);
							nCpysize += nDmasize;
						}
					//update input buffer read pointer
					rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
					AIP_CHKRC(rc);
					HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), produce a frame, size %d, pts [0x%x][0x%x] Buffer = %p\n", __func__, __LINE__, nFrameSize, nPtsHi, nPtsLo, pFrameDes->pBuffer);
					} else {
						sEventInfo.sFormatInfo.eDataFmt = AIP_FORMAT_PCM;
						pCallbackCtx = hrx_dev->pEventCtx;
						rc = hrx_dev->pEventCB(AIP_EVENT_FORMAT_CHANGE, (void *)&sEventInfo, pCallbackCtx);
						AIP_CHKRC(rc);
						HRX_LOG(AIP_DEBUG, "[AIP]:%s(%d), input audio format changed to PCM\n",
								__func__, __LINE__);
						hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_STABLE;
						//update input buffer read pointer
						rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
						AIP_CHKRC(rc);
					}
				}
			} else {
					hrx_dev->eSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
					hrx_dev->ePreSignalStatus = AIP_SIGNAL_STATUS_UNSTABLE;
					hrx_dev->nInputThres   = DEFAULT_BURST_SIZE;
					pAudioType->eDataFmt   = AIP_FORMAT_UNDEF;
					rc = aip_buffer_rd_update(pDataBuffer, nBurstSize);
					dev_warn(hrx_dev->dev, "[AIP]:%s(%d), unstable data\n", __func__, __LINE__);
			}
		}
	}
	return rc;
}

static int aip_main_task(void *pParam)
{
	int rc;
	struct syna_hrx_v4l2_dev *hrx_dev;

	hrx_dev = (struct syna_hrx_v4l2_dev *)pParam;
	while (!kthread_should_stop()) {

		rc = down_interruptible(&hrx_dev->aip_main_sem);
		if (unlikely(rc < 0))
			return rc;

		/* return if AIP is not running */
		if (hrx_dev->aip_status == AIP_STATUS_CLOSE)
			return 0;

		if (hrx_dev->aip_status != AIP_STATUS_START)
			continue;

		rc = aip_main_task_process(hrx_dev);

	}
	return rc;
}

int aip_create_main_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;

	sema_init(&hrx_dev->aip_main_sem, 0);

	ret = AMPMsgQ_Init(&hrx_dev->aip_main_msg_queue, HRX_ISR_MSGQ_SIZE);
	if (unlikely(ret != S_OK)) {
		HRX_LOG(AIP_ERROR, "hrx_msg_queue init: failed, err:%8x\n", ret);
		return -1;
	}

	hrx_dev->aip_main_task = kthread_run(aip_main_task, hrx_dev, "AIP MAIN ISR Thread");
	if (IS_ERR(hrx_dev->aip_main_task))
		return -1;

	return 0;
}

void aip_stop_main_task(struct syna_hrx_v4l2_dev *hrx_dev)
{
	unsigned int ret;
	CC_MSG_t msg;

	up(&hrx_dev->aip_main_sem);
	kthread_stop(hrx_dev->aip_main_task);

	do {
		ret = AMPMsgQ_DequeueRead(&hrx_dev->aip_main_msg_queue, &msg);
	} while (likely(ret == 1));
	sema_init(&hrx_dev->aip_main_sem, 0);
	ret = AMPMsgQ_Destroy(&hrx_dev->aip_main_msg_queue);
	if (unlikely(ret != S_OK))
		HRX_LOG(AIP_ERROR, "%s:%d: HRX MsgQ Destroy FAILED, err:%8x\n", __func__, __LINE__, ret);
}
