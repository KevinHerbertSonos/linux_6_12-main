/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021 Synaptics Incorporated */

#ifndef __HDMI_IN_IF_H
#define __HDMI_IN_IF_H

typedef enum {
	HRX_STATUS_UNSTABLE,
	HRX_STATUS_HPD_STABLE,
	HRX_STATUS_VIP_STABLE
} ENUM_HRX_STATUS;

typedef enum {
	AIP_SOURCE_INVALID = -1,
	AIP_SOURCE_AVIF = 2,
} ENUM_AIP_SOURCE_TYPE;

typedef enum {
	AIP_FORMAT_INVALID = -1,
	AIP_FORMAT_UNDEF = 0,
	AIP_FORMAT_PCM,
	AIP_FORMAT_TOTAL,
} ENUM_AIP_DATA_FORMAT;

typedef struct {
	ENUM_AIP_DATA_FORMAT    eDataFmt;
	ENUM_AIP_DATA_FORMAT eDecFmt;
	u8    nChanNr;
	u8    nBitDepth;
	u8    nHbr;
	unsigned int    nFs;
} AIP_AUDIO_TYPE;

typedef struct {
	unsigned int    nPtsHi;
	unsigned int    nPtsLo;
} AIP_TIME_STAMP;

typedef struct {
	AIP_TIME_STAMP    sTimeStamp;
	VOID    *pBuffer;
	unsigned int    nAllocLen;
	unsigned int    nFillLen;
	unsigned int    nOffset;
	u32  uFlag; /**< The flag of memory space defined in amp_bd_tag.h*/
} AIP_FRAME_DES;

typedef struct {
	ENUM_AIP_DATA_FORMAT eDataFmt;
} AIP_EVENT_FORMAT_INFO;

typedef struct {
	AIP_TIME_STAMP    sTimeStamp;
} AIP_EVENT_TIMESTAMP;

typedef union {
	AIP_EVENT_FORMAT_INFO    sFormatInfo;
	AIP_EVENT_TIMESTAMP    sTimeStampInfo;
} AIP_EVENT_INFO;

typedef struct _AIP_ALSA_CMD_DATA {
	u32 channels;
	u32 sample_rate;
	u32 frame_rate;/* frame is not same as sample_rate when hbr */
	u32 hbr;
	u32 bit_depth;
	int dec_type;
	u32 format;
	char *data;
	void *allocFn;
	void *freeFn;
	void *eventFn;
	void *hdmi;
	u32 period_bytes;
} AIP_ALSA_CMD_DATA;

typedef enum {
	AIP_EVENT_INVALID = -1,
	AIP_EVENT_FRAME_READY,
	AIP_EVENT_FORMAT_CHANGE,
	AIP_EVENT_OVERFLOW,
	AIP_EVENT_SIGNAL_STABLE,
	AIP_EVENT_SIGNAL_UNSTABLE,
	AIP_EVENT_GET_TIMESTAMP,
	AIP_EVENT_MUTE,
	AIP_EVENT_UNMUTE,
} ENUM_AIP_EVENT_ID;

struct alsa_aip_ops {
	/*
	 * for interface ops
	 */
	int (*get_hrx_status)(ENUM_HRX_STATUS *);
	int (*config)(AIP_ALSA_CMD_DATA *);
	int (*start)(void);
	int (*stop)(void);
	int (*close)(void);
};

typedef int (*aip_frame_alloc_cb)(void **pFrame,
	int *nSize,
	void *pContextParam);
typedef int (*aip_frame_free_cb)(void *pFrame, void *pContextParam);
typedef int (*aip_event_cb)(int nEventID,
	void *pEventInfo,
	void *pContextParam);

int aip_alsa_set_ops(const struct alsa_aip_ops *ptr);

#endif //__HDMI_IN_IF_H
