// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#include "hrx-drv.h"

static int aip_reset_audio_type(AIP_AUDIO_TYPE *pAudioType)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pAudioType == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pAudioType->eDataFmt = AIP_FORMAT_INVALID;
	pAudioType->nChanNr = 2;
	pAudioType->nBitDepth = 16;
	pAudioType->nFs = 48000;

EXIT:
	return rc;
}

int aip_buffer_create(AIP_BUFFER *pAipBuffer, unsigned int nBufSize, unsigned int nPadSize, VPP_MEM_LIST *memlist)
{
	int rc, i;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;
	if (nBufSize <= 0 || nPadSize <= 0) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pAipBuffer->vpp_aip_buf_mem_handle.size = nBufSize + nPadSize;
	VPP_MEM_AllocateMemory(memlist, VPP_MEM_TYPE_DMA,
		&pAipBuffer->vpp_aip_buf_mem_handle, 0);

	pAipBuffer->pBufferBase = pAipBuffer->vpp_aip_buf_mem_handle.k_addr;
	pAipBuffer->pStart[0] = pAipBuffer->vpp_aip_buf_mem_handle.k_addr;
	pAipBuffer->nStartOffset[0] = 0;
	for (i = 1; i < AIP_MAX_IN_PAIR_NR; i++) {
		pAipBuffer->pStart[i] = NULL;
		pAipBuffer->nStartOffset[i] = 0;
	}
	pAipBuffer->nPairsNr = 1;
	pAipBuffer->nAllocLen = nBufSize + nPadSize;
	pAipBuffer->nOrigBufSize = nBufSize;
	pAipBuffer->nBufSize = nBufSize;
	pAipBuffer->nOrigPadSize = nPadSize;
	pAipBuffer->nPadSize = nPadSize;
	pAipBuffer->nWrDataCnt = 0;
	pAipBuffer->nRdDataCnt = 0;

    /* control tag buffer */
	pCtrlTagBuf = &pAipBuffer->sCtrlTagBuf;
	pCtrlTagBuf->pCtrlTagStart =
	(AIP_CTRL_TAG *)kzalloc(sizeof(AIP_CTRL_TAG)*AIP_CTRL_TAG_SIZE, GFP_KERNEL);
	if (pCtrlTagBuf->pCtrlTagStart == NULL) {
		rc = HRX_AIP_ENOMEM;
		goto EXIT;
	}
	memset(pCtrlTagBuf->pCtrlTagStart, 0, sizeof(AIP_CTRL_TAG)*AIP_CTRL_TAG_SIZE);
	pCtrlTagBuf->nIndexSize = AIP_CTRL_TAG_SIZE;
	pCtrlTagBuf->nRdIndex = 0;
	pCtrlTagBuf->nWrIndex = 0;

	aip_reset_audio_type(&pAipBuffer->sAudioType);

	pAipBuffer->tmp_buffer = kmalloc(nBufSize + nPadSize, GFP_KERNEL);

EXIT:
	return rc;
}

int aip_bufer_split_to_fourpairs(AIP_BUFFER *pBuffer, unsigned int nChanNr)
{
	int rc;
	int i, nSizePerPair;
	void *nBufferBase;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nSizePerPair = pBuffer->nAllocLen >> 2;
	nBufferBase = (void *)(pBuffer->pBufferBase);
	for (i = 0; i < AIP_MAX_IN_PAIR_NR; i++) {
		pBuffer->pStart[i] = (void *)nBufferBase;
		nBufferBase += nSizePerPair;
		pBuffer->nPreWrOffset[i] = 0;
		pBuffer->nWrOffset[i] = 0;
		pBuffer->nDirtyBytes[i] = 0;
		pBuffer->nStartOffset[i] = i*nSizePerPair;
	}
	pBuffer->nPadSize >>= 2;
	pBuffer->nBufSize >>= 2;
	pBuffer->nPairsNr = AIP_MAX_IN_PAIR_NR;
	pBuffer->nRdOffset = 0;
	pBuffer->nWrDataCnt = 0;
	pBuffer->nRdDataCnt = 0;

	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;
	pCtrlTagBuf->nRdIndex = 0;
	pCtrlTagBuf->nWrIndex = 0;

	aip_buffer_set_chan_num(pBuffer, nChanNr);

EXIT:
	return rc;
}

int aip_buffer_mergeto_onepair(AIP_BUFFER *pBuffer)
{
	int rc;

	rc = HRX_AIP_OK;

	return rc;

}

int aip_buffer_free(AIP_BUFFER *pBuffer, VPP_MEM_LIST *memlist)
{
	int rc;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;

	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;

	kfree(pBuffer->tmp_buffer);
	kfree(pCtrlTagBuf->pCtrlTagStart);

	/* de-allocate memory for the buffer */
	if (!pBuffer->pBufferBase)
		return (HRX_AIP_EBADCALL);

	VPP_MEM_FreeMemory(memlist,
			VPP_MEM_TYPE_DMA,
			&pBuffer->vpp_aip_buf_mem_handle);

	pBuffer->pBufferBase = NULL;

	return rc;
}

int aip_buffer_reset(AIP_BUFFER *pBuffer)
{
	int rc, i;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pBuffer->nPairsNr = 1;
	pBuffer->nBufSize = pBuffer->nOrigBufSize;
	pBuffer->nPadSize = pBuffer->nOrigPadSize;
	pBuffer->nRdOffset = 0;
	for (i = 0; i < AIP_MAX_IN_PAIR_NR; i++) {
		pBuffer->nPreWrOffset[i] = 0;
		pBuffer->nWrOffset[i] = 0;
		pBuffer->nDirtyBytes[i] = 0;
	}
	pBuffer->nWrDataCnt = 0;
	pBuffer->nRdDataCnt = 0;

	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;
	pCtrlTagBuf->nRdIndex = 0;
	pCtrlTagBuf->nWrIndex = 0;

	aip_reset_audio_type(&pBuffer->sAudioType);

	pBuffer->sLastTimeStamp.nPtsHi = 0;
	pBuffer->sLastTimeStamp.nPtsLo = 0;
	pBuffer->sTimeStamp.nPtsHi = 0;
	pBuffer->sTimeStamp.nPtsLo = 0;
EXIT:

	return rc;
}

int aip_buffer_get_fullness(AIP_BUFFER *pBuffer, int *pFullness)
{
	int rc;
	int i, nPair, nFullness, nTemp;
	int nRdOff, nWrOff, nBufSize;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || pFullness == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nFullness = 0x40000000;
	nPair = pBuffer->nPairsNr;
	nRdOff = pBuffer->nRdOffset;
	nBufSize = pBuffer->nBufSize;
	for (i = 0; i < nPair; i++) {
		nWrOff = pBuffer->nWrOffset[i];
		nTemp = (nWrOff >= nRdOff) ?
		(nWrOff - nRdOff) :
		(nWrOff + nBufSize - nRdOff);
		nFullness = MIN(nFullness, nTemp);
	}
	*pFullness = nFullness;

EXIT:

	return rc;
}

int aip_buffer_get_fullness_nowrap(AIP_BUFFER *pBuffer, int *pFullness)
{
	int rc;
	int i, nPair, nFullness, nTemp;
	int nRdOff, nWrOff, nBufSize;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || pFullness == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nFullness = 0x40000000;
	nPair = pBuffer->nPairsNr;
	nRdOff = pBuffer->nRdOffset;
	nBufSize = pBuffer->nBufSize;
	for (i = 0; i < nPair; i++) {
		nWrOff = pBuffer->nWrOffset[i];
		nTemp = (nWrOff < nRdOff) ? (nBufSize-nRdOff) : (nWrOff-nRdOff);
		nFullness = MIN(nFullness, nTemp);
	}
	*pFullness = nFullness;

EXIT:
	return rc;
}

int aip_buffer_get_space(AIP_BUFFER *pBuffer, int *pSpace)
{
	int rc;
	int i, nPair, nSpace, nTemp;
	int nRdOff, nWrOff, nBufSize;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || pSpace == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nSpace = 0x40000000;
	nPair = pBuffer->nPairsNr;
	nRdOff = pBuffer->nRdOffset;
	nBufSize = pBuffer->nBufSize;
	for (i = 0; i < nPair; i++) {
		nWrOff = pBuffer->nWrOffset[i];
		nTemp = (nWrOff >= nRdOff) ?
		(nRdOff - nWrOff + nBufSize) :
		(nRdOff - nWrOff);
		nSpace = MIN(nSpace, nTemp);
	}
	*pSpace = nSpace - 1;
EXIT:
	return rc;
}

int aip_buffer_get_space_nowrap(AIP_BUFFER *pBuffer, int *pSpace)
{
	int rc;
	int i, nPair, nSpace, nTemp;
	int nRdOff, nWrOff, nBufSize;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || pSpace == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nSpace = 0x40000000;
	nPair = pBuffer->nPairsNr;
	nRdOff = pBuffer->nRdOffset;
	nBufSize = pBuffer->nBufSize;
	for (i = 0; i < nPair; i++) {
		nWrOff = pBuffer->nWrOffset[i];
		nTemp = (nWrOff >= nRdOff) ?
		(nBufSize - nWrOff) :
		(nRdOff - nWrOff);
		nSpace = MIN(nSpace, nTemp);
	}
	*pSpace = nSpace - 1;
EXIT:
	return rc;
}

int aip_buffer_get_prespace(AIP_BUFFER *pBuffer, int *pSpace)
{
	int rc;
	int i, nPair, nSpace, nTemp;
	int nRdOff, nWrOff, nBufSize;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pSpace == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nSpace = 0x40000000;
	nPair = pBuffer->nPairsNr;
	nRdOff = pBuffer->nRdOffset;
	nBufSize = pBuffer->nBufSize;
	for (i = 0; i < nPair; i++) {
		nWrOff = pBuffer->nPreWrOffset[i];
		nTemp = (nWrOff >= nRdOff) ?
		(nRdOff - nWrOff + nBufSize) :
		(nRdOff - nWrOff);
		nSpace = MIN(nSpace, nTemp);
	}
	*pSpace = nSpace - 1;
EXIT:
	return rc;
}

int aip_buffer_get_prespace_nowrap(AIP_BUFFER *pBuffer, int *pSpace)
{
	int rc;
	int i, nPair, nSpace, nTemp;
	int nRdOff, nWrOff, nBufSize;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || pSpace == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nSpace = 0x40000000;
	nPair = pBuffer->nPairsNr;
	nRdOff = pBuffer->nRdOffset;
	nBufSize = pBuffer->nBufSize;
	for (i = 0; i < nPair; i++) {
		nWrOff = pBuffer->nPreWrOffset[i];
		nTemp = (nWrOff >= nRdOff) ?
		(nBufSize - nWrOff) :
		(nRdOff - nWrOff - 1);
		nTemp = (nRdOff == 0) ? nTemp - 1 : nTemp;
		nSpace = MIN(nSpace, nTemp);
	}
	*pSpace = nSpace;
EXIT:
	return rc;
}

int aip_buffer_get_rd_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppRdAddr)
{
	int rc;
	int nRdOff;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || ppRdAddr == NULL || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nRdOff = pBuffer->nRdOffset;
	*ppRdAddr = (void *)((char *)pBuffer->pStart[nPair] + nRdOff);

EXIT:

	return rc;
}

int aip_buffer_get_wr_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr)
{
	int rc;
	int nWrOff;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || ppWrAddr == NULL || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nWrOff = pBuffer->nWrOffset[nPair];
	*ppWrAddr = (void *)((char *)pBuffer->pStart[nPair] + nWrOff);
EXIT:
	return rc;
}

int aip_buffer_get_pre_wr_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr)
{
	int rc;
	int nPreWrOff;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || ppWrAddr == NULL || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nPreWrOff = pBuffer->nPreWrOffset[nPair];
	*ppWrAddr = (void *)((char *)pBuffer->pStart[nPair] + nPreWrOff);
EXIT:
	return rc;
}

int aip_buffer_get_start_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || ppWrAddr == NULL || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	*ppWrAddr = (void *)((char *)pBuffer->pStart[nPair]);
EXIT:

	return rc;
}

int aip_buffer_get_end_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || ppWrAddr == NULL || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	*ppWrAddr = (void *)((char *)pBuffer->pStart[nPair] + pBuffer->nBufSize);
EXIT:
	return rc;
}

int aip_buffer_get_inc_addr(AIP_BUFFER *pBuffer, unsigned int nPair, int nInc, void *pRef, void **ppAddr)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || ppAddr == NULL || nPair >= AIP_MAX_IN_PAIR_NR || nInc < 0) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	if ((char *)pRef >= (char *)pBuffer->pStart[nPair]+pBuffer->nBufSize) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	if (((char *)pRef + nInc) < ((char *)pBuffer->pStart[nPair]+pBuffer->nBufSize))
		*ppAddr = (void *)((char *)pRef + nInc);
	else
		*ppAddr = (void *)((char *)pBuffer->pStart[nPair] + ((char *)pRef - (char *)pBuffer->pStart[nPair] + nInc) % (pBuffer->nBufSize));
EXIT:

	return rc;
}

int aip_buffer_rd_update(AIP_BUFFER *pBuffer, int nAdv)
{
	int rc;
	int nRdOff, nBufSize;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || nAdv < 0) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nRdOff = pBuffer->nRdOffset;
	nBufSize = pBuffer->nBufSize;
	nRdOff = (nRdOff + nAdv >= nBufSize) ?
		(nRdOff + nAdv - nBufSize) : (nRdOff + nAdv);
	pBuffer->nRdOffset = nRdOff;
	pBuffer->nRdDataCnt += nAdv;
EXIT:
	return rc;
}

int aip_buffer_wr_update(AIP_BUFFER *pBuffer, unsigned int nPair, int nAdv)
{
	int rc;
	int nWrOff, nBufSize, cAdv;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || nAdv < 0 || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	//Delaying update by one interrupt
	cAdv = nAdv;
	nAdv = pBuffer->nDirtyBytes[nPair];
	pBuffer->nDirtyBytes[nPair] = cAdv;

	nWrOff = pBuffer->nWrOffset[nPair];
	nBufSize = pBuffer->nBufSize;
	nWrOff = (nWrOff + nAdv >= nBufSize) ?
	(nWrOff + nAdv - nBufSize) : (nWrOff + nAdv);
	pBuffer->nWrOffset[nPair] = nWrOff;

	if (nPair == 0)
		pBuffer->nWrDataCnt += nAdv;

	pBuffer->bTimeStampUpdated = FALSE;
EXIT:
	return rc;
}

int aip_buffer_pre_wr_update(AIP_BUFFER *pBuffer, unsigned int nPair, int nAdv)
{
	int rc;
	int nWrOff, nBufSize;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || nAdv < 0 || nPair >= AIP_MAX_IN_PAIR_NR) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	nWrOff = pBuffer->nPreWrOffset[nPair];
	nBufSize = pBuffer->nBufSize;
	nWrOff = (nWrOff + nAdv >= nBufSize) ?
	(nWrOff + nAdv - nBufSize) : (nWrOff + nAdv);
	pBuffer->nPreWrOffset[nPair] = nWrOff;

EXIT:
	return rc;
}

int aip_buffer_get_pairs_nr(AIP_BUFFER *pBuffer, unsigned int *pPairs)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pPairs == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	*pPairs = pBuffer->nPairsNr;
EXIT:
	return rc;
}

int aip_buffer_set_timestamp(AIP_BUFFER *pBuffer, unsigned int nPtsHi, unsigned int nPtsLo)
{
	int rc;
	unsigned int nTimeStampDelta;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nTimeStampDelta = pBuffer->nTimeStampDelta;

	pBuffer->sLastTimeStamp.nPtsHi = pBuffer->sTimeStamp.nPtsHi;
	pBuffer->sLastTimeStamp.nPtsLo = pBuffer->sTimeStamp.nPtsLo;

	if (nPtsLo > nTimeStampDelta) {
		nPtsLo -= nTimeStampDelta;
	} else if (nPtsHi & 1) {
		nPtsHi = 0;
		nPtsLo = (u64)(0x100000000LL - nTimeStampDelta) + nPtsLo;
	} else {
		nPtsHi = 1;
		nPtsLo = 0xffffffff - nTimeStampDelta + nPtsLo;
	}

    /* the msb bit treats as valid bit */
	pBuffer->sTimeStamp.nPtsHi = (nPtsHi & 0x1) | 0x80000000;
	pBuffer->sTimeStamp.nPtsLo = nPtsLo;

	pBuffer->bTimeStampUpdated = TRUE;
EXIT:
	return rc;
}

int aip_buffer_get_timestamp(AIP_BUFFER *pBuffer, unsigned int *pPtsHi, unsigned int *pPtsLo)
{
	int rc;
	AIP_TIME_STAMP *pTimeStamp;
	int nRdOff, nWrOff;
	unsigned int nPtsOff;
	unsigned int nPtsHi, nPtsLo, nPts45k;
	unsigned int nFs;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL || pPtsHi == NULL || pPtsLo == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nRdOff = pBuffer->nRdOffset;
	nWrOff = pBuffer->nWrOffset[0];

	if (nWrOff == 0 || nWrOff == pBuffer->nBufSize) {
		if (pBuffer->bTimeStampUpdated)
			pTimeStamp = &pBuffer->sLastTimeStamp;
		else
			pTimeStamp = &pBuffer->sTimeStamp;
	} else if (nRdOff > nWrOff)
		pTimeStamp = &pBuffer->sLastTimeStamp;
	else
		pTimeStamp = &pBuffer->sTimeStamp;

	nFs = pBuffer->sAudioType.nFs;

	nPtsOff = ((nRdOff >> 3) * (u64)AIP_SYS_CLK_90K) / nFs;

	nPtsHi = pTimeStamp->nPtsHi;
	nPtsLo = pTimeStamp->nPtsLo;

	nPts45k = (nPtsLo >> 1) | ((nPtsHi & 0x1) << 31);
	nPts45k += nPtsOff >> 1;

	*pPtsHi = ((nPts45k & 0x80000000) >> 31) | 0x80000000;
	*pPtsLo = nPts45k << 1;

EXIT:
	return rc;
}

int aip_buffer_update_timestamp(AIP_BUFFER *pBuffer, unsigned int nSamples, unsigned int nFs)
{
	int rc;
	UINT nPtsAdv, nPtsHi, nPtsLo, nPts45k;

	rc = HRX_AIP_OK;

	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	nPtsHi = pBuffer->sTimeStamp.nPtsHi;
	nPtsLo = pBuffer->sTimeStamp.nPtsLo;

	nPts45k = (nPtsLo >> 1) | ((nPtsHi & 0x1) << 31);
	nPtsAdv = (nSamples * (u64)AIP_SYS_CLK_90K) / nFs;
	nPts45k += nPtsAdv >> 1;

	pBuffer->sTimeStamp.nPtsHi |= (nPts45k & 0x80000000) >> 31;
	pBuffer->sTimeStamp.nPtsLo = nPts45k << 1;

EXIT:
	return rc;
}

int aip_buffer_neednew_timestamp(AIP_BUFFER *pBuffer, bool *pNeedNewTimeStamp)
{
	int rc;

	rc = HRX_AIP_OK;
	*pNeedNewTimeStamp = FALSE;

	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	if (pBuffer->nWrOffset[0] == 0 ||
		pBuffer->nWrOffset[0] == pBuffer->nBufSize) {
		*pNeedNewTimeStamp = TRUE;
	}

EXIT:
	return rc;
}

int aip_buffer_set_samplerate(AIP_BUFFER *pBuffer, unsigned int nFs)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || nFs > 192000) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pBuffer->sAudioType.nFs = nFs;

EXIT:
	return rc;
}

int aip_buffer_set_timestamp_delta(AIP_BUFFER *pBuffer, unsigned int nTimeStampDelta)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pBuffer->nTimeStampDelta = nTimeStampDelta;

EXIT:
	return rc;
}

int aip_buffer_set_chan_num(AIP_BUFFER *pBuffer, unsigned int nChanNr)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || nChanNr > 8) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pBuffer->sAudioType.nChanNr = nChanNr;

EXIT:
	return rc;
}

int aip_buffer_get_chan_num(AIP_BUFFER *pBuffer, unsigned int *pChanNr)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pChanNr == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	*pChanNr = pBuffer->sAudioType.nChanNr;
EXIT:
	return rc;
}

int aip_buffer_set_audio_dec_format(AIP_BUFFER *pBuffer, unsigned int nFormat)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || nFormat >= AIP_FORMAT_TOTAL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pBuffer->sAudioType.eDecFmt = nFormat;
EXIT:
	return rc;

}

int aip_buffer_set_audio_bitdepth(AIP_BUFFER *pBuffer, u8 nBitDepth)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pBuffer->sAudioType.nBitDepth = nBitDepth;

EXIT:
	return rc;
}

int aip_buffer_set_audio_hbr(AIP_BUFFER *pBuffer, u8 nIsHbr)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pBuffer->sAudioType.nHbr = nIsHbr;

EXIT:
	return rc;
}

int aip_buffer_get_audio_hbr(AIP_BUFFER *pBuffer, u8 *nIsHbr)
{
	int rc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	*nIsHbr = pBuffer->sAudioType.nHbr;

EXIT:
	return rc;
}

int aip_buffer_get_ctrl_tag_space(AIP_BUFFER *pBuffer, int *pSpace)
{
	int rc;
	int nSpace;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pSpace == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;

	nSpace = pCtrlTagBuf->nRdIndex - pCtrlTagBuf->nWrIndex;
	if (nSpace <= 0)
		nSpace += pCtrlTagBuf->nIndexSize;

	*pSpace = nSpace - 1;
EXIT:
	return rc;
}

int aip_buffer_get_ctrl_tag_fullness(AIP_BUFFER *pBuffer, int *pFullness)
{
	HRESULT rc;
	INT nFullness;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pFullness == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;

	nFullness = pCtrlTagBuf->nWrIndex - pCtrlTagBuf->nRdIndex;
	if (nFullness < 0)
		nFullness += pCtrlTagBuf->nIndexSize;

	*pFullness = nFullness;
EXIT:
	return rc;
}

int aip_buffer_write_ctrl_tag(AIP_BUFFER *pBuffer, AIP_CTRL_TAG *pCtrlTag)
{
	int rc;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;
	AIP_CTRL_TAG *pCtrlTagDst;
	INT nSpace;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pCtrlTag == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;

	rc = aip_buffer_get_ctrl_tag_space(pBuffer, &nSpace);
	AIP_CHKRC(rc);

	if (nSpace > 0) {
		pCtrlTag->nDataCnt = pBuffer->nWrDataCnt;
		pCtrlTagDst = pCtrlTagBuf->pCtrlTagStart + pCtrlTagBuf->nWrIndex;
		memcpy(pCtrlTagDst, pCtrlTag, sizeof(AIP_CTRL_TAG));
		pCtrlTagBuf->nWrIndex = (pCtrlTagBuf->nWrIndex + 1 >= pCtrlTagBuf->nIndexSize) ?
		0 : pCtrlTagBuf->nWrIndex + 1;
	} else
		rc = HRX_AIP_ENOSPACE;

EXIT:
	return rc;
}

static int aip_buffer_peek_ctrl_tag(AIP_BUFFER *pBuffer, AIP_CTRL_TAG *pCtrlTag)
{
	int rc;
	int nFullness;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;
	AIP_CTRL_TAG *pCtrlTagSrc;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pCtrlTag == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;

	rc = aip_buffer_get_ctrl_tag_fullness(pBuffer, &nFullness);
	AIP_CHKRC(rc);

	if (nFullness > 0) {
		pCtrlTagSrc = pCtrlTagBuf->pCtrlTagStart + pCtrlTagBuf->nRdIndex;
		memcpy(pCtrlTag, pCtrlTagSrc, sizeof(AIP_CTRL_TAG));
	} else
		rc = HRX_AIP_EEMPTY;

EXIT:
	return rc;
}

int aip_buffer_read_ctrl_tag(AIP_BUFFER *pBuffer, AIP_CTRL_TAG *pCtrlTag)
{
	int rc;
	int nFullness;
	AIP_CTRL_TAG_BUF *pCtrlTagBuf;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || pCtrlTag == NULL) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}

	pCtrlTagBuf = &pBuffer->sCtrlTagBuf;

	rc = aip_buffer_get_ctrl_tag_fullness(pBuffer, &nFullness);
	AIP_CHKRC(rc);

	if (nFullness > 0) {
		rc = aip_buffer_peek_ctrl_tag(pBuffer, pCtrlTag);
		if (rc == HRX_AIP_OK) {
			if (pBuffer->nRdDataCnt >= pCtrlTag->nDataCnt) {
				pCtrlTagBuf->nRdIndex =
					(pCtrlTagBuf->nRdIndex + 1 > pCtrlTagBuf->nIndexSize) ?
					0 : pCtrlTagBuf->nRdIndex + 1;
			} else
				rc = HRX_AIP_ENOTREADY;
		}
	} else

		rc = HRX_AIP_EEMPTY;

EXIT:
	return rc;
}

int aip_buffer_copy_to_pad(AIP_BUFFER *pBuffer, int nThreshold, VPP_MEM_LIST *memlist)
{
	int rc;
	int nFullness, nFullnessNoWrap, nCopySize;
	unsigned int nPair, nPairsNr;
	void *pDst, *pSrc;
	int nShmOffset;

	rc = HRX_AIP_OK;
	if (pBuffer == NULL || nThreshold < 0) {
		rc = HRX_AIP_EBADPARAM;
		goto EXIT;
	}
	rc = aip_buffer_get_fullness(pBuffer, &nFullness);
	AIP_CHKRC(rc);
	if (nFullness < nThreshold) {
		rc = HRX_AIP_EBADCALL;
		goto EXIT;
	}
	rc = aip_buffer_get_fullness_nowrap(pBuffer, &nFullnessNoWrap);
	AIP_CHKRC(rc);
	if (nFullnessNoWrap < nThreshold) {
		nCopySize = nThreshold - nFullnessNoWrap;
		if (nCopySize > pBuffer->nPadSize) {
			rc = HRX_AIP_EBADCALL;
			goto EXIT;
		}

		nPairsNr = pBuffer->nPairsNr;

		for (nPair = 0; nPair < nPairsNr; nPair++) {
			nShmOffset = pBuffer->nStartOffset[nPair];
			rc = aip_buffer_get_start_addr(pBuffer, nPair, &pSrc);
			AIP_CHKRC(rc);
			rc = aip_buffer_get_end_addr(pBuffer, nPair, &pDst);
			AIP_CHKRC(rc);
			VPP_MEM_FlushCache(memlist, &pBuffer->vpp_aip_buf_mem_handle, nCopySize);
			memcpy(pDst, pSrc, nCopySize);
		}
	}
EXIT:
	return rc;
}
