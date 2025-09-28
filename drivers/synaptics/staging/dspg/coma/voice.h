/*
 *  drivers/staging/dspg/coma/voice.h - voice/RTP character device
 *
 *  Copyright (C) 2007 NXP Semiconductors
 *  Copyright (C) 2008 - 2012 DSPG Technologies GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef VOICE_H
#define VOICE_H

#include <linux/ioctl.h>
#include <linux/sched/types.h>
#include <uapi/linux/sched/types.h>

#ifdef __KERNEL__
#include <linux/in.h>
#include <linux/in6.h>
#else
#include <netinet/in.h>
#endif

#define MAX_NUM_OF_CODEC	6
#define MAX_DYNAMIC_CODEC_LEN	15
#define RTCP_MAX_SDES		50
#define MAX_SDES_VAL_LEN	50
#define MAX_SDES_ITEMS		8
#define DEFAULT_DURATION	20
#define RTCP_PT_SR		200
#define RTCP_PT_RR		201
#define RTCP_PT_BYE		203
#define SYMMETRIC_TX_RTP_CNT	10
#define VOICE_LOOP_TEST		0
#define NUM_PACKET_TO_RETRANSMIT 6

#define G711_FRAME_SIZE		160
#define G722_FRAME_SIZE		160
#define G729_FRAME_SIZE		10

#pragma pack (1)

/* struct comfort noise options */
typedef struct {
	int mode_rx;		/* RX mode */
	int mode_tx;		/* TX mode */
	int max_sid_update;	/* MAX SID update */
	int vad_detect_level;	/* VAD detect level */
	int vad_hangover;	/* VAD hangover level */
	int level_rx;		/* RX level */
	int level_tx;		/* TX level */
} rtp_cng_opts;

/* supported RTP payload types */
#define RTP_SESSION_PT_G722	9
#define RTP_SESSION_PT_G726	2
#define RTP_SESSION_PT_G729	18
#define RTP_SESSION_PT_G711A	8
#define RTP_SESSION_PT_G711U	0
#define RTP_SESSION_PT_G723	4
#define RTP_SESSION_PT_DYN_FIRST 96
#define RTP_SESSION_PT_ILBC	98
#define RTP_SESSION_PT_EVT_TEL	101
#define RTP_SESSION_PT_DYN_LAST	127

/* codec list */
typedef struct {
	char rx_pt; /* RX codec one of the value defined in  RTP_SESSION_PT_ */
	char CodecStr[MAX_DYNAMIC_CODEC_LEN]; /* codec string */
} rx_pt_list;

/* RTP codec options */
#define RTP_CODEC_OPT_NONE			0
#define RTP_CODEC_OPT_G723_5K3			0x00000001
#define RTP_CODEC_OPT_PLC			0x00000002
#define RTP_CODEC_OPT_VAD			0x00000004
#define RTP_CODEC_OPT_G726_NIBBLE_REVERSED	0x00000008
#define RTP_CODEC_OPT_G726_16K			0x00000010
#define RTP_CODEC_OPT_G726_24K			0x00000020
#define RTP_CODEC_OPT_G726_32K			0x00000040
#define RTP_CODEC_OPT_G726_40K			0x00000080
#define RTP_CODEC_OPT_ILBC_15K2			0x00000100
#define RTP_CODEC_OPT_AMRWB_24K			0x00800000

typedef enum
{
   RTP_CODEC_VBR_AMRWB_660=1,        /* AMR-WB 6.60 kbps */
   RTP_CODEC_VBR_AMRWB_885,           /* AMR-WB 8.85 kbps */
   RTP_CODEC_VBR_AMRWB_1265,          /* AMR-WB 12.65 kbps */
   RTP_CODEC_VBR_AMRWB_1425,          /* AMR-WB 14.25 kbps */
   RTP_CODEC_VBR_AMRWB_1585,          /* AMR-WB 15.85 kbps */
   RTP_CODEC_VBR_AMRWB_1825,          /* AMR-WB 18.25 kbps */
   RTP_CODEC_VBR_AMRWB_1985,          /* AMR-WB 19.85 kbps */
   RTP_CODEC_VBR_AMRWB_2305,          /* AMR-WB 23.05 kbps */
   RTP_CODEC_VBR_AMRWB_2385 		 /* AMR-WB 23.85 kbps */
}RTP_AMR_BIT_RATE;


#define RTP_CODEC_VBR_AMRWD_MODESET_660         0x00000001
#define RTP_CODEC_VBR_AMRWD_MODESET_885         0x00000002
#define RTP_CODEC_VBR_AMRWD_MODESET_1265        0x00000004
#define RTP_CODEC_VBR_AMRWD_MODESET_1425        0x00000008
#define RTP_CODEC_VBR_AMRWD_MODESET_1585        0x00000010
#define RTP_CODEC_VBR_AMRWD_MODESET_1825        0x00000020
#define RTP_CODEC_VBR_AMRWD_MODESET_1985        0x00000040
#define RTP_CODEC_VBR_AMRWD_MODESET_2305        0x00000080
#define RTP_CODEC_VBR_AMRWD_MODESET_2385        0x00000100

/* G722.1 codec related parameters */

/* G7221 standard bit rates */
#define RTP_CODEC_BITRATE_G7221_16k_24		24000
#define RTP_CODEC_BITRATE_G7221_16k_32		32000
#define RTP_CODEC_BITRATE_G7221_32k_24		24000
#define RTP_CODEC_BITRATE_G7221_32k_32		32000
#define RTP_CODEC_BITRATE_G7221_32k_48		48000

typedef struct {
	int nSamplingRate;      /* sampling rate. currently 16Khz is used */
	int nBitRate;           /* bit rate in bps */
	int nFrameSize;         /* Frame size */
	int nPackFormat;	/* PackFormat RFC5577/siren */
} rtp_g7221_codec_param;

/* LC3 sampling rates*/
#define RTP_CODEC_LC3_NB_SAMPLING_RATE	8000
#define RTP_CODEC_LC3_WB_SAMPLING_RATE	16000
#define RTP_CODEC_LC3_SWB_SAMPLING_RATE	32000

/* LC3 bit rates */
#define RTP_CODEC_LC3_DEF_BITRATE_NB	16000
#define RTP_CODEC_LC3_DEF_BITRATE_WB	32000
#define RTP_CODEC_LC3_DEF_BITRATE_SWB	64000

/* LC3 frame Length */
#define RTP_CODEC_LC3_FRAMELENGTH	80

/* LC3 Decoder EP enable */
#define RTP_CODEC_LC3_DEC_EP_DISABLE	0
#define RTP_CODEC_LC3_DEC_EP_ENABLE	1

/*LC3 ERROR PROTECTION modes*/
typedef enum {
	RTP_CODEC_LC3_ENC_EP_DISABLED = 0,
	RTP_CODEC_LC3_ENC_EP_MINIMUM,
	RTP_CODEC_LC3_ENC_EP_MODERATE,
	RTP_CODEC_LC3_ENC_EP_STRONG,
	RTP_CODEC_LC3_ENC_EP_MAXIMUM
} RTP_LC3_ENC_ERROR_PROTECTION_MODE;

typedef struct {
	int nSamplingRate;	/* Supported Sampling rate- 8k(NB), 16k(WB) or 32k(SWB) */
	int nFrameLength;	/* Supported values -10ms, 5ms, 2.5ms. VOIP supports only 10ms*/
	int nEncBitRate;	/* Supported BitRates- 16kbps(NB), 32kbps(WB), 64kbps(SWB) */
	int nEncEpMode;		/* Error protection mode for Encoder */
	int nDecEpEnable;	/* Decoder Error protection enable/disable */
} rtp_lc3_codec_param;

/* ISAC Codec related parameters */
/* ISAC frame size in samples*/
#define RTP_CODEC_ISAC_30_FRAMESIZE_SAMPLES     480
#define RTP_CODEC_ISAC_60_FRAMESIZE_SAMPLES     960

/* ISAC coding modes*/
enum {
	RTP_CODEC_ISAC_CI = 1,
	RTP_CODEC_ISAC_CA
} RTP_ISAC_CODING_MODE;

typedef struct {
	int nCodingMode; /*coding mode i.e CI or CA mode*/
	int nFrameSize; /*Frame size 30 or 60 in number of samples per frame*/
	int nBitRate; /*bit rate in bps - currently not used*/
} rtp_isac_codec_param;

/** struct  rtp_vbr_codec param - specifies a VoIP variable bit rate(vbr) codecs (AMR-WB,Opus,Isec
 *
 * BOOL bFec;             fec request
 * BOOL bVadFlag;       Select VAD Flag
 * BOOL bVbr;             Select Variable bit rate
 * U32 nBitRate;          Select Bit Rate
 * U32 nMaxRate;        Select Maximum Rate for single frame
 * U32 nFrameSize;     Select Frame Size
 * U32 nApplication;      voip, music, low delay
 * U32 nComplexity;      0-10

 */

typedef struct
{

   int bFec;            /**< fec request */
   int bVadFlag;        /**< Select VAD Flag */
   int bVbr;            /**< Select Variable bit rate */
   int nBitRate;         /**< Select Bit Rate */
   int nMaxRate;         /**< Select Maximum Rate for single frame */
   int nFrameSize;       /**< Select Frame Size */
   int nApplication;     /**< voip, music, low delay*/
   int nComplexity;      /**< 0-10*/

}rtp_opus_codec_param;


typedef struct
{

   int octetalign;            /**<If 1, octet-aligned  operation SHALL be used.  If 0 or if not present,
				               bandwidth-efficient operation is employed */
   int bVadFlag;        /**< Select VAD Flag */
   int nBitRate;         /**< Select Bit Rate */
   int nFrameSize;       /**< Select Frame Size */
   int modechangeperiod;     /**  Rx Side : The value of N SHALL be either 1 or 2. if parameter is not present, mode changes are allowed at
							       Any time during the session. a value of 2 allows the sender to change mode every second frame-
								block , a value of 1 allows the sender to change mode every  frame-block */
   int modechangecapability; /* Tx Side : The value of N SHALL be either 1 or 2. A value of 1 indicates codec may be changed at any point ,
								   A value of 2 indicates that the client has the capability to restrict the mode change
								   period to 2, and thus that the client can correctly interoperate with a receiver requiring a
								   mode-change-period=2*/
   int crc;      /**If 1, frame CRCs SHALL be  included in the payload.  If 0 or not present, CRCs  SHALL NOT be used */
   int robustsorting; /** If 1,the payload SHALL employ robust payload sorting.  If 0 or if
			               not present, simple payload sorting SHALL be used */
   int interleaving; /**		Indicates that frame-block level interleaving SHALL
				               be used for the session   */
    int maxred;	 /**maximum duration in milliseconds that elapses between
            			   the primary (first) transmission of a frame and any
			               redundant transmission that the sender will use  */
	int nModeSetOpts;	/**< Bitwise mode set options. If 0 or not present mode set restriction will not be applied for CMR*/
	int bModeChangeNeighbor;	  /**<If 1, Mode-Change_neighbor will be used for CMR. If 0 or not present, it will not be used */
	int requestmode;  /* test field : cmr request to remote end*/
}rtp_amrwb_codec_param;

typedef union {
	int u8PlaceHolderArray[32]; /* Currently not used */
	rtp_amrwb_codec_param amrwb;
	rtp_opus_codec_param   opus;
	rtp_g7221_codec_param g7221; /* G722.1 codec related parameters */
	rtp_isac_codec_param isac;
	rtp_lc3_codec_param lc3;
}vbr_codec;

typedef struct {
	/*!< event RTP_EVENT_RX_SSRC_CHANGE reported after X number
	of packets with changed SSRC*/
	unsigned short report_ssrc_change;
	/*!< event RTP_EVENT_RX_PAYLOAD_CHANGE reported after X number
	of packets with changed Codec Payload Type*/
	unsigned short report_codec_payload_change;
}rtp_session_event_config;



/** struct rtp_codec - specifies a VoIP codec
 *
 * @duration:    packet duration in ms
 * @rx_pt:       only packets with this payload get accepted from the net
 * @rx_pt_event: only event packets with this payload get accepted from the net
 * @tx_pt:       rtp payload type, specifies codec used for encoding voice data
 * @tx_pt_event: rtp payload type used to transmit events
 * @opts:        enable codec specific capabilities (see RTP_CODEC_OPT_*)
 */
typedef struct {
	int duration;	/* packet duration in ms */
	int opts;	/* codec options value defined in RTP_CODEC_ */
	int Timestamp;
	int ssrc;	/* synchronization source */
	int rx_pt_event;/* only event packets with this payload get accepted from the nw */
	int tx_pt;	/* rtp payload type, specifies codec used for encoding voice data */
	int tx_pt_event;/* rtp payload type used to transmit events */
	rtp_cng_opts cng;			/* comfort noise options */
	char CodecStr[MAX_DYNAMIC_CODEC_LEN+1];	/* confirmed codec string */
	rx_pt_list rx_list[MAX_NUM_OF_CODEC];	/* codec list */
	vbr_codec  vbrCodecParam; /* Variable bit rate codec configuration */
} rtp_codec;

/* configuration for jitter buffer type */
typedef enum {
	RTP_JIB_TYPE_FIXED = 0,
	RTP_JIB_TYPE_ADAPTIVE,
	RTP_JIB_TYPE_SILENCE,
} rtp_jib_type;

/* configuration for jitter buffer */
typedef struct {
	int max_len; /* in 10ms units */
	int min_len; /* in 10ms units */
	int type;
	int Th_resync;
	int target_delay;        /* in 10ms units */
	int monitoring_interval; /* in 10ms units */
	int stepsize_reset_time;
	int slope;               /* in samples per 10ms */
	int pos_adapt_step_size;    /* positive adaptation stepsize for fast expansion */
	int pos_adapt_step_size_m;   /* positive adaptation stepsize for medium expansion*/
} rtp_jib_config;

/* configuration for T.38 sessions */
typedef struct {
	unsigned int lsRedundancy;	/* level of redundancy of low speed packets */
	unsigned int hsRedundancy;	/* level of redundancy of high speed packets */
	unsigned int ecnOn;		/* ECN always on. Parameter ignored */
} t38_config;

#define MAX_PKT_LOSS_INCREASE_THRESHOLD 50
#define MIN_PKT_LOSS_INCREASE_THRESHOLD 10
#define MAX_PKT_LOSS_DECREASE_THRESHOLD 100
#define MIN_PKT_LOSS_DECREASE_THRESHOLD 20
#define MAX_POSITIVE_ADAPTATION_RATE 50
#define MIN_POSITIVE_ADAPTATION_RATE 3
#define MAX_NEGATIVE_ADAPTATION_RATE 40
#define MIN_NEGATIVE_ADAPTATION_RATE 2
#define MAX_ADAPTATION_LIMIT 50
#define MIN_ADAPTATION_LIMIT 10
#define MAX_RTP_RETRANSMISSION_BUFFER 1000
#define MIN_RTP_RETRANSMISSION_BUFFER 180

typedef struct{
        unsigned int packet_loss_increase_threshold;    /** in typically 20, min10, max50**/
	unsigned int packet_loss_decrease_threshold;    /** typically 40, min20, max100 **/
	unsigned int positive_adaptation_rate;          /** typically 6ms, min 3ms, max 50ms**/
	unsigned int negative_adaptation_rate;          /** typically 4ms, min 2ms, max 40ms**/
	unsigned int max_adaptation_limit;              /** typically 30ms, min 10ms, max 50ms **/
}rtp_pkt_loss_detection_adaptation_config;

/* SDP parameter change indicator */
typedef enum {
	VOICE_SDP_PARAM_CODEC,	/* codec changed */
	VOICE_SDP_PARAM_MODE,	/* mode changed */
	VOICE_SDP_PARAM_PTIME,	/* ptime changed */
	VOICE_SDP_PARAM_JIB	/* jitter buffer changed */
} VOICE_SDP_CHANGE;

/* audio enumerations for call HOLD/UNHOLD */
typedef enum {
	RTP_MODE_SEND_ONLY=1,	/* audio configuration is only for transmit direction */
	RTP_MODE_REC_ONLY,	/* audio configuration is only for receive direction  */
	RTP_MODE_ACTIVE,	/* audio configuration is active */
	RTP_MODE_INACTIVE	/* audio configuration is inactive */
} RTP_MODE_UPDATE;

/* audio enumerations for RTP media looping */
typedef enum {
	RTP_LOOP_LEVEL_NONE,
	RTP_LOOP_IP_LEVEL,		/* loop at IP level */
	RTP_LOOP_DSP_LEVEL,		/* loop at DSP level */
	RTP_LOOP_RTP_LEVEL,		/* loop at RTP level */
	RTP_LOOP_ENCAPSULATED_RTP_LEVEL	/* encapsulated RTP loopback level */
} RTP_MEDIA_LOOP_LEVEL;

/* DTAM call mode */
typedef enum {
	RTP_APP_VOIP_USER,
	RTP_APP_VOIP_KERNEL,
	RTP_APP_RTPSTACK_IN_APP,
	RTP_APP_VOIP_APP,
} RTP_CALL_MODE;

#define RTP_SESSION_SRTP_KEY_LEN	48
#define RTP_SESSION_SRTP_MKI_LEN	256

#define RTP_SESSION_OPT_SRTP_TXAUTH32	0x01
#define RTP_SESSION_OPT_SRTP_RXAUTH32	0x02
#define RTP_SESSION_OPT_SRTP_128	0x04
#define RTP_SESSION_OPT_SRTP_192	0x08
#define RTP_SESSION_OPT_SRTP_256	0x10
#define RTP_SESSION_OPT_SRTP		0x20

/** struct srtp_session_config - structure to configure a srtp session
 *
 * @opts:       set options for the rtp session (see SRTP_SESSION_OPT_*)
 */
typedef struct {
	/*!< SRTP Options*/
	unsigned int opts;
	/*!< SRTP local Key (32 bytes[master] + 14 bytes[salt]
	 *	 + 2 bytes fo structure padding
	 */
	unsigned char key_loc[RTP_SESSION_SRTP_KEY_LEN];
	/*!< SRTP remote Key (32 bytes[master] + 14 bytes[salt]
	 *	+ 2 bytes fo structure padding
	 */
	unsigned char key_dist[RTP_SESSION_SRTP_KEY_LEN];
	/* when rtp and rtcp have differnet master/salt keys mki will be used */
	unsigned int mki_length; /* length of mki value */
	/* mki value btw 0 - 255 bytes(not 256) */
	unsigned char mki_value[RTP_SESSION_SRTP_MKI_LEN];
} srtp_session_config;

/* RTP session options */
#define RTP_SESSION_OPT_NONE		0
#define RTP_ENABLE_SYMMETRIC_RSP	0x00000200
#define RTP_SESSION_OPT_DTMF		0x00002000
#define RTP_SESSION_OPT_USE_JIB		0x00100000
#define RTP_SESSION_OPT_RTCP_ON		0x00200000
#define RTP_SESSION_OPT_USERSPACE	0x00400000
#define RTP_SESSION_OPT_APP		0x00080000
#define RTP_SESSION_OPT_RX_HOLD		0x10000000
#define RTP_SESSION_OPT_TX_HOLD		0x20000000
#define RTP_SESSION_OPT_TX_TONE		0x40000000
#define RTP_SESSION_OPT_T38_SESSION	0x80000000
#define RTP_SESSION_OPT_PCMEXT_SESSION	0x00040000

/** struct rtp_session_config - structure to configure a rtp session
 *
 * @codec:      codec to be used for the rtp session
 * @jib_config: configuration of the jib, if used
 * @opts:       set options for the rtp session (see RTP_SESSION_OPT_*)
 *
 */
typedef enum
{
        RTP_RETRANSMISSION_DISABLE = 0,
        RTP_RETRANSMISSION_ENABLE
}rtp_retransmission_mode;

/**The Type of RFC-2198 Redundancy **/

#define MAX_REDUNDANT_ENCODING_CODEC 3

typedef enum{
        REDUNDANCY_DISABLE =0 ,
        REDUNDANCY_DTMF_ONLY,
        REDUNDANCY_AUDIO_ONLY,
        REDUNDANCY_DTMF_AUDIO
}REDUNDANCY_MODE;

typedef struct _rtp_redundancy_config{
        unsigned int            rtp_redundancy_mode;    /* The mode of RTP Redundancy **/
        unsigned int            rtp_redundancy_level;    /* The level of RTP Redundancy **/
        unsigned int            rtp_redundant_tx_ptype_audio;  /** The TX payload type for Audio Redundancy **/
        unsigned int            rtp_redundant_rx_ptype_audio;  /** The RX payload type for Audio Redundancy **/
        unsigned int            rtp_redundant_tx_ptype_dtmf;   /** The TX payload type for Dtmf Redundancy **/
        unsigned int            rtp_redundant_rx_ptype_dtmf;  /** The RX payload type for Dtmf Redundancy **/
	rx_pt_list              rtp_redundant_codec_list[MAX_REDUNDANT_ENCODING_CODEC];
}rtp_redundancy_config;

typedef struct {
	unsigned int dtmf2833numEndPackets;	/* number of DTMF 2833 end pkts to be sent out */
	int opts;				/* session options */
	unsigned int SymmRTPTxPktCnt;		/* symetric packet count */
	unsigned int current_time;		/* current time received from user space */
	int audio_mode;				/* audio mode */
	int media_loop_level;			/* media loopback configuration */
	int rtcp_mux;				/* multiplex rtp & rtcp packets on a single port */
	int lib_rtp_mode;			/* rtp call mode */
	rtp_jib_config jib_config;		/* JIB jitter buffer configuration */
	t38_config t38_cfg;			/* t38 configurations */
	rtp_codec codec;			/* codec configuration */
	int sid_update;
	unsigned int rtpRetransmissionMode;
	unsigned int            rtp_retransmission_buffer_size;            /**Size of RTP Retransmission buffer interms of ms **/
	srtp_session_config srtp_config;	/* SRTP configuration */
	rtp_redundancy_config   redundancy_config;  	/** RTP Redundancy config according to RFC 2198 **/
	int voip_line_id;                                /* This is the voip line id on which call is established*/
	int session_id;    				/** This is the RTP session Id ****/
	rtp_session_event_config rtp_ses_event_config;
} rtp_session_config;

#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
struct thread_priority_param {
	int policy;
	struct sched_param sched;
};
#endif

/* RTCP options*/
#define RTCP_KERNEL_MODE	0x00000010 /* RTCP running in kernel mode */
#define RTCP_USER_MODE		0x00000020 /* RTCP running in user mode */
#define RTCP_MODE_RX		0x00000001 /* RTCP mode RX */
#define RTCP_MODE_TX		0x00000002 /* RTCP mode TX */

/* enum rtcp_mode - to store kernel mode or user mode rtcp */
enum rtcp_mode {
	user_mode = 0,
	kernel_mode,
};

typedef enum
{
        RTCP_FB_DISABLE = 0,
        RTCP_FB_ENABLE
}rtcp_feedback_type;

/** struct rctp_session_config - structure to configure a rtcp session
 *
 * @channel:       channel number on which RTP is running
 * @rtcp_interval: interval at which RTCP packets are sent in sec
 * @opts:          set options for the rtcp session (Currenlty not used)
 */
typedef struct {
	int rtcp_interval;
	unsigned int opts;
	char sdesItem[MAX_SDES_ITEMS][MAX_SDES_VAL_LEN];
	unsigned int rtcpFbType;
	unsigned int fb_bw;
 	unsigned int fb_trr_interval;
	rtp_pkt_loss_detection_adaptation_config adaptation_config;  /**Packet loss detecion adaption configuration **/
	unsigned int max_rtt;
        unsigned int gmin;
        unsigned int thinning;
        unsigned int rbType;
	srtp_session_config srtp_config;        /* SRTP configuration */
} rtcp_session_config;


typedef enum {
	RTP_DTMF_STATUS_START = 0,
	RTP_DTMF_STATUS_END,
} rtp_dtmf_status;

#define DTMF_END_PKT_CNT	4

/* DTMF options */
#define RTP_EVT_MARKER_SET	0x00000001

/** struct rtp_dtmf_event - a dtmf event
 *
 * @status:   see enum dtmf_status
 * @event:    event id
 * @volume:   volume of dtmf tone
 * @duration: duration of event
 */
typedef struct {
	int status;
	int duration;
	int EvtDuration;
	int MaxEvtDuration;
	int Opts;
	int event;
	int volume;
	int reserved_bit;
} rtp_dtmf_event;

/** struct rtp_generic_event - a generic(NSE)event
 *
 * @dynamic PT: dynamic PT value
 * @event:      event id
 */
typedef struct {
	int eventId;			/* event id */
	int dynamicPt;			/* dynamic payload */
} rtp_generic_event;

/* userspace query interface */
/* RTCP report types */
typedef enum {
	RTCP_TYPE_SR   = 200,		/* sender report */
	RTCP_TYPE_RR   = 201,		/* receiver report */
	RTCP_TYPE_SDES = 202,		/* session descriptor */
	RTCP_TYPE_BYE  = 203,		/* bye */
	RTCP_TYPE_APP  = 204,		/* application defined */
	RTCP_TYPE_XR  = 207		/* extended report */
} rtcp_type_t;

/* RTP header definition */
typedef struct {
	unsigned char bVpxcc;		/* protocol version, padding flag, header extension flag, CSRC count */
	unsigned char bMarkerPayload;	/* marker bit & payload type */
	unsigned short wSeq;		/* sequence number */
	unsigned int dwTs;		/* timestamp */
	unsigned int dwSsrc;		/* synchronization source */
} RTP_HDR;

/* RTP encapsulation packet header */
typedef struct {
	unsigned int rxTS;		/* receive time stamp */
	unsigned char fRcc;		/* fragmentation, reserved, CSRC count */
	unsigned char bMarkerPayload;	/* marker bit & payload type */
	unsigned short wSeq;		/* sequence number */
	unsigned int dwTs;		/* transmit timestamp */
	unsigned int dwSsrc;		/* Source synchronization source */
} RTP_ENCAP_HDR;

/* definition of RTP session event */
typedef struct {
	unsigned int dwTs;		/* timestamp the event occurred */
	unsigned char bEvent;		/* current event */
	unsigned char bVolume;		/* volume & end bit */
	unsigned short wDuration;	/* duration of event */
} RTP_EVENT;

/* RTCP common header word */
typedef struct {
	unsigned char bVpc;		/* protocol version, padding flag, count */
	unsigned char bPacketType;
	unsigned short wLen;		/* pkt len in words, w/o this word */
} RTCP_CM;

/* reception report block */
typedef struct {
	unsigned int dwSsrc;		/* data source being reported */
	unsigned char bFraction;	/* fraction lost since last SR/RR */
	unsigned char bLost[3];		/* cumul. no. pkts lost (signed!) */
	unsigned int dwLastSeq;		/* extended last seq. no. received */
	unsigned int dwJitter;		/* interarrival jitter */
	unsigned int dwLsr;		/* last SR packet from this source */
	unsigned int dwDlsr;		/* delay since last SR packet */
} RTCP_RR_T;

/* SDES item */
/* RTCP chunk types */
typedef enum {
	RTCP_SDES_END   = 0,		/* end chunk */
	RTCP_SDES_CNAME = 1,		/* canonical name */
	RTCP_SDES_NAME  = 2,		/* name */
	RTCP_SDES_EMAIL = 3,		/* email address of member */
	RTCP_SDES_PHONE = 4,		/* phone number of member */
	RTCP_SDES_LOC   = 5,		/* location of member */
	RTCP_SDES_TOOL  = 6,
	RTCP_SDES_NOTE  = 7,
	RTCP_SDES_PRIV  = 8
} rtcp_sdes_type_t;

typedef struct {
	rtcp_sdes_type_t bType;		/* type of item (rtcp_sdes_type_t) */
	unsigned char bLength;		/* length of item (in octets) */
	char data[2];			/* text, not null-terminated [2] for padding */
} rtcp_sdes_item_t;

typedef struct {
	unsigned int dwSsrc;		/* sender generating this report */
	unsigned int dwNtp_sec;		/* NTP timestamp: seconds */
	unsigned int dwNtp_frac;	/* NTP timestamp: fraction */
	unsigned int dwRtp_ts;		/* RTP timestamp */
	unsigned int dwPsent;		/* packets sent */
	unsigned int dwOsent;		/* octets sent */
	RTCP_RR_T rr[1];		/* variable-length list */
} RTCP_SR;
#define MAX_RTCP_GENERIC_NACK_FB_MSG 20
#define RTCP_NACK_FB_MASK_SIZE 16

typedef enum{
	RTCP_TRANSPORTFB_GENERIC_NACK = 1,
	RTCP_TRANSPORTFB_UNASSIGNED
}rtcp_tranportfb_fmt_type;

typedef struct {
	unsigned short wLostSeqNo;
	unsigned short wLostBitMask;
} RTCP_GENERIC_NACK_FB;
typedef struct {
	unsigned int	dwSsrc;                 // data source being reported
	RTCP_GENERIC_NACK_FB nack_fb[MAX_RTCP_GENERIC_NACK_FB_MSG];
} RTCP_FB;

/* One RTCP packet */
typedef struct {
	RTCP_CM common; /* common header */
	union {
		/* sender report (SR) */
		RTCP_SR sr;

		/* reception report (RR) */
		struct {
			unsigned int dwSsrc; /* receiver generating this report */
			RTCP_RR_T rr[1];     /* variable-length list */
		} rr;

		/* source description (SDES) */
		struct rtcp_sdes {
			unsigned int dwSsrc;      /* first SSRC/CSRC */
			rtcp_sdes_item_t item[1]; /* list of SDES items */
		} sdes;

		/* BYE */
		struct {
			unsigned int dwSsrc[1]; /* list of sources */
			/* can't express trailing text for reason */
		} bye;
		//FB
		struct {
			unsigned int dwSsrc;
			RTCP_FB fb;
		}fb;
		//Application FeedBack
		struct {
			unsigned int ssrc_sender;
			unsigned int ssrc_mediasource;
		}app_fb;
	} r;
} RTCP_HDR;
typedef struct rtcp_sdes rtcp_sdes_t;

typedef struct {
	int rtp_pkts_sent;
	int rtp_pkts_recvd;
	int rtp_bytes_sent;
	int rtp_bytes_recvd;
} SYS_SPECIFIC_STATS;

typedef struct {
	unsigned int rtp_pktsSent;
	unsigned int rtp_pktsRecvd;
	unsigned int rtp_bytesSent;
	unsigned int rtp_bytesReceived;
	unsigned int rtp_interarrivalJitter;
	unsigned int rtp_roundTripDelay;
	unsigned int rtp_pktsLost;
	unsigned int rtp_pktsDropped;
	unsigned int rtp_decodeLatency;
	unsigned char rtp_rxPt;
} LINE_SPECIFIC_STATS;

typedef struct {
	SYS_SPECIFIC_STATS sys_stats;
	LINE_SPECIFIC_STATS line_stats[4];
} RTP_VOICE_STATS;

typedef enum {
	RTP_QUERY,
	RTCP_QUERY,
	RTP_QUERY_VOICE_STATISTICS,
	RTP_ICMP,
	RTP_EVT_NSE
} MEDIA_QUERY_TYPE;

typedef struct {
	unsigned int eMedia;
	unsigned int iChannelID;
	unsigned int iRTCPQueryType;
	RTCP_HDR RTCPPkt;
	RTP_EVENT RtpEvent;
	RTP_VOICE_STATS RtpVoiceStats;
} RTCP_QUERY_RESPONSE;

typedef struct {
	MEDIA_QUERY_TYPE eMedia;
	unsigned short iRTCPQueryType;
	int iChannelID;
} MEDIA_QUERY_REQUEST;

typedef MEDIA_QUERY_REQUEST* PMEDIA_QUERY_REQUEST;

typedef struct {
	MEDIA_QUERY_TYPE eMedia;
	RTCP_QUERY_RESPONSE sRTCPQueryResponse;
} MEDIA_QUERY_RESPONSE;

#define RTCP_QUERY_CURRENT_PARAMS	0x00000100
#define RTCP_QUERY_INCOMING_REPORTS	0x00000001
#define RTCP_QUERY_OUTGOING_REPORTS	0x00000010
#define RTP_QUERY_EVENT_REPORTS		0x00000011

/* RTP header definition */
typedef struct {
	unsigned char vpxcc;		/* protocol version, padding flag, header extension flag, CSRC count */
	unsigned char mrk_pt;		/* marker bit & payload type */
	unsigned short seq_no;		/* sequence number */
	unsigned int time_stamp;	/* timestamp */
	unsigned int ssrc;		/* synchronization source */
} t_st_rtp_hdr;

/* cfifo packet type */
#define CFIFO_RTP_PACKET  1
#define CFIFO_RTCP_PACKET 2

#define RTP_APP_RTP_HDR_SIZE 12
#ifndef __KERNEL__
#if     BYTE_ORDER == LITTLE_ENDIAN
#define CG0_HTONS(u16_H) ( ((u8)((u16_H)>> 0)<< 8) | ((u8)((u16_H)>> 8)<< 0) )
#define CG0_NTOHS(u16_N) CG0_HTONS(u16_N)
#define CG0_HTONL(u16_H) ( ((u8)((u16_H)>> 0)<<24) | ((u8)((u16_H)>> 8)<<16)  | ((u8)((u16_H)>>16)<<8)  | ((u8)((u16_H)>>24)<<0) )
#define CG0_NTOHL(u16_N) CG0_HTONL(u16_N)
#elif   BYTE_ORDER == BIG_ENDIAN
	#define CG0_HTONS(u16_H)
	#define CG0_NTOHS(u16_N)
	#define CG0_HTONL(u16_H)
	#define CG0_NTOHL(u16_N)
#else
	#error "undefined/invalid unsigned char order"
#endif
#endif

/** struct voice_kernelmode - holds parameters for kernelmode rtp
 *
 * @sock_fd: file descriptor pointing to socket to be used for rtp rx/tx
 * @remote_addr: address of peer
 */
struct voice_kernelmode {
	int sock_fd;
	struct sockaddr_storage remote_addr;
};

/** struct voice_kernelmode_rtcp - holds parameters for kernelmode rtcp
 *
 * @sock_fd: file descriptor pointing to socket to be used for rtcp rx/tx
 * @remote_addr: address of peer
 */
struct voice_kernelmode_rtcp {
	int sock_fd;
	struct sockaddr_storage remote_addr;
	rtcp_session_config rtcpCfg;
};

struct voice_kernelmode_ip {
	int rtp_sock_fd,rtcp_sock_fd;
	struct sockaddr_storage rtp_remote_addr,rtcp_remote_addr;
};

struct voice_fifo_info {
	unsigned long enc_offset;
	unsigned long enc_size;
	unsigned long dec_offset;
	unsigned long dec_size;
};

struct voice_css_error {
	int error_num;
};

/* ioctl definitions */
#define VOICE_IOC_MAGIC		'V'

#define VOICE_IOCSETCODEC	_IOW(VOICE_IOC_MAGIC, 0, rtp_session_config *)
#define VOICE_IOCFLUSH		_IO (VOICE_IOC_MAGIC, 1)
#define VOICE_IOCKERNELMODE	_IOW(VOICE_IOC_MAGIC, 2, struct voice_kernelmode *)
#define VOICE_IOCSENDDTMF	_IOW(VOICE_IOC_MAGIC, 3, rtp_dtmf_event *)
#define VOICE_IOCGETDTMF	_IOR(VOICE_IOC_MAGIC, 4, rtp_dtmf_event *)
#define VOICE_IOCRTCP		_IOW(VOICE_IOC_MAGIC, 5, struct voice_kernelmode_rtcp *)
#define VOICE_IOCUPDATE_SESSION	_IOW(VOICE_IOC_MAGIC, 6, rtp_session_config *)
#define VOICE_IOCGENERIC_EVENT	_IOW(VOICE_IOC_MAGIC, 7, rtp_generic_event *)
#define VOICE_IOCSTOP_SESSION	_IO (VOICE_IOC_MAGIC, 8)
#define VOICE_IOCUPDATE_CHAN_PARAMS _IOW(VOICE_IOC_MAGIC, 9, \
					struct voice_kernelmode_ip *)
#define VOICE_IOCGET_FIFO_INFO	_IOR(VOICE_IOC_MAGIC, 10, struct voice_fifo_info *)
#define VOICE_IOCGET_LAST_CSS_ERROR _IOR(VOICE_IOC_MAGIC, 11, struct voice_css_error *)
#define VOICE_IOCRTCPUPDATE_SESSION  _IOW(VOICE_IOC_MAGIC, 12, struct rtcp_session_config *)
#define VOICE_IOCVOCODERSTART _IOW(VOICE_IOC_MAGIC, 13, void *)
#ifndef CONFIG_CORDLESS_USER_SPACE_FIFOS
#define VOICE_IOCSETPRIORITY	_IOW(VOICE_IOC_MAGIC, 14, \
				struct thread_priority_param *)

#define VOICE_IOC_MAXNR		14
#else
#define VOICE_IOC_MAXNR		13
#endif

/* struct __rtp_packet_header :- Specify Packet Header information between Kernel & cordless
 * @packetType: Describe packet type for RTP/RTCP
 * @ttl: time to live value for packet
 * @receiptTime : packet receipt time
 */

struct rtp_packet_header {
	unsigned long packetType;
	unsigned long ttl;
	unsigned long receiptTime;
};

#endif
