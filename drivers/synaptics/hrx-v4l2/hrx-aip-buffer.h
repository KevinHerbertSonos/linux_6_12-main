// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021 Synaptics Incorporated */

#ifndef _HRX_AIP_BUFFER_H_
#define _HRX_AIP_BUFFER_H_

#define AIP_MAX_OBJECTS_NR  (1)
#define AIP_MAX_FRAME_NR    (8)
#define AIP_DEFAULT_BUF_SIZE    (1024*128*3)
#define AIP_DEFAULT_PAD_SIZE    (1024*64*3)
#define AIP_DEFAULT_CHUNK_SIZE  (1024*6)
#define AIP_MAX_IN_PAIR_NR  (4)
#define AIP_MAX_DMA_CMD_NR  (8)
#define AIP_CTRL_TAG_SIZE   (64)
#define AIP_SYS_CLK_90K     (90000)

#define AIP_CHKRC(rc)   do {    \
	if (rc != HRX_AIP_OK) {         \
		pr_err("RC=0x%x", rc); \
	}                   \
}                       \
while (0)

typedef enum {
	AIP_TAG_INVALID = 0x0,
	AIP_TAG_SIGNAL_STATUS = 0x1,
	AIP_TAG_OVERFLOW = 0x2,
} AIP_CTRL_TAG_TYPE;

typedef struct AIP_CTRL_SIGNAL_STATUS_T {
	unsigned int    nSignalStatus;
} AIP_CTRL_SIGNAL_STATUS;

typedef struct AIP_CTRL_DATA_OVERFLOW_T {
	unsigned int    nOverflowSize;
} AIP_CTRL_DATA_OVERFLOW;

typedef struct AIP_CTRL_TAG_UNION_T {
	AIP_CTRL_SIGNAL_STATUS  sSignalStatus;
	AIP_CTRL_DATA_OVERFLOW  sDataOverflow;
} AIP_CTRL_TAG_UNION;

typedef struct AIP_CONTROL_TAG_T {
	unsigned int nCtrlTagType   :8;
	unsigned int nRsvd      :24;
	unsigned int nDataCnt;
	AIP_CTRL_TAG_UNION sCtrlTagUnion;
} AIP_CTRL_TAG;

typedef struct AIP_CTRL_TAG_BUF_T {
	AIP_CTRL_TAG *pCtrlTagStart;
	int nIndexSize;
	INT nRdIndex;
	INT nWrIndex;
} AIP_CTRL_TAG_BUF;

typedef struct aip_buffer_t {
	VPP_MEM vpp_aip_buf_mem_handle;
	void *pBufferBase;
	void *pStart[AIP_MAX_IN_PAIR_NR];
	unsigned int nPairsNr;
	int nStartOffset[AIP_MAX_IN_PAIR_NR];
	int nAllocLen;
	int nOrigBufSize;
	int nBufSize;
	int nOrigPadSize;
	int nPadSize;
	int nRdOffset;
	int nPreWrOffset[AIP_MAX_IN_PAIR_NR];
	int nWrOffset[AIP_MAX_IN_PAIR_NR];
	int nDirtyBytes[AIP_MAX_IN_PAIR_NR];
	int nWrDataCnt;
	unsigned int nRdDataCnt;
	AIP_CTRL_TAG_BUF sCtrlTagBuf;
	AIP_AUDIO_TYPE sAudioType;
	// time stamp corresponding to the start of buffer
	AIP_TIME_STAMP sLastTimeStamp;
	AIP_TIME_STAMP sTimeStamp;
	bool bTimeStampUpdated;
	unsigned int nTimeStampDelta;
	void *tmp_buffer;
} AIP_BUFFER;

int aip_buffer_create(AIP_BUFFER *pAipBuffer, unsigned int nBufSize, unsigned int nPadSize, VPP_MEM_LIST *memlist);
int aip_bufer_split_to_fourpairs(AIP_BUFFER *pBuffer, unsigned int nChanNr);
int aip_buffer_mergeto_onepair(AIP_BUFFER *pBuffer);
int aip_buffer_free(AIP_BUFFER *pBuffer, VPP_MEM_LIST *memlist);
int aip_buffer_reset(AIP_BUFFER *pBuffer);
int aip_buffer_get_fullness(AIP_BUFFER *pBuffer, int *pFullness);
int aip_buffer_get_fullness_nowrap(AIP_BUFFER *pBuffer, int *pFullness);
int aip_buffer_get_space(AIP_BUFFER *pBuffer, int *pSpace);
int aip_buffer_get_space_nowrap(AIP_BUFFER *pBuffer, int *pSpace);
int aip_buffer_get_prespace(AIP_BUFFER *pBuffer, int *pSpace);
int aip_buffer_get_prespace_nowrap(AIP_BUFFER *pBuffer, int *pSpace);
int aip_buffer_get_rd_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppRdAddr);
int aip_buffer_get_wr_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr);
int aip_buffer_get_pre_wr_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr);
int aip_buffer_get_start_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr);
int aip_buffer_get_end_addr(AIP_BUFFER *pBuffer, unsigned int nPair, void **ppWrAddr);
int aip_buffer_get_inc_addr(AIP_BUFFER *pBuffer, unsigned int nPair, int nInc, void *pRef, void **ppAddr);
int aip_buffer_rd_update(AIP_BUFFER *pBuffer, int nAdv);
int aip_buffer_wr_update(AIP_BUFFER *pBuffer, unsigned int nPair, int nAdv);
int aip_buffer_pre_wr_update(AIP_BUFFER *pBuffer, unsigned int nPair, int nAdv);
int aip_buffer_get_pairs_nr(AIP_BUFFER *pBuffer, unsigned int *pPairs);

int aip_buffer_set_timestamp(AIP_BUFFER *pBuffer, unsigned int nPtsHi, unsigned int nPtsLo);
int aip_buffer_get_timestamp(AIP_BUFFER *pBuffer, unsigned int *pPtsHi, unsigned int *pPtsLo);
int aip_buffer_update_timestamp(AIP_BUFFER *pBuffer, unsigned int nSamples, unsigned int nFs);
int aip_buffer_neednew_timestamp(AIP_BUFFER *pBuffer, bool *pNeedNewTimeStamp);

int aip_buffer_get_ctrl_tag_space(AIP_BUFFER *pBuffer, int *pSpace);
int aip_buffer_get_ctrl_tag_fullness(AIP_BUFFER *pBuffer, int *pFullness);
int aip_buffer_write_ctrl_tag(AIP_BUFFER *pBuffer, AIP_CTRL_TAG *pCtrlTag);
int aip_buffer_read_ctrl_tag(AIP_BUFFER *pBuffer, AIP_CTRL_TAG *pCtrlTag);
int aip_buffer_copy_to_pad(AIP_BUFFER *pBuffer, int nThreshold, VPP_MEM_LIST *memlist);

int aip_buffer_set_samplerate(AIP_BUFFER *pBuffer, unsigned int nFs);
int aip_buffer_set_timestamp_delta(AIP_BUFFER *pBuffer, unsigned int nTimeStampDelta);
int aip_buffer_set_chan_num(AIP_BUFFER *pBuffer, unsigned int nChanNr);
int aip_buffer_get_chan_num(AIP_BUFFER *pBuffer, unsigned int *pChanNr);
int aip_buffer_set_audio_dec_format(AIP_BUFFER *pBuffer, unsigned int nFormat);
int aip_buffer_set_audio_bitdepth(AIP_BUFFER *pBuffer, u8 nBitDepth);
int aip_buffer_set_audio_hbr(AIP_BUFFER *pBuffer, u8 nIsHbr);
int aip_buffer_get_audio_hbr(AIP_BUFFER *pBuffer, u8 *nIsHbr);

#endif //_HRX_AIP_BUFFER_H_
