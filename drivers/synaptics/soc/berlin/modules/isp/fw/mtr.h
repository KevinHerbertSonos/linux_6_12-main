/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef mtr_h
#define mtr_h (){}
#pragma pack(1)
#ifdef __cplusplus
extern "C" {
#endif
#ifndef _DOCC_H_BITOPS_
#define _DOCC_H_BITOPS_ (){}
#define _bSETMASK_(b)                                      ((b)<32 ? (1<<((b)&31)) : 0)
#define _NSETMASK_(msb,lsb)                                (_bSETMASK_((msb)+1)-_bSETMASK_(lsb))
#define _bCLRMASK_(b)                                      (~_bSETMASK_(b))
#define _NCLRMASK_(msb,lsb)                                (~_NSETMASK_(msb,lsb))
#define _BFGET_(r,msb,lsb)                                 (_NSETMASK_((msb)-(lsb),0)&((r)>>(lsb)))
#define _BFSET_(r,msb,lsb,v)                               do { (r)&=_NCLRMASK_(msb,lsb); (r)|=_NSETMASK_(msb,lsb)&((v)<<(lsb)); } while(0)
#endif
#ifndef h_E_MTR
#define h_E_MTR (){}
#define        E_MTR_mode_8b_v4_y                                       0x0
#define        E_MTR_mode_8b_v4_uv                                      0x1
#define        E_MTR_mode_8b_v4_uyvy                                    0x2
#define        E_MTR_mode_8b_v4_argb                                    0x3
#define        E_MTR_mode_8b_v1_y                                       0x4
#define        E_MTR_mode_8b_v1_uv                                      0x5
#define        E_MTR_mode_8b_v1_uyvy                                    0x6
#define        E_MTR_mode_8b_v1_argb                                    0x7
#define        E_MTR_mode_p10b_v4_y                                     0x8
#define        E_MTR_mode_p10b_v4_uv                                    0x9
#define        E_MTR_mode_p10b_v4_uyvy                                  0xA
#define        E_MTR_mode_p10b_v4_argb                                  0xB
#define        E_MTR_mode_d10b_v1_y                                     0xC
#define        E_MTR_mode_d10b_v1_uv                                    0xD
#define        E_MTR_mode_p10b_v1_uyvy                                  0xE
#define        E_MTR_mode_p10b_v1_argb                                  0xF
#define        E_MTR_mode_r10b_v1_raw                                   0x10
#define        E_MTR_mode_r12b_v1_raw                                   0x11
#define        E_MTR_mode_r16b_v1_raw                                   0x12
#define        E_MTR_mode_r20b_v1_raw                                   0x13
#define        E_MTR_mode_8b_v8_y                                       0x14
#define        E_MTR_mode_8b_v8_uv                                      0x15
#define        E_MTR_mode_8b_v1_rgb                                     0x16
#define        E_MTR_mode_10b_v8_y                                      0x17
#define        E_MTR_mode_10b_v8_uv                                     0x18
#define        E_MTR_mode_10b_v6_y                                      0x19
#define        E_MTR_mode_10b_v6_uv                                     0x1A
	typedef struct SIE_E_MTR {
		UNSG32 u_mode                                      :  5;
		UNSG32 u_meta                                      :  2;
		UNSG32 RSVDx0_b7                                   : 25;
	} SIE_E_MTR;
	SIGN32 E_MTR_drvrd(SIE_E_MTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
	SIGN32 E_MTR_drvwr(SIE_E_MTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
	void E_MTR_reset(SIE_E_MTR *p);
	SIGN32 E_MTR_cmp  (SIE_E_MTR *p, SIE_E_MTR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define E_MTR_check(p,pie,pfx,hLOG) E_MTR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define E_MTR_print(p,    pfx,hLOG) E_MTR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_user
#define h_MTR_user (){}
#define     RA_MTR_user_dat                                0x0000
	typedef struct SIE_MTR_user {
#define     w32MTR_user_dat                                {\
	UNSG32 udat_uv                                     :  1;\
	UNSG32 udat_bid                                    :  4;\
	UNSG32 udat_enable                                 :  1;\
	UNSG32 udat_meta                                   :  2;\
	UNSG32 RSVDx0_b8                                   : 24;\
}
	union { UNSG32 u32MTR_user_dat;
		struct w32MTR_user_dat;
	};
} SIE_MTR_user;
typedef union  T32MTR_user_dat
{ UNSG32 u32;
	struct w32MTR_user_dat;
} T32MTR_user_dat;
typedef union  TMTR_user_dat
{ UNSG32 u32[1];
	struct {
		struct w32MTR_user_dat;
	};
} TMTR_user_dat;
SIGN32 MTR_user_drvrd(SIE_MTR_user *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_user_drvwr(SIE_MTR_user *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_user_reset(SIE_MTR_user *p);
SIGN32 MTR_user_cmp  (SIE_MTR_user *p, SIE_MTR_user *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_user_check(p,pie,pfx,hLOG) MTR_user_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_user_print(p,    pfx,hLOG) MTR_user_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_tid
#define h_MTR_tid (){}
#define     RA_MTR_tid_dat                                 0x0000
typedef struct SIE_MTR_tid {
#define     w32MTR_tid_dat                                 {\
	UNSG32 udat_uv                                     :  1;\
	UNSG32 udat_bid                                    :  4;\
	UNSG32 udat_wt                                     :  1;\
	UNSG32 RSVDx0_b6                                   : 26;\
}
union { UNSG32 u32MTR_tid_dat;
	struct w32MTR_tid_dat;
};
} SIE_MTR_tid;
typedef union  T32MTR_tid_dat
{ UNSG32 u32;
	struct w32MTR_tid_dat;
} T32MTR_tid_dat;
typedef union  TMTR_tid_dat
{ UNSG32 u32[1];
	struct {
		struct w32MTR_tid_dat;
	};
} TMTR_tid_dat;
SIGN32 MTR_tid_drvrd(SIE_MTR_tid *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_tid_drvwr(SIE_MTR_tid *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_tid_reset(SIE_MTR_tid *p);
SIGN32 MTR_tid_cmp  (SIE_MTR_tid *p, SIE_MTR_tid *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_tid_check(p,pie,pfx,hLOG) MTR_tid_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_tid_print(p,    pfx,hLOG) MTR_tid_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_roi
#define h_MTR_roi (){}
#define     RA_MTR_roi_roix                                0x0000
#define     RA_MTR_roi_roiy                                0x0004
typedef struct SIE_MTR_roi {
#define     w32MTR_roi_roix                                {\
	UNSG32 uroix_xs                                    : 12;\
	UNSG32 uroix_xm                                    : 12;\
	UNSG32 RSVDx0_b24                                  :  8;\
}
union { UNSG32 u32MTR_roi_roix;
	struct w32MTR_roi_roix;
};
#define     w32MTR_roi_roiy                                {\
	UNSG32 uroiy_ys                                    : 14;\
	UNSG32 uroiy_ym                                    : 14;\
	UNSG32 RSVDx4_b28                                  :  4;\
}
union { UNSG32 u32MTR_roi_roiy;
	struct w32MTR_roi_roiy;
};
} SIE_MTR_roi;
typedef union  T32MTR_roi_roix
{ UNSG32 u32;
	struct w32MTR_roi_roix;
} T32MTR_roi_roix;
typedef union  T32MTR_roi_roiy
{ UNSG32 u32;
	struct w32MTR_roi_roiy;
} T32MTR_roi_roiy;
typedef union  TMTR_roi_roix
{ UNSG32 u32[1];
	struct {
		struct w32MTR_roi_roix;
	};
} TMTR_roi_roix;
typedef union  TMTR_roi_roiy
{ UNSG32 u32[1];
	struct {
		struct w32MTR_roi_roiy;
	};
} TMTR_roi_roiy;
SIGN32 MTR_roi_drvrd(SIE_MTR_roi *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_roi_drvwr(SIE_MTR_roi *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_roi_reset(SIE_MTR_roi *p);
SIGN32 MTR_roi_cmp  (SIE_MTR_roi *p, SIE_MTR_roi *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_roi_check(p,pie,pfx,hLOG) MTR_roi_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_roi_print(p,    pfx,hLOG) MTR_roi_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_header0
#define h_MTR_header0 (){}
#define     RA_MTR_header0_meta                            0x0000
#define        MTR_header0_meta_stride_64B                              0x0
#define        MTR_header0_meta_stride_128B                             0x1
#define        MTR_header0_meta_stride_256B                             0x2
#define        MTR_header0_meta_stride_512B                             0x3
#define        MTR_header0_meta_stride_1024B                            0x4
#define        MTR_header0_meta_stride_2048B                            0x5
#define        MTR_header0_meta_stride_4096B                            0x6
#define        MTR_header0_meta_stride_8192B                            0x7
#define        MTR_header0_meta_stride_16384B                           0x8
#define     RA_MTR_header0_buffer                          0x0004
#define     RA_MTR_header0_frame                           0x0008
#define     RA_MTR_header0_offset                          0x000C
#define     RA_MTR_header0_addr_shuffle                    0x0010
#define     RA_MTR_header0_addr_shuffle_1                  0x0014
typedef struct SIE_MTR_header0 {
#define     w32MTR_header0_meta                            {\
	UNSG32 umeta_stride                                :  4;\
	UNSG32 umeta_format                                :  2;\
	UNSG32 umeta_width                                 :  5;\
	UNSG32 umeta_height                                : 10;\
	UNSG32 umeta_qos_en                                :  1;\
	UNSG32 umeta_qos                                   :  4;\
	UNSG32 umeta_prefetch_disable                      :  1;\
	UNSG32 RSVDx0_b27                                  :  5;\
}
union { UNSG32 u32MTR_header0_meta;
	struct w32MTR_header0_meta;
};
#define     w32MTR_header0_buffer                          {\
	UNSG32 sbuffer_frm_ofst                            :  8;\
	UNSG32 ubuffer_format                              :  5;\
	UNSG32 ubuffer_force_uncmpr                        :  1;\
	UNSG32 ubuffer_addr_shuffle_en                     :  1;\
	UNSG32 ubuffer_flow_ctrl                           :  1;\
	UNSG32 RSVDx4_b16                                  : 16;\
}
union { UNSG32 u32MTR_header0_buffer;
	struct w32MTR_header0_buffer;
};
#define     w32MTR_header0_frame                           {\
	UNSG32 uframe_stride                               : 32;\
}
union { UNSG32 u32MTR_header0_frame;
	struct w32MTR_header0_frame;
};
#define     w32MTR_header0_offset                          {\
	UNSG32 uoffset_x                                   : 11;\
	UNSG32 uoffset_y                                   : 14;\
	UNSG32 RSVDxC_b25                                  :  7;\
}
union { UNSG32 u32MTR_header0_offset;
	struct w32MTR_header0_offset;
};
#define     w32MTR_header0_addr_shuffle                    {\
	UNSG32 uaddr_shuffle_bit_06                        :  5;\
	UNSG32 uaddr_shuffle_bit_07                        :  5;\
	UNSG32 uaddr_shuffle_bit_08                        :  5;\
	UNSG32 uaddr_shuffle_bit_09                        :  5;\
	UNSG32 uaddr_shuffle_bit_10                        :  5;\
	UNSG32 uaddr_shuffle_bit_11                        :  5;\
	UNSG32 RSVDx10_b30                                 :  2;\
}
union { UNSG32 u32MTR_header0_addr_shuffle;
	struct w32MTR_header0_addr_shuffle;
};
#define     w32MTR_header0_addr_shuffle_1                  {\
	UNSG32 uaddr_shuffle_1_bit_12                      :  5;\
	UNSG32 uaddr_shuffle_1_bit_13                      :  5;\
	UNSG32 uaddr_shuffle_1_bit_14                      :  5;\
	UNSG32 uaddr_shuffle_1_bit_15                      :  5;\
	UNSG32 RSVDx14_b20                                 : 12;\
}
union { UNSG32 u32MTR_header0_addr_shuffle_1;
	struct w32MTR_header0_addr_shuffle_1;
};
} SIE_MTR_header0;
typedef union  T32MTR_header0_meta
{ UNSG32 u32;
	struct w32MTR_header0_meta;
} T32MTR_header0_meta;
typedef union  T32MTR_header0_buffer
{ UNSG32 u32;
	struct w32MTR_header0_buffer;
} T32MTR_header0_buffer;
typedef union  T32MTR_header0_frame
{ UNSG32 u32;
	struct w32MTR_header0_frame;
} T32MTR_header0_frame;
typedef union  T32MTR_header0_offset
{ UNSG32 u32;
	struct w32MTR_header0_offset;
} T32MTR_header0_offset;
typedef union  T32MTR_header0_addr_shuffle
{ UNSG32 u32;
	struct w32MTR_header0_addr_shuffle;
} T32MTR_header0_addr_shuffle;
typedef union  T32MTR_header0_addr_shuffle_1
{ UNSG32 u32;
	struct w32MTR_header0_addr_shuffle_1;
} T32MTR_header0_addr_shuffle_1;
typedef union  TMTR_header0_meta
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header0_meta;
	};
} TMTR_header0_meta;
typedef union  TMTR_header0_buffer
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header0_buffer;
	};
} TMTR_header0_buffer;
typedef union  TMTR_header0_frame
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header0_frame;
	};
} TMTR_header0_frame;
typedef union  TMTR_header0_offset
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header0_offset;
	};
} TMTR_header0_offset;
typedef union  TMTR_header0_addr_shuffle
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header0_addr_shuffle;
	};
} TMTR_header0_addr_shuffle;
typedef union  TMTR_header0_addr_shuffle_1
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header0_addr_shuffle_1;
	};
} TMTR_header0_addr_shuffle_1;
SIGN32 MTR_header0_drvrd(SIE_MTR_header0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_header0_drvwr(SIE_MTR_header0 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_header0_reset(SIE_MTR_header0 *p);
SIGN32 MTR_header0_cmp  (SIE_MTR_header0 *p, SIE_MTR_header0 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_header0_check(p,pie,pfx,hLOG) MTR_header0_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_header0_print(p,    pfx,hLOG) MTR_header0_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_header
#define h_MTR_header (){}
#define     RA_MTR_header_meta                             0x0000
#define        MTR_header_meta_stride_64B                               0x0
#define        MTR_header_meta_stride_128B                              0x1
#define        MTR_header_meta_stride_256B                              0x2
#define        MTR_header_meta_stride_512B                              0x3
#define        MTR_header_meta_stride_1024B                             0x4
#define        MTR_header_meta_stride_2048B                             0x5
#define        MTR_header_meta_stride_4096B                             0x6
#define        MTR_header_meta_stride_8192B                             0x7
#define        MTR_header_meta_stride_16384B                            0x8
#define        MTR_header_meta_format_V16H16V1                          0x0
#define        MTR_header_meta_format_V8H16V2                           0x1
#define        MTR_header_meta_format_V4H16V4                           0x2
#define        MTR_header_meta_format_V8H32V1                           0x4
#define        MTR_header_meta_format_V4H32V2                           0x5
#define        MTR_header_meta_format_V2H32V4                           0x6
#define        MTR_header_meta_format_V1H32V2                           0x8
#define        MTR_header_meta_format_V1H16V4                           0x9
#define        MTR_header_meta_format_V1H8V8                            0xA
#define        MTR_header_meta_format_V1H4V16                           0xB
#define        MTR_header_meta_format_V1H64V1                           0xC
#define        MTR_header_meta_ctype_64B                                0x0
#define        MTR_header_meta_ctype_32B                                0x1
#define     RA_MTR_header_buffer                           0x0004
#define     RA_MTR_header_pos                              0x0008
typedef struct SIE_MTR_header {
#define     w32MTR_header_meta                             {\
	UNSG32 umeta_stride                                :  4;\
	UNSG32 umeta_format                                :  4;\
	UNSG32 umeta_prefetch_disable                      :  1;\
	UNSG32 umeta_flow_ctrl                             :  1;\
	UNSG32 umeta_qos_en                                :  1;\
	UNSG32 umeta_qos                                   :  4;\
	UNSG32 umeta_cbase                                 :  5;\
	UNSG32 umeta_cways                                 :  3;\
	UNSG32 umeta_ctype                                 :  1;\
	UNSG32 umeta_last_mosaicy                          :  1;\
	UNSG32 umeta_mosaic_xnum                           :  2;\
	UNSG32 umeta_weave                                 :  1;\
	UNSG32 umeta_byte                                  :  1;\
	UNSG32 umeta_hflip                                 :  1;\
	UNSG32 umeta_vflip                                 :  1;\
	UNSG32 RSVDx0_b31                                  :  1;\
}
union { UNSG32 u32MTR_header_meta;
	struct w32MTR_header_meta;
};
#define     w32MTR_header_buffer                           {\
	UNSG32 ubuffer_format                              :  5;\
	UNSG32 ubuffer_force_uncmpr                        :  1;\
	UNSG32 RSVDx4_b6                                   : 26;\
}
union { UNSG32 u32MTR_header_buffer;
	struct w32MTR_header_buffer;
};
#define     w32MTR_header_pos                              {\
	UNSG32 upos_startx                                 :  8;\
	UNSG32 upos_endx                                   :  8;\
	UNSG32 upos_height                                 : 16;\
}
union { UNSG32 u32MTR_header_pos;
	struct w32MTR_header_pos;
};
} SIE_MTR_header;
typedef union  T32MTR_header_meta
{ UNSG32 u32;
	struct w32MTR_header_meta;
} T32MTR_header_meta;
typedef union  T32MTR_header_buffer
{ UNSG32 u32;
	struct w32MTR_header_buffer;
} T32MTR_header_buffer;
typedef union  T32MTR_header_pos
{ UNSG32 u32;
	struct w32MTR_header_pos;
} T32MTR_header_pos;
typedef union  TMTR_header_meta
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header_meta;
	};
} TMTR_header_meta;
typedef union  TMTR_header_buffer
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header_buffer;
	};
} TMTR_header_buffer;
typedef union  TMTR_header_pos
{ UNSG32 u32[1];
	struct {
		struct w32MTR_header_pos;
	};
} TMTR_header_pos;
SIGN32 MTR_header_drvrd(SIE_MTR_header *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_header_drvwr(SIE_MTR_header *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_header_reset(SIE_MTR_header *p);
SIGN32 MTR_header_cmp  (SIE_MTR_header *p, SIE_MTR_header *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_header_check(p,pie,pfx,hLOG) MTR_header_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_header_print(p,    pfx,hLOG) MTR_header_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_meta_tag
#define h_MTR_meta_tag (){}
typedef struct SIE_MTR_meta_tag {
	UNSG32 u_x                                         :  7;
	UNSG32 u_y                                         : 10;
	UNSG32 u_uv                                        :  1;
	UNSG32 u_bid                                       :  4;
	UNSG32 u_valid                                     :  1;
	UNSG32 u_pending                                   :  1;
	UNSG32 u_ts                                        :  1;
	UNSG32 RSVDx0_b25                                  :  7;
} SIE_MTR_meta_tag;
SIGN32 MTR_meta_tag_drvrd(SIE_MTR_meta_tag *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_meta_tag_drvwr(SIE_MTR_meta_tag *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_meta_tag_reset(SIE_MTR_meta_tag *p);
SIGN32 MTR_meta_tag_cmp  (SIE_MTR_meta_tag *p, SIE_MTR_meta_tag *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_meta_tag_check(p,pie,pfx,hLOG) MTR_meta_tag_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_meta_tag_print(p,    pfx,hLOG) MTR_meta_tag_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_meta_tagx
#define h_MTR_meta_tagx (){}
typedef struct SIE_MTR_meta_tagx {
	UNSG32 u_x                                         :  4;
	UNSG32 u_y                                         : 13;
	UNSG32 u_uv                                        :  1;
	UNSG32 u_bid                                       :  4;
	UNSG32 u_valid                                     :  1;
	UNSG32 u_pending                                   :  1;
	UNSG32 u_ts                                        :  1;
	UNSG32 RSVDx0_b25                                  :  7;
} SIE_MTR_meta_tagx;
SIGN32 MTR_meta_tagx_drvrd(SIE_MTR_meta_tagx *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_meta_tagx_drvwr(SIE_MTR_meta_tagx *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_meta_tagx_reset(SIE_MTR_meta_tagx *p);
SIGN32 MTR_meta_tagx_cmp  (SIE_MTR_meta_tagx *p, SIE_MTR_meta_tagx *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_meta_tagx_check(p,pie,pfx,hLOG) MTR_meta_tagx_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_meta_tagx_print(p,    pfx,hLOG) MTR_meta_tagx_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_meta_addr
#define h_MTR_meta_addr (){}
#define     RA_MTR_meta_addr_MTR_header                    0x0000
#define     RA_MTR_meta_addr_user                          0x000C
#define     RA_MTR_meta_addr_pos                           0x0010
#define     RA_MTR_meta_addr_base                          0x0014
#define     RA_MTR_meta_addr_mosaic                        0x0018
#define     RA_MTR_meta_addr_id                            0x001C
typedef struct SIE_MTR_meta_addr {
	SIE_MTR_header                                   ie_MTR_header;
#define     w32MTR_meta_addr_user                          {\
	UNSG32 uuser_dat                                   : 24;\
	UNSG32 RSVDxC_b24                                  :  8;\
}
	union { UNSG32 u32MTR_meta_addr_user;
		struct w32MTR_meta_addr_user;
	};
#define     w32MTR_meta_addr_pos                           {\
	UNSG32 upos_x                                      : 18;\
	UNSG32 upos_y                                      : 14;\
}
union { UNSG32 u32MTR_meta_addr_pos;
	struct w32MTR_meta_addr_pos;
};
#define     w32MTR_meta_addr_base                          {\
	UNSG32 ubase_addr                                  : 32;\
}
union { UNSG32 u32MTR_meta_addr_base;
	struct w32MTR_meta_addr_base;
};
#define     w32MTR_meta_addr_mosaic                        {\
	UNSG32 umosaic_addr                                : 32;\
}
union { UNSG32 u32MTR_meta_addr_mosaic;
	struct w32MTR_meta_addr_mosaic;
};
#define     w32MTR_meta_addr_id                            {\
	UNSG32 uid_ts                                      :  1;\
	UNSG32 uid_uv                                      :  1;\
	UNSG32 uid_bid                                     :  4;\
	UNSG32 RSVDx1C_b6                                  : 26;\
}
union { UNSG32 u32MTR_meta_addr_id;
	struct w32MTR_meta_addr_id;
};
} SIE_MTR_meta_addr;
typedef union  T32MTR_meta_addr_user
{ UNSG32 u32;
	struct w32MTR_meta_addr_user;
} T32MTR_meta_addr_user;
typedef union  T32MTR_meta_addr_pos
{ UNSG32 u32;
	struct w32MTR_meta_addr_pos;
} T32MTR_meta_addr_pos;
typedef union  T32MTR_meta_addr_base
{ UNSG32 u32;
	struct w32MTR_meta_addr_base;
} T32MTR_meta_addr_base;
typedef union  T32MTR_meta_addr_mosaic
{ UNSG32 u32;
	struct w32MTR_meta_addr_mosaic;
} T32MTR_meta_addr_mosaic;
typedef union  T32MTR_meta_addr_id
{ UNSG32 u32;
	struct w32MTR_meta_addr_id;
} T32MTR_meta_addr_id;
typedef union  TMTR_meta_addr_user
{ UNSG32 u32[1];
	struct {
		struct w32MTR_meta_addr_user;
	};
} TMTR_meta_addr_user;
typedef union  TMTR_meta_addr_pos
{ UNSG32 u32[1];
	struct {
		struct w32MTR_meta_addr_pos;
	};
} TMTR_meta_addr_pos;
typedef union  TMTR_meta_addr_base
{ UNSG32 u32[1];
	struct {
		struct w32MTR_meta_addr_base;
	};
} TMTR_meta_addr_base;
typedef union  TMTR_meta_addr_mosaic
{ UNSG32 u32[1];
	struct {
		struct w32MTR_meta_addr_mosaic;
	};
} TMTR_meta_addr_mosaic;
typedef union  TMTR_meta_addr_id
{ UNSG32 u32[1];
	struct {
		struct w32MTR_meta_addr_id;
	};
} TMTR_meta_addr_id;
SIGN32 MTR_meta_addr_drvrd(SIE_MTR_meta_addr *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_meta_addr_drvwr(SIE_MTR_meta_addr *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_meta_addr_reset(SIE_MTR_meta_addr *p);
SIGN32 MTR_meta_addr_cmp  (SIE_MTR_meta_addr *p, SIE_MTR_meta_addr *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_meta_addr_check(p,pie,pfx,hLOG) MTR_meta_addr_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_meta_addr_print(p,    pfx,hLOG) MTR_meta_addr_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTRR_frame
#define h_MTRR_frame (){}
#define     RA_MTRR_frame_stride                           0x0000
#define     RA_MTRR_frame_base                             0x0004
typedef struct SIE_MTRR_frame {
#define     w32MTRR_frame_stride                           {\
	UNSG32 ustride_val                                 : 26;\
	UNSG32 RSVDx0_b26                                  :  6;\
}
union { UNSG32 u32MTRR_frame_stride;
	struct w32MTRR_frame_stride;
};
#define     w32MTRR_frame_base                             {\
	UNSG32 ubase_val                                   : 22;\
	UNSG32 RSVDx4_b22                                  : 10;\
}
union { UNSG32 u32MTRR_frame_base;
	struct w32MTRR_frame_base;
};
} SIE_MTRR_frame;
typedef union  T32MTRR_frame_stride
{ UNSG32 u32;
	struct w32MTRR_frame_stride;
} T32MTRR_frame_stride;
typedef union  T32MTRR_frame_base
{ UNSG32 u32;
	struct w32MTRR_frame_base;
} T32MTRR_frame_base;
typedef union  TMTRR_frame_stride
{ UNSG32 u32[1];
	struct {
		struct w32MTRR_frame_stride;
	};
} TMTRR_frame_stride;
typedef union  TMTRR_frame_base
{ UNSG32 u32[1];
	struct {
		struct w32MTRR_frame_base;
	};
} TMTRR_frame_base;
SIGN32 MTRR_frame_drvrd(SIE_MTRR_frame *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTRR_frame_drvwr(SIE_MTRR_frame *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTRR_frame_reset(SIE_MTRR_frame *p);
SIGN32 MTRR_frame_cmp  (SIE_MTRR_frame *p, SIE_MTRR_frame *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTRR_frame_check(p,pie,pfx,hLOG) MTRR_frame_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTRR_frame_print(p,    pfx,hLOG) MTRR_frame_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR_R_HDR
#define h_MTR_R_HDR (){}
#define     RA_MTR_R_HDR_meta                              0x0000
#define     RA_MTR_R_HDR_header                            0x0004
#define     RA_MTR_R_HDR_MTR_header                        0x0004
#define     RA_MTR_R_HDR_mosaic                            0x0010
typedef struct SIE_MTR_R_HDR {
#define     w32MTR_R_HDR_meta                              {\
	UNSG32 umeta_base                                  : 22;\
	UNSG32 umeta_ts                                    :  1;\
	UNSG32 umeta_valid                                 :  1;\
	UNSG32 RSVDx0_b24                                  :  8;\
}
union { UNSG32 u32MTR_R_HDR_meta;
	struct w32MTR_R_HDR_meta;
};
SIE_MTR_header                                   ie_MTR_header;
#define     w32MTR_R_HDR_mosaic                            {\
	UNSG32 umosaic_base                                : 22;\
	UNSG32 RSVDx10_b22                                 : 10;\
}
union { UNSG32 u32MTR_R_HDR_mosaic;
	struct w32MTR_R_HDR_mosaic;
};
} SIE_MTR_R_HDR;
typedef union  T32MTR_R_HDR_meta
{ UNSG32 u32;
	struct w32MTR_R_HDR_meta;
} T32MTR_R_HDR_meta;
typedef union  T32MTR_R_HDR_mosaic
{ UNSG32 u32;
	struct w32MTR_R_HDR_mosaic;
} T32MTR_R_HDR_mosaic;
typedef union  TMTR_R_HDR_meta
{ UNSG32 u32[1];
	struct {
		struct w32MTR_R_HDR_meta;
	};
} TMTR_R_HDR_meta;
typedef union  TMTR_R_HDR_mosaic
{ UNSG32 u32[1];
	struct {
		struct w32MTR_R_HDR_mosaic;
	};
} TMTR_R_HDR_mosaic;
SIGN32 MTR_R_HDR_drvrd(SIE_MTR_R_HDR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_R_HDR_drvwr(SIE_MTR_R_HDR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_R_HDR_reset(SIE_MTR_R_HDR *p);
SIGN32 MTR_R_HDR_cmp  (SIE_MTR_R_HDR *p, SIE_MTR_R_HDR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_R_HDR_check(p,pie,pfx,hLOG) MTR_R_HDR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_R_HDR_print(p,    pfx,hLOG) MTR_R_HDR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_META_TAG
#define h_META_TAG (){}
#define     RA_META_TAG_word                               0x0000
typedef struct SIE_META_TAG {
	SIE_MTR_meta_tag                                 ie_word[64];
} SIE_META_TAG;
SIGN32 META_TAG_drvrd(SIE_META_TAG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 META_TAG_drvwr(SIE_META_TAG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void META_TAG_reset(SIE_META_TAG *p);
SIGN32 META_TAG_cmp  (SIE_META_TAG *p, SIE_META_TAG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define META_TAG_check(p,pie,pfx,hLOG) META_TAG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define META_TAG_print(p,    pfx,hLOG) META_TAG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTRR_CFG
#define h_MTRR_CFG (){}
#define     RA_MTRR_CFG_cfg                                0x0000
#define        MTRR_CFG_cfg_stride_64B                                  0x0
#define        MTRR_CFG_cfg_stride_128B                                 0x1
#define        MTRR_CFG_cfg_stride_256B                                 0x2
#define        MTRR_CFG_cfg_stride_512B                                 0x3
#define        MTRR_CFG_cfg_stride_1024B                                0x4
#define        MTRR_CFG_cfg_stride_2048B                                0x5
#define        MTRR_CFG_cfg_stride_4096B                                0x6
#define        MTRR_CFG_cfg_stride_8192B                                0x7
#define        MTRR_CFG_cfg_stride_16384B                               0x8
#define        MTRR_CFG_cfg_format_V16H16V1                             0x0
#define        MTRR_CFG_cfg_format_V8H16V2                              0x1
#define        MTRR_CFG_cfg_format_V4H16V4                              0x2
#define        MTRR_CFG_cfg_format_V8H32V1                              0x4
#define        MTRR_CFG_cfg_format_V4H32V2                              0x5
#define        MTRR_CFG_cfg_format_V2H32V4                              0x6
#define        MTRR_CFG_cfg_format_V1H32V2                              0x8
#define        MTRR_CFG_cfg_format_V1H16V4                              0x9
#define        MTRR_CFG_cfg_format_V1H8V8                               0xA
#define        MTRR_CFG_cfg_format_V1H4V16                              0xB
#define        MTRR_CFG_cfg_format_V1H64V1                              0xC
#define     RA_MTRR_CFG_base                               0x0004
typedef struct SIE_MTRR_CFG {
#define     w32MTRR_CFG_cfg                                {\
	UNSG32 ucfg_stride                                 :  4;\
	UNSG32 ucfg_format                                 :  4;\
	UNSG32 ucfg_enable                                 :  1;\
	UNSG32 ucfg_mode                                   :  5;\
	UNSG32 ucfg_prefetch_disable                       :  1;\
	UNSG32 ucfg_cbase                                  :  5;\
	UNSG32 ucfg_cways                                  :  3;\
	UNSG32 ucfg_auto_tag_clr                           :  1;\
	UNSG32 ucfg_mosaic_xnum                            :  2;\
	UNSG32 ucfg_mosaic_ynum                            :  2;\
	UNSG32 ucfg_weave                                  :  1;\
	UNSG32 ucfg_ctype                                  :  1;\
	UNSG32 ucfg_hflip                                  :  1;\
	UNSG32 ucfg_vflip                                  :  1;\
}
union { UNSG32 u32MTRR_CFG_cfg;
	struct w32MTRR_CFG_cfg;
};
#define     w32MTRR_CFG_base                               {\
	UNSG32 ubase_addr                                  : 20;\
	UNSG32 RSVDx4_b20                                  : 12;\
}
union { UNSG32 u32MTRR_CFG_base;
	struct w32MTRR_CFG_base;
};
} SIE_MTRR_CFG;
typedef union  T32MTRR_CFG_cfg
{ UNSG32 u32;
	struct w32MTRR_CFG_cfg;
} T32MTRR_CFG_cfg;
typedef union  T32MTRR_CFG_base
{ UNSG32 u32;
	struct w32MTRR_CFG_base;
} T32MTRR_CFG_base;
typedef union  TMTRR_CFG_cfg
{ UNSG32 u32[1];
	struct {
		struct w32MTRR_CFG_cfg;
	};
} TMTRR_CFG_cfg;
typedef union  TMTRR_CFG_base
{ UNSG32 u32[1];
	struct {
		struct w32MTRR_CFG_base;
	};
} TMTRR_CFG_base;
SIGN32 MTRR_CFG_drvrd(SIE_MTRR_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTRR_CFG_drvwr(SIE_MTRR_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTRR_CFG_reset(SIE_MTRR_CFG *p);
SIGN32 MTRR_CFG_cmp  (SIE_MTRR_CFG *p, SIE_MTRR_CFG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTRR_CFG_check(p,pie,pfx,hLOG) MTRR_CFG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTRR_CFG_print(p,    pfx,hLOG) MTRR_CFG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTRW_CFG
#define h_MTRW_CFG (){}
#define     RA_MTRW_CFG_cfg                                0x0000
#define        MTRW_CFG_cfg_stride_64B                                  0x0
#define        MTRW_CFG_cfg_stride_128B                                 0x1
#define        MTRW_CFG_cfg_stride_256B                                 0x2
#define        MTRW_CFG_cfg_stride_512B                                 0x3
#define        MTRW_CFG_cfg_stride_1024B                                0x4
#define        MTRW_CFG_cfg_stride_2048B                                0x5
#define        MTRW_CFG_cfg_stride_4096B                                0x6
#define        MTRW_CFG_cfg_stride_8192B                                0x7
#define        MTRW_CFG_cfg_stride_16384B                               0x8
#define        MTRW_CFG_cfg_format_V16H16V1                             0x0
#define        MTRW_CFG_cfg_format_V8H16V2                              0x1
#define        MTRW_CFG_cfg_format_V4H16V4                              0x2
#define        MTRW_CFG_cfg_format_V8H32V1                              0x4
#define        MTRW_CFG_cfg_format_V4H32V2                              0x5
#define        MTRW_CFG_cfg_format_V2H32V4                              0x6
#define        MTRW_CFG_cfg_format_V1H32V2                              0x8
#define        MTRW_CFG_cfg_format_V1H16V4                              0x9
#define        MTRW_CFG_cfg_format_V1H8V8                               0xA
#define        MTRW_CFG_cfg_format_V1H4V16                              0xB
#define        MTRW_CFG_cfg_format_V1H64V1                              0xC
#define     RA_MTRW_CFG_base                               0x0004
typedef struct SIE_MTRW_CFG {
#define     w32MTRW_CFG_cfg                                {\
	UNSG32 ucfg_stride                                 :  4;\
	UNSG32 ucfg_format                                 :  4;\
	UNSG32 ucfg_enable                                 :  1;\
	UNSG32 ucfg_mode                                   :  5;\
	UNSG32 ucfg_force_uncmpr                           :  1;\
	UNSG32 ucfg_weave                                  :  1;\
	UNSG32 ucfg_rsvd_0                                 :  1;\
	UNSG32 ucfg_hflip                                  :  1;\
	UNSG32 ucfg_vflip                                  :  1;\
	UNSG32 RSVDx0_b19                                  : 13;\
}
union { UNSG32 u32MTRW_CFG_cfg;
	struct w32MTRW_CFG_cfg;
};
#define     w32MTRW_CFG_base                               {\
	UNSG32 ubase_addr                                  : 20;\
	UNSG32 RSVDx4_b20                                  : 12;\
}
union { UNSG32 u32MTRW_CFG_base;
	struct w32MTRW_CFG_base;
};
} SIE_MTRW_CFG;
typedef union  T32MTRW_CFG_cfg
{ UNSG32 u32;
	struct w32MTRW_CFG_cfg;
} T32MTRW_CFG_cfg;
typedef union  T32MTRW_CFG_base
{ UNSG32 u32;
	struct w32MTRW_CFG_base;
} T32MTRW_CFG_base;
typedef union  TMTRW_CFG_cfg
{ UNSG32 u32[1];
	struct {
		struct w32MTRW_CFG_cfg;
	};
} TMTRW_CFG_cfg;
typedef union  TMTRW_CFG_base
{ UNSG32 u32[1];
	struct {
		struct w32MTRW_CFG_base;
	};
} TMTRW_CFG_base;
SIGN32 MTRW_CFG_drvrd(SIE_MTRW_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTRW_CFG_drvwr(SIE_MTRW_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTRW_CFG_reset(SIE_MTRW_CFG *p);
SIGN32 MTRW_CFG_cmp  (SIE_MTRW_CFG *p, SIE_MTRW_CFG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTRW_CFG_check(p,pie,pfx,hLOG) MTRW_CFG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTRW_CFG_print(p,    pfx,hLOG) MTRW_CFG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MMU_vm_tag
#define h_MMU_vm_tag (){}
typedef struct SIE_MMU_vm_tag {
	UNSG32 u_idx                                       : 13;
	UNSG32 u_uv                                        :  1;
	UNSG32 u_bid                                       :  4;
	UNSG32 u_wt                                        :  1;
	UNSG32 u_valid                                     :  1;
	UNSG32 u_pending                                   :  1;
	UNSG32 RSVDx0_b21                                  : 11;
} SIE_MMU_vm_tag;
SIGN32 MMU_vm_tag_drvrd(SIE_MMU_vm_tag *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MMU_vm_tag_drvwr(SIE_MMU_vm_tag *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MMU_vm_tag_reset(SIE_MMU_vm_tag *p);
SIGN32 MMU_vm_tag_cmp  (SIE_MMU_vm_tag *p, SIE_MMU_vm_tag *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MMU_vm_tag_check(p,pie,pfx,hLOG) MMU_vm_tag_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MMU_vm_tag_print(p,    pfx,hLOG) MMU_vm_tag_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MMU_TAG
#define h_MMU_TAG (){}
#define     RA_MMU_TAG_word                                0x0000
typedef struct SIE_MMU_TAG {
	SIE_MMU_vm_tag                                   ie_word[64];
} SIE_MMU_TAG;
SIGN32 MMU_TAG_drvrd(SIE_MMU_TAG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MMU_TAG_drvwr(SIE_MMU_TAG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MMU_TAG_reset(SIE_MMU_TAG *p);
SIGN32 MMU_TAG_cmp  (SIE_MMU_TAG *p, SIE_MMU_TAG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MMU_TAG_check(p,pie,pfx,hLOG) MMU_TAG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MMU_TAG_print(p,    pfx,hLOG) MMU_TAG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MMU_CFG
#define h_MMU_CFG (){}
#define     RA_MMU_CFG_pbm                                 0x0000
#define     RA_MMU_CFG_vm                                  0x0004
#define        MMU_CFG_vm_mode_normal                                   0x0
#define        MMU_CFG_vm_mode_section                                  0x1
#define        MMU_CFG_vm_mode_ovp                                      0x2
#define        MMU_CFG_vm_mode_pfu                                      0x3
#define        MMU_CFG_vm_mode_auto                                     0x4
#define     RA_MMU_CFG_cdesc                               0x0008
#define        MMU_CFG_cdesc_rpTyp_lru                                  0x0
#define        MMU_CFG_cdesc_rpTyp_fifo                                 0x1
#define        MMU_CFG_cdesc_type_64B                                   0x0
#define        MMU_CFG_cdesc_type_32B                                   0x1
typedef struct SIE_MMU_CFG {
#define     w32MMU_CFG_pbm                                 {\
	UNSG32 upbm_shy_bw                                 :  3;\
	UNSG32 upbm_shy_pos                                :  1;\
	UNSG32 upbm_stride_64B                             : 22;\
	UNSG32 upbm_tileMode                               :  1;\
	UNSG32 upbm_pm_enable                              :  1;\
	UNSG32 upbm_shuffle_en                             :  1;\
	UNSG32 upbm_bm_enable                              :  1;\
	UNSG32 upbm_weave                                  :  1;\
	UNSG32 RSVDx0_b31                                  :  1;\
}
union { UNSG32 u32MMU_CFG_pbm;
	struct w32MMU_CFG_pbm;
};
#define     w32MMU_CFG_vm                                  {\
	UNSG32 uvm_enable                                  :  1;\
	UNSG32 uvm_prefecth_disable                        :  1;\
	UNSG32 uvm_mode                                    :  3;\
	UNSG32 uvm_mTyp                                    :  1;\
	UNSG32 uvm_delta                                   :  3;\
	UNSG32 uvm_base                                    : 20;\
	UNSG32 uvm_auto_tag_clr                            :  1;\
	UNSG32 RSVDx4_b30                                  :  2;\
}
union { UNSG32 u32MMU_CFG_vm;
	struct w32MMU_CFG_vm;
};
#define     w32MMU_CFG_cdesc                               {\
	UNSG32 ucdesc_base                                 :  6;\
	UNSG32 ucdesc_ways                                 :  3;\
	UNSG32 ucdesc_rpTyp                                :  1;\
	UNSG32 ucdesc_type                                 :  1;\
	UNSG32 RSVDx8_b11                                  : 21;\
}
union { UNSG32 u32MMU_CFG_cdesc;
	struct w32MMU_CFG_cdesc;
};
} SIE_MMU_CFG;
typedef union  T32MMU_CFG_pbm
{ UNSG32 u32;
	struct w32MMU_CFG_pbm;
} T32MMU_CFG_pbm;
typedef union  T32MMU_CFG_vm
{ UNSG32 u32;
	struct w32MMU_CFG_vm;
} T32MMU_CFG_vm;
typedef union  T32MMU_CFG_cdesc
{ UNSG32 u32;
	struct w32MMU_CFG_cdesc;
} T32MMU_CFG_cdesc;
typedef union  TMMU_CFG_pbm
{ UNSG32 u32[1];
	struct {
		struct w32MMU_CFG_pbm;
	};
} TMMU_CFG_pbm;
typedef union  TMMU_CFG_vm
{ UNSG32 u32[1];
	struct {
		struct w32MMU_CFG_vm;
	};
} TMMU_CFG_vm;
typedef union  TMMU_CFG_cdesc
{ UNSG32 u32[1];
	struct {
		struct w32MMU_CFG_cdesc;
	};
} TMMU_CFG_cdesc;
SIGN32 MMU_CFG_drvrd(SIE_MMU_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MMU_CFG_drvwr(SIE_MMU_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MMU_CFG_reset(SIE_MMU_CFG *p);
SIGN32 MMU_CFG_cmp  (SIE_MMU_CFG *p, SIE_MMU_CFG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MMU_CFG_check(p,pie,pfx,hLOG) MMU_CFG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MMU_CFG_print(p,    pfx,hLOG) MMU_CFG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_IDMAP
#define h_IDMAP (){}
typedef struct SIE_IDMAP {
	UNSG32 u_idx00                                     :  3;
	UNSG32 u_idx01                                     :  3;
	UNSG32 u_idx02                                     :  3;
	UNSG32 u_idx03                                     :  3;
	UNSG32 u_idx04                                     :  3;
	UNSG32 u_idx05                                     :  3;
	UNSG32 u_idx06                                     :  3;
	UNSG32 u_idx07                                     :  3;
	UNSG32 RSVDx0_b24                                  :  8;
	UNSG32 u_idx08                                     :  3;
	UNSG32 u_idx09                                     :  3;
	UNSG32 u_idx10                                     :  3;
	UNSG32 u_idx11                                     :  3;
	UNSG32 u_idx12                                     :  3;
	UNSG32 u_idx13                                     :  3;
	UNSG32 u_idx14                                     :  3;
	UNSG32 u_idx15                                     :  3;
	UNSG32 RSVDx4_b24                                  :  8;
} SIE_IDMAP;
SIGN32 IDMAP_drvrd(SIE_IDMAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 IDMAP_drvwr(SIE_IDMAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void IDMAP_reset(SIE_IDMAP *p);
SIGN32 IDMAP_cmp  (SIE_IDMAP *p, SIE_IDMAP *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define IDMAP_check(p,pie,pfx,hLOG) IDMAP_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define IDMAP_print(p,    pfx,hLOG) IDMAP_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMETA_OFST
#define h_VxGMETA_OFST (){}
#define     RA_VxGMETA_OFST_offset                         0x0000
typedef struct SIE_VxGMETA_OFST {
#define     w32VxGMETA_OFST_offset                         {\
	UNSG32 uoffset_luma                                : 16;\
	UNSG32 uoffset_chroma                              : 16;\
}
union { UNSG32 u32VxGMETA_OFST_offset;
	struct w32VxGMETA_OFST_offset;
};
} SIE_VxGMETA_OFST;
typedef union  T32VxGMETA_OFST_offset
{ UNSG32 u32;
	struct w32VxGMETA_OFST_offset;
} T32VxGMETA_OFST_offset;
typedef union  TVxGMETA_OFST_offset
{ UNSG32 u32[1];
	struct {
		struct w32VxGMETA_OFST_offset;
	};
} TVxGMETA_OFST_offset;
SIGN32 VxGMETA_OFST_drvrd(SIE_VxGMETA_OFST *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMETA_OFST_drvwr(SIE_VxGMETA_OFST *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMETA_OFST_reset(SIE_VxGMETA_OFST *p);
SIGN32 VxGMETA_OFST_cmp  (SIE_VxGMETA_OFST *p, SIE_VxGMETA_OFST *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMETA_OFST_check(p,pie,pfx,hLOG) VxGMETA_OFST_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMETA_OFST_print(p,    pfx,hLOG) VxGMETA_OFST_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMETA_FMT
#define h_VxGMETA_FMT (){}
#define     RA_VxGMETA_FMT_cfg                             0x0000
#define        VxGMETA_FMT_cfg_stride_64B                               0x0
#define        VxGMETA_FMT_cfg_stride_128B                              0x1
#define        VxGMETA_FMT_cfg_stride_256B                              0x2
#define        VxGMETA_FMT_cfg_stride_512B                              0x3
#define        VxGMETA_FMT_cfg_stride_1024B                             0x4
#define        VxGMETA_FMT_cfg_stride_2048B                             0x5
#define        VxGMETA_FMT_cfg_stride_4096B                             0x6
#define        VxGMETA_FMT_cfg_stride_8192B                             0x7
#define        VxGMETA_FMT_cfg_stride_16384B                            0x8
#define     RA_VxGMETA_FMT_roix                            0x0004
#define     RA_VxGMETA_FMT_roiy                            0x0008
typedef struct SIE_VxGMETA_FMT {
#define     w32VxGMETA_FMT_cfg                             {\
	UNSG32 ucfg_stride                                 :  4;\
	UNSG32 ucfg_format                                 :  2;\
	UNSG32 ucfg_mode                                   :  5;\
	UNSG32 ucfg_weave                                  :  1;\
	UNSG32 RSVDx0_b12                                  : 20;\
}
union { UNSG32 u32VxGMETA_FMT_cfg;
	struct w32VxGMETA_FMT_cfg;
};
#define     w32VxGMETA_FMT_roix                            {\
	UNSG32 uroix_xs                                    : 12;\
	UNSG32 uroix_xm                                    : 12;\
	UNSG32 RSVDx4_b24                                  :  8;\
}
union { UNSG32 u32VxGMETA_FMT_roix;
	struct w32VxGMETA_FMT_roix;
};
#define     w32VxGMETA_FMT_roiy                            {\
	UNSG32 uroiy_ys                                    : 14;\
	UNSG32 uroiy_ym                                    : 14;\
	UNSG32 RSVDx8_b28                                  :  4;\
}
union { UNSG32 u32VxGMETA_FMT_roiy;
	struct w32VxGMETA_FMT_roiy;
};
} SIE_VxGMETA_FMT;
typedef union  T32VxGMETA_FMT_cfg
{ UNSG32 u32;
	struct w32VxGMETA_FMT_cfg;
} T32VxGMETA_FMT_cfg;
typedef union  T32VxGMETA_FMT_roix
{ UNSG32 u32;
	struct w32VxGMETA_FMT_roix;
} T32VxGMETA_FMT_roix;
typedef union  T32VxGMETA_FMT_roiy
{ UNSG32 u32;
	struct w32VxGMETA_FMT_roiy;
} T32VxGMETA_FMT_roiy;
typedef union  TVxGMETA_FMT_cfg
{ UNSG32 u32[1];
	struct {
		struct w32VxGMETA_FMT_cfg;
	};
} TVxGMETA_FMT_cfg;
typedef union  TVxGMETA_FMT_roix
{ UNSG32 u32[1];
	struct {
		struct w32VxGMETA_FMT_roix;
	};
} TVxGMETA_FMT_roix;
typedef union  TVxGMETA_FMT_roiy
{ UNSG32 u32[1];
	struct {
		struct w32VxGMETA_FMT_roiy;
	};
} TVxGMETA_FMT_roiy;
SIGN32 VxGMETA_FMT_drvrd(SIE_VxGMETA_FMT *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMETA_FMT_drvwr(SIE_VxGMETA_FMT *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMETA_FMT_reset(SIE_VxGMETA_FMT *p);
SIGN32 VxGMETA_FMT_cmp  (SIE_VxGMETA_FMT *p, SIE_VxGMETA_FMT *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMETA_FMT_check(p,pie,pfx,hLOG) VxGMETA_FMT_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMETA_FMT_print(p,    pfx,hLOG) VxGMETA_FMT_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMETA
#define h_VxGMETA (){}
#define     RA_VxGMETA_base                                0x0000
#define     RA_VxGMETA_offset                              0x0004
#define     RA_VxGMETA_cdesc                               0x0044
#define     RA_VxGMETA_cfg                                 0x0048
#define     RA_VxGMETA_METAIDMAP                           0x004C
#define     RA_VxGMETA_metaidmap                           0x004C
#define     RA_VxGMETA_METAY                               0x0054
#define     RA_VxGMETA_metay                               0x0054
#define     RA_VxGMETA_METAC                               0x00B4
#define     RA_VxGMETA_metac                               0x00B4
typedef struct SIE_VxGMETA {
#define     w32VxGMETA_base                                {\
	UNSG32 ubase_addr                                  : 20;\
	UNSG32 RSVDx0_b20                                  : 12;\
}
union { UNSG32 u32VxGMETA_base;
	struct w32VxGMETA_base;
};
SIE_VxGMETA_OFST                                 ie_offset[16];
#define     w32VxGMETA_cdesc                               {\
	UNSG32 ucdesc_base_l                               :  5;\
	UNSG32 ucdesc_ways_l                               :  3;\
	UNSG32 ucdesc_base_c                               :  5;\
	UNSG32 ucdesc_ways_c                               :  3;\
	UNSG32 ucdesc_auto_tag_clr                         :  1;\
	UNSG32 ucdesc_type_l                               :  1;\
	UNSG32 ucdesc_type_c                               :  1;\
	UNSG32 RSVDx44_b19                                 : 13;\
}
union { UNSG32 u32VxGMETA_cdesc;
	struct w32VxGMETA_cdesc;
};
#define     w32VxGMETA_cfg                                 {\
	UNSG32 ucfg_enable                                 : 32;\
}
union { UNSG32 u32VxGMETA_cfg;
	struct w32VxGMETA_cfg;
};
SIE_IDMAP                                        ie_metaidmap;
SIE_VxGMETA_FMT                                  ie_metay[8];
SIE_VxGMETA_FMT                                  ie_metac[8];
} SIE_VxGMETA;
typedef union  T32VxGMETA_base
{ UNSG32 u32;
	struct w32VxGMETA_base;
} T32VxGMETA_base;
typedef union  T32VxGMETA_cdesc
{ UNSG32 u32;
	struct w32VxGMETA_cdesc;
} T32VxGMETA_cdesc;
typedef union  T32VxGMETA_cfg
{ UNSG32 u32;
	struct w32VxGMETA_cfg;
} T32VxGMETA_cfg;
typedef union  TVxGMETA_base
{ UNSG32 u32[1];
	struct {
		struct w32VxGMETA_base;
	};
} TVxGMETA_base;
typedef union  TVxGMETA_cdesc
{ UNSG32 u32[1];
	struct {
		struct w32VxGMETA_cdesc;
	};
} TVxGMETA_cdesc;
typedef union  TVxGMETA_cfg
{ UNSG32 u32[1];
	struct {
		struct w32VxGMETA_cfg;
	};
} TVxGMETA_cfg;
SIGN32 VxGMETA_drvrd(SIE_VxGMETA *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMETA_drvwr(SIE_VxGMETA *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMETA_reset(SIE_VxGMETA *p);
SIGN32 VxGMETA_cmp  (SIE_VxGMETA *p, SIE_VxGMETA *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMETA_check(p,pie,pfx,hLOG) VxGMETA_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMETA_print(p,    pfx,hLOG) VxGMETA_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMMU_STRD
#define h_VxGMMU_STRD (){}
#define     RA_VxGMMU_STRD_stride                          0x0000
typedef struct SIE_VxGMMU_STRD {
#define     w32VxGMMU_STRD_stride                          {\
	UNSG32 ustride_luma                                : 16;\
	UNSG32 ustride_chroma                              : 16;\
}
union { UNSG32 u32VxGMMU_STRD_stride;
	struct w32VxGMMU_STRD_stride;
};
} SIE_VxGMMU_STRD;
typedef union  T32VxGMMU_STRD_stride
{ UNSG32 u32;
	struct w32VxGMMU_STRD_stride;
} T32VxGMMU_STRD_stride;
typedef union  TVxGMMU_STRD_stride
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMU_STRD_stride;
	};
} TVxGMMU_STRD_stride;
SIGN32 VxGMMU_STRD_drvrd(SIE_VxGMMU_STRD *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMMU_STRD_drvwr(SIE_VxGMMU_STRD *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMMU_STRD_reset(SIE_VxGMMU_STRD *p);
SIGN32 VxGMMU_STRD_cmp  (SIE_VxGMMU_STRD *p, SIE_VxGMMU_STRD *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMMU_STRD_check(p,pie,pfx,hLOG) VxGMMU_STRD_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMMU_STRD_print(p,    pfx,hLOG) VxGMMU_STRD_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMMU_BASE
#define h_VxGMMU_BASE (){}
#define     RA_VxGMMU_BASE_base                            0x0000
typedef struct SIE_VxGMMU_BASE {
#define     w32VxGMMU_BASE_base                            {\
	UNSG32 ubase_val                                   : 20;\
	UNSG32 RSVDx0_b20                                  : 12;\
}
union { UNSG32 u32VxGMMU_BASE_base;
	struct w32VxGMMU_BASE_base;
};
} SIE_VxGMMU_BASE;
typedef union  T32VxGMMU_BASE_base
{ UNSG32 u32;
	struct w32VxGMMU_BASE_base;
} T32VxGMMU_BASE_base;
typedef union  TVxGMMU_BASE_base
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMU_BASE_base;
	};
} TVxGMMU_BASE_base;
SIGN32 VxGMMU_BASE_drvrd(SIE_VxGMMU_BASE *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMMU_BASE_drvwr(SIE_VxGMMU_BASE *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMMU_BASE_reset(SIE_VxGMMU_BASE *p);
SIGN32 VxGMMU_BASE_cmp  (SIE_VxGMMU_BASE *p, SIE_VxGMMU_BASE *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMMU_BASE_check(p,pie,pfx,hLOG) VxGMMU_BASE_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMMU_BASE_print(p,    pfx,hLOG) VxGMMU_BASE_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMMUW_CFG
#define h_VxGMMUW_CFG (){}
#define     RA_VxGMMUW_CFG_pbm                             0x0000
#define     RA_VxGMMUW_CFG_vm                              0x0004
#define        VxGMMUW_CFG_vm_mode_normal                               0x0
#define        VxGMMUW_CFG_vm_mode_section                              0x1
#define        VxGMMUW_CFG_vm_mode_ovp                                  0x2
#define        VxGMMUW_CFG_vm_mode_pfu                                  0x3
#define        VxGMMUW_CFG_vm_mode_auto                                 0x4
#define     RA_VxGMMUW_CFG_cdesc                           0x0008
#define        VxGMMUW_CFG_cdesc_rpTyp_lru                              0x0
#define        VxGMMUW_CFG_cdesc_rpTyp_fifo                             0x1
#define        VxGMMUW_CFG_cdesc_type_64B                               0x0
#define        VxGMMUW_CFG_cdesc_type_32B                               0x1
#define     RA_VxGMMUW_CFG_roix                            0x000C
#define     RA_VxGMMUW_CFG_roiy                            0x0010
typedef struct SIE_VxGMMUW_CFG {
#define     w32VxGMMUW_CFG_pbm                             {\
	UNSG32 upbm_shy_bw                                 :  3;\
	UNSG32 upbm_shy_pos                                :  1;\
	UNSG32 upbm_stride_64B                             : 22;\
	UNSG32 upbm_tileMode                               :  1;\
	UNSG32 upbm_pm_enable                              :  1;\
	UNSG32 upbm_shuffle_en                             :  1;\
	UNSG32 upbm_bm_enable                              :  1;\
	UNSG32 upbm_weave                                  :  1;\
	UNSG32 RSVDx0_b31                                  :  1;\
}
union { UNSG32 u32VxGMMUW_CFG_pbm;
	struct w32VxGMMUW_CFG_pbm;
};
#define     w32VxGMMUW_CFG_vm                              {\
	UNSG32 uvm_enable                                  :  1;\
	UNSG32 uvm_prefecth_disable                        :  1;\
	UNSG32 uvm_mode                                    :  3;\
	UNSG32 uvm_mTyp                                    :  1;\
	UNSG32 uvm_delta                                   :  3;\
	UNSG32 uvm_base                                    : 20;\
	UNSG32 uvm_auto_tag_clr                            :  1;\
	UNSG32 RSVDx4_b30                                  :  2;\
}
union { UNSG32 u32VxGMMUW_CFG_vm;
	struct w32VxGMMUW_CFG_vm;
};
#define     w32VxGMMUW_CFG_cdesc                           {\
	UNSG32 ucdesc_base                                 :  6;\
	UNSG32 ucdesc_ways                                 :  3;\
	UNSG32 ucdesc_rpTyp                                :  1;\
	UNSG32 ucdesc_type                                 :  1;\
	UNSG32 RSVDx8_b11                                  : 21;\
}
union { UNSG32 u32VxGMMUW_CFG_cdesc;
	struct w32VxGMMUW_CFG_cdesc;
};
#define     w32VxGMMUW_CFG_roix                            {\
	UNSG32 uroix_xs                                    : 12;\
	UNSG32 uroix_xm                                    : 12;\
	UNSG32 RSVDxC_b24                                  :  8;\
}
union { UNSG32 u32VxGMMUW_CFG_roix;
	struct w32VxGMMUW_CFG_roix;
};
#define     w32VxGMMUW_CFG_roiy                            {\
	UNSG32 uroiy_ys                                    : 14;\
	UNSG32 uroiy_ym                                    : 14;\
	UNSG32 RSVDx10_b28                                 :  4;\
}
union { UNSG32 u32VxGMMUW_CFG_roiy;
	struct w32VxGMMUW_CFG_roiy;
};
} SIE_VxGMMUW_CFG;
typedef union  T32VxGMMUW_CFG_pbm
{ UNSG32 u32;
	struct w32VxGMMUW_CFG_pbm;
} T32VxGMMUW_CFG_pbm;
typedef union  T32VxGMMUW_CFG_vm
{ UNSG32 u32;
	struct w32VxGMMUW_CFG_vm;
} T32VxGMMUW_CFG_vm;
typedef union  T32VxGMMUW_CFG_cdesc
{ UNSG32 u32;
	struct w32VxGMMUW_CFG_cdesc;
} T32VxGMMUW_CFG_cdesc;
typedef union  T32VxGMMUW_CFG_roix
{ UNSG32 u32;
	struct w32VxGMMUW_CFG_roix;
} T32VxGMMUW_CFG_roix;
typedef union  T32VxGMMUW_CFG_roiy
{ UNSG32 u32;
	struct w32VxGMMUW_CFG_roiy;
} T32VxGMMUW_CFG_roiy;
typedef union  TVxGMMUW_CFG_pbm
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMUW_CFG_pbm;
	};
} TVxGMMUW_CFG_pbm;
typedef union  TVxGMMUW_CFG_vm
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMUW_CFG_vm;
	};
} TVxGMMUW_CFG_vm;
typedef union  TVxGMMUW_CFG_cdesc
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMUW_CFG_cdesc;
	};
} TVxGMMUW_CFG_cdesc;
typedef union  TVxGMMUW_CFG_roix
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMUW_CFG_roix;
	};
} TVxGMMUW_CFG_roix;
typedef union  TVxGMMUW_CFG_roiy
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMUW_CFG_roiy;
	};
} TVxGMMUW_CFG_roiy;
SIGN32 VxGMMUW_CFG_drvrd(SIE_VxGMMUW_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMMUW_CFG_drvwr(SIE_VxGMMUW_CFG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMMUW_CFG_reset(SIE_VxGMMUW_CFG *p);
SIGN32 VxGMMUW_CFG_cmp  (SIE_VxGMMUW_CFG *p, SIE_VxGMMUW_CFG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMMUW_CFG_check(p,pie,pfx,hLOG) VxGMMUW_CFG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMMUW_CFG_print(p,    pfx,hLOG) VxGMMUW_CFG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMMUW
#define h_VxGMMUW (){}
#define     RA_VxGMMUW_MMU                                 0x0000
#define     RA_VxGMMUW_mmu                                 0x0000
typedef struct SIE_VxGMMUW {
	SIE_VxGMMUW_CFG                                  ie_mmu[4];
} SIE_VxGMMUW;
SIGN32 VxGMMUW_drvrd(SIE_VxGMMUW *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMMUW_drvwr(SIE_VxGMMUW *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMMUW_reset(SIE_VxGMMUW *p);
SIGN32 VxGMMUW_cmp  (SIE_VxGMMUW *p, SIE_VxGMMUW *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMMUW_check(p,pie,pfx,hLOG) VxGMMUW_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMMUW_print(p,    pfx,hLOG) VxGMMUW_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMMUR
#define h_VxGMMUR (){}
#define     RA_VxGMMUR_cfg                                 0x0000
#define     RA_VxGMMUR_MMUIDMAP                            0x0004
#define     RA_VxGMMUR_mmuidmap                            0x0004
#define     RA_VxGMMUR_STRIDE                              0x000C
#define     RA_VxGMMUR_stride                              0x000C
#define     RA_VxGMMUR_BASE                                0x002C
#define     RA_VxGMMUR_base                                0x002C
typedef struct SIE_VxGMMUR {
#define     w32VxGMMUR_cfg                                 {\
	UNSG32 ucfg_pm_bw                                  :  3;\
	UNSG32 ucfg_pm_pos                                 :  1;\
	UNSG32 ucfg_pm_enable                              :  1;\
	UNSG32 ucfg_vm_enable                              :  1;\
	UNSG32 ucfg_shuffle_en                             :  1;\
	UNSG32 ucfg_bm_enable                              :  1;\
	UNSG32 ucfg_cbase_y                                :  6;\
	UNSG32 ucfg_cways_y                                :  3;\
	UNSG32 ucfg_rpTyp_y                                :  1;\
	UNSG32 ucfg_cbase_uv                               :  6;\
	UNSG32 ucfg_cways_uv                               :  3;\
	UNSG32 ucfg_rpTyp_uv                               :  1;\
	UNSG32 ucfg_auto_tag_clr                           :  1;\
	UNSG32 ucfg_ctype_y                                :  1;\
	UNSG32 ucfg_ctype_uv                               :  1;\
	UNSG32 RSVDx0_b31                                  :  1;\
}
union { UNSG32 u32VxGMMUR_cfg;
	struct w32VxGMMUR_cfg;
};
SIE_IDMAP                                        ie_mmuidmap;
SIE_VxGMMU_STRD                                  ie_stride[8];
SIE_VxGMMU_BASE                                  ie_base[32];
} SIE_VxGMMUR;
typedef union  T32VxGMMUR_cfg
{ UNSG32 u32;
	struct w32VxGMMUR_cfg;
} T32VxGMMUR_cfg;
typedef union  TVxGMMUR_cfg
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMUR_cfg;
	};
} TVxGMMUR_cfg;
SIGN32 VxGMMUR_drvrd(SIE_VxGMMUR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMMUR_drvwr(SIE_VxGMMUR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMMUR_reset(SIE_VxGMMUR *p);
SIGN32 VxGMMUR_cmp  (SIE_VxGMMUR *p, SIE_VxGMMUR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMMUR_check(p,pie,pfx,hLOG) VxGMMUR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMMUR_print(p,    pfx,hLOG) VxGMMUR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMMU
#define h_VxGMMU (){}
#define     RA_VxGMMU_MMU_TAG                              0x0000
#define     RA_VxGMMU_mmu_tags                             0x0000
#define     RA_VxGMMU_ctrl                                 0x0100
#define     RA_VxGMMU_cfg                                  0x0104
#define        VxGMMU_cfg_pageSz_4K                                     0x0
#define        VxGMMU_cfg_pageSz_16K                                    0x1
#define        VxGMMU_cfg_pageSz_64K                                    0x2
#define        VxGMMU_cfg_pageSz_256K                                   0x3
#define        VxGMMU_cfg_pageSz_1M                                     0x4
#define        VxGMMU_cfg_pageSz_2M                                     0x5
#define     RA_VxGMMU_MMUW                                 0x0108
#define     RA_VxGMMU_mmuw                                 0x0108
#define     RA_VxGMMU_MMUR                                 0x0158
#define     RA_VxGMMU_mmur                                 0x0158
typedef struct SIE_VxGMMU {
	SIE_MMU_TAG                                      ie_mmu_tags;
#define     w32VxGMMU_ctrl                                 {\
	UNSG32 uctrl_kickoff                               :  1;\
	UNSG32 uctrl_page_fault                            :  1;\
	UNSG32 uctrl_pfa_idx                               : 16;\
	UNSG32 uctrl_pfa_tid                               :  6;\
	UNSG32 uctrl_safe_adr                              :  8;\
}
	union { UNSG32 u32VxGMMU_ctrl;
		struct w32VxGMMU_ctrl;
	};
#define     w32VxGMMU_cfg                                  {\
	UNSG32 ucfg_pageSz                                 :  3;\
	UNSG32 ucfg_qos_en                                 :  1;\
	UNSG32 ucfg_qos                                    :  4;\
	UNSG32 ucfg_banks                                  :  1;\
	UNSG32 ucfg_sysSz                                  :  2;\
	UNSG32 ucfg_tarSz                                  :  2;\
	UNSG32 ucfg_Reserved                               :  1;\
	UNSG32 RSVDx104_b14                                : 18;\
}
union { UNSG32 u32VxGMMU_cfg;
	struct w32VxGMMU_cfg;
};
SIE_VxGMMUW                                      ie_mmuw;
SIE_VxGMMUR                                      ie_mmur;
UNSG8 RSVDx204                                    [252];
} SIE_VxGMMU;
typedef union  T32VxGMMU_ctrl
{ UNSG32 u32;
	struct w32VxGMMU_ctrl;
} T32VxGMMU_ctrl;
typedef union  T32VxGMMU_cfg
{ UNSG32 u32;
	struct w32VxGMMU_cfg;
} T32VxGMMU_cfg;
typedef union  TVxGMMU_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMU_ctrl;
	};
} TVxGMMU_ctrl;
typedef union  TVxGMMU_cfg
{ UNSG32 u32[1];
	struct {
		struct w32VxGMMU_cfg;
	};
} TVxGMMU_cfg;
SIGN32 VxGMMU_drvrd(SIE_VxGMMU *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMMU_drvwr(SIE_VxGMMU *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMMU_reset(SIE_VxGMMU *p);
SIGN32 VxGMMU_cmp  (SIE_VxGMMU *p, SIE_VxGMMU *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMMU_check(p,pie,pfx,hLOG) VxGMMU_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMMU_print(p,    pfx,hLOG) VxGMMU_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTRR
#define h_MTRR (){}
#define     RA_MTRR_MTR                                    0x0000
#define     RA_MTRR_mtr                                    0x0000
#define     RA_MTRR_MMU                                    0x0008
#define     RA_MTRR_mmu                                    0x0008
#define     RA_MTRR_roix                                   0x0014
#define     RA_MTRR_roiy                                   0x0018
typedef struct SIE_MTRR {
	SIE_MTRR_CFG                                     ie_mtr;
	SIE_MMU_CFG                                      ie_mmu;
#define     w32MTRR_roix                                   {\
	UNSG32 uroix_xs                                    : 12;\
	UNSG32 uroix_xm                                    : 12;\
	UNSG32 RSVDx14_b24                                 :  8;\
}
	union { UNSG32 u32MTRR_roix;
		struct w32MTRR_roix;
	};
#define     w32MTRR_roiy                                   {\
	UNSG32 uroiy_ys                                    : 14;\
	UNSG32 uroiy_ym                                    : 14;\
	UNSG32 RSVDx18_b28                                 :  4;\
}
union { UNSG32 u32MTRR_roiy;
	struct w32MTRR_roiy;
};
} SIE_MTRR;
typedef union  T32MTRR_roix
{ UNSG32 u32;
	struct w32MTRR_roix;
} T32MTRR_roix;
typedef union  T32MTRR_roiy
{ UNSG32 u32;
	struct w32MTRR_roiy;
} T32MTRR_roiy;
typedef union  TMTRR_roix
{ UNSG32 u32[1];
	struct {
		struct w32MTRR_roix;
	};
} TMTRR_roix;
typedef union  TMTRR_roiy
{ UNSG32 u32[1];
	struct {
		struct w32MTRR_roiy;
	};
} TMTRR_roiy;
SIGN32 MTRR_drvrd(SIE_MTRR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTRR_drvwr(SIE_MTRR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTRR_reset(SIE_MTRR *p);
SIGN32 MTRR_cmp  (SIE_MTRR *p, SIE_MTRR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTRR_check(p,pie,pfx,hLOG) MTRR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTRR_print(p,    pfx,hLOG) MTRR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTRW
#define h_MTRW (){}
#define     RA_MTRW_MTR                                    0x0000
#define     RA_MTRW_mtr                                    0x0000
#define     RA_MTRW_MMU                                    0x0008
#define     RA_MTRW_mmu                                    0x0008
#define     RA_MTRW_roix                                   0x0014
#define     RA_MTRW_roiy                                   0x0018
typedef struct SIE_MTRW {
	SIE_MTRW_CFG                                     ie_mtr;
	SIE_MMU_CFG                                      ie_mmu;
#define     w32MTRW_roix                                   {\
	UNSG32 uroix_xs                                    : 12;\
	UNSG32 uroix_xm                                    : 12;\
	UNSG32 RSVDx14_b24                                 :  8;\
}
	union { UNSG32 u32MTRW_roix;
		struct w32MTRW_roix;
	};
#define     w32MTRW_roiy                                   {\
	UNSG32 uroiy_ys                                    : 14;\
	UNSG32 uroiy_ym                                    : 14;\
	UNSG32 RSVDx18_b28                                 :  4;\
}
union { UNSG32 u32MTRW_roiy;
	struct w32MTRW_roiy;
};
} SIE_MTRW;
typedef union  T32MTRW_roix
{ UNSG32 u32;
	struct w32MTRW_roix;
} T32MTRW_roix;
typedef union  T32MTRW_roiy
{ UNSG32 u32;
	struct w32MTRW_roiy;
} T32MTRW_roiy;
typedef union  TMTRW_roix
{ UNSG32 u32[1];
	struct {
		struct w32MTRW_roix;
	};
} TMTRW_roix;
typedef union  TMTRW_roiy
{ UNSG32 u32[1];
	struct {
		struct w32MTRW_roiy;
	};
} TMTRW_roiy;
SIGN32 MTRW_drvrd(SIE_MTRW *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTRW_drvwr(SIE_MTRW *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTRW_reset(SIE_MTRW *p);
SIGN32 MTRW_cmp  (SIE_MTRW *p, SIE_MTRW *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTRW_check(p,pie,pfx,hLOG) MTRW_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTRW_print(p,    pfx,hLOG) MTRW_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AVIOMTR
#define h_AVIOMTR (){}
#define     RA_AVIOMTR_Version                             0x0000
#define     RA_AVIOMTR_Build                               0x0004
#define     RA_AVIOMTR_MMU_TAG                             0x0100
#define     RA_AVIOMTR_mmu_tags                            0x0100
#define     RA_AVIOMTR_META_TAG                            0x0200
#define     RA_AVIOMTR_meta_tags                           0x0200
#define     RA_AVIOMTR_kickoffR0                           0x0300
#define     RA_AVIOMTR_kickoffR1                           0x0304
#define     RA_AVIOMTR_kickoffR2                           0x0308
#define     RA_AVIOMTR_kickoffR3                           0x030C
#define     RA_AVIOMTR_kickoffR4                           0x0310
#define     RA_AVIOMTR_kickoffR5                           0x0314
#define     RA_AVIOMTR_kickoffR6                           0x0318
#define     RA_AVIOMTR_kickoffR7                           0x031C
#define     RA_AVIOMTR_kickoffR8                           0x0320
#define     RA_AVIOMTR_kickoffR9                           0x0324
#define     RA_AVIOMTR_kickoffR10                          0x0328
#define     RA_AVIOMTR_kickoffR11                          0x032C
#define     RA_AVIOMTR_cfg                                 0x0330
#define        AVIOMTR_cfg_mmu_pageSz_4K                                0x0
#define        AVIOMTR_cfg_mmu_pageSz_16K                               0x1
#define        AVIOMTR_cfg_mmu_pageSz_64K                               0x2
#define        AVIOMTR_cfg_mmu_pageSz_256K                              0x3
#define        AVIOMTR_cfg_mmu_pageSz_1M                                0x4
#define        AVIOMTR_cfg_mmu_pageSz_2M                                0x5
#define     RA_AVIOMTR_pfa                                 0x0334
#define     RA_AVIOMTR_MTRR                                0x0338
#define     RA_AVIOMTR_mtrr                                0x0338
typedef struct SIE_AVIOMTR {
#define     w32AVIOMTR_Version                             {\
	UNSG32 uVersion_build                              :  8;\
	UNSG32 uVersion_rsvd                               :  8;\
	UNSG32 uVersion_minor                              :  8;\
	UNSG32 uVersion_major                              :  8;\
}
union { UNSG32 u32AVIOMTR_Version;
	struct w32AVIOMTR_Version;
};
#define     w32AVIOMTR_Build                               {\
	UNSG32 uBuild_version                              : 32;\
}
union { UNSG32 u32AVIOMTR_Build;
	struct w32AVIOMTR_Build;
};
UNSG8 RSVDx8                                      [248];
SIE_MMU_TAG                                      ie_mmu_tags;
SIE_META_TAG                                     ie_meta_tags;
#define     w32AVIOMTR_kickoffR0                           {\
	UNSG32 ukickoffR0_rthread0                         :  1;\
	UNSG32 RSVDx300_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR0;
	struct w32AVIOMTR_kickoffR0;
};
#define     w32AVIOMTR_kickoffR1                           {\
	UNSG32 ukickoffR1_rthread1                         :  1;\
	UNSG32 RSVDx304_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR1;
	struct w32AVIOMTR_kickoffR1;
};
#define     w32AVIOMTR_kickoffR2                           {\
	UNSG32 ukickoffR2_rthread2                         :  1;\
	UNSG32 RSVDx308_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR2;
	struct w32AVIOMTR_kickoffR2;
};
#define     w32AVIOMTR_kickoffR3                           {\
	UNSG32 ukickoffR3_rthread3                         :  1;\
	UNSG32 RSVDx30C_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR3;
	struct w32AVIOMTR_kickoffR3;
};
#define     w32AVIOMTR_kickoffR4                           {\
	UNSG32 ukickoffR4_rthread4                         :  1;\
	UNSG32 RSVDx310_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR4;
	struct w32AVIOMTR_kickoffR4;
};
#define     w32AVIOMTR_kickoffR5                           {\
	UNSG32 ukickoffR5_rthread5                         :  1;\
	UNSG32 RSVDx314_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR5;
	struct w32AVIOMTR_kickoffR5;
};
#define     w32AVIOMTR_kickoffR6                           {\
	UNSG32 ukickoffR6_rthread6                         :  1;\
	UNSG32 RSVDx318_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR6;
	struct w32AVIOMTR_kickoffR6;
};
#define     w32AVIOMTR_kickoffR7                           {\
	UNSG32 ukickoffR7_rthread7                         :  1;\
	UNSG32 RSVDx31C_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR7;
	struct w32AVIOMTR_kickoffR7;
};
#define     w32AVIOMTR_kickoffR8                           {\
	UNSG32 ukickoffR8_rthread8                         :  1;\
	UNSG32 RSVDx320_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR8;
	struct w32AVIOMTR_kickoffR8;
};
#define     w32AVIOMTR_kickoffR9                           {\
	UNSG32 ukickoffR9_rthread9                         :  1;\
	UNSG32 RSVDx324_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR9;
	struct w32AVIOMTR_kickoffR9;
};
#define     w32AVIOMTR_kickoffR10                          {\
	UNSG32 ukickoffR10_rthread10                       :  1;\
	UNSG32 RSVDx328_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR10;
	struct w32AVIOMTR_kickoffR10;
};
#define     w32AVIOMTR_kickoffR11                          {\
	UNSG32 ukickoffR11_rthread11                       :  1;\
	UNSG32 RSVDx32C_b1                                 : 31;\
}
union { UNSG32 u32AVIOMTR_kickoffR11;
	struct w32AVIOMTR_kickoffR11;
};
#define     w32AVIOMTR_cfg                                 {\
	UNSG32 ucfg_mtrr_flow_ctrl                         :  1;\
	UNSG32 ucfg_mtrr_qos_en                            :  1;\
	UNSG32 ucfg_mtrr_qos                               :  4;\
	UNSG32 ucfg_mtrr_dis_mtid                          :  1;\
	UNSG32 ucfg_mmu_pageSz                             :  3;\
	UNSG32 ucfg_mmu_qos_en                             :  1;\
	UNSG32 ucfg_mmu_qos                                :  4;\
	UNSG32 ucfg_mmu_banks                              :  1;\
	UNSG32 ucfg_mmu_sysSz                              :  2;\
	UNSG32 ucfg_mmu_tarSz                              :  2;\
	UNSG32 ucfg_Reserved                               :  1;\
	UNSG32 RSVDx330_b21                                : 11;\
}
union { UNSG32 u32AVIOMTR_cfg;
	struct w32AVIOMTR_cfg;
};
#define     w32AVIOMTR_pfa                                 {\
	UNSG32 upfa_page_fault                             :  1;\
	UNSG32 upfa_pfa_idx                                : 16;\
	UNSG32 upfa_pfa_tid                                :  6;\
	UNSG32 upfa_safe_adr                               :  9;\
}
union { UNSG32 u32AVIOMTR_pfa;
	struct w32AVIOMTR_pfa;
};
SIE_MTRR                                         ie_mtrr[12];
UNSG8 RSVDx488                                    [120];
} SIE_AVIOMTR;
typedef union  T32AVIOMTR_Version
{ UNSG32 u32;
	struct w32AVIOMTR_Version;
} T32AVIOMTR_Version;
typedef union  T32AVIOMTR_Build
{ UNSG32 u32;
	struct w32AVIOMTR_Build;
} T32AVIOMTR_Build;
typedef union  T32AVIOMTR_kickoffR0
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR0;
} T32AVIOMTR_kickoffR0;
typedef union  T32AVIOMTR_kickoffR1
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR1;
} T32AVIOMTR_kickoffR1;
typedef union  T32AVIOMTR_kickoffR2
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR2;
} T32AVIOMTR_kickoffR2;
typedef union  T32AVIOMTR_kickoffR3
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR3;
} T32AVIOMTR_kickoffR3;
typedef union  T32AVIOMTR_kickoffR4
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR4;
} T32AVIOMTR_kickoffR4;
typedef union  T32AVIOMTR_kickoffR5
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR5;
} T32AVIOMTR_kickoffR5;
typedef union  T32AVIOMTR_kickoffR6
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR6;
} T32AVIOMTR_kickoffR6;
typedef union  T32AVIOMTR_kickoffR7
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR7;
} T32AVIOMTR_kickoffR7;
typedef union  T32AVIOMTR_kickoffR8
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR8;
} T32AVIOMTR_kickoffR8;
typedef union  T32AVIOMTR_kickoffR9
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR9;
} T32AVIOMTR_kickoffR9;
typedef union  T32AVIOMTR_kickoffR10
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR10;
} T32AVIOMTR_kickoffR10;
typedef union  T32AVIOMTR_kickoffR11
{ UNSG32 u32;
	struct w32AVIOMTR_kickoffR11;
} T32AVIOMTR_kickoffR11;
typedef union  T32AVIOMTR_cfg
{ UNSG32 u32;
	struct w32AVIOMTR_cfg;
} T32AVIOMTR_cfg;
typedef union  T32AVIOMTR_pfa
{ UNSG32 u32;
	struct w32AVIOMTR_pfa;
} T32AVIOMTR_pfa;
typedef union  TAVIOMTR_Version
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_Version;
	};
} TAVIOMTR_Version;
typedef union  TAVIOMTR_Build
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_Build;
	};
} TAVIOMTR_Build;
typedef union  TAVIOMTR_kickoffR0
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR0;
	};
} TAVIOMTR_kickoffR0;
typedef union  TAVIOMTR_kickoffR1
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR1;
	};
} TAVIOMTR_kickoffR1;
typedef union  TAVIOMTR_kickoffR2
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR2;
	};
} TAVIOMTR_kickoffR2;
typedef union  TAVIOMTR_kickoffR3
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR3;
	};
} TAVIOMTR_kickoffR3;
typedef union  TAVIOMTR_kickoffR4
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR4;
	};
} TAVIOMTR_kickoffR4;
typedef union  TAVIOMTR_kickoffR5
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR5;
	};
} TAVIOMTR_kickoffR5;
typedef union  TAVIOMTR_kickoffR6
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR6;
	};
} TAVIOMTR_kickoffR6;
typedef union  TAVIOMTR_kickoffR7
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR7;
	};
} TAVIOMTR_kickoffR7;
typedef union  TAVIOMTR_kickoffR8
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR8;
	};
} TAVIOMTR_kickoffR8;
typedef union  TAVIOMTR_kickoffR9
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR9;
	};
} TAVIOMTR_kickoffR9;
typedef union  TAVIOMTR_kickoffR10
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR10;
	};
} TAVIOMTR_kickoffR10;
typedef union  TAVIOMTR_kickoffR11
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_kickoffR11;
	};
} TAVIOMTR_kickoffR11;
typedef union  TAVIOMTR_cfg
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_cfg;
	};
} TAVIOMTR_cfg;
typedef union  TAVIOMTR_pfa
{ UNSG32 u32[1];
	struct {
		struct w32AVIOMTR_pfa;
	};
} TAVIOMTR_pfa;
SIGN32 AVIOMTR_drvrd(SIE_AVIOMTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 AVIOMTR_drvwr(SIE_AVIOMTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void AVIOMTR_reset(SIE_AVIOMTR *p);
SIGN32 AVIOMTR_cmp  (SIE_AVIOMTR *p, SIE_AVIOMTR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define AVIOMTR_check(p,pie,pfx,hLOG) AVIOMTR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define AVIOMTR_print(p,    pfx,hLOG) AVIOMTR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ISPMTR
#define h_ISPMTR (){}
#define     RA_ISPMTR_Version                              0x0000
#define     RA_ISPMTR_Build                                0x0004
#define     RA_ISPMTR_MMU_TAG                              0x0100
#define     RA_ISPMTR_mmu_tags                             0x0100
#define     RA_ISPMTR_META_TAG                             0x0200
#define     RA_ISPMTR_meta_tags                            0x0200
#define     RA_ISPMTR_kickoffR0                            0x0300
#define     RA_ISPMTR_kickoffR1                            0x0304
#define     RA_ISPMTR_kickoffR2                            0x0308
#define     RA_ISPMTR_kickoffR3                            0x030C
#define     RA_ISPMTR_kickoffR4                            0x0310
#define     RA_ISPMTR_kickoffR5                            0x0314
#define     RA_ISPMTR_kickoffR6                            0x0318
#define     RA_ISPMTR_kickoffR7                            0x031C
#define     RA_ISPMTR_kickoffR8                            0x0320
#define     RA_ISPMTR_kickoffR9                            0x0324
#define     RA_ISPMTR_kickoffR10                           0x0328
#define     RA_ISPMTR_kickoffR11                           0x032C
#define     RA_ISPMTR_kickoffR12                           0x0330
#define     RA_ISPMTR_kickoffR13                           0x0334
#define     RA_ISPMTR_kickoffR14                           0x0338
#define     RA_ISPMTR_kickoffR15                           0x033C
#define     RA_ISPMTR_kickoffW0                            0x0340
#define     RA_ISPMTR_kickoffW1                            0x0344
#define     RA_ISPMTR_kickoffW2                            0x0348
#define     RA_ISPMTR_kickoffW3                            0x034C
#define     RA_ISPMTR_kickoffW4                            0x0350
#define     RA_ISPMTR_kickoffW5                            0x0354
#define     RA_ISPMTR_kickoffW6                            0x0358
#define     RA_ISPMTR_kickoffW7                            0x035C
#define     RA_ISPMTR_kickoffW8                            0x0360
#define     RA_ISPMTR_kickoffW9                            0x0364
#define     RA_ISPMTR_kickoffW10                           0x0368
#define     RA_ISPMTR_kickoffW11                           0x036C
#define     RA_ISPMTR_kickoffW12                           0x0370
#define     RA_ISPMTR_kickoffW13                           0x0374
#define     RA_ISPMTR_kickoffW14                           0x0378
#define     RA_ISPMTR_kickoffW15                           0x037C
#define     RA_ISPMTR_cfg                                  0x0380
#define        ISPMTR_cfg_mmu_pageSz_4K                                 0x0
#define        ISPMTR_cfg_mmu_pageSz_16K                                0x1
#define        ISPMTR_cfg_mmu_pageSz_64K                                0x2
#define        ISPMTR_cfg_mmu_pageSz_256K                               0x3
#define        ISPMTR_cfg_mmu_pageSz_1M                                 0x4
#define        ISPMTR_cfg_mmu_pageSz_2M                                 0x5
#define     RA_ISPMTR_pfa                                  0x0384
#define     RA_ISPMTR_MTRR                                 0x0388
#define     RA_ISPMTR_mtrr                                 0x0388
#define     RA_ISPMTR_MTRW                                 0x0548
#define     RA_ISPMTR_mtrw                                 0x0548
typedef struct SIE_ISPMTR {
#define     w32ISPMTR_Version                              {\
	UNSG32 uVersion_build                              :  8;\
	UNSG32 uVersion_rsvd                               :  8;\
	UNSG32 uVersion_minor                              :  8;\
	UNSG32 uVersion_major                              :  8;\
}
union { UNSG32 u32ISPMTR_Version;
	struct w32ISPMTR_Version;
};
#define     w32ISPMTR_Build                                {\
	UNSG32 uBuild_version                              : 32;\
}
union { UNSG32 u32ISPMTR_Build;
	struct w32ISPMTR_Build;
};
UNSG8 RSVDx8                                      [248];
SIE_MMU_TAG                                      ie_mmu_tags;
SIE_META_TAG                                     ie_meta_tags;
#define     w32ISPMTR_kickoffR0                            {\
	UNSG32 ukickoffR0_rthread0                         :  1;\
	UNSG32 RSVDx300_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR0;
	struct w32ISPMTR_kickoffR0;
};
#define     w32ISPMTR_kickoffR1                            {\
	UNSG32 ukickoffR1_rthread1                         :  1;\
	UNSG32 RSVDx304_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR1;
	struct w32ISPMTR_kickoffR1;
};
#define     w32ISPMTR_kickoffR2                            {\
	UNSG32 ukickoffR2_rthread2                         :  1;\
	UNSG32 RSVDx308_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR2;
	struct w32ISPMTR_kickoffR2;
};
#define     w32ISPMTR_kickoffR3                            {\
	UNSG32 ukickoffR3_rthread3                         :  1;\
	UNSG32 RSVDx30C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR3;
	struct w32ISPMTR_kickoffR3;
};
#define     w32ISPMTR_kickoffR4                            {\
	UNSG32 ukickoffR4_rthread4                         :  1;\
	UNSG32 RSVDx310_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR4;
	struct w32ISPMTR_kickoffR4;
};
#define     w32ISPMTR_kickoffR5                            {\
	UNSG32 ukickoffR5_rthread5                         :  1;\
	UNSG32 RSVDx314_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR5;
	struct w32ISPMTR_kickoffR5;
};
#define     w32ISPMTR_kickoffR6                            {\
	UNSG32 ukickoffR6_rthread6                         :  1;\
	UNSG32 RSVDx318_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR6;
	struct w32ISPMTR_kickoffR6;
};
#define     w32ISPMTR_kickoffR7                            {\
	UNSG32 ukickoffR7_rthread7                         :  1;\
	UNSG32 RSVDx31C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR7;
	struct w32ISPMTR_kickoffR7;
};
#define     w32ISPMTR_kickoffR8                            {\
	UNSG32 ukickoffR8_rthread8                         :  1;\
	UNSG32 RSVDx320_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR8;
	struct w32ISPMTR_kickoffR8;
};
#define     w32ISPMTR_kickoffR9                            {\
	UNSG32 ukickoffR9_rthread9                         :  1;\
	UNSG32 RSVDx324_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR9;
	struct w32ISPMTR_kickoffR9;
};
#define     w32ISPMTR_kickoffR10                           {\
	UNSG32 ukickoffR10_rthread10                       :  1;\
	UNSG32 RSVDx328_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR10;
	struct w32ISPMTR_kickoffR10;
};
#define     w32ISPMTR_kickoffR11                           {\
	UNSG32 ukickoffR11_rthread11                       :  1;\
	UNSG32 RSVDx32C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR11;
	struct w32ISPMTR_kickoffR11;
};
#define     w32ISPMTR_kickoffR12                           {\
	UNSG32 ukickoffR12_rthread12                       :  1;\
	UNSG32 RSVDx330_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR12;
	struct w32ISPMTR_kickoffR12;
};
#define     w32ISPMTR_kickoffR13                           {\
	UNSG32 ukickoffR13_rthread13                       :  1;\
	UNSG32 RSVDx334_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR13;
	struct w32ISPMTR_kickoffR13;
};
#define     w32ISPMTR_kickoffR14                           {\
	UNSG32 ukickoffR14_rthread14                       :  1;\
	UNSG32 RSVDx338_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR14;
	struct w32ISPMTR_kickoffR14;
};
#define     w32ISPMTR_kickoffR15                           {\
	UNSG32 ukickoffR15_rthread15                       :  1;\
	UNSG32 RSVDx33C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffR15;
	struct w32ISPMTR_kickoffR15;
};
#define     w32ISPMTR_kickoffW0                            {\
	UNSG32 ukickoffW0_wthread0                         :  1;\
	UNSG32 RSVDx340_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW0;
	struct w32ISPMTR_kickoffW0;
};
#define     w32ISPMTR_kickoffW1                            {\
	UNSG32 ukickoffW1_wthread1                         :  1;\
	UNSG32 RSVDx344_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW1;
	struct w32ISPMTR_kickoffW1;
};
#define     w32ISPMTR_kickoffW2                            {\
	UNSG32 ukickoffW2_wthread2                         :  1;\
	UNSG32 RSVDx348_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW2;
	struct w32ISPMTR_kickoffW2;
};
#define     w32ISPMTR_kickoffW3                            {\
	UNSG32 ukickoffW3_wthread3                         :  1;\
	UNSG32 RSVDx34C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW3;
	struct w32ISPMTR_kickoffW3;
};
#define     w32ISPMTR_kickoffW4                            {\
	UNSG32 ukickoffW4_wthread4                         :  1;\
	UNSG32 RSVDx350_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW4;
	struct w32ISPMTR_kickoffW4;
};
#define     w32ISPMTR_kickoffW5                            {\
	UNSG32 ukickoffW5_wthread5                         :  1;\
	UNSG32 RSVDx354_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW5;
	struct w32ISPMTR_kickoffW5;
};
#define     w32ISPMTR_kickoffW6                            {\
	UNSG32 ukickoffW6_wthread6                         :  1;\
	UNSG32 RSVDx358_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW6;
	struct w32ISPMTR_kickoffW6;
};
#define     w32ISPMTR_kickoffW7                            {\
	UNSG32 ukickoffW7_wthread7                         :  1;\
	UNSG32 RSVDx35C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW7;
	struct w32ISPMTR_kickoffW7;
};
#define     w32ISPMTR_kickoffW8                            {\
	UNSG32 ukickoffW8_wthread8                         :  1;\
	UNSG32 RSVDx360_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW8;
	struct w32ISPMTR_kickoffW8;
};
#define     w32ISPMTR_kickoffW9                            {\
	UNSG32 ukickoffW9_wthread9                         :  1;\
	UNSG32 RSVDx364_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW9;
	struct w32ISPMTR_kickoffW9;
};
#define     w32ISPMTR_kickoffW10                           {\
	UNSG32 ukickoffW10_wthread10                       :  1;\
	UNSG32 RSVDx368_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW10;
	struct w32ISPMTR_kickoffW10;
};
#define     w32ISPMTR_kickoffW11                           {\
	UNSG32 ukickoffW11_wthread11                       :  1;\
	UNSG32 RSVDx36C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW11;
	struct w32ISPMTR_kickoffW11;
};
#define     w32ISPMTR_kickoffW12                           {\
	UNSG32 ukickoffW12_wthread12                       :  1;\
	UNSG32 RSVDx370_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW12;
	struct w32ISPMTR_kickoffW12;
};
#define     w32ISPMTR_kickoffW13                           {\
	UNSG32 ukickoffW13_wthread13                       :  1;\
	UNSG32 RSVDx374_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW13;
	struct w32ISPMTR_kickoffW13;
};
#define     w32ISPMTR_kickoffW14                           {\
	UNSG32 ukickoffW14_wthread14                       :  1;\
	UNSG32 RSVDx378_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW14;
	struct w32ISPMTR_kickoffW14;
};
#define     w32ISPMTR_kickoffW15                           {\
	UNSG32 ukickoffW15_wthread15                       :  1;\
	UNSG32 RSVDx37C_b1                                 : 31;\
}
union { UNSG32 u32ISPMTR_kickoffW15;
	struct w32ISPMTR_kickoffW15;
};
#define     w32ISPMTR_cfg                                  {\
	UNSG32 ucfg_mtrr_flow_ctrl                         :  1;\
	UNSG32 ucfg_mtrr_qos_en                            :  1;\
	UNSG32 ucfg_mtrr_qos                               :  4;\
	UNSG32 ucfg_mtrr_dis_mtid                          :  1;\
	UNSG32 ucfg_mmu_pageSz                             :  3;\
	UNSG32 ucfg_mmu_qos_en                             :  1;\
	UNSG32 ucfg_mmu_qos                                :  4;\
	UNSG32 ucfg_mmu_banks                              :  1;\
	UNSG32 ucfg_mmu_sysSz                              :  2;\
	UNSG32 ucfg_mmu_tarSz                              :  2;\
	UNSG32 ucfg_Reserved                               :  1;\
	UNSG32 RSVDx380_b21                                : 11;\
}
union { UNSG32 u32ISPMTR_cfg;
	struct w32ISPMTR_cfg;
};
#define     w32ISPMTR_pfa                                  {\
	UNSG32 upfa_page_fault                             :  1;\
	UNSG32 upfa_pfa_idx                                : 16;\
	UNSG32 upfa_pfa_tid                                :  6;\
	UNSG32 upfa_safe_adr                               :  9;\
}
union { UNSG32 u32ISPMTR_pfa;
	struct w32ISPMTR_pfa;
};
SIE_MTRR                                         ie_mtrr[16];
SIE_MTRW                                         ie_mtrw[16];
UNSG8 RSVDx708                                    [248];
} SIE_ISPMTR;
typedef union  T32ISPMTR_Version
{ UNSG32 u32;
	struct w32ISPMTR_Version;
} T32ISPMTR_Version;
typedef union  T32ISPMTR_Build
{ UNSG32 u32;
	struct w32ISPMTR_Build;
} T32ISPMTR_Build;
typedef union  T32ISPMTR_kickoffR0
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR0;
} T32ISPMTR_kickoffR0;
typedef union  T32ISPMTR_kickoffR1
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR1;
} T32ISPMTR_kickoffR1;
typedef union  T32ISPMTR_kickoffR2
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR2;
} T32ISPMTR_kickoffR2;
typedef union  T32ISPMTR_kickoffR3
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR3;
} T32ISPMTR_kickoffR3;
typedef union  T32ISPMTR_kickoffR4
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR4;
} T32ISPMTR_kickoffR4;
typedef union  T32ISPMTR_kickoffR5
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR5;
} T32ISPMTR_kickoffR5;
typedef union  T32ISPMTR_kickoffR6
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR6;
} T32ISPMTR_kickoffR6;
typedef union  T32ISPMTR_kickoffR7
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR7;
} T32ISPMTR_kickoffR7;
typedef union  T32ISPMTR_kickoffR8
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR8;
} T32ISPMTR_kickoffR8;
typedef union  T32ISPMTR_kickoffR9
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR9;
} T32ISPMTR_kickoffR9;
typedef union  T32ISPMTR_kickoffR10
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR10;
} T32ISPMTR_kickoffR10;
typedef union  T32ISPMTR_kickoffR11
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR11;
} T32ISPMTR_kickoffR11;
typedef union  T32ISPMTR_kickoffR12
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR12;
} T32ISPMTR_kickoffR12;
typedef union  T32ISPMTR_kickoffR13
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR13;
} T32ISPMTR_kickoffR13;
typedef union  T32ISPMTR_kickoffR14
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR14;
} T32ISPMTR_kickoffR14;
typedef union  T32ISPMTR_kickoffR15
{ UNSG32 u32;
	struct w32ISPMTR_kickoffR15;
} T32ISPMTR_kickoffR15;
typedef union  T32ISPMTR_kickoffW0
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW0;
} T32ISPMTR_kickoffW0;
typedef union  T32ISPMTR_kickoffW1
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW1;
} T32ISPMTR_kickoffW1;
typedef union  T32ISPMTR_kickoffW2
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW2;
} T32ISPMTR_kickoffW2;
typedef union  T32ISPMTR_kickoffW3
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW3;
} T32ISPMTR_kickoffW3;
typedef union  T32ISPMTR_kickoffW4
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW4;
} T32ISPMTR_kickoffW4;
typedef union  T32ISPMTR_kickoffW5
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW5;
} T32ISPMTR_kickoffW5;
typedef union  T32ISPMTR_kickoffW6
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW6;
} T32ISPMTR_kickoffW6;
typedef union  T32ISPMTR_kickoffW7
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW7;
} T32ISPMTR_kickoffW7;
typedef union  T32ISPMTR_kickoffW8
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW8;
} T32ISPMTR_kickoffW8;
typedef union  T32ISPMTR_kickoffW9
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW9;
} T32ISPMTR_kickoffW9;
typedef union  T32ISPMTR_kickoffW10
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW10;
} T32ISPMTR_kickoffW10;
typedef union  T32ISPMTR_kickoffW11
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW11;
} T32ISPMTR_kickoffW11;
typedef union  T32ISPMTR_kickoffW12
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW12;
} T32ISPMTR_kickoffW12;
typedef union  T32ISPMTR_kickoffW13
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW13;
} T32ISPMTR_kickoffW13;
typedef union  T32ISPMTR_kickoffW14
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW14;
} T32ISPMTR_kickoffW14;
typedef union  T32ISPMTR_kickoffW15
{ UNSG32 u32;
	struct w32ISPMTR_kickoffW15;
} T32ISPMTR_kickoffW15;
typedef union  T32ISPMTR_cfg
{ UNSG32 u32;
	struct w32ISPMTR_cfg;
} T32ISPMTR_cfg;
typedef union  T32ISPMTR_pfa
{ UNSG32 u32;
	struct w32ISPMTR_pfa;
} T32ISPMTR_pfa;
typedef union  TISPMTR_Version
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_Version;
	};
} TISPMTR_Version;
typedef union  TISPMTR_Build
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_Build;
	};
} TISPMTR_Build;
typedef union  TISPMTR_kickoffR0
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR0;
	};
} TISPMTR_kickoffR0;
typedef union  TISPMTR_kickoffR1
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR1;
	};
} TISPMTR_kickoffR1;
typedef union  TISPMTR_kickoffR2
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR2;
	};
} TISPMTR_kickoffR2;
typedef union  TISPMTR_kickoffR3
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR3;
	};
} TISPMTR_kickoffR3;
typedef union  TISPMTR_kickoffR4
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR4;
	};
} TISPMTR_kickoffR4;
typedef union  TISPMTR_kickoffR5
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR5;
	};
} TISPMTR_kickoffR5;
typedef union  TISPMTR_kickoffR6
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR6;
	};
} TISPMTR_kickoffR6;
typedef union  TISPMTR_kickoffR7
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR7;
	};
} TISPMTR_kickoffR7;
typedef union  TISPMTR_kickoffR8
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR8;
	};
} TISPMTR_kickoffR8;
typedef union  TISPMTR_kickoffR9
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR9;
	};
} TISPMTR_kickoffR9;
typedef union  TISPMTR_kickoffR10
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR10;
	};
} TISPMTR_kickoffR10;
typedef union  TISPMTR_kickoffR11
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR11;
	};
} TISPMTR_kickoffR11;
typedef union  TISPMTR_kickoffR12
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR12;
	};
} TISPMTR_kickoffR12;
typedef union  TISPMTR_kickoffR13
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR13;
	};
} TISPMTR_kickoffR13;
typedef union  TISPMTR_kickoffR14
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR14;
	};
} TISPMTR_kickoffR14;
typedef union  TISPMTR_kickoffR15
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffR15;
	};
} TISPMTR_kickoffR15;
typedef union  TISPMTR_kickoffW0
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW0;
	};
} TISPMTR_kickoffW0;
typedef union  TISPMTR_kickoffW1
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW1;
	};
} TISPMTR_kickoffW1;
typedef union  TISPMTR_kickoffW2
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW2;
	};
} TISPMTR_kickoffW2;
typedef union  TISPMTR_kickoffW3
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW3;
	};
} TISPMTR_kickoffW3;
typedef union  TISPMTR_kickoffW4
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW4;
	};
} TISPMTR_kickoffW4;
typedef union  TISPMTR_kickoffW5
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW5;
	};
} TISPMTR_kickoffW5;
typedef union  TISPMTR_kickoffW6
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW6;
	};
} TISPMTR_kickoffW6;
typedef union  TISPMTR_kickoffW7
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW7;
	};
} TISPMTR_kickoffW7;
typedef union  TISPMTR_kickoffW8
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW8;
	};
} TISPMTR_kickoffW8;
typedef union  TISPMTR_kickoffW9
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW9;
	};
} TISPMTR_kickoffW9;
typedef union  TISPMTR_kickoffW10
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW10;
	};
} TISPMTR_kickoffW10;
typedef union  TISPMTR_kickoffW11
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW11;
	};
} TISPMTR_kickoffW11;
typedef union  TISPMTR_kickoffW12
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW12;
	};
} TISPMTR_kickoffW12;
typedef union  TISPMTR_kickoffW13
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW13;
	};
} TISPMTR_kickoffW13;
typedef union  TISPMTR_kickoffW14
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW14;
	};
} TISPMTR_kickoffW14;
typedef union  TISPMTR_kickoffW15
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_kickoffW15;
	};
} TISPMTR_kickoffW15;
typedef union  TISPMTR_cfg
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_cfg;
	};
} TISPMTR_cfg;
typedef union  TISPMTR_pfa
{ UNSG32 u32[1];
	struct {
		struct w32ISPMTR_pfa;
	};
} TISPMTR_pfa;
SIGN32 ISPMTR_drvrd(SIE_ISPMTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 ISPMTR_drvwr(SIE_ISPMTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void ISPMTR_reset(SIE_ISPMTR *p);
SIGN32 ISPMTR_cmp  (SIE_ISPMTR *p, SIE_ISPMTR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define ISPMTR_check(p,pie,pfx,hLOG) ISPMTR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define ISPMTR_print(p,    pfx,hLOG) ISPMTR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VxGMTR
#define h_VxGMTR (){}
#define     RA_VxGMTR_Version                              0x0000
#define     RA_VxGMTR_Build                                0x0004
#define     RA_VxGMTR_MMU_TAG                              0x0100
#define     RA_VxGMTR_mmu_tags                             0x0100
#define     RA_VxGMTR_META_TAG                             0x0200
#define     RA_VxGMTR_meta_tags                            0x0200
#define     RA_VxGMTR_kickoffR0                            0x0300
#define     RA_VxGMTR_kickoffR1                            0x0304
#define     RA_VxGMTR_kickoffR2                            0x0308
#define     RA_VxGMTR_kickoffR3                            0x030C
#define     RA_VxGMTR_kickoffR4                            0x0310
#define     RA_VxGMTR_kickoffR5                            0x0314
#define     RA_VxGMTR_kickoffR6                            0x0318
#define     RA_VxGMTR_kickoffR7                            0x031C
#define     RA_VxGMTR_kickoffR8                            0x0320
#define     RA_VxGMTR_kickoffR9                            0x0324
#define     RA_VxGMTR_kickoffR10                           0x0328
#define     RA_VxGMTR_kickoffR11                           0x032C
#define     RA_VxGMTR_kickoffR12                           0x0330
#define     RA_VxGMTR_kickoffR13                           0x0334
#define     RA_VxGMTR_kickoffR14                           0x0338
#define     RA_VxGMTR_kickoffR15                           0x033C
#define     RA_VxGMTR_kickoffR16                           0x0340
#define     RA_VxGMTR_kickoffR17                           0x0344
#define     RA_VxGMTR_kickoffR18                           0x0348
#define     RA_VxGMTR_kickoffR19                           0x034C
#define     RA_VxGMTR_kickoffR20                           0x0350
#define     RA_VxGMTR_kickoffR21                           0x0354
#define     RA_VxGMTR_kickoffR22                           0x0358
#define     RA_VxGMTR_kickoffR23                           0x035C
#define     RA_VxGMTR_kickoffR24                           0x0360
#define     RA_VxGMTR_kickoffR25                           0x0364
#define     RA_VxGMTR_kickoffR26                           0x0368
#define     RA_VxGMTR_kickoffR27                           0x036C
#define     RA_VxGMTR_kickoffR28                           0x0370
#define     RA_VxGMTR_kickoffR29                           0x0374
#define     RA_VxGMTR_kickoffR30                           0x0378
#define     RA_VxGMTR_kickoffR31                           0x037C
#define     RA_VxGMTR_kickoffW0                            0x0380
#define     RA_VxGMTR_kickoffW1                            0x0384
#define     RA_VxGMTR_kickoffW2                            0x0388
#define     RA_VxGMTR_kickoffW3                            0x038C
#define     RA_VxGMTR_cfg                                  0x0390
#define        VxGMTR_cfg_mmu_pageSz_4K                                 0x0
#define        VxGMTR_cfg_mmu_pageSz_16K                                0x1
#define        VxGMTR_cfg_mmu_pageSz_64K                                0x2
#define        VxGMTR_cfg_mmu_pageSz_256K                               0x3
#define        VxGMTR_cfg_mmu_pageSz_1M                                 0x4
#define        VxGMTR_cfg_mmu_pageSz_2M                                 0x5
#define     RA_VxGMTR_pfa                                  0x0394
#define     RA_VxGMTR_VxGMETA                              0x0398
#define     RA_VxGMTR_metar                                0x0398
#define     RA_VxGMTR_VxGMMUR                              0x04AC
#define     RA_VxGMTR_mmur                                 0x04AC
#define     RA_VxGMTR_MTRW                                 0x0558
#define     RA_VxGMTR_mtrw                                 0x0558
typedef struct SIE_VxGMTR {
#define     w32VxGMTR_Version                              {\
	UNSG32 uVersion_build                              :  8;\
	UNSG32 uVersion_rsvd                               :  8;\
	UNSG32 uVersion_minor                              :  8;\
	UNSG32 uVersion_major                              :  8;\
}
union { UNSG32 u32VxGMTR_Version;
	struct w32VxGMTR_Version;
};
#define     w32VxGMTR_Build                                {\
	UNSG32 uBuild_version                              : 32;\
}
union { UNSG32 u32VxGMTR_Build;
	struct w32VxGMTR_Build;
};
UNSG8 RSVDx8                                      [248];
SIE_MMU_TAG                                      ie_mmu_tags;
SIE_META_TAG                                     ie_meta_tags;
#define     w32VxGMTR_kickoffR0                            {\
	UNSG32 ukickoffR0_rthread0                         :  1;\
	UNSG32 RSVDx300_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR0;
	struct w32VxGMTR_kickoffR0;
};
#define     w32VxGMTR_kickoffR1                            {\
	UNSG32 ukickoffR1_rthread1                         :  1;\
	UNSG32 RSVDx304_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR1;
	struct w32VxGMTR_kickoffR1;
};
#define     w32VxGMTR_kickoffR2                            {\
	UNSG32 ukickoffR2_rthread2                         :  1;\
	UNSG32 RSVDx308_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR2;
	struct w32VxGMTR_kickoffR2;
};
#define     w32VxGMTR_kickoffR3                            {\
	UNSG32 ukickoffR3_rthread3                         :  1;\
	UNSG32 RSVDx30C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR3;
	struct w32VxGMTR_kickoffR3;
};
#define     w32VxGMTR_kickoffR4                            {\
	UNSG32 ukickoffR4_rthread4                         :  1;\
	UNSG32 RSVDx310_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR4;
	struct w32VxGMTR_kickoffR4;
};
#define     w32VxGMTR_kickoffR5                            {\
	UNSG32 ukickoffR5_rthread5                         :  1;\
	UNSG32 RSVDx314_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR5;
	struct w32VxGMTR_kickoffR5;
};
#define     w32VxGMTR_kickoffR6                            {\
	UNSG32 ukickoffR6_rthread6                         :  1;\
	UNSG32 RSVDx318_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR6;
	struct w32VxGMTR_kickoffR6;
};
#define     w32VxGMTR_kickoffR7                            {\
	UNSG32 ukickoffR7_rthread7                         :  1;\
	UNSG32 RSVDx31C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR7;
	struct w32VxGMTR_kickoffR7;
};
#define     w32VxGMTR_kickoffR8                            {\
	UNSG32 ukickoffR8_rthread8                         :  1;\
	UNSG32 RSVDx320_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR8;
	struct w32VxGMTR_kickoffR8;
};
#define     w32VxGMTR_kickoffR9                            {\
	UNSG32 ukickoffR9_rthread9                         :  1;\
	UNSG32 RSVDx324_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR9;
	struct w32VxGMTR_kickoffR9;
};
#define     w32VxGMTR_kickoffR10                           {\
	UNSG32 ukickoffR10_rthread10                       :  1;\
	UNSG32 RSVDx328_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR10;
	struct w32VxGMTR_kickoffR10;
};
#define     w32VxGMTR_kickoffR11                           {\
	UNSG32 ukickoffR11_rthread11                       :  1;\
	UNSG32 RSVDx32C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR11;
	struct w32VxGMTR_kickoffR11;
};
#define     w32VxGMTR_kickoffR12                           {\
	UNSG32 ukickoffR12_rthread12                       :  1;\
	UNSG32 RSVDx330_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR12;
	struct w32VxGMTR_kickoffR12;
};
#define     w32VxGMTR_kickoffR13                           {\
	UNSG32 ukickoffR13_rthread13                       :  1;\
	UNSG32 RSVDx334_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR13;
	struct w32VxGMTR_kickoffR13;
};
#define     w32VxGMTR_kickoffR14                           {\
	UNSG32 ukickoffR14_rthread14                       :  1;\
	UNSG32 RSVDx338_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR14;
	struct w32VxGMTR_kickoffR14;
};
#define     w32VxGMTR_kickoffR15                           {\
	UNSG32 ukickoffR15_rthread15                       :  1;\
	UNSG32 RSVDx33C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR15;
	struct w32VxGMTR_kickoffR15;
};
#define     w32VxGMTR_kickoffR16                           {\
	UNSG32 ukickoffR16_rthread16                       :  1;\
	UNSG32 RSVDx340_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR16;
	struct w32VxGMTR_kickoffR16;
};
#define     w32VxGMTR_kickoffR17                           {\
	UNSG32 ukickoffR17_rthread17                       :  1;\
	UNSG32 RSVDx344_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR17;
	struct w32VxGMTR_kickoffR17;
};
#define     w32VxGMTR_kickoffR18                           {\
	UNSG32 ukickoffR18_rthread18                       :  1;\
	UNSG32 RSVDx348_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR18;
	struct w32VxGMTR_kickoffR18;
};
#define     w32VxGMTR_kickoffR19                           {\
	UNSG32 ukickoffR19_rthread19                       :  1;\
	UNSG32 RSVDx34C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR19;
	struct w32VxGMTR_kickoffR19;
};
#define     w32VxGMTR_kickoffR20                           {\
	UNSG32 ukickoffR20_rthread20                       :  1;\
	UNSG32 RSVDx350_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR20;
	struct w32VxGMTR_kickoffR20;
};
#define     w32VxGMTR_kickoffR21                           {\
	UNSG32 ukickoffR21_rthread21                       :  1;\
	UNSG32 RSVDx354_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR21;
	struct w32VxGMTR_kickoffR21;
};
#define     w32VxGMTR_kickoffR22                           {\
	UNSG32 ukickoffR22_rthread22                       :  1;\
	UNSG32 RSVDx358_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR22;
	struct w32VxGMTR_kickoffR22;
};
#define     w32VxGMTR_kickoffR23                           {\
	UNSG32 ukickoffR23_rthread23                       :  1;\
	UNSG32 RSVDx35C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR23;
	struct w32VxGMTR_kickoffR23;
};
#define     w32VxGMTR_kickoffR24                           {\
	UNSG32 ukickoffR24_rthread24                       :  1;\
	UNSG32 RSVDx360_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR24;
	struct w32VxGMTR_kickoffR24;
};
#define     w32VxGMTR_kickoffR25                           {\
	UNSG32 ukickoffR25_rthread25                       :  1;\
	UNSG32 RSVDx364_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR25;
	struct w32VxGMTR_kickoffR25;
};
#define     w32VxGMTR_kickoffR26                           {\
	UNSG32 ukickoffR26_rthread26                       :  1;\
	UNSG32 RSVDx368_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR26;
	struct w32VxGMTR_kickoffR26;
};
#define     w32VxGMTR_kickoffR27                           {\
	UNSG32 ukickoffR27_rthread27                       :  1;\
	UNSG32 RSVDx36C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR27;
	struct w32VxGMTR_kickoffR27;
};
#define     w32VxGMTR_kickoffR28                           {\
	UNSG32 ukickoffR28_rthread28                       :  1;\
	UNSG32 RSVDx370_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR28;
	struct w32VxGMTR_kickoffR28;
};
#define     w32VxGMTR_kickoffR29                           {\
	UNSG32 ukickoffR29_rthread29                       :  1;\
	UNSG32 RSVDx374_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR29;
	struct w32VxGMTR_kickoffR29;
};
#define     w32VxGMTR_kickoffR30                           {\
	UNSG32 ukickoffR30_rthread30                       :  1;\
	UNSG32 RSVDx378_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR30;
	struct w32VxGMTR_kickoffR30;
};
#define     w32VxGMTR_kickoffR31                           {\
	UNSG32 ukickoffR31_rthread31                       :  1;\
	UNSG32 RSVDx37C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffR31;
	struct w32VxGMTR_kickoffR31;
};
#define     w32VxGMTR_kickoffW0                            {\
	UNSG32 ukickoffW0_wthread0                         :  1;\
	UNSG32 RSVDx380_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffW0;
	struct w32VxGMTR_kickoffW0;
};
#define     w32VxGMTR_kickoffW1                            {\
	UNSG32 ukickoffW1_wthread1                         :  1;\
	UNSG32 RSVDx384_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffW1;
	struct w32VxGMTR_kickoffW1;
};
#define     w32VxGMTR_kickoffW2                            {\
	UNSG32 ukickoffW2_wthread2                         :  1;\
	UNSG32 RSVDx388_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffW2;
	struct w32VxGMTR_kickoffW2;
};
#define     w32VxGMTR_kickoffW3                            {\
	UNSG32 ukickoffW3_wthread3                         :  1;\
	UNSG32 RSVDx38C_b1                                 : 31;\
}
union { UNSG32 u32VxGMTR_kickoffW3;
	struct w32VxGMTR_kickoffW3;
};
#define     w32VxGMTR_cfg                                  {\
	UNSG32 ucfg_mtrr_flow_ctrl                         :  1;\
	UNSG32 ucfg_mtrr_qos_en                            :  1;\
	UNSG32 ucfg_mtrr_qos                               :  4;\
	UNSG32 ucfg_mtrr_dis_mtid                          :  1;\
	UNSG32 ucfg_mmu_pageSz                             :  3;\
	UNSG32 ucfg_mmu_qos_en                             :  1;\
	UNSG32 ucfg_mmu_qos                                :  4;\
	UNSG32 ucfg_mmu_banks                              :  1;\
	UNSG32 ucfg_mmu_sysSz                              :  2;\
	UNSG32 ucfg_mmu_tarSz                              :  2;\
	UNSG32 ucfg_Reserved                               :  1;\
	UNSG32 RSVDx390_b21                                : 11;\
}
union { UNSG32 u32VxGMTR_cfg;
	struct w32VxGMTR_cfg;
};
#define     w32VxGMTR_pfa                                  {\
	UNSG32 upfa_page_fault                             :  1;\
	UNSG32 upfa_pfa_idx                                : 16;\
	UNSG32 upfa_pfa_tid                                :  6;\
	UNSG32 upfa_safe_adr                               :  9;\
}
union { UNSG32 u32VxGMTR_pfa;
	struct w32VxGMTR_pfa;
};
SIE_VxGMETA                                      ie_metar;
SIE_VxGMMUR                                      ie_mmur;
SIE_MTRW                                         ie_mtrw[4];
UNSG8 RSVDx5C8                                    [56];
} SIE_VxGMTR;
typedef union  T32VxGMTR_Version
{ UNSG32 u32;
	struct w32VxGMTR_Version;
} T32VxGMTR_Version;
typedef union  T32VxGMTR_Build
{ UNSG32 u32;
	struct w32VxGMTR_Build;
} T32VxGMTR_Build;
typedef union  T32VxGMTR_kickoffR0
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR0;
} T32VxGMTR_kickoffR0;
typedef union  T32VxGMTR_kickoffR1
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR1;
} T32VxGMTR_kickoffR1;
typedef union  T32VxGMTR_kickoffR2
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR2;
} T32VxGMTR_kickoffR2;
typedef union  T32VxGMTR_kickoffR3
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR3;
} T32VxGMTR_kickoffR3;
typedef union  T32VxGMTR_kickoffR4
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR4;
} T32VxGMTR_kickoffR4;
typedef union  T32VxGMTR_kickoffR5
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR5;
} T32VxGMTR_kickoffR5;
typedef union  T32VxGMTR_kickoffR6
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR6;
} T32VxGMTR_kickoffR6;
typedef union  T32VxGMTR_kickoffR7
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR7;
} T32VxGMTR_kickoffR7;
typedef union  T32VxGMTR_kickoffR8
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR8;
} T32VxGMTR_kickoffR8;
typedef union  T32VxGMTR_kickoffR9
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR9;
} T32VxGMTR_kickoffR9;
typedef union  T32VxGMTR_kickoffR10
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR10;
} T32VxGMTR_kickoffR10;
typedef union  T32VxGMTR_kickoffR11
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR11;
} T32VxGMTR_kickoffR11;
typedef union  T32VxGMTR_kickoffR12
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR12;
} T32VxGMTR_kickoffR12;
typedef union  T32VxGMTR_kickoffR13
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR13;
} T32VxGMTR_kickoffR13;
typedef union  T32VxGMTR_kickoffR14
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR14;
} T32VxGMTR_kickoffR14;
typedef union  T32VxGMTR_kickoffR15
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR15;
} T32VxGMTR_kickoffR15;
typedef union  T32VxGMTR_kickoffR16
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR16;
} T32VxGMTR_kickoffR16;
typedef union  T32VxGMTR_kickoffR17
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR17;
} T32VxGMTR_kickoffR17;
typedef union  T32VxGMTR_kickoffR18
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR18;
} T32VxGMTR_kickoffR18;
typedef union  T32VxGMTR_kickoffR19
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR19;
} T32VxGMTR_kickoffR19;
typedef union  T32VxGMTR_kickoffR20
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR20;
} T32VxGMTR_kickoffR20;
typedef union  T32VxGMTR_kickoffR21
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR21;
} T32VxGMTR_kickoffR21;
typedef union  T32VxGMTR_kickoffR22
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR22;
} T32VxGMTR_kickoffR22;
typedef union  T32VxGMTR_kickoffR23
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR23;
} T32VxGMTR_kickoffR23;
typedef union  T32VxGMTR_kickoffR24
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR24;
} T32VxGMTR_kickoffR24;
typedef union  T32VxGMTR_kickoffR25
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR25;
} T32VxGMTR_kickoffR25;
typedef union  T32VxGMTR_kickoffR26
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR26;
} T32VxGMTR_kickoffR26;
typedef union  T32VxGMTR_kickoffR27
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR27;
} T32VxGMTR_kickoffR27;
typedef union  T32VxGMTR_kickoffR28
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR28;
} T32VxGMTR_kickoffR28;
typedef union  T32VxGMTR_kickoffR29
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR29;
} T32VxGMTR_kickoffR29;
typedef union  T32VxGMTR_kickoffR30
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR30;
} T32VxGMTR_kickoffR30;
typedef union  T32VxGMTR_kickoffR31
{ UNSG32 u32;
	struct w32VxGMTR_kickoffR31;
} T32VxGMTR_kickoffR31;
typedef union  T32VxGMTR_kickoffW0
{ UNSG32 u32;
	struct w32VxGMTR_kickoffW0;
} T32VxGMTR_kickoffW0;
typedef union  T32VxGMTR_kickoffW1
{ UNSG32 u32;
	struct w32VxGMTR_kickoffW1;
} T32VxGMTR_kickoffW1;
typedef union  T32VxGMTR_kickoffW2
{ UNSG32 u32;
	struct w32VxGMTR_kickoffW2;
} T32VxGMTR_kickoffW2;
typedef union  T32VxGMTR_kickoffW3
{ UNSG32 u32;
	struct w32VxGMTR_kickoffW3;
} T32VxGMTR_kickoffW3;
typedef union  T32VxGMTR_cfg
{ UNSG32 u32;
	struct w32VxGMTR_cfg;
} T32VxGMTR_cfg;
typedef union  T32VxGMTR_pfa
{ UNSG32 u32;
	struct w32VxGMTR_pfa;
} T32VxGMTR_pfa;
typedef union  TVxGMTR_Version
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_Version;
	};
} TVxGMTR_Version;
typedef union  TVxGMTR_Build
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_Build;
	};
} TVxGMTR_Build;
typedef union  TVxGMTR_kickoffR0
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR0;
	};
} TVxGMTR_kickoffR0;
typedef union  TVxGMTR_kickoffR1
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR1;
	};
} TVxGMTR_kickoffR1;
typedef union  TVxGMTR_kickoffR2
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR2;
	};
} TVxGMTR_kickoffR2;
typedef union  TVxGMTR_kickoffR3
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR3;
	};
} TVxGMTR_kickoffR3;
typedef union  TVxGMTR_kickoffR4
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR4;
	};
} TVxGMTR_kickoffR4;
typedef union  TVxGMTR_kickoffR5
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR5;
	};
} TVxGMTR_kickoffR5;
typedef union  TVxGMTR_kickoffR6
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR6;
	};
} TVxGMTR_kickoffR6;
typedef union  TVxGMTR_kickoffR7
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR7;
	};
} TVxGMTR_kickoffR7;
typedef union  TVxGMTR_kickoffR8
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR8;
	};
} TVxGMTR_kickoffR8;
typedef union  TVxGMTR_kickoffR9
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR9;
	};
} TVxGMTR_kickoffR9;
typedef union  TVxGMTR_kickoffR10
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR10;
	};
} TVxGMTR_kickoffR10;
typedef union  TVxGMTR_kickoffR11
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR11;
	};
} TVxGMTR_kickoffR11;
typedef union  TVxGMTR_kickoffR12
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR12;
	};
} TVxGMTR_kickoffR12;
typedef union  TVxGMTR_kickoffR13
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR13;
	};
} TVxGMTR_kickoffR13;
typedef union  TVxGMTR_kickoffR14
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR14;
	};
} TVxGMTR_kickoffR14;
typedef union  TVxGMTR_kickoffR15
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR15;
	};
} TVxGMTR_kickoffR15;
typedef union  TVxGMTR_kickoffR16
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR16;
	};
} TVxGMTR_kickoffR16;
typedef union  TVxGMTR_kickoffR17
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR17;
	};
} TVxGMTR_kickoffR17;
typedef union  TVxGMTR_kickoffR18
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR18;
	};
} TVxGMTR_kickoffR18;
typedef union  TVxGMTR_kickoffR19
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR19;
	};
} TVxGMTR_kickoffR19;
typedef union  TVxGMTR_kickoffR20
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR20;
	};
} TVxGMTR_kickoffR20;
typedef union  TVxGMTR_kickoffR21
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR21;
	};
} TVxGMTR_kickoffR21;
typedef union  TVxGMTR_kickoffR22
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR22;
	};
} TVxGMTR_kickoffR22;
typedef union  TVxGMTR_kickoffR23
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR23;
	};
} TVxGMTR_kickoffR23;
typedef union  TVxGMTR_kickoffR24
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR24;
	};
} TVxGMTR_kickoffR24;
typedef union  TVxGMTR_kickoffR25
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR25;
	};
} TVxGMTR_kickoffR25;
typedef union  TVxGMTR_kickoffR26
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR26;
	};
} TVxGMTR_kickoffR26;
typedef union  TVxGMTR_kickoffR27
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR27;
	};
} TVxGMTR_kickoffR27;
typedef union  TVxGMTR_kickoffR28
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR28;
	};
} TVxGMTR_kickoffR28;
typedef union  TVxGMTR_kickoffR29
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR29;
	};
} TVxGMTR_kickoffR29;
typedef union  TVxGMTR_kickoffR30
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR30;
	};
} TVxGMTR_kickoffR30;
typedef union  TVxGMTR_kickoffR31
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffR31;
	};
} TVxGMTR_kickoffR31;
typedef union  TVxGMTR_kickoffW0
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffW0;
	};
} TVxGMTR_kickoffW0;
typedef union  TVxGMTR_kickoffW1
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffW1;
	};
} TVxGMTR_kickoffW1;
typedef union  TVxGMTR_kickoffW2
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffW2;
	};
} TVxGMTR_kickoffW2;
typedef union  TVxGMTR_kickoffW3
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_kickoffW3;
	};
} TVxGMTR_kickoffW3;
typedef union  TVxGMTR_cfg
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_cfg;
	};
} TVxGMTR_cfg;
typedef union  TVxGMTR_pfa
{ UNSG32 u32[1];
	struct {
		struct w32VxGMTR_pfa;
	};
} TVxGMTR_pfa;
SIGN32 VxGMTR_drvrd(SIE_VxGMTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 VxGMTR_drvwr(SIE_VxGMTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void VxGMTR_reset(SIE_VxGMTR *p);
SIGN32 VxGMTR_cmp  (SIE_VxGMTR *p, SIE_VxGMTR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define VxGMTR_check(p,pie,pfx,hLOG) VxGMTR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define VxGMTR_print(p,    pfx,hLOG) VxGMTR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MTR
#define h_MTR (){}
#define     RA_MTR_cfg                                     0x0000
#define        MTR_cfg_mmu_pageSz_4K                                    0x0
#define        MTR_cfg_mmu_pageSz_16K                                   0x1
#define        MTR_cfg_mmu_pageSz_64K                                   0x2
#define        MTR_cfg_mmu_pageSz_256K                                  0x3
#define        MTR_cfg_mmu_pageSz_1M                                    0x4
#define        MTR_cfg_mmu_pageSz_2M                                    0x5
#define     RA_MTR_MTRR                                    0x0004
#define     RA_MTR_mtrr                                    0x0004
#define     RA_MTR_MTRW                                    0x0384
#define     RA_MTR_mtrw                                    0x0384
#define     RA_MTR_offset                                  0x0544
typedef struct SIE_MTR {
#define     w32MTR_cfg                                     {\
	UNSG32 ucfg_mtrr_flow_ctrl                         :  1;\
	UNSG32 ucfg_mtrr_qos_en                            :  1;\
	UNSG32 ucfg_mtrr_qos                               :  4;\
	UNSG32 ucfg_mtrr_dis_mtid                          :  1;\
	UNSG32 ucfg_mmu_pageSz                             :  3;\
	UNSG32 ucfg_mmu_qos_en                             :  1;\
	UNSG32 ucfg_mmu_qos                                :  4;\
	UNSG32 ucfg_mmu_banks                              :  1;\
	UNSG32 ucfg_mmu_sysSz                              :  2;\
	UNSG32 ucfg_mmu_tarSz                              :  2;\
	UNSG32 ucfg_Reserved                               :  1;\
	UNSG32 RSVDx0_b21                                  : 11;\
}
union { UNSG32 u32MTR_cfg;
	struct w32MTR_cfg;
};
SIE_MTRR                                         ie_mtrr[32];
SIE_MTRW                                         ie_mtrw[16];
SIE_VxGMETA_OFST                                 ie_offset[16];
} SIE_MTR;
typedef union  T32MTR_cfg
{ UNSG32 u32;
	struct w32MTR_cfg;
} T32MTR_cfg;
typedef union  TMTR_cfg
{ UNSG32 u32[1];
	struct {
		struct w32MTR_cfg;
	};
} TMTR_cfg;
SIGN32 MTR_drvrd(SIE_MTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MTR_drvwr(SIE_MTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MTR_reset(SIE_MTR *p);
SIGN32 MTR_cmp  (SIE_MTR *p, SIE_MTR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MTR_check(p,pie,pfx,hLOG) MTR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MTR_print(p,    pfx,hLOG) MTR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
}
#endif
#pragma  pack()
#endif
