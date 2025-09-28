/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __VPU_FW_DATA_H__
#define __VPU_FW_DATA_H__

#ifdef __KERNEL__
#include <linux/types.h>

#else /* for ta compiler */
#ifndef BIT
#define BIT(nr)         ((unsigned long)(1) << (nr))
#endif

typedef char s8;
typedef short s16;
typedef int s32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#ifdef __aarch64__
typedef unsigned long u64;
#else
typedef unsigned long long u64;
#endif
/* __KERNEL__ */
#endif

#ifdef CONFIG_OPTEE
#define VPU_VOID_PTR uint64_t
#else
#define VPU_VOID_PTR uint32_t
#endif

#define VPU_TA_VER_MAJOR(ver)			((ver >> 24U) & 0x0fU)
#define VPU_TA_VER_MINOR(ver)			((ver >> 16U) & 0x0fU)
#define VPU_TA_API_VER_MAJOR(ver)		((ver >> 8U) & 0x0fU)
#define VPU_TA_API_VER_MINOR(ver)		((ver) & 0x0fU)
#define VPU_TA_VERSION_MAKE(ta_major, ta_minor, api_major, api_minor) \
	(((ta_major & 0x0fU) << 24U) | ((ta_minor & 0x0fU) << 16) |   \
	 ((api_major & 0x0fU) << 8U) | ((api_minor & 0x0fU)))

#define VPU_TA_VERSION_MAJOR			(6)
#define VPU_TA_ENC_API_VER_MAJOR		(4)
#define VPU_TA_DEC_API_VER_MAJOR		(5)

#define BERLIN_VPU_STATUS_ERROR			BIT(0)
#define BERLIN_VPU_STATUS_WAIT_INTERRUPTER	BIT(1)
#define BERLIN_VPU_STATUS_DONE			BIT(2)
/**
 * This flag is used for marking current context could end in those
 * the FW bi-state events
 */
#define BERLIN_VPU_STATUS_PROC_END_SAFE		BIT(3)
#define BERLIN_VPU_STATUS_POP_BREAK		BIT(4)
#define BERLIN_VPU_STATUS_NEW_SEQUENCE		BIT(5)
#define BERLIN_VPU_STATUS_WAIT_INPUT_BUF	BIT(7)
#define BERLIN_VPU_STATUS_WAIT_OUTPUT_BUF	BIT(8)
#define BERLIN_VPU_STATUS_WAIT_AUX_BUF		BIT(9)
#define BERLIN_VPU_STATUS_WAIT_DS_BUF		BIT(10)
#define BERLIN_VPU_STATUS_PUSH_INPUT_BUFS	BIT(11)
#define BERLIN_VPU_STATUS_PUSH_OUTPUT_BUFS	BIT(12)
#define BERLIN_VPU_STATUS_PUSH_AUX_BUFS		BIT(13)
/* legacy API */
#define BERLIN_VPU_STATUS_LEGACY_UD_ERROR	BIT(14)

#define BERLIN_VPU_CFG_LEGACY_API_ENABLED	BIT(0)
#define BERLIN_VPU_CFG_AUX_POOL_ENABLED		BIT(1)
#define BERLIN_VPU_CFG_LEGACY_RAW_BS_PUSH	BIT(2)
#define BERLIN_VPU_CFG_LEGACY_API_METADATA	BIT(3)

#define BERLIN_BUF_FLAG_ERROR			0x00000002
#define BERLIN_BUF_FLAG_DONE			0x00000004
#define BERLIN_BUF_FLAG_SEQ_HEADER		0x00000008
#define BERLIN_BUF_FLAG_KEYFRAME		0x00000008
#define BERLIN_BUF_FLAG_PFRAME			0x00000010
#define BERLIN_BUF_FLAG_BFRAME			0x00000040
#define BERLIN_BUF_FLAG_COMPRESSED		0x10000000

#define SYNA_VPU_BUF_MAX_PLANES			(8U)
#define BERLIN_BUF_V4G_MVTP_PLANE_IDX		(4U)

#define BERLIN_GFP_FLAG_NO_MAPPING		0x20000000
#define BERLIN_GFP_FLAG_NO_CACHE_INVALIDATE	0x10000000
#define BERLIN_GFP_FLAG_NO_CACHE_CLEAN		0x08000000

#define BERLIN_VPU_CFG_RESERVED_SIZE		(256U)
#define BERLIN_VPU_CTX_RESERVED_SIZE		(4096U)

/* We just use these macro to tell current pixel format */
#define SYNA_VXG_NV12		(0x3231564e)
#define SYNA_VXG_NV15		(0x3531564e)
#define SYNA_VXG_V4H1		(0x31483456)
#define SYNA_VXG_V4H3P8		(0x33483456)

#define SYNA_H1_ENC_FOURCC(a, b, c, d) \
	((u32)(d) | ((u32)(c) << 8) | ((u32)(b) << 16) | ((u32)(a) << 24))

#define SYNA_VXG_FOURCC(a, b, c, d) \
	((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

#define SYNA_H1_ENC_H264	SYNA_H1_ENC_FOURCC('H', '2', '6', '4')
#define SYNA_H1_ENC_VP8		SYNA_H1_ENC_FOURCC('\0', 'V', 'P', '8')

#define SYNA_VXG_MPEG2		SYNA_VXG_FOURCC('M', 'P', 'G', '2')
#define SYNA_VXG_H264		SYNA_VXG_FOURCC('H', '2', '6', '4')
#define SYNA_VXG_H265		SYNA_VXG_FOURCC('H', '2', '6', '5')
#define SYNA_VXG_VP8		SYNA_VXG_FOURCC('V', 'P', '8', '\0')
#define SYNA_VXG_VP9		SYNA_VXG_FOURCC('V', 'P', '9', '\0')
#define SYNA_VXG_AV1		SYNA_VXG_FOURCC('A', 'V', '1', '\0')

enum {
	VMETA_GET_CONST_V2,
	VMETA_OPEN_V2,
	VMETA_CLOSE_V2,

	VPU_REG_BUF_V2 = 3,
	VPU_UNREG_BUF_V2,

	VPU_SET_DEBUG_LEVEL_V2 = 5,
	VPU_CLK_CTRL_V2,

	VDEC_CREATE_INSTANCE_V2 = 7,
	VDEC_CONFIG_STREAM_V2,
	VDEC_PUSH_BUFFER_V2,
	VDEC_POP_BUFFER_V2,
	VDEC_DECODE_STREAM_V2,
	VDEC_DRAIN_POOL_V2,
	/* was TIMEOUT */
	VDEC_ABORT_HW_V2,
	/* was CLOSE_STREAM */
	VDEC_RESET_INSTANCE_V2,
	VDEC_STREAM_SWITCH_IN_V2,
	VDEC_STREAM_SWITCH_OUT_V2,
	VDEC_DESTROY_INSTANCE_V2,

	VENC_CREATE_INSTANCE_V2 = 18,
	VENC_CONFIG_STREAM_V2,
	VENC_PUSH_BUFFER_V2,
	VENC_POP_BUFFER_V2,
	VENC_ENCODE_STREAM_V2,
	VENC_CLOSE_STREAM_V2,
	VENC_STREAM_SWITCH_IN_V2,
	VENC_STREAM_SWITCH_OUT_V2,
	VENC_DESTROY_INSTANCE_V2,

	VPU_CTX_SWITCH_INST_v2 = 27,

	VMETA_CMD_MAX_V2
};

enum syna_vpu_mem_type {
	/* memid */
	CONTIGUOUS_MEM = 0x0,
	SCATTER_MEM = 0x1,
	/* phy addr input, TA would register a memid */
	CONTIGUOUS_IMPORT = 0x2,

	MEM_ID_CONTINOUS = CONTIGUOUS_MEM,
	MEM_ID_SCATTER = SCATTER_MEM,
	PHY_CONTINOUS = CONTIGUOUS_IMPORT,
};

enum syna_vpu_buf_type {
	DECODER_STREAM = 0x0,
	DECODER_PICTURE = 0x1,
	ENCODER_PICTURE = 0x2,
	ENCODER_STREAM = 0x4,
};

enum syna_vpu_fw_dec_buf {
	/* input queue */
	SYNA_VPU_FW_VDEC_STRM,
	/* output queue */
	SYNA_VPU_FW_VDEC_REF,
	/* post processor queue */
	SYNA_VPU_FW_VDEC_DISP,
};

enum syna_vpu_fw_enc_buf {
	/* input queue */
	SYNA_VPU_FW_VENC_VID,
	/* output queue */
	SYNA_VPU_FW_VENC_STRM,
	/* reconstruction queue */
	SYNA_VPU_FW_VENC_RECON,
	/* downscale queue */
	SYNA_VPU_FW_VENC_DS,
};

/* It would use the same value as enum v4l2_field */
enum syna_frame_field {
	SYNA_FRAME_FILED_ANY,
	SYNA_FRAME_FILED_NONE,
	SYNA_FRAME_FILED_TOP,
	SYNA_FRAME_FILED_BOTTOM,
	SYNA_FRAME_FIELD_INTERLACED_TB = 8,
	SYNA_FRAME_FIELD_INTERLACED_BT,
};

/* ***VPU firmware APIs begins here*** */
/* VPU Firmware version: 34790 */
#ifdef __KERNEL__
enum syna_dec_channel
{
	VDEC_DISP_CH0 = 0,
	VDEC_DISP_CH1,
	VDEC_DISP_CH_NUM
};
#endif

#ifdef __KERNEL__
enum syna_dec_format
{
	VDEC_OUTPUT_UYVY = 0,
	VDEC_OUTPUT_VYUY,
	VDEC_OUTPUT_YUYV,
	VDEC_OUTPUT_YVYU,
	VDEC_OUTPUT_SPUV,
	VDEC_OUTPUT_SPUV_10bDWA,
	VDEC_OUTPUT_SPVU,
	VDEC_OUTPUT_YUV,
	VDEC_OUTPUT_YVU,
	VDEC_OUTPUT_TILE,
	VDEC_OUTPUT_TILE_TILE,
	VDEC_OUTPUT_TILE_DUPLICATE = VDEC_OUTPUT_TILE_TILE,
	VDEC_OUTPUT_NONE,
	VDEC_OUTPUT_AUTO,
	VDEC_OUTPUT_TILE_420SP_AUTO,
	VDEC_OUTPUT_MULTI_CHANNEL,
	VDEC_OUTPUT_FORMAT_NUM
};
#endif

#ifdef __KERNEL__
enum syna_enc_format
{
	VENC_YUV420_TILE = 0,
	VENC_YUV422_UYVY,
	VENC_YUV422_YUYV,
	VENC_YUV420_PLANE,
	VENC_YUV422_PLANE,
	VENC_YUV420_SP,
	VENC_YUV420_SPA,
	VENC_YUV420_YV12,
	VENC_RGBA,
	VENC_BGRA,
	VENC_RGB565,
	VENC_BGR565,
	VENC_RGB555,
	VENC_BGR555,
	VENC_RGB444,
	VENC_BGR444,
	VENC_RGB888,
	VENC_BGR888,
	VENC_RGB101010,
	VENC_BGR101010,
	VENC_INPUT_UNSUPPORTED
};
#endif

#ifdef __KERNEL__
enum syna_enc_profile
{
	VCENC_HEVC_PROFILE_MAIN = 0,
	VCENC_HEVC_PROFILE_MAIN_STILL_PICTURE = 1,
	VCENC_HEVC_PROFILE_MAIN_10 = 2,
	VCENC_H264_PROFILE_BASE = 9,
	VCENC_H264_PROFILE_MAIN = 10,
	VCENC_H264_PROFILE_HIGH = 11,
	VCENC_H264_PROFILE_HIGH_10 = 12,
	VENC_PROFILE_BASE = 66,
	VENC_PROFILE_MAIN = 77,
	VENC_PROFILE_EXTENDED = 88,
	VENC_PROFILE_HIGH = 100
};
#endif

#ifdef __KERNEL__
enum syna_enc_level
{
	VENC_LEVEL_1_0 = 10,
	VENC_LEVEL_1_1,
	VENC_LEVEL_1_2,
	VENC_LEVEL_1_3,
	VENC_LEVEL_2_0 = 20,
	VENC_LEVEL_2_1,
	VENC_LEVEL_2_2,
	VENC_LEVEL_3_0 = 30,
	VENC_LEVEL_3_1,
	VENC_LEVEL_3_2,
	VENC_LEVEL_4_0 = 40,
	VENC_LEVEL_4_1,
	VENC_LEVEL_4_2,
	VENC_LEVEL_5_0 = 50,
	VENC_LEVEL_5_1,
	VENC_LEVEL_5_2,
	VENC_LEVEL_6_0 = 60,
	VENC_LEVEL_6_1,
	VENC_LEVEL_6_2
};
#endif

#ifdef __KERNEL__
enum syna_enc_pic_type
{
	VENC_PIC_TYPE_I,
	VENC_PIC_TYPE_P,
	VENC_PIC_TYPE_PSKIP,
	VENC_PIC_TYPE_B,
	VENC_PIC_TYPE_BREF,
	VENC_PIC_TYPE_FORCE_IDR,
	VENC_PIC_TYPE_FORCE_I,
	VENC_PIC_TYPE_NA
};
#endif

#ifdef __KERNEL__
enum syna_venc_ref_opt
{
	VENC_NO_REFERENCE_NO_REFRESH = 0,
	VENC_REFERENCE = 1,
	VENC_REFRESH = 2,
	VENC_REFERENCE_AND_REFRESH = 3
};
#endif

struct syna_vpu_hw_info
{
	unsigned int hw_version;
	VPU_VOID_PTR fw_version;
	VPU_VOID_PTR version_string;
	unsigned int vmeta_enable;
	unsigned int v2g_enable;
	unsigned int g2_enable;
	unsigned int g1_enable;
	unsigned int h1_0_enable;
	unsigned int h1_1_enable;
	unsigned int vc8000e_enable;
	unsigned int rsvd0;
	unsigned int rsvd1;
}__attribute__ ((aligned(4)));

struct syna_vdec_config
{
	VPU_VOID_PTR user_data;
	VPU_VOID_PTR internal_data;
	VPU_VOID_PTR vdec_cntxt;
	VPU_VOID_PTR base_strm;
	unsigned int format;
	unsigned int hw_context_addr;
	VPU_VOID_PTR hw_context_vaddr;
	unsigned int strm_pool_addr;
	unsigned int strm_pool_size;
	unsigned int mon_width;
	unsigned int mon_height;
	unsigned int max_width;
	unsigned int max_height;
	unsigned int fmtc_in_stride;
	unsigned int min_strm_size;
	unsigned int no_reordering;
	unsigned int ref_padding_mode;
	unsigned int pp_scan_mode;
	unsigned int pp_filter_mode;
	unsigned int pp_hscale;
	unsigned int pp_vscale;
	unsigned int pp_hscale_numer;
	unsigned int pp_hscale_denom;
	unsigned int dblk_mode;
	unsigned int mvc_mode;
	unsigned int aso_mode;
	unsigned int slc_opt_flag;
	unsigned int encry_flag;
	unsigned int disable_mem32_read;
	unsigned int disable_vld_dcg;
	unsigned int disable_gmc23;
	unsigned int enable_sei_3d;
	VPU_VOID_PTR dma_obj;
	unsigned int roi_x;
	unsigned int roi_y;
	unsigned int roi_w;
	unsigned int roi_h;
	unsigned int slc_row;
	unsigned int frame_in_mode;
	unsigned int p_frame_seek;
	unsigned int idr_seek;
	unsigned int idr_err_skip;
	unsigned int post_deblocking;
	unsigned char user_dpb_size;
	unsigned char user_max_reorder;
	unsigned char reserved3;
	unsigned char reserved4;
	unsigned int rvcombo_flag;
	unsigned int output_mode;
	unsigned int output_mode_ch[VDEC_DISP_CH_NUM];
	unsigned int tile_ref_flag;
	unsigned int tile_8bmode;
	unsigned int no_res_change;
	unsigned int force_ref_out;
	unsigned int disable_dis_out;
	unsigned int vxrd_paddr;
	unsigned int p3wr_paddr;
	unsigned int rotation_allow;
	unsigned int rotate_mode;
	unsigned int disable_min_strm_size;
	unsigned int dpb_force_flush;
	unsigned int skipNonReference;
	unsigned int dither_enable;
	unsigned int dither_bit;
	unsigned char dbg_log_enable;
	unsigned char turbo_mode;
	unsigned char reserved1;
	unsigned char reserved2;
	unsigned int ref_stride;
	unsigned int dis_stride;
	unsigned int mtuTile;
	unsigned int mtuDisp;
	unsigned int fetch_collect;
	unsigned int max_support_layer;
	unsigned int buf_combined_mode;
	unsigned int enable_deinterlace;
	unsigned int vp8_strm_mode;
	unsigned int dis_offset_uv;
	unsigned int dis_offset_uv1;
	unsigned int mtr_enable;
	unsigned int mtr_x_align_mode;
	unsigned int mtr_meta_base;
	unsigned int qos_w_high;
	unsigned int qos_w_low;
	unsigned int qos_r_high;
	unsigned int qos_r_low;
	unsigned char h265_tile_opt;
	unsigned char h264_low_latency_output;
	unsigned char reserved5;
	unsigned char reserved6;
	unsigned char reserved7;
	unsigned char disable_gdr;
	unsigned char operating_point;
	unsigned char config_mode;
	unsigned int mmu_enable;
	unsigned char mmu_page_size;
	unsigned int mmu_page_base;
	unsigned int error_pic_threshold;
	unsigned int av1_lgst_param;
	unsigned char shuffle_en;
	unsigned char fast_channel_zapping;
	unsigned char mtu_dhub;
	unsigned char fast_mode;
	unsigned char mtr_force_uncmpr;
	unsigned char overallocation_mode;
	unsigned char super_mode;
	unsigned char qos_mmu;
	unsigned int wdt_cnt;
}__attribute__ ((aligned(4)));

struct syna_venc_strm_config
{
	VPU_VOID_PTR user_data;
	VPU_VOID_PTR internal_data;
	VPU_VOID_PTR venc_cntxt;
	unsigned int format;
	unsigned int hw_context_addr;
	VPU_VOID_PTR hw_context_vaddr;
	unsigned int src_yuv_format;
	unsigned int use_1stpass;
	unsigned int frm_width;
	unsigned int frm_height;
	unsigned int field_flag;
	unsigned int interlaced_frame;
	unsigned int mbaff_enable;
	unsigned int disable_deblocking;
	unsigned int poc_type;
	unsigned int entropy_mode;
	unsigned int transform_8x8_flag;
	unsigned int ref_num_in_buf;
	unsigned int sar_width;
	unsigned int sar_height;
	unsigned int profile_idc;
	unsigned int level_idc;
	unsigned int yuv_downsample;
	unsigned int vp8_strm_mode;
	unsigned int vp8_mvprob_mode;
	unsigned int vp8_multi_channel;
	unsigned int dis_offset;
	unsigned int dis_stride;
	unsigned int dis_buf_size;
	unsigned int frame_rate_num;
	unsigned int frame_rate_den;
	unsigned int src_video_format;
	unsigned int enc_mode;
	unsigned int b_hybrid;
	unsigned int express_top;
	unsigned int express_bottom;
	unsigned int express_left;
	unsigned int express_right;
	unsigned int turbo_rate;
	unsigned int gop_type;
	unsigned int gop_size;
	unsigned int open_gop;
	unsigned int switch_gop;
	unsigned int scene_change;
	unsigned int init_qp;
	unsigned int vbv_size;
	unsigned int hrd_rate;
	unsigned int min_rate;
	unsigned int max_rate;
	unsigned int dri_mb_rows;
	int rc_sel;
	unsigned int min_qp_idx;
	unsigned int max_qp_idx;
	unsigned int tracking_window;
	unsigned int short_term_min_rate;
	unsigned int short_term_max_rate;
	unsigned int short_term_window;
	unsigned int par;
	unsigned int avc_on;
	unsigned int aud_on;
	unsigned int src_width;
	unsigned int src_height;
	unsigned int x_offset;
	unsigned int y_offset;
	unsigned int src_offset_uv;
	unsigned int src_offset_uv1;
	unsigned int ds_width;
	unsigned int ds_height;
	unsigned int rotation;
	unsigned int quarter_mv;
	unsigned int pictureRc;
	unsigned int mbRc;
	unsigned int pictureSkip;
	int qpHdr;
	unsigned int qpMin;
	unsigned int qpMax;
	unsigned int bitPerSecond;
	unsigned int hrd;
	unsigned int hrdCpbSize;
	unsigned int gopLen;
	int intraQpDelta;
	unsigned int fixedIntraQp;
	int mbQpAdjustment;
	unsigned int motion_x;
	unsigned int motion_y;
	unsigned int motion_w;
	unsigned int motion_h;
	unsigned int vp9_strm_mode;
	unsigned int compressor;
	unsigned int enableSao;
	unsigned int dump_rbuf;
	unsigned int video_full_range_flag;
	unsigned int color_primaries;
	unsigned int transfer_characteristics;
	unsigned int matrix_coeffs;
	unsigned char svctLevel;
	int svcQpDelta;
	unsigned char simulcast;
	unsigned int sliceSize;
	unsigned int cirStart;
	unsigned int cirInterval;
	unsigned int longTermPicRate;
}__attribute__ ((aligned(4)));

struct syna_venc_2ndpass_param
{
	unsigned int pic_type;
	unsigned int pic_sn;
	unsigned int pic_qp;
	unsigned int bitPerSecond;
	unsigned int hrdCpbSize;
	unsigned int sliceSize;
	unsigned int ipf;
	unsigned int ltrf;
}__attribute__ ((aligned(4)));

/* ***VPU firmware APIs struct end here*** */

struct syna_vpu_fw_info {
	u32 hw_version;
	u32 rsvd;
	u32 version_string;
	u32 vpu_context_size;
	u32 vpu_hw_context_size;
	u32 vdec_strm_context_size;
	u32 vdec_strm_hw_context_size;
	u32 venc_strm_context_size;
	u32 venc_strm_hw_context_size;
	u32 vdec_pool_size;
	u32 venc_pool_size;
} __attribute__ ((aligned(4)));

struct syna_vpu_fw_data {
	struct syna_vpu_fw_info fw_info;
	struct syna_vpu_hw_info hw_info;
	u32 fw_api_ver;
} __attribute__ ((aligned(4)));

struct syna_tz_generic_buf {
	u32 type /* enum syna_vpu_mem_type */ ;
	u32 size;
	/* secure info */
	u32 memid;
	/*
	 * When we have a valid memid, the final value of addr and vaddr
	 * should always plus the value in offset field. In the other
	 * word, addr and vaddr here would always be the value from memid.
	 */
	u32 offset;
	/* Physical address for CMA or bus(iommu) address */
	u64 addr;
	/* virtual address in a TA, without offset */
	VPU_VOID_PTR vaddr;
	u32 gfp_flags;
} __attribute__ ((aligned(8)));

struct syna_vpu_tz_buf {
	u32 buf_type;		/* enum syna_vpu_buf_type */
	u32 flags;
	u32 errors;
	u32 field;		/* enum syna_frame_field */

	/* cropping */
	u32 startx;
	u32 starty;
	u32 width;
	u32 height;

	u32 bytesused[SYNA_VPU_BUF_MAX_PLANES];

	u32 mtr_cfg0[8];
	u32 mtr_cfg1[8];

	u64 timestamp;
	/*
	 * V4G planes:
	 * luma
	 * chroma
	 * mtr luma
	 * mtr chroma
	 * mvtp
	 */
	struct syna_tz_generic_buf planes[SYNA_VPU_BUF_MAX_PLANES];
} __attribute__ ((aligned(8)));

struct idx_queue {
	uint32_t index[32];
	uint32_t push;
	uint32_t pop;
};

struct syna_vdec_seq_desc {
	u32 luma_size;
	u32 chroma_size;
	u32 meta_luma_size;
	u32 meta_chroma_size;
	u32 tctx_size;

	u32 disp_luma_size;
	u32 disp_chroma_size;

	u32 format;
	u32 startx;
	u32 starty;
	u32 width;
	u32 height;
	u16 max_ref_nums;
	u16 max_dpb_size;
} __attribute__ ((aligned(4)));

struct syna_vpu_ctrl {
	struct {
		uint32_t flags;

		uint8_t reserved0;
		uint32_t processed_bytes;
		uint32_t reserved1[5];
	} status __attribute__ ((aligned(4)));

	/* bit stream buffer */
	struct idx_queue sbuf;

	/* display or video frame buffer */
	struct idx_queue dbuf;

	/**
	 * reference buffer for the decoder(hardware output)
	 * reconstruction buffer for the encoder
	 */
	struct idx_queue rbuf;

	struct idx_queue dsbuf;

	struct syna_vpu_tz_buf inbufs[32];
	/* tile(ref) bufs for decoder */
	struct syna_vpu_tz_buf outbufs[32];
	/* post-display(linear) bufs for decoder, reconstruct for encoder */
	union {
		struct syna_vpu_tz_buf auxbufs[32];
		struct syna_vpu_tz_buf p_dispbufs[32];
	};
	struct syna_vpu_tz_buf dsbufs[32];

	/* self */
	struct syna_tz_generic_buf ctrl_buf;
	/**
	 * layout:
	 * **************************************
	 * TA internal instance data(PAGE_SIZE)
	 * **************************************
	 * strm_context_size
	 * **************************************
	 * Aligned to page size(4K)
	 * **************************************
	 * strm_hw_context_size
	 * **************************************
	 * Aligned to page size(4K)
	 * **************************************
	 */
	struct syna_tz_generic_buf ctx_buf;

	struct syna_tz_generic_buf mtr_base;
	struct {
		struct v4g_mmu_cfg {
			u32 en:1;
			u32 bank_mapping:1;
			u32 page_mapping:1;
			u32 shuffle:1;
			u32 reseved:12;
			u32 pm_h_log2_minus2:4;
			u32 pm_w_log2_minus6:4;
		} mmu;
	} v4g_ext_cfg __attribute__ ((aligned(4)));

	u32 cfg_flags;

	struct syna_vdec_seq_desc seq_desc;
	struct syna_venc_2ndpass_param venc_2ndpass_para[32];
	u32 venc_2ndpass_para_stats;

	struct h1_ext_cfg {
		u32 header_mode;
	} h1_extra_cfg __attribute__ ((aligned(4)));
} __attribute__ ((aligned(8)));

/* __VPU_FW_DATA_H__ */
#endif
