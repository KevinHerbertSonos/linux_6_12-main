// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 */
#ifndef lcdc_h
#define lcdc_h (){}
#include "ctypes.h"
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
    #define _BFSET_(r,msb,lsb,v)                               do{ (r)&=_NCLRMASK_(msb,lsb); (r)|=_NSETMASK_(msb,lsb)&((v)<<(lsb)); }while(0)
#endif
#ifndef h_BITMAP32
#define h_BITMAP32 (){}
    #define     RA_BITMAP32_SEL                                0x0000
    #define     RA_BITMAP32_SEL1                               0x0004
    #define     RA_BITMAP32_SEL2                               0x0008
    #define     RA_BITMAP32_SEL3                               0x000C
    #define     RA_BITMAP32_SEL4                               0x0010
    #define     RA_BITMAP32_SEL5                               0x0014
    typedef struct SIE_BITMAP32 {
    #define     w32BITMAP32_SEL                                {\
            UNSG32 uSEL_BIT_POS0                               :  5;\
            UNSG32 uSEL_BIT_POS1                               :  5;\
            UNSG32 uSEL_BIT_POS2                               :  5;\
            UNSG32 uSEL_BIT_POS3                               :  5;\
            UNSG32 uSEL_BIT_POS4                               :  5;\
            UNSG32 uSEL_BIT_POS5                               :  5;\
            UNSG32 RSVDx0_b30                                  :  2;\
          }
    union { UNSG32 u32BITMAP32_SEL;
            struct w32BITMAP32_SEL;
          };
    #define     w32BITMAP32_SEL1                               {\
            UNSG32 uSEL_BIT_POS6                               :  5;\
            UNSG32 uSEL_BIT_POS7                               :  5;\
            UNSG32 uSEL_BIT_POS8                               :  5;\
            UNSG32 uSEL_BIT_POS9                               :  5;\
            UNSG32 uSEL_BIT_POS10                              :  5;\
            UNSG32 uSEL_BIT_POS11                              :  5;\
            UNSG32 RSVDx4_b30                                  :  2;\
          }
    union { UNSG32 u32BITMAP32_SEL1;
            struct w32BITMAP32_SEL1;
          };
    #define     w32BITMAP32_SEL2                               {\
            UNSG32 uSEL_BIT_POS12                              :  5;\
            UNSG32 uSEL_BIT_POS13                              :  5;\
            UNSG32 uSEL_BIT_POS14                              :  5;\
            UNSG32 uSEL_BIT_POS15                              :  5;\
            UNSG32 uSEL_BIT_POS16                              :  5;\
            UNSG32 uSEL_BIT_POS17                              :  5;\
            UNSG32 RSVDx8_b30                                  :  2;\
          }
    union { UNSG32 u32BITMAP32_SEL2;
            struct w32BITMAP32_SEL2;
          };
    #define     w32BITMAP32_SEL3                               {\
            UNSG32 uSEL_BIT_POS18                              :  5;\
            UNSG32 uSEL_BIT_POS19                              :  5;\
            UNSG32 uSEL_BIT_POS20                              :  5;\
            UNSG32 uSEL_BIT_POS21                              :  5;\
            UNSG32 uSEL_BIT_POS22                              :  5;\
            UNSG32 uSEL_BIT_POS23                              :  5;\
            UNSG32 RSVDxC_b30                                  :  2;\
          }
    union { UNSG32 u32BITMAP32_SEL3;
            struct w32BITMAP32_SEL3;
          };
    #define     w32BITMAP32_SEL4                               {\
            UNSG32 uSEL_BIT_POS24                              :  5;\
            UNSG32 uSEL_BIT_POS25                              :  5;\
            UNSG32 uSEL_BIT_POS26                              :  5;\
            UNSG32 uSEL_BIT_POS27                              :  5;\
            UNSG32 uSEL_BIT_POS28                              :  5;\
            UNSG32 uSEL_BIT_POS29                              :  5;\
            UNSG32 RSVDx10_b30                                 :  2;\
          }
    union { UNSG32 u32BITMAP32_SEL4;
            struct w32BITMAP32_SEL4;
          };
    #define     w32BITMAP32_SEL5                               {\
            UNSG32 uSEL_BIT_POS30                              :  5;\
            UNSG32 uSEL_BIT_POS31                              :  5;\
            UNSG32 RSVDx14_b10                                 : 22;\
          }
    union { UNSG32 u32BITMAP32_SEL5;
            struct w32BITMAP32_SEL5;
          };
    } SIE_BITMAP32;
    typedef union  T32BITMAP32_SEL
          { UNSG32 u32;
            struct w32BITMAP32_SEL;
                 } T32BITMAP32_SEL;
    typedef union  T32BITMAP32_SEL1
          { UNSG32 u32;
            struct w32BITMAP32_SEL1;
                 } T32BITMAP32_SEL1;
    typedef union  T32BITMAP32_SEL2
          { UNSG32 u32;
            struct w32BITMAP32_SEL2;
                 } T32BITMAP32_SEL2;
    typedef union  T32BITMAP32_SEL3
          { UNSG32 u32;
            struct w32BITMAP32_SEL3;
                 } T32BITMAP32_SEL3;
    typedef union  T32BITMAP32_SEL4
          { UNSG32 u32;
            struct w32BITMAP32_SEL4;
                 } T32BITMAP32_SEL4;
    typedef union  T32BITMAP32_SEL5
          { UNSG32 u32;
            struct w32BITMAP32_SEL5;
                 } T32BITMAP32_SEL5;
    typedef union  TBITMAP32_SEL
          { UNSG32 u32[6];
            struct {
            struct w32BITMAP32_SEL;
            struct w32BITMAP32_SEL1;
            struct w32BITMAP32_SEL2;
            struct w32BITMAP32_SEL3;
            struct w32BITMAP32_SEL4;
            struct w32BITMAP32_SEL5;
                   };
                 } TBITMAP32_SEL;
     SIGN32 BITMAP32_drvrd(SIE_BITMAP32 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 BITMAP32_drvwr(SIE_BITMAP32 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void BITMAP32_reset(SIE_BITMAP32 *p);
     SIGN32 BITMAP32_cmp  (SIE_BITMAP32 *p, SIE_BITMAP32 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define BITMAP32_check(p,pie,pfx,hLOG) BITMAP32_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define BITMAP32_print(p,    pfx,hLOG) BITMAP32_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_oneReg
#define h_oneReg (){}
    typedef struct SIE_oneReg {
            UNSG32 u_0x00000000                                : 32;
    } SIE_oneReg;
     SIGN32 oneReg_drvrd(SIE_oneReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 oneReg_drvwr(SIE_oneReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void oneReg_reset(SIE_oneReg *p);
     SIGN32 oneReg_cmp  (SIE_oneReg *p, SIE_oneReg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define oneReg_check(p,pie,pfx,hLOG) oneReg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define oneReg_print(p,    pfx,hLOG) oneReg_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_LCDC_REG
#define h_LCDC_REG (){}
    #define     RA_LCDC_REG_dummy                              0x0000
    typedef struct SIE_LCDC_REG {
              SIE_oneReg                                       ie_dummy[6143];
    } SIE_LCDC_REG;
     SIGN32 LCDC_REG_drvrd(SIE_LCDC_REG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 LCDC_REG_drvwr(SIE_LCDC_REG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void LCDC_REG_reset(SIE_LCDC_REG *p);
     SIGN32 LCDC_REG_cmp  (SIE_LCDC_REG *p, SIE_LCDC_REG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define LCDC_REG_check(p,pie,pfx,hLOG) LCDC_REG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define LCDC_REG_print(p,    pfx,hLOG) LCDC_REG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_LCDC
#define h_LCDC (){}
    #define     RA_LCDC_cfgReg                                 0x0000
    #define     RA_LCDC_CTRL1                                  0x8000
    #define     RA_LCDC_CTRL2                                  0x8004
    #define     RA_LCDC_CTRL3                                  0x8008
    #define     RA_LCDC_CTRL4                                  0x800C
    #define     RA_LCDC_CTRL5                                  0x8010
    #define     RA_LCDC_CTRL6                                  0x8014
    #define     RA_LCDC_CTRL7                                  0x8018
    #define     RA_LCDC_STATUS0                                0x801C
    #define     RA_LCDC_STATUS1                                0x8020
    #define     RA_LCDC_bitmap32_ctrl                          0x8024
    #define     RA_LCDC_dummy1                                 0x803C
    typedef struct SIE_LCDC {
              SIE_LCDC_REG                                     ie_cfgReg;
             UNSG8 RSVD_cfgReg                                 [8196];
    #define     w32LCDC_CTRL1                                  {\
            UNSG32 uCTRL1_wordTot_R0                           : 32;\
          }
    union { UNSG32 u32LCDC_CTRL1;
            struct w32LCDC_CTRL1;
          };
    #define     w32LCDC_CTRL2                                  {\
            UNSG32 uCTRL2_packSel_R0                           :  3;\
            UNSG32 uCTRL2_byteoffsetRGB                        :  2;\
            UNSG32 uCTRL2_nonStdResEn_R0                       :  1;\
            UNSG32 uCTRL2_flushCnt_R0                          :  4;\
            UNSG32 uCTRL2_pixlineTot_R0                        : 13;\
            UNSG32 RSVDx8004_b23                               :  9;\
          }
    union { UNSG32 u32LCDC_CTRL2;
            struct w32LCDC_CTRL2;
          };
    #define     w32LCDC_CTRL3                                  {\
            UNSG32 uCTRL3_ClientR0_start                       :  1;\
            UNSG32 uCTRL3_ClientR0_clear                       :  1;\
            UNSG32 RSVDx8008_b2                                : 30;\
          }
    union { UNSG32 u32LCDC_CTRL3;
            struct w32LCDC_CTRL3;
          };
    #define     w32LCDC_CTRL4                                  {\
            UNSG32 uCTRL4_bitmap32_en                          :  1;\
            UNSG32 uCTRL4_bitmap128_en                         :  1;\
            UNSG32 uCTRL4_en_syncinit2_wait                    :  1;\
            UNSG32 RSVDx800C_b3                                : 29;\
          }
    union { UNSG32 u32LCDC_CTRL4;
            struct w32LCDC_CTRL4;
          };
    #define     w32LCDC_CTRL5                                  {\
            UNSG32 uCTRL5_contiguous16                         :  1;\
            UNSG32 uCTRL5_b16as18                              :  1;\
            UNSG32 uCTRL5_cpu_mode18                           :  1;\
            UNSG32 uCTRL5_cpu_cmd_shift1                       :  1;\
            UNSG32 uCTRL5_cpu_mode24                           :  1;\
            UNSG32 uCTRL5_cpu_mux_exp_en                       :  1;\
            UNSG32 uCTRL5_cpu_mode16as18                       :  1;\
            UNSG32 uCTRL5_cpu_mode16as24                       :  1;\
            UNSG32 uCTRL5_oen_inv                              :  1;\
            UNSG32 uCTRL5_en_req_DE                            :  1;\
            UNSG32 uCTRL5_halt_en_front                        :  1;\
            UNSG32 uCTRL5_halt_en_back                         :  1;\
            UNSG32 uCTRL5_full_level_14_en                     :  1;\
            UNSG32 uCTRL5_bg_delay_ctrl_en                     :  1;\
            UNSG32 RSVDx8010_b14                               : 18;\
          }
    union { UNSG32 u32LCDC_CTRL5;
            struct w32LCDC_CTRL5;
          };
    #define     w32LCDC_CTRL6                                  {\
            UNSG32 uCTRL6_frst_halt_gen_delay                  : 11;\
            UNSG32 uCTRL6_debug_out_sel                        :  5;\
            UNSG32 RSVDx8014_b16                               : 16;\
          }
    union { UNSG32 u32LCDC_CTRL6;
            struct w32LCDC_CTRL6;
          };
    #define     w32LCDC_CTRL7                                  {\
            UNSG32 uCTRL7_oen_reg                              : 24;\
            UNSG32 uCTRL7_oen_reg_en                           :  1;\
            UNSG32 uCTRL7_data_inv_en                          :  1;\
            UNSG32 RSVDx8018_b26                               :  6;\
          }
    union { UNSG32 u32LCDC_CTRL7;
            struct w32LCDC_CTRL7;
          };
    #define     w32LCDC_STATUS0                                {\
            UNSG32 uSTATUS0_TGVCNT                             : 11;\
            UNSG32 uSTATUS0_SHUFFVCNT                          : 11;\
            UNSG32 RSVDx801C_b22                               : 10;\
          }
    union { UNSG32 u32LCDC_STATUS0;
            struct w32LCDC_STATUS0;
          };
    #define     w32LCDC_STATUS1                                {\
            UNSG32 uSTATUS1_GPVCNT                             : 11;\
            UNSG32 RSVDx8020_b11                               : 21;\
          }
    union { UNSG32 u32LCDC_STATUS1;
            struct w32LCDC_STATUS1;
          };
              SIE_BITMAP32                                     ie_bitmap32_ctrl;
    #define     w32LCDC_dummy1                                 {\
            UNSG32 udummy1_STS                                 : 32;\
          }
    union { UNSG32 u32LCDC_dummy1;
            struct w32LCDC_dummy1;
          };
             UNSG8 RSVDx8040                                   [32704];
    } SIE_LCDC;
    typedef union  T32LCDC_CTRL1
          { UNSG32 u32;
            struct w32LCDC_CTRL1;
                 } T32LCDC_CTRL1;
    typedef union  T32LCDC_CTRL2
          { UNSG32 u32;
            struct w32LCDC_CTRL2;
                 } T32LCDC_CTRL2;
    typedef union  T32LCDC_CTRL3
          { UNSG32 u32;
            struct w32LCDC_CTRL3;
                 } T32LCDC_CTRL3;
    typedef union  T32LCDC_CTRL4
          { UNSG32 u32;
            struct w32LCDC_CTRL4;
                 } T32LCDC_CTRL4;
    typedef union  T32LCDC_CTRL5
          { UNSG32 u32;
            struct w32LCDC_CTRL5;
                 } T32LCDC_CTRL5;
    typedef union  T32LCDC_CTRL6
          { UNSG32 u32;
            struct w32LCDC_CTRL6;
                 } T32LCDC_CTRL6;
    typedef union  T32LCDC_CTRL7
          { UNSG32 u32;
            struct w32LCDC_CTRL7;
                 } T32LCDC_CTRL7;
    typedef union  T32LCDC_STATUS0
          { UNSG32 u32;
            struct w32LCDC_STATUS0;
                 } T32LCDC_STATUS0;
    typedef union  T32LCDC_STATUS1
          { UNSG32 u32;
            struct w32LCDC_STATUS1;
                 } T32LCDC_STATUS1;
    typedef union  T32LCDC_dummy1
          { UNSG32 u32;
            struct w32LCDC_dummy1;
                 } T32LCDC_dummy1;
    typedef union  TLCDC_CTRL1
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_CTRL1;
                   };
                 } TLCDC_CTRL1;
    typedef union  TLCDC_CTRL2
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_CTRL2;
                   };
                 } TLCDC_CTRL2;
    typedef union  TLCDC_CTRL3
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_CTRL3;
                   };
                 } TLCDC_CTRL3;
    typedef union  TLCDC_CTRL4
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_CTRL4;
                   };
                 } TLCDC_CTRL4;
    typedef union  TLCDC_CTRL5
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_CTRL5;
                   };
                 } TLCDC_CTRL5;
    typedef union  TLCDC_CTRL6
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_CTRL6;
                   };
                 } TLCDC_CTRL6;
    typedef union  TLCDC_CTRL7
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_CTRL7;
                   };
                 } TLCDC_CTRL7;
    typedef union  TLCDC_STATUS0
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_STATUS0;
                   };
                 } TLCDC_STATUS0;
    typedef union  TLCDC_STATUS1
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_STATUS1;
                   };
                 } TLCDC_STATUS1;
    typedef union  TLCDC_dummy1
          { UNSG32 u32[1];
            struct {
            struct w32LCDC_dummy1;
                   };
                 } TLCDC_dummy1;
     SIGN32 LCDC_drvrd(SIE_LCDC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 LCDC_drvwr(SIE_LCDC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void LCDC_reset(SIE_LCDC *p);
     SIGN32 LCDC_cmp  (SIE_LCDC *p, SIE_LCDC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define LCDC_check(p,pie,pfx,hLOG) LCDC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define LCDC_print(p,    pfx,hLOG) LCDC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
