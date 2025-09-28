// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 */
#ifndef avioGbl_h
#define avioGbl_h (){}
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
#ifndef h_abipll
#define h_abipll (){}
    #define     RA_abipll_ctrlA                                0x0000
    #define     RA_abipll_ctrlB                                0x0004
    #define     RA_abipll_ctrlC                                0x0008
    #define     RA_abipll_ctrlD                                0x000C
    #define     RA_abipll_ctrlE                                0x0010
    #define     RA_abipll_ctrlF                                0x0014
    #define     RA_abipll_ctrlG                                0x0018
    #define     RA_abipll_status                               0x001C
    typedef struct SIE_abipll {
    #define     w32abipll_ctrlA                                {\
            UNSG32 uctrlA_RESET                                :  1;\
            UNSG32 uctrlA_BYPASS                               :  1;\
            UNSG32 uctrlA_NEWDIV                               :  1;\
            UNSG32 uctrlA_RANGE                                :  3;\
            UNSG32 RSVDx0_b6                                   : 26;\
          }
    union { UNSG32 u32abipll_ctrlA;
            struct w32abipll_ctrlA;
          };
    #define     w32abipll_ctrlB                                {\
            UNSG32 uctrlB_SSMF                                 :  4;\
            UNSG32 uctrlB_SSMD                                 :  3;\
            UNSG32 uctrlB_SSE_RSVD                             :  1;\
            UNSG32 uctrlB_SSE                                  :  1;\
            UNSG32 uctrlB_SSDS                                 :  1;\
            UNSG32 RSVDx4_b10                                  : 22;\
          }
    union { UNSG32 u32abipll_ctrlB;
            struct w32abipll_ctrlB;
          };
    #define     w32abipll_ctrlC                                {\
            UNSG32 uctrlC_DIVR                                 :  6;\
            UNSG32 RSVDx8_b6                                   : 26;\
          }
    union { UNSG32 u32abipll_ctrlC;
            struct w32abipll_ctrlC;
          };
    #define     w32abipll_ctrlD                                {\
            UNSG32 uctrlD_DIVFI                                :  9;\
            UNSG32 RSVDxC_b9                                   : 23;\
          }
    union { UNSG32 u32abipll_ctrlD;
            struct w32abipll_ctrlD;
          };
    #define     w32abipll_ctrlE                                {\
            UNSG32 uctrlE_DIVFF                                : 24;\
            UNSG32 RSVDx10_b24                                 :  8;\
          }
    union { UNSG32 u32abipll_ctrlE;
            struct w32abipll_ctrlE;
          };
    #define     w32abipll_ctrlF                                {\
            UNSG32 uctrlF_DIVQ                                 :  5;\
            UNSG32 RSVDx14_b5                                  : 27;\
          }
    union { UNSG32 u32abipll_ctrlF;
            struct w32abipll_ctrlF;
          };
    #define     w32abipll_ctrlG                                {\
            UNSG32 uctrlG_DIVQF                                :  3;\
            UNSG32 RSVDx18_b3                                  : 29;\
          }
    union { UNSG32 u32abipll_ctrlG;
            struct w32abipll_ctrlG;
          };
    #define     w32abipll_status                               {\
            UNSG32 ustatus_LOCK                                :  1;\
            UNSG32 ustatus_DIVACK                              :  1;\
            UNSG32 RSVDx1C_b2                                  : 30;\
          }
    union { UNSG32 u32abipll_status;
            struct w32abipll_status;
          };
    } SIE_abipll;
    typedef union  T32abipll_ctrlA
          { UNSG32 u32;
            struct w32abipll_ctrlA;
                 } T32abipll_ctrlA;
    typedef union  T32abipll_ctrlB
          { UNSG32 u32;
            struct w32abipll_ctrlB;
                 } T32abipll_ctrlB;
    typedef union  T32abipll_ctrlC
          { UNSG32 u32;
            struct w32abipll_ctrlC;
                 } T32abipll_ctrlC;
    typedef union  T32abipll_ctrlD
          { UNSG32 u32;
            struct w32abipll_ctrlD;
                 } T32abipll_ctrlD;
    typedef union  T32abipll_ctrlE
          { UNSG32 u32;
            struct w32abipll_ctrlE;
                 } T32abipll_ctrlE;
    typedef union  T32abipll_ctrlF
          { UNSG32 u32;
            struct w32abipll_ctrlF;
                 } T32abipll_ctrlF;
    typedef union  T32abipll_ctrlG
          { UNSG32 u32;
            struct w32abipll_ctrlG;
                 } T32abipll_ctrlG;
    typedef union  T32abipll_status
          { UNSG32 u32;
            struct w32abipll_status;
                 } T32abipll_status;
    typedef union  Tabipll_ctrlA
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlA;
                   };
                 } Tabipll_ctrlA;
    typedef union  Tabipll_ctrlB
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlB;
                   };
                 } Tabipll_ctrlB;
    typedef union  Tabipll_ctrlC
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlC;
                   };
                 } Tabipll_ctrlC;
    typedef union  Tabipll_ctrlD
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlD;
                   };
                 } Tabipll_ctrlD;
    typedef union  Tabipll_ctrlE
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlE;
                   };
                 } Tabipll_ctrlE;
    typedef union  Tabipll_ctrlF
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlF;
                   };
                 } Tabipll_ctrlF;
    typedef union  Tabipll_ctrlG
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlG;
                   };
                 } Tabipll_ctrlG;
    typedef union  Tabipll_status
          { UNSG32 u32[1];
            struct {
            struct w32abipll_status;
                   };
                 } Tabipll_status;
     SIGN32 abipll_drvrd(SIE_abipll *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 abipll_drvwr(SIE_abipll *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void abipll_reset(SIE_abipll *p);
     SIGN32 abipll_cmp  (SIE_abipll *p, SIE_abipll *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define abipll_check(p,pie,pfx,hLOG) abipll_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define abipll_print(p,    pfx,hLOG) abipll_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DPHYTX
#define h_DPHYTX (){}
    #define     RA_DPHYTX_DPHY_CTL0                            0x0000
    #define     RA_DPHYTX_DPHY_CTL1                            0x0004
    #define     RA_DPHYTX_DPHY_CTL2                            0x0008
    #define     RA_DPHYTX_DPHY_CTL3                            0x000C
    #define     RA_DPHYTX_DPHY_CTL4                            0x0010
    #define     RA_DPHYTX_DPHY_CTL5                            0x0014
    #define     RA_DPHYTX_DPHY_CTL6                            0x0018
    #define     RA_DPHYTX_DPHY_CTL7                            0x001C
    #define     RA_DPHYTX_DPHY_CTL8                            0x0020
    #define     RA_DPHYTX_DPHY_RB0                             0x0024
    #define     RA_DPHYTX_DPHY_RB1                             0x0028
    #define     RA_DPHYTX_DPHY_RB2                             0x002C
    #define     RA_DPHYTX_DPHY_RB3                             0x0030
    #define     RA_DPHYTX_DPHY_PLL0                            0x0034
    #define     RA_DPHYTX_DPHY_PLL1                            0x0038
    #define     RA_DPHYTX_DPHY_PLL2                            0x003C
    #define     RA_DPHYTX_DPHY_PLLRB0                          0x0040
    #define     RA_DPHYTX_DPHY_PLLRB1                          0x0044
    typedef struct SIE_DPHYTX {
    #define     w32DPHYTX_DPHY_CTL0                            {\
            UNSG32 uDPHY_CTL0_BiuCtrlPhyEn                     :  1;\
            UNSG32 RSVDx0_b1                                   : 31;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL0;
            struct w32DPHYTX_DPHY_CTL0;
          };
    #define     w32DPHYTX_DPHY_CTL1                            {\
            UNSG32 uDPHY_CTL1_shutdownz                        :  1;\
            UNSG32 uDPHY_CTL1_rstz                             :  1;\
            UNSG32 uDPHY_CTL1_forcepll                         :  1;\
            UNSG32 uDPHY_CTL1_txrequesthsclk                   :  1;\
            UNSG32 uDPHY_CTL1_enableclkBIU                     :  1;\
            UNSG32 uDPHY_CTL1_turndisable_0                    :  1;\
            UNSG32 uDPHY_CTL1_forcerxmode_0                    :  1;\
            UNSG32 uDPHY_CTL1_basedir_0                        :  1;\
            UNSG32 uDPHY_CTL1_forcetxstopmode_0                :  1;\
            UNSG32 uDPHY_CTL1_forcetxstopmode_1                :  1;\
            UNSG32 uDPHY_CTL1_forcetxstopmode_2                :  1;\
            UNSG32 uDPHY_CTL1_forcetxstopmode_3                :  1;\
            UNSG32 uDPHY_CTL1_enable_0                         :  1;\
            UNSG32 uDPHY_CTL1_enable_1                         :  1;\
            UNSG32 uDPHY_CTL1_enable_2                         :  1;\
            UNSG32 uDPHY_CTL1_enable_3                         :  1;\
            UNSG32 uDPHY_CTL1_hsfreqrange                      :  7;\
            UNSG32 uDPHY_CTL1_cont_en                          :  1;\
            UNSG32 uDPHY_CTL1_cfgclkfreqrange                  :  6;\
            UNSG32 uDPHY_CTL1_biston                           :  1;\
            UNSG32 RSVDx4_b31                                  :  1;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL1;
            struct w32DPHYTX_DPHY_CTL1;
          };
    #define     w32DPHYTX_DPHY_CTL2                            {\
            UNSG32 uDPHY_CTL2_txulpsclkBIU                     :  1;\
            UNSG32 uDPHY_CTL2_txulpsexitclk                    :  1;\
            UNSG32 uDPHY_CTL2_turnrequest_0                    :  1;\
            UNSG32 RSVDx8_b3                                   : 29;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL2;
            struct w32DPHYTX_DPHY_CTL2;
          };
    #define     w32DPHYTX_DPHY_CTL3                            {\
            UNSG32 uDPHY_CTL3_txdataesc_0                      :  8;\
            UNSG32 uDPHY_CTL3_txtriggeresc_0                   :  4;\
            UNSG32 uDPHY_CTL3_txrequestesc_0                   :  1;\
            UNSG32 uDPHY_CTL3_txlpdtesc_0                      :  1;\
            UNSG32 uDPHY_CTL3_txulpsesc_0                      :  1;\
            UNSG32 uDPHY_CTL3_txulpsexit_0                     :  1;\
            UNSG32 uDPHY_CTL3_txvalidesc_0                     :  1;\
            UNSG32 RSVDxC_b17                                  : 15;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL3;
            struct w32DPHYTX_DPHY_CTL3;
          };
    #define     w32DPHYTX_DPHY_CTL4                            {\
            UNSG32 uDPHY_CTL4_txdataesc_1                      :  8;\
            UNSG32 uDPHY_CTL4_txtriggeresc_1                   :  4;\
            UNSG32 uDPHY_CTL4_txrequestesc_1                   :  1;\
            UNSG32 uDPHY_CTL4_txlpdtesc_1                      :  1;\
            UNSG32 uDPHY_CTL4_txulpsesc_1                      :  1;\
            UNSG32 uDPHY_CTL4_txulpsexit_1                     :  1;\
            UNSG32 uDPHY_CTL4_txvalidesc_1                     :  1;\
            UNSG32 RSVDx10_b17                                 : 15;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL4;
            struct w32DPHYTX_DPHY_CTL4;
          };
    #define     w32DPHYTX_DPHY_CTL5                            {\
            UNSG32 uDPHY_CTL5_txdataesc_2                      :  8;\
            UNSG32 uDPHY_CTL5_txtriggeresc_2                   :  4;\
            UNSG32 uDPHY_CTL5_txrequestesc_2                   :  1;\
            UNSG32 uDPHY_CTL5_txlpdtesc_2                      :  1;\
            UNSG32 uDPHY_CTL5_txulpsesc_2                      :  1;\
            UNSG32 uDPHY_CTL5_txulpsexit_2                     :  1;\
            UNSG32 uDPHY_CTL5_txvalidesc_2                     :  1;\
            UNSG32 RSVDx14_b17                                 : 15;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL5;
            struct w32DPHYTX_DPHY_CTL5;
          };
    #define     w32DPHYTX_DPHY_CTL6                            {\
            UNSG32 uDPHY_CTL6_txdataesc_3                      :  8;\
            UNSG32 uDPHY_CTL6_txtriggeresc_3                   :  4;\
            UNSG32 uDPHY_CTL6_txrequestesc_3                   :  1;\
            UNSG32 uDPHY_CTL6_txlpdtesc_3                      :  1;\
            UNSG32 uDPHY_CTL6_txulpsesc_3                      :  1;\
            UNSG32 uDPHY_CTL6_txulpsexit_3                     :  1;\
            UNSG32 uDPHY_CTL6_txvalidesc_3                     :  1;\
            UNSG32 RSVDx18_b17                                 : 15;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL6;
            struct w32DPHYTX_DPHY_CTL6;
          };
    #define     w32DPHYTX_DPHY_CTL7                            {\
            UNSG32 uDPHY_CTL7_txblaneclkSel                    :  1;\
            UNSG32 uDPHY_CTL7_txskewcalhsBIU                   :  1;\
            UNSG32 uDPHY_CTL7_txrequestdatahs_0                :  1;\
            UNSG32 uDPHY_CTL7_txrequestdatahs_1                :  1;\
            UNSG32 uDPHY_CTL7_txrequestdatahs_2                :  1;\
            UNSG32 uDPHY_CTL7_txrequestdatahs_3                :  1;\
            UNSG32 RSVDx1C_b6                                  : 26;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL7;
            struct w32DPHYTX_DPHY_CTL7;
          };
    #define     w32DPHYTX_DPHY_CTL8                            {\
            UNSG32 uDPHY_CTL8_txdatahs_0                       :  8;\
            UNSG32 uDPHY_CTL8_txdatahs_1                       :  8;\
            UNSG32 uDPHY_CTL8_txdatahs_2                       :  8;\
            UNSG32 uDPHY_CTL8_txdatahs_3                       :  8;\
          }
    union { UNSG32 u32DPHYTX_DPHY_CTL8;
            struct w32DPHYTX_DPHY_CTL8;
          };
    #define     w32DPHYTX_DPHY_RB0                             {\
            UNSG32 uDPHY_RB0_cont_data                         : 11;\
            UNSG32 uDPHY_RB0_lock                              :  1;\
            UNSG32 uDPHY_RB0_stopstateclk                      :  1;\
            UNSG32 uDPHY_RB0_ulpsactivenotclk                  :  1;\
            UNSG32 uDPHY_RB0_stopstatedata_0                   :  1;\
            UNSG32 uDPHY_RB0_stopstatedata_1                   :  1;\
            UNSG32 uDPHY_RB0_stopstatedata_2                   :  1;\
            UNSG32 uDPHY_RB0_stopstatedata_3                   :  1;\
            UNSG32 uDPHY_RB0_ulpsactivenot_0                   :  1;\
            UNSG32 uDPHY_RB0_ulpsactivenot_1                   :  1;\
            UNSG32 uDPHY_RB0_ulpsactivenot_2                   :  1;\
            UNSG32 uDPHY_RB0_ulpsactivenot_3                   :  1;\
            UNSG32 RSVDx24_b22                                 : 10;\
          }
    union { UNSG32 u32DPHYTX_DPHY_RB0;
            struct w32DPHYTX_DPHY_RB0;
          };
    #define     w32DPHYTX_DPHY_RB1                             {\
            UNSG32 uDPHY_RB1_txreadyhs_0                       :  1;\
            UNSG32 uDPHY_RB1_txreadyhs_1                       :  1;\
            UNSG32 uDPHY_RB1_txreadyhs_2                       :  1;\
            UNSG32 uDPHY_RB1_txreadyhs_3                       :  1;\
            UNSG32 RSVDx28_b4                                  : 28;\
          }
    union { UNSG32 u32DPHYTX_DPHY_RB1;
            struct w32DPHYTX_DPHY_RB1;
          };
    #define     w32DPHYTX_DPHY_RB2                             {\
            UNSG32 uDPHY_RB2_txreadyesc_0                      :  1;\
            UNSG32 uDPHY_RB2_txreadyesc_1                      :  1;\
            UNSG32 uDPHY_RB2_txreadyesc_2                      :  1;\
            UNSG32 uDPHY_RB2_txreadyesc_3                      :  1;\
            UNSG32 uDPHY_RB2_errcontrol_0                      :  1;\
            UNSG32 uDPHY_RB2_errcontentionlp0_0                :  1;\
            UNSG32 uDPHY_RB2_errcontentionlp1_0                :  1;\
            UNSG32 RSVDx2C_b7                                  : 25;\
          }
    union { UNSG32 u32DPHYTX_DPHY_RB2;
            struct w32DPHYTX_DPHY_RB2;
          };
    #define     w32DPHYTX_DPHY_RB3                             {\
            UNSG32 uDPHY_RB3_rxdataesc_0                       :  8;\
            UNSG32 uDPHY_RB3_rxtriggeresc_0                    :  4;\
            UNSG32 uDPHY_RB3_rxlpdtesc_0                       :  1;\
            UNSG32 uDPHY_RB3_rxulpsesc_0                       :  1;\
            UNSG32 uDPHY_RB3_rxvalidesc_0                      :  1;\
            UNSG32 uDPHY_RB3_direction_0                       :  1;\
            UNSG32 uDPHY_RB3_erresc_0                          :  1;\
            UNSG32 uDPHY_RB3_errsyncesc_0                      :  1;\
            UNSG32 RSVDx30_b18                                 : 14;\
          }
    union { UNSG32 u32DPHYTX_DPHY_RB3;
            struct w32DPHYTX_DPHY_RB3;
          };
    #define     w32DPHYTX_DPHY_PLL0                            {\
            UNSG32 uDPHY_PLL0_updatepll                        :  1;\
            UNSG32 RSVDx34_b1                                  : 31;\
          }
    union { UNSG32 u32DPHYTX_DPHY_PLL0;
            struct w32DPHYTX_DPHY_PLL0;
          };
    #define     w32DPHYTX_DPHY_PLL1                            {\
            UNSG32 uDPHY_PLL1_n                                :  4;\
            UNSG32 uDPHY_PLL1_m                                : 10;\
            UNSG32 uDPHY_PLL1_vco_cntrl                        :  6;\
            UNSG32 uDPHY_PLL1_prop_cntrl                       :  6;\
            UNSG32 uDPHY_PLL1_int_cntrl                        :  6;\
          }
    union { UNSG32 u32DPHYTX_DPHY_PLL1;
            struct w32DPHYTX_DPHY_PLL1;
          };
    #define     w32DPHYTX_DPHY_PLL2                            {\
            UNSG32 uDPHY_PLL2_gmp_cntrl                        :  2;\
            UNSG32 uDPHY_PLL2_cpbias_cntrl                     :  7;\
            UNSG32 uDPHY_PLL2_clksel                           :  2;\
            UNSG32 uDPHY_PLL2_force_lock                       :  1;\
            UNSG32 uDPHY_PLL2_pll_shadow_control               :  1;\
            UNSG32 uDPHY_PLL2_shadow_clear                     :  1;\
            UNSG32 uDPHY_PLL2_gp_clk_en                        :  1;\
            UNSG32 RSVDx3C_b15                                 : 17;\
          }
    union { UNSG32 u32DPHYTX_DPHY_PLL2;
            struct w32DPHYTX_DPHY_PLL2;
          };
    #define     w32DPHYTX_DPHY_PLLRB0                          {\
            UNSG32 uDPHY_PLLRB0_n_obs                          :  4;\
            UNSG32 uDPHY_PLLRB0_m_obs                          : 10;\
            UNSG32 uDPHY_PLLRB0_vco_cntrl_obs                  :  6;\
            UNSG32 uDPHY_PLLRB0_prop_cntrl_obs                 :  6;\
            UNSG32 uDPHY_PLLRB0_int_cntrl_obs                  :  6;\
          }
    union { UNSG32 u32DPHYTX_DPHY_PLLRB0;
            struct w32DPHYTX_DPHY_PLLRB0;
          };
    #define     w32DPHYTX_DPHY_PLLRB1                          {\
            UNSG32 uDPHY_PLLRB1_gmp_cntrl_obs                  :  2;\
            UNSG32 uDPHY_PLLRB1_cpbias_cntrl_obs               :  7;\
            UNSG32 uDPHY_PLLRB1_pll_shadow_control_obs         :  1;\
            UNSG32 uDPHY_PLLRB1_lock_pll                       :  1;\
            UNSG32 RSVDx44_b11                                 : 21;\
          }
    union { UNSG32 u32DPHYTX_DPHY_PLLRB1;
            struct w32DPHYTX_DPHY_PLLRB1;
          };
    } SIE_DPHYTX;
    typedef union  T32DPHYTX_DPHY_CTL0
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL0;
                 } T32DPHYTX_DPHY_CTL0;
    typedef union  T32DPHYTX_DPHY_CTL1
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL1;
                 } T32DPHYTX_DPHY_CTL1;
    typedef union  T32DPHYTX_DPHY_CTL2
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL2;
                 } T32DPHYTX_DPHY_CTL2;
    typedef union  T32DPHYTX_DPHY_CTL3
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL3;
                 } T32DPHYTX_DPHY_CTL3;
    typedef union  T32DPHYTX_DPHY_CTL4
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL4;
                 } T32DPHYTX_DPHY_CTL4;
    typedef union  T32DPHYTX_DPHY_CTL5
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL5;
                 } T32DPHYTX_DPHY_CTL5;
    typedef union  T32DPHYTX_DPHY_CTL6
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL6;
                 } T32DPHYTX_DPHY_CTL6;
    typedef union  T32DPHYTX_DPHY_CTL7
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL7;
                 } T32DPHYTX_DPHY_CTL7;
    typedef union  T32DPHYTX_DPHY_CTL8
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_CTL8;
                 } T32DPHYTX_DPHY_CTL8;
    typedef union  T32DPHYTX_DPHY_RB0
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_RB0;
                 } T32DPHYTX_DPHY_RB0;
    typedef union  T32DPHYTX_DPHY_RB1
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_RB1;
                 } T32DPHYTX_DPHY_RB1;
    typedef union  T32DPHYTX_DPHY_RB2
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_RB2;
                 } T32DPHYTX_DPHY_RB2;
    typedef union  T32DPHYTX_DPHY_RB3
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_RB3;
                 } T32DPHYTX_DPHY_RB3;
    typedef union  T32DPHYTX_DPHY_PLL0
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_PLL0;
                 } T32DPHYTX_DPHY_PLL0;
    typedef union  T32DPHYTX_DPHY_PLL1
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_PLL1;
                 } T32DPHYTX_DPHY_PLL1;
    typedef union  T32DPHYTX_DPHY_PLL2
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_PLL2;
                 } T32DPHYTX_DPHY_PLL2;
    typedef union  T32DPHYTX_DPHY_PLLRB0
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_PLLRB0;
                 } T32DPHYTX_DPHY_PLLRB0;
    typedef union  T32DPHYTX_DPHY_PLLRB1
          { UNSG32 u32;
            struct w32DPHYTX_DPHY_PLLRB1;
                 } T32DPHYTX_DPHY_PLLRB1;
    typedef union  TDPHYTX_DPHY_CTL0
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL0;
                   };
                 } TDPHYTX_DPHY_CTL0;
    typedef union  TDPHYTX_DPHY_CTL1
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL1;
                   };
                 } TDPHYTX_DPHY_CTL1;
    typedef union  TDPHYTX_DPHY_CTL2
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL2;
                   };
                 } TDPHYTX_DPHY_CTL2;
    typedef union  TDPHYTX_DPHY_CTL3
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL3;
                   };
                 } TDPHYTX_DPHY_CTL3;
    typedef union  TDPHYTX_DPHY_CTL4
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL4;
                   };
                 } TDPHYTX_DPHY_CTL4;
    typedef union  TDPHYTX_DPHY_CTL5
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL5;
                   };
                 } TDPHYTX_DPHY_CTL5;
    typedef union  TDPHYTX_DPHY_CTL6
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL6;
                   };
                 } TDPHYTX_DPHY_CTL6;
    typedef union  TDPHYTX_DPHY_CTL7
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL7;
                   };
                 } TDPHYTX_DPHY_CTL7;
    typedef union  TDPHYTX_DPHY_CTL8
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_CTL8;
                   };
                 } TDPHYTX_DPHY_CTL8;
    typedef union  TDPHYTX_DPHY_RB0
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_RB0;
                   };
                 } TDPHYTX_DPHY_RB0;
    typedef union  TDPHYTX_DPHY_RB1
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_RB1;
                   };
                 } TDPHYTX_DPHY_RB1;
    typedef union  TDPHYTX_DPHY_RB2
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_RB2;
                   };
                 } TDPHYTX_DPHY_RB2;
    typedef union  TDPHYTX_DPHY_RB3
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_RB3;
                   };
                 } TDPHYTX_DPHY_RB3;
    typedef union  TDPHYTX_DPHY_PLL0
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_PLL0;
                   };
                 } TDPHYTX_DPHY_PLL0;
    typedef union  TDPHYTX_DPHY_PLL1
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_PLL1;
                   };
                 } TDPHYTX_DPHY_PLL1;
    typedef union  TDPHYTX_DPHY_PLL2
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_PLL2;
                   };
                 } TDPHYTX_DPHY_PLL2;
    typedef union  TDPHYTX_DPHY_PLLRB0
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_PLLRB0;
                   };
                 } TDPHYTX_DPHY_PLLRB0;
    typedef union  TDPHYTX_DPHY_PLLRB1
          { UNSG32 u32[1];
            struct {
            struct w32DPHYTX_DPHY_PLLRB1;
                   };
                 } TDPHYTX_DPHY_PLLRB1;
     SIGN32 DPHYTX_drvrd(SIE_DPHYTX *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DPHYTX_drvwr(SIE_DPHYTX *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DPHYTX_reset(SIE_DPHYTX *p);
     SIGN32 DPHYTX_cmp  (SIE_DPHYTX *p, SIE_DPHYTX *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DPHYTX_check(p,pie,pfx,hLOG) DPHYTX_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DPHYTX_print(p,    pfx,hLOG) DPHYTX_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SRAMPWR
#define h_SRAMPWR (){}
    #define     RA_SRAMPWR_ctrl                                0x0000
    #define        SRAMPWR_ctrl_SD_ON                                       0x0
    #define        SRAMPWR_ctrl_SD_SHUTDWN                                  0x1
    #define        SRAMPWR_ctrl_DSLP_ON                                     0x0
    #define        SRAMPWR_ctrl_DSLP_DEEPSLP                                0x1
    #define        SRAMPWR_ctrl_SLP_ON                                      0x0
    #define        SRAMPWR_ctrl_SLP_SLEEP                                   0x1
    typedef struct SIE_SRAMPWR {
    #define     w32SRAMPWR_ctrl                                {\
            UNSG32 uctrl_SD                                    :  1;\
            UNSG32 uctrl_DSLP                                  :  1;\
            UNSG32 uctrl_SLP                                   :  1;\
            UNSG32 RSVDx0_b3                                   : 29;\
          }
    union { UNSG32 u32SRAMPWR_ctrl;
            struct w32SRAMPWR_ctrl;
          };
    } SIE_SRAMPWR;
    typedef union  T32SRAMPWR_ctrl
          { UNSG32 u32;
            struct w32SRAMPWR_ctrl;
                 } T32SRAMPWR_ctrl;
    typedef union  TSRAMPWR_ctrl
          { UNSG32 u32[1];
            struct {
            struct w32SRAMPWR_ctrl;
                   };
                 } TSRAMPWR_ctrl;
     SIGN32 SRAMPWR_drvrd(SIE_SRAMPWR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SRAMPWR_drvwr(SIE_SRAMPWR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SRAMPWR_reset(SIE_SRAMPWR *p);
     SIGN32 SRAMPWR_cmp  (SIE_SRAMPWR *p, SIE_SRAMPWR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SRAMPWR_check(p,pie,pfx,hLOG) SRAMPWR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SRAMPWR_print(p,    pfx,hLOG) SRAMPWR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SRAMRWTC
#define h_SRAMRWTC (){}
    #define     RA_SRAMRWTC_ctrl0                              0x0000
    #define     RA_SRAMRWTC_ctrl1                              0x0004
    #define     RA_SRAMRWTC_ctrl2                              0x0008
    typedef struct SIE_SRAMRWTC {
    #define     w32SRAMRWTC_ctrl0                              {\
            UNSG32 uctrl0_RF1P                                 :  4;\
            UNSG32 uctrl0_UHDRF1P                              :  4;\
            UNSG32 uctrl0_RF2P                                 :  8;\
            UNSG32 uctrl0_UHDRF2P                              :  8;\
            UNSG32 uctrl0_UHDRF2P_ULVT                         :  8;\
          }
    union { UNSG32 u32SRAMRWTC_ctrl0;
            struct w32SRAMRWTC_ctrl0;
          };
    #define     w32SRAMRWTC_ctrl1                              {\
            UNSG32 uctrl1_SHDMBSR1P                            :  4;\
            UNSG32 uctrl1_SHDSBSR1P                            :  4;\
            UNSG32 uctrl1_SHCMBSR1P_SSEG                       :  4;\
            UNSG32 uctrl1_SHCMBSR1P_USEG                       :  4;\
            UNSG32 uctrl1_SHCSBSR1P                            :  4;\
            UNSG32 uctrl1_SHCSBSR1P_CUSTM                      :  4;\
            UNSG32 uctrl1_SPSRAM_WT0                           :  4;\
            UNSG32 uctrl1_SPSRAM_WT1                           :  4;\
          }
    union { UNSG32 u32SRAMRWTC_ctrl1;
            struct w32SRAMRWTC_ctrl1;
          };
    #define     w32SRAMRWTC_ctrl2                              {\
            UNSG32 uctrl2_L1CACHE                              :  4;\
            UNSG32 uctrl2_DPSR2P                               :  4;\
            UNSG32 uctrl2_ROM                                  :  8;\
            UNSG32 RSVDx8_b16                                  : 16;\
          }
    union { UNSG32 u32SRAMRWTC_ctrl2;
            struct w32SRAMRWTC_ctrl2;
          };
    } SIE_SRAMRWTC;
    typedef union  T32SRAMRWTC_ctrl0
          { UNSG32 u32;
            struct w32SRAMRWTC_ctrl0;
                 } T32SRAMRWTC_ctrl0;
    typedef union  T32SRAMRWTC_ctrl1
          { UNSG32 u32;
            struct w32SRAMRWTC_ctrl1;
                 } T32SRAMRWTC_ctrl1;
    typedef union  T32SRAMRWTC_ctrl2
          { UNSG32 u32;
            struct w32SRAMRWTC_ctrl2;
                 } T32SRAMRWTC_ctrl2;
    typedef union  TSRAMRWTC_ctrl0
          { UNSG32 u32[1];
            struct {
            struct w32SRAMRWTC_ctrl0;
                   };
                 } TSRAMRWTC_ctrl0;
    typedef union  TSRAMRWTC_ctrl1
          { UNSG32 u32[1];
            struct {
            struct w32SRAMRWTC_ctrl1;
                   };
                 } TSRAMRWTC_ctrl1;
    typedef union  TSRAMRWTC_ctrl2
          { UNSG32 u32[1];
            struct {
            struct w32SRAMRWTC_ctrl2;
                   };
                 } TSRAMRWTC_ctrl2;
     SIGN32 SRAMRWTC_drvrd(SIE_SRAMRWTC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SRAMRWTC_drvwr(SIE_SRAMRWTC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SRAMRWTC_reset(SIE_SRAMRWTC *p);
     SIGN32 SRAMRWTC_cmp  (SIE_SRAMRWTC *p, SIE_SRAMRWTC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SRAMRWTC_check(p,pie,pfx,hLOG) SRAMRWTC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SRAMRWTC_print(p,    pfx,hLOG) SRAMRWTC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_INT
#define h_INT (){}
    #define     RA_INT_EN                                      0x0000
    #define     RA_INT_STS                                     0x0004
    typedef struct SIE_INT {
    #define     w32INT_EN                                      {\
            UNSG32 uEN_bus                                     : 15;\
            UNSG32 RSVDx0_b15                                  : 17;\
          }
    union { UNSG32 u32INT_EN;
            struct w32INT_EN;
          };
    #define     w32INT_STS                                     {\
            UNSG32 uSTS_bus                                    : 15;\
            UNSG32 RSVDx4_b15                                  : 17;\
          }
    union { UNSG32 u32INT_STS;
            struct w32INT_STS;
          };
    } SIE_INT;
    typedef union  T32INT_EN
          { UNSG32 u32;
            struct w32INT_EN;
                 } T32INT_EN;
    typedef union  T32INT_STS
          { UNSG32 u32;
            struct w32INT_STS;
                 } T32INT_STS;
    typedef union  TINT_EN
          { UNSG32 u32[1];
            struct {
            struct w32INT_EN;
                   };
                 } TINT_EN;
    typedef union  TINT_STS
          { UNSG32 u32[1];
            struct {
            struct w32INT_STS;
                   };
                 } TINT_STS;
     SIGN32 INT_drvrd(SIE_INT *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 INT_drvwr(SIE_INT *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void INT_reset(SIE_INT *p);
     SIGN32 INT_cmp  (SIE_INT *p, SIE_INT *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define INT_check(p,pie,pfx,hLOG) INT_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define INT_print(p,    pfx,hLOG) INT_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AVIO_debug_ctrl
#define h_AVIO_debug_ctrl (){}
    #define     RA_AVIO_debug_ctrl_Ctrl0                       0x0000
    typedef struct SIE_AVIO_debug_ctrl {
    #define     w32AVIO_debug_ctrl_Ctrl0                       {\
            UNSG32 uCtrl0_debug_ctrl0                          :  5;\
            UNSG32 RSVDx0_b5                                   : 27;\
          }
    union { UNSG32 u32AVIO_debug_ctrl_Ctrl0;
            struct w32AVIO_debug_ctrl_Ctrl0;
          };
    } SIE_AVIO_debug_ctrl;
    typedef union  T32AVIO_debug_ctrl_Ctrl0
          { UNSG32 u32;
            struct w32AVIO_debug_ctrl_Ctrl0;
                 } T32AVIO_debug_ctrl_Ctrl0;
    typedef union  TAVIO_debug_ctrl_Ctrl0
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_debug_ctrl_Ctrl0;
                   };
                 } TAVIO_debug_ctrl_Ctrl0;
     SIGN32 AVIO_debug_ctrl_drvrd(SIE_AVIO_debug_ctrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AVIO_debug_ctrl_drvwr(SIE_AVIO_debug_ctrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AVIO_debug_ctrl_reset(SIE_AVIO_debug_ctrl *p);
     SIGN32 AVIO_debug_ctrl_cmp  (SIE_AVIO_debug_ctrl *p, SIE_AVIO_debug_ctrl *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AVIO_debug_ctrl_check(p,pie,pfx,hLOG) AVIO_debug_ctrl_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AVIO_debug_ctrl_print(p,    pfx,hLOG) AVIO_debug_ctrl_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_Dummy3Reg
#define h_Dummy3Reg (){}
    typedef struct SIE_Dummy3Reg {
            UNSG32 u_0x0                                       : 32;
    } SIE_Dummy3Reg;
     SIGN32 Dummy3Reg_drvrd(SIE_Dummy3Reg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 Dummy3Reg_drvwr(SIE_Dummy3Reg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void Dummy3Reg_reset(SIE_Dummy3Reg *p);
     SIGN32 Dummy3Reg_cmp  (SIE_Dummy3Reg *p, SIE_Dummy3Reg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define Dummy3Reg_check(p,pie,pfx,hLOG) Dummy3Reg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define Dummy3Reg_print(p,    pfx,hLOG) Dummy3Reg_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AVIOGBLREG3
#define h_AVIOGBLREG3 (){}
    #define     RA_AVIOGBLREG3_Dummy3                          0x0000
    typedef struct SIE_AVIOGBLREG3 {
              SIE_Dummy3Reg                                    ie_Dummy3[2048];
    } SIE_AVIOGBLREG3;
     SIGN32 AVIOGBLREG3_drvrd(SIE_AVIOGBLREG3 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AVIOGBLREG3_drvwr(SIE_AVIOGBLREG3 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AVIOGBLREG3_reset(SIE_AVIOGBLREG3 *p);
     SIGN32 AVIOGBLREG3_cmp  (SIE_AVIOGBLREG3 *p, SIE_AVIOGBLREG3 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AVIOGBLREG3_check(p,pie,pfx,hLOG) AVIOGBLREG3_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AVIOGBLREG3_print(p,    pfx,hLOG) AVIOGBLREG3_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_APLL_WRAP
#define h_APLL_WRAP (){}
    #define     RA_APLL_WRAP_APLL_CLK1_CTRL                    0x0000
    #define        APLL_WRAP_APLL_CLK1_CTRL_clkSel_d2                       0x1
    #define        APLL_WRAP_APLL_CLK1_CTRL_clkSel_d4                       0x2
    #define        APLL_WRAP_APLL_CLK1_CTRL_clkSel_d6                       0x3
    #define        APLL_WRAP_APLL_CLK1_CTRL_clkSel_d8                       0x4
    #define        APLL_WRAP_APLL_CLK1_CTRL_clkSel_d12                      0x5
    #define     RA_APLL_WRAP_CTRL0                             0x0004
    #define     RA_APLL_WRAP_APLL                              0x0008
    typedef struct SIE_APLL_WRAP {
    #define     w32APLL_WRAP_APLL_CLK1_CTRL                    {\
            UNSG32 uAPLL_CLK1_CTRL_clkSwitch                   :  1;\
            UNSG32 uAPLL_CLK1_CTRL_clkD3Switch                 :  1;\
            UNSG32 uAPLL_CLK1_CTRL_clkSel                      :  3;\
            UNSG32 uAPLL_CLK1_CTRL_clkEn                       :  1;\
            UNSG32 RSVDx0_b6                                   : 26;\
          }
    union { UNSG32 u32APLL_WRAP_APLL_CLK1_CTRL;
            struct w32APLL_WRAP_APLL_CLK1_CTRL;
          };
    #define     w32APLL_WRAP_CTRL0                             {\
            UNSG32 uCTRL0_clk_sel0                             :  4;\
            UNSG32 uCTRL0_clk_sel1                             :  1;\
            UNSG32 uCTRL0_clk_sel2                             :  2;\
            UNSG32 uCTRL0_clk_sel3                             :  1;\
            UNSG32 RSVDx4_b8                                   : 24;\
          }
    union { UNSG32 u32APLL_WRAP_CTRL0;
            struct w32APLL_WRAP_CTRL0;
          };
              SIE_abipll                                       ie_APLL;
    } SIE_APLL_WRAP;
    typedef union  T32APLL_WRAP_APLL_CLK1_CTRL
          { UNSG32 u32;
            struct w32APLL_WRAP_APLL_CLK1_CTRL;
                 } T32APLL_WRAP_APLL_CLK1_CTRL;
    typedef union  T32APLL_WRAP_CTRL0
          { UNSG32 u32;
            struct w32APLL_WRAP_CTRL0;
                 } T32APLL_WRAP_CTRL0;
    typedef union  TAPLL_WRAP_APLL_CLK1_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32APLL_WRAP_APLL_CLK1_CTRL;
                   };
                 } TAPLL_WRAP_APLL_CLK1_CTRL;
    typedef union  TAPLL_WRAP_CTRL0
          { UNSG32 u32[1];
            struct {
            struct w32APLL_WRAP_CTRL0;
                   };
                 } TAPLL_WRAP_CTRL0;
     SIGN32 APLL_WRAP_drvrd(SIE_APLL_WRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 APLL_WRAP_drvwr(SIE_APLL_WRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void APLL_WRAP_reset(SIE_APLL_WRAP *p);
     SIGN32 APLL_WRAP_cmp  (SIE_APLL_WRAP *p, SIE_APLL_WRAP *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define APLL_WRAP_check(p,pie,pfx,hLOG) APLL_WRAP_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define APLL_WRAP_print(p,    pfx,hLOG) APLL_WRAP_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_VPLL_WRAP
#define h_VPLL_WRAP (){}
    #define     RA_VPLL_WRAP_VPLL_CTRL                         0x0000
    #define        VPLL_WRAP_VPLL_CTRL_clkOSel_d2                           0x1
    #define        VPLL_WRAP_VPLL_CTRL_clkOSel_d4                           0x2
    #define        VPLL_WRAP_VPLL_CTRL_clkOSel_d6                           0x3
    #define        VPLL_WRAP_VPLL_CTRL_clkOSel_d8                           0x4
    #define        VPLL_WRAP_VPLL_CTRL_clkOSel_d12                          0x5
    #define        VPLL_WRAP_VPLL_CTRL_clkO1Sel_d2                          0x1
    #define        VPLL_WRAP_VPLL_CTRL_clkO1Sel_d4                          0x2
    #define        VPLL_WRAP_VPLL_CTRL_clkO1Sel_d6                          0x3
    #define        VPLL_WRAP_VPLL_CTRL_clkO1Sel_d8                          0x4
    #define        VPLL_WRAP_VPLL_CTRL_clkO1Sel_d12                         0x5
    #define     RA_VPLL_WRAP_VPLL                              0x0004
    typedef struct SIE_VPLL_WRAP {
    #define     w32VPLL_WRAP_VPLL_CTRL                         {\
            UNSG32 uVPLL_CTRL_clkOSwitch                       :  1;\
            UNSG32 uVPLL_CTRL_clkOD3Switch                     :  1;\
            UNSG32 uVPLL_CTRL_clkOSel                          :  3;\
            UNSG32 uVPLL_CTRL_clkOEn                           :  1;\
            UNSG32 uVPLL_CTRL_clkO1Switch                      :  1;\
            UNSG32 uVPLL_CTRL_clkO1D3Switch                    :  1;\
            UNSG32 uVPLL_CTRL_clkO1Sel                         :  3;\
            UNSG32 uVPLL_CTRL_clkO1En                          :  1;\
            UNSG32 RSVDx0_b12                                  : 20;\
          }
    union { UNSG32 u32VPLL_WRAP_VPLL_CTRL;
            struct w32VPLL_WRAP_VPLL_CTRL;
          };
              SIE_abipll                                       ie_VPLL;
    } SIE_VPLL_WRAP;
    typedef union  T32VPLL_WRAP_VPLL_CTRL
          { UNSG32 u32;
            struct w32VPLL_WRAP_VPLL_CTRL;
                 } T32VPLL_WRAP_VPLL_CTRL;
    typedef union  TVPLL_WRAP_VPLL_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32VPLL_WRAP_VPLL_CTRL;
                   };
                 } TVPLL_WRAP_VPLL_CTRL;
     SIGN32 VPLL_WRAP_drvrd(SIE_VPLL_WRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 VPLL_WRAP_drvwr(SIE_VPLL_WRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void VPLL_WRAP_reset(SIE_VPLL_WRAP *p);
     SIGN32 VPLL_WRAP_cmp  (SIE_VPLL_WRAP *p, SIE_VPLL_WRAP *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define VPLL_WRAP_check(p,pie,pfx,hLOG) VPLL_WRAP_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define VPLL_WRAP_print(p,    pfx,hLOG) VPLL_WRAP_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_avioGbl
#define h_avioGbl (){}
    #define     RA_avioGbl_MEMMAP_MIPI                         0x0000
    #define     RA_avioGbl_VPLL0_WRAP                          0x2000
    #define     RA_avioGbl_VPLL1_WRAP                          0x2024
    #define     RA_avioGbl_APLL0_WRAP                          0x2048
    #define     RA_avioGbl_APLL1_WRAP                          0x2070
    #define     RA_avioGbl_AVIO_debug_ctrl                     0x2098
    #define     RA_avioGbl_DPHYTX                              0x209C
    #define     RA_avioGbl_AVPLLA_CLK_EN                       0x20E4
    #define     RA_avioGbl_SWPDWN_CTRL                         0x20E8
    #define     RA_avioGbl_RWTC_31to0                          0x20EC
    #define     RA_avioGbl_RWTC_57to32                         0x20F0
    #define     RA_avioGbl_CTRL                                0x20F4
    #define     RA_avioGbl_CTRL0                               0x20F8
    #define     RA_avioGbl_AIO64bDHUB_SRAMPWR                  0x20FC
    #define     RA_avioGbl_VPP128bDHUB_SRAMPWR                 0x2100
    #define     RA_avioGbl_MIPI_SRAMPWR                        0x2104
    #define     RA_avioGbl_SRAMRWTC                            0x2108
    #define     RA_avioGbl_GIC_INT0                            0x2114
    #define     RA_avioGbl_GIC_INT1                            0x211C
    #define     RA_avioGbl_GIC_INT2                            0x2124
    #define     RA_avioGbl_GIC_INT3                            0x212C
    #define     RA_avioGbl_GIC_INT4                            0x2134
    #define     RA_avioGbl_GIC_INT5                            0x213C
    #define     RA_avioGbl_GIC_INT6                            0x2144
    #define     RA_avioGbl_GIC_INT7                            0x214C
    #define     RA_avioGbl_GIC_INT8                            0x2154
    #define     RA_avioGbl_GIC_INT9                            0x215C
    #define     RA_avioGbl_GIC_INT10                           0x2164
    #define     RA_avioGbl_GIC_INT11                           0x216C
    #define     RA_avioGbl_GIC_INT12                           0x2174
    #define     RA_avioGbl_GIC_INT13                           0x217C
    #define     RA_avioGbl_GIC_INT                             0x2184
    #define     RA_avioGbl_INTR_CTRL                           0x2188
    #define     RA_avioGbl_INTR_STS                            0x218C
    #define     RA_avioGbl_DAIF_DRT_CTRL                       0x2190
    #define     RA_avioGbl_DCLS_CTRL                           0x2194
    #define     RA_avioGbl_SWRST_CTRL                          0x2198
    #define     RA_avioGbl_LCDC_CTRL                           0x219C
    #define     RA_avioGbl_LCDC2_CTRL                          0x21A0
    #define     RA_avioGbl_PTRACK_CTRL                         0x21A4
    #define     RA_avioGbl_MIPI_CTRL                           0x21A8
    #define     RA_avioGbl_MIPI_CTRL1                          0x21AC
    #define     RA_avioGbl_MIPI_CTRL_STS                       0x21B0
    typedef struct SIE_avioGbl {
              SIE_AVIOGBLREG3                                  ie_MEMMAP_MIPI;
              SIE_VPLL_WRAP                                    ie_VPLL0_WRAP;
              SIE_VPLL_WRAP                                    ie_VPLL1_WRAP;
              SIE_APLL_WRAP                                    ie_APLL0_WRAP;
              SIE_APLL_WRAP                                    ie_APLL1_WRAP;
              SIE_AVIO_debug_ctrl                              ie_AVIO_debug_ctrl;
              SIE_DPHYTX                                       ie_DPHYTX;
    #define     w32avioGbl_AVPLLA_CLK_EN                       {\
            UNSG32 uAVPLLA_CLK_EN_ctrl                         :  6;\
            UNSG32 uAVPLLA_CLK_EN_dbg_mux_sel                  :  1;\
            UNSG32 RSVDx20E4_b7                                : 25;\
          }
    union { UNSG32 u32avioGbl_AVPLLA_CLK_EN;
            struct w32avioGbl_AVPLLA_CLK_EN;
          };
    #define     w32avioGbl_SWPDWN_CTRL                         {\
            UNSG32 uSWPDWN_CTRL_VPLL0_PD                       :  1;\
            UNSG32 uSWPDWN_CTRL_VPLL1_PD                       :  1;\
            UNSG32 uSWPDWN_CTRL_APLL0_PD                       :  1;\
            UNSG32 uSWPDWN_CTRL_APLL1_PD                       :  1;\
            UNSG32 RSVDx20E8_b4                                : 28;\
          }
    union { UNSG32 u32avioGbl_SWPDWN_CTRL;
            struct w32avioGbl_SWPDWN_CTRL;
          };
    #define     w32avioGbl_RWTC_31to0                          {\
            UNSG32 uRWTC_31to0_value                           : 32;\
          }
    union { UNSG32 u32avioGbl_RWTC_31to0;
            struct w32avioGbl_RWTC_31to0;
          };
    #define     w32avioGbl_RWTC_57to32                         {\
            UNSG32 uRWTC_57to32_value                          : 26;\
            UNSG32 RSVDx20F0_b26                               :  6;\
          }
    union { UNSG32 u32avioGbl_RWTC_57to32;
            struct w32avioGbl_RWTC_57to32;
          };
    #define     w32avioGbl_CTRL                                {\
            UNSG32 uCTRL_AIODHUB_dyCG_en                       :  1;\
            UNSG32 uCTRL_AIODHUB_CG_en                         :  1;\
            UNSG32 uCTRL_VPPDHUB_dyCG_en                       :  1;\
            UNSG32 uCTRL_VPPDHUB_CG_en                         :  1;\
            UNSG32 uCTRL_INTR_EN                               :  4;\
            UNSG32 uCTRL_BCM_FIFO_FLUSH                        :  1;\
            UNSG32 uCTRL_BCMQ_FIFO_FLUSH                       :  1;\
            UNSG32 RSVDx20F4_b10                               : 22;\
          }
    union { UNSG32 u32avioGbl_CTRL;
            struct w32avioGbl_CTRL;
          };
    #define     w32avioGbl_CTRL0                               {\
            UNSG32 uCTRL0_I2S1_MCLK_SEL                        :  3;\
            UNSG32 uCTRL0_I2S2_MCLK_SEL                        :  3;\
            UNSG32 uCTRL0_i2s1_mclk_inv                        :  1;\
            UNSG32 uCTRL0_i2s2_mclk_inv                        :  1;\
            UNSG32 uCTRL0_I2S1_MCLK_OEN                        :  1;\
            UNSG32 uCTRL0_I2S2_MCLK_OEN                        :  1;\
            UNSG32 uCTRL0_PDM_CLK_OEN                          :  1;\
            UNSG32 uCTRL0_daif_clk_inv                         :  1;\
            UNSG32 uCTRL0_dcls_clk_inv                         :  1;\
            UNSG32 RSVDx20F8_b13                               : 19;\
          }
    union { UNSG32 u32avioGbl_CTRL0;
            struct w32avioGbl_CTRL0;
          };
              SIE_SRAMPWR                                      ie_AIO64bDHUB_SRAMPWR;
              SIE_SRAMPWR                                      ie_VPP128bDHUB_SRAMPWR;
              SIE_SRAMPWR                                      ie_MIPI_SRAMPWR;
              SIE_SRAMRWTC                                     ie_SRAMRWTC;
              SIE_INT                                          ie_GIC_INT0;
              SIE_INT                                          ie_GIC_INT1;
              SIE_INT                                          ie_GIC_INT2;
              SIE_INT                                          ie_GIC_INT3;
              SIE_INT                                          ie_GIC_INT4;
              SIE_INT                                          ie_GIC_INT5;
              SIE_INT                                          ie_GIC_INT6;
              SIE_INT                                          ie_GIC_INT7;
              SIE_INT                                          ie_GIC_INT8;
              SIE_INT                                          ie_GIC_INT9;
              SIE_INT                                          ie_GIC_INT10;
              SIE_INT                                          ie_GIC_INT11;
              SIE_INT                                          ie_GIC_INT12;
              SIE_INT                                          ie_GIC_INT13;
    #define     w32avioGbl_GIC_INT                             {\
            UNSG32 uGIC_INT_status                             : 15;\
            UNSG32 RSVDx2184_b15                               : 17;\
          }
    union { UNSG32 u32avioGbl_GIC_INT;
            struct w32avioGbl_GIC_INT;
          };
    #define     w32avioGbl_INTR_CTRL                           {\
            UNSG32 uINTR_CTRL_mipi_int_en                      :  1;\
            UNSG32 uINTR_CTRL_ptrack1_int_en                   :  1;\
            UNSG32 uINTR_CTRL_ptrack2_int_en                   :  1;\
            UNSG32 uINTR_CTRL_lcdc1_int_en                     :  1;\
            UNSG32 uINTR_CTRL_lcdc2_int_en                     :  1;\
            UNSG32 uINTR_CTRL_daif_int_en                      :  1;\
            UNSG32 uINTR_CTRL_daif_pwr_ok_int_en               :  1;\
            UNSG32 uINTR_CTRL_daif_ana_int_en                  :  1;\
            UNSG32 uINTR_CTRL_drt_int_en                       :  1;\
            UNSG32 uINTR_CTRL_dclass_int_en                    :  1;\
            UNSG32 uINTR_CTRL_daif_pwr_ok_gic_int_en           :  1;\
            UNSG32 uINTR_CTRL_daif_pwr_ok_int_pol              :  1;\
            UNSG32 uINTR_CTRL_daif_pwr_ok_int_edge             :  1;\
            UNSG32 uINTR_CTRL_daif_if_int_ana_pol              :  1;\
            UNSG32 RSVDx2188_b14                               : 18;\
          }
    union { UNSG32 u32avioGbl_INTR_CTRL;
            struct w32avioGbl_INTR_CTRL;
          };
    #define     w32avioGbl_INTR_STS                            {\
            UNSG32 uINTR_STS_gic_daif_pwr_ok_int               :  1;\
            UNSG32 RSVDx218C_b1                                : 31;\
          }
    union { UNSG32 u32avioGbl_INTR_STS;
            struct w32avioGbl_INTR_STS;
          };
    #define     w32avioGbl_DAIF_DRT_CTRL                       {\
            UNSG32 uDAIF_DRT_CTRL_clk_src_sel0                 :  3;\
            UNSG32 uDAIF_DRT_CTRL_div_val1                     :  6;\
            UNSG32 uDAIF_DRT_CTRL_div_val2                     :  4;\
            UNSG32 uDAIF_DRT_CTRL_ClkEn                        :  1;\
            UNSG32 uDAIF_DRT_CTRL_daif_if_clk_inv              :  1;\
            UNSG32 uDAIF_DRT_CTRL_drt_clk_sel                  :  1;\
            UNSG32 uDAIF_DRT_CTRL_drt_clk_en                   :  1;\
            UNSG32 uDAIF_DRT_CTRL_drt_sysClk_en                :  1;\
            UNSG32 uDAIF_DRT_CTRL_daif_clk_en                  :  1;\
            UNSG32 uDAIF_DRT_CTRL_daif_sysClk_en               :  1;\
            UNSG32 RSVDx2190_b20                               : 12;\
          }
    union { UNSG32 u32avioGbl_DAIF_DRT_CTRL;
            struct w32avioGbl_DAIF_DRT_CTRL;
          };
    #define     w32avioGbl_DCLS_CTRL                           {\
            UNSG32 uDCLS_CTRL_clk_src_sel0                     :  3;\
            UNSG32 uDCLS_CTRL_div_val1                         :  6;\
            UNSG32 uDCLS_CTRL_div_val2                         :  4;\
            UNSG32 uDCLS_CTRL_ClkEn                            :  1;\
            UNSG32 uDCLS_CTRL_dcls_sclk_inv                    :  1;\
            UNSG32 uDCLS_CTRL_dcls_sysClk_en                   :  1;\
            UNSG32 RSVDx2194_b16                               : 16;\
          }
    union { UNSG32 u32avioGbl_DCLS_CTRL;
            struct w32avioGbl_DCLS_CTRL;
          };
    #define     w32avioGbl_SWRST_CTRL                          {\
            UNSG32 uSWRST_CTRL_aioSyncRstn                     :  1;\
            UNSG32 uSWRST_CTRL_dpiSyncRstn                     :  1;\
            UNSG32 uSWRST_CTRL_lcdc1SyncRstn                   :  1;\
            UNSG32 uSWRST_CTRL_lcdc1SysSyncRstn                :  1;\
            UNSG32 uSWRST_CTRL_lcdc2SyncRstn                   :  1;\
            UNSG32 uSWRST_CTRL_lcdc2SysSyncRstn                :  1;\
            UNSG32 uSWRST_CTRL_daifSyncRstn                    :  1;\
            UNSG32 uSWRST_CTRL_daif_nares_SyncRstn             :  1;\
            UNSG32 uSWRST_CTRL_daifSysSyncRstn                 :  1;\
            UNSG32 uSWRST_CTRL_drtSyncRstn                     :  1;\
            UNSG32 uSWRST_CTRL_drtSysSyncRstn                  :  1;\
            UNSG32 uSWRST_CTRL_dclsSyncRstn                    :  1;\
            UNSG32 uSWRST_CTRL_dclsSysSyncRstn                 :  1;\
            UNSG32 uSWRST_CTRL_ptrack1SysSyncRstn              :  1;\
            UNSG32 uSWRST_CTRL_ptrack1SyncRstn                 :  1;\
            UNSG32 uSWRST_CTRL_ptrack2SysSyncRstn              :  1;\
            UNSG32 uSWRST_CTRL_ptrack2SyncRstn                 :  1;\
            UNSG32 RSVDx2198_b17                               : 15;\
          }
    union { UNSG32 u32avioGbl_SWRST_CTRL;
            struct w32avioGbl_SWRST_CTRL;
          };
    #define     w32avioGbl_LCDC_CTRL                           {\
            UNSG32 uLCDC_CTRL_lcdc1_clk_sel                    :  1;\
            UNSG32 uLCDC_CTRL_lcdc2_clk_sel                    :  1;\
            UNSG32 uLCDC_CTRL_lcdc1_ClkSel                     :  3;\
            UNSG32 uLCDC_CTRL_lcdc1_ClkSwitch                  :  1;\
            UNSG32 uLCDC_CTRL_lcdc1_ClkD3Switch                :  1;\
            UNSG32 uLCDC_CTRL_lcdc1_ClkEn                      :  1;\
            UNSG32 uLCDC_CTRL_lcdc1_sysClk_en                  :  1;\
            UNSG32 uLCDC_CTRL_lcdc2_ClkSel                     :  3;\
            UNSG32 uLCDC_CTRL_lcdc2_ClkSwitch                  :  1;\
            UNSG32 uLCDC_CTRL_lcdc2_ClkD3Switch                :  1;\
            UNSG32 uLCDC_CTRL_lcdc2_ClkEn                      :  1;\
            UNSG32 uLCDC_CTRL_lcdc2_sysClk_en                  :  1;\
            UNSG32 RSVDx219C_b16                               : 16;\
          }
    union { UNSG32 u32avioGbl_LCDC_CTRL;
            struct w32avioGbl_LCDC_CTRL;
          };
    #define     w32avioGbl_LCDC2_CTRL                          {\
            UNSG32 uLCDC2_CTRL_eof_frst_sel                    :  1;\
            UNSG32 uLCDC2_CTRL_tear_sftrst                     :  1;\
            UNSG32 uLCDC2_CTRL_clken_ctrl0                     :  1;\
            UNSG32 uLCDC2_CTRL_clken_ctrl1                     :  1;\
            UNSG32 uLCDC2_CTRL_clken_ctrl2                     :  1;\
            UNSG32 uLCDC2_CTRL_clken_ctrl3                     :  1;\
            UNSG32 RSVDx21A0_b6                                : 26;\
          }
    union { UNSG32 u32avioGbl_LCDC2_CTRL;
            struct w32avioGbl_LCDC2_CTRL;
          };
    #define     w32avioGbl_PTRACK_CTRL                         {\
            UNSG32 uPTRACK_CTRL_ptrack1_sysClk_en              :  1;\
            UNSG32 uPTRACK_CTRL_ptrack1_clk_en                 :  1;\
            UNSG32 uPTRACK_CTRL_ptrack2_sysClk_en              :  1;\
            UNSG32 uPTRACK_CTRL_ptrack2_clk_en                 :  1;\
            UNSG32 RSVDx21A4_b4                                : 28;\
          }
    union { UNSG32 u32avioGbl_PTRACK_CTRL;
            struct w32avioGbl_PTRACK_CTRL;
          };
    #define     w32avioGbl_MIPI_CTRL                           {\
            UNSG32 uMIPI_CTRL_mipiClkg_en                      :  1;\
            UNSG32 uMIPI_CTRL_mipiSysClkg_en                   :  1;\
            UNSG32 uMIPI_CTRL_dpidataen_pol                    :  1;\
            UNSG32 uMIPI_CTRL_dpivsync_pol                     :  1;\
            UNSG32 uMIPI_CTRL_dpihsync_pol                     :  1;\
            UNSG32 uMIPI_CTRL_dpishutd_pol                     :  1;\
            UNSG32 uMIPI_CTRL_dpicolorm_pol                    :  1;\
            UNSG32 uMIPI_CTRL_colormode                        :  1;\
            UNSG32 uMIPI_CTRL_shutdn                           :  1;\
            UNSG32 uMIPI_CTRL_force_pll_on                     :  1;\
            UNSG32 uMIPI_CTRL_tear_request_pulse               :  1;\
            UNSG32 uMIPI_CTRL_updatecfg_pulse                  :  1;\
            UNSG32 uMIPI_CTRL_tear_sftrst                      :  1;\
            UNSG32 uMIPI_CTRL_dsi_te_in_sel                    :  1;\
            UNSG32 uMIPI_CTRL_dsi_te_in_sense                  :  1;\
            UNSG32 uMIPI_CTRL_dsi_te_enable                    :  1;\
            UNSG32 RSVDx21A8_b16                               : 16;\
          }
    union { UNSG32 u32avioGbl_MIPI_CTRL;
            struct w32avioGbl_MIPI_CTRL;
          };
    #define     w32avioGbl_MIPI_CTRL1                          {\
            UNSG32 uMIPI_CTRL_dsitedelay                       : 22;\
            UNSG32 uMIPI_CTRL_edpi_mode                        :  1;\
            UNSG32 RSVDx21AC_b23                               :  9;\
          }
    union { UNSG32 u32avioGbl_MIPI_CTRL1;
            struct w32avioGbl_MIPI_CTRL1;
          };
    #define     w32avioGbl_MIPI_CTRL_STS                       {\
            UNSG32 uMIPI_CTRL_STS_dsitewait                    :  1;\
            UNSG32 RSVDx21B0_b1                                : 31;\
          }
    union { UNSG32 u32avioGbl_MIPI_CTRL_STS;
            struct w32avioGbl_MIPI_CTRL_STS;
          };
             UNSG8 RSVDx21B4                                   [7756];
    } SIE_avioGbl;
    typedef union  T32avioGbl_AVPLLA_CLK_EN
          { UNSG32 u32;
            struct w32avioGbl_AVPLLA_CLK_EN;
                 } T32avioGbl_AVPLLA_CLK_EN;
    typedef union  T32avioGbl_SWPDWN_CTRL
          { UNSG32 u32;
            struct w32avioGbl_SWPDWN_CTRL;
                 } T32avioGbl_SWPDWN_CTRL;
    typedef union  T32avioGbl_RWTC_31to0
          { UNSG32 u32;
            struct w32avioGbl_RWTC_31to0;
                 } T32avioGbl_RWTC_31to0;
    typedef union  T32avioGbl_RWTC_57to32
          { UNSG32 u32;
            struct w32avioGbl_RWTC_57to32;
                 } T32avioGbl_RWTC_57to32;
    typedef union  T32avioGbl_CTRL
          { UNSG32 u32;
            struct w32avioGbl_CTRL;
                 } T32avioGbl_CTRL;
    typedef union  T32avioGbl_CTRL0
          { UNSG32 u32;
            struct w32avioGbl_CTRL0;
                 } T32avioGbl_CTRL0;
    typedef union  T32avioGbl_GIC_INT
          { UNSG32 u32;
            struct w32avioGbl_GIC_INT;
                 } T32avioGbl_GIC_INT;
    typedef union  T32avioGbl_INTR_CTRL
          { UNSG32 u32;
            struct w32avioGbl_INTR_CTRL;
                 } T32avioGbl_INTR_CTRL;
    typedef union  T32avioGbl_INTR_STS
          { UNSG32 u32;
            struct w32avioGbl_INTR_STS;
                 } T32avioGbl_INTR_STS;
    typedef union  T32avioGbl_DAIF_DRT_CTRL
          { UNSG32 u32;
            struct w32avioGbl_DAIF_DRT_CTRL;
                 } T32avioGbl_DAIF_DRT_CTRL;
    typedef union  T32avioGbl_DCLS_CTRL
          { UNSG32 u32;
            struct w32avioGbl_DCLS_CTRL;
                 } T32avioGbl_DCLS_CTRL;
    typedef union  T32avioGbl_SWRST_CTRL
          { UNSG32 u32;
            struct w32avioGbl_SWRST_CTRL;
                 } T32avioGbl_SWRST_CTRL;
    typedef union  T32avioGbl_LCDC_CTRL
          { UNSG32 u32;
            struct w32avioGbl_LCDC_CTRL;
                 } T32avioGbl_LCDC_CTRL;
    typedef union  T32avioGbl_LCDC2_CTRL
          { UNSG32 u32;
            struct w32avioGbl_LCDC2_CTRL;
                 } T32avioGbl_LCDC2_CTRL;
    typedef union  T32avioGbl_PTRACK_CTRL
          { UNSG32 u32;
            struct w32avioGbl_PTRACK_CTRL;
                 } T32avioGbl_PTRACK_CTRL;
    typedef union  T32avioGbl_MIPI_CTRL
          { UNSG32 u32;
            struct w32avioGbl_MIPI_CTRL;
                 } T32avioGbl_MIPI_CTRL;
    typedef union  T32avioGbl_MIPI_CTRL1
          { UNSG32 u32;
            struct w32avioGbl_MIPI_CTRL1;
                 } T32avioGbl_MIPI_CTRL1;
    typedef union  T32avioGbl_MIPI_CTRL_STS
          { UNSG32 u32;
            struct w32avioGbl_MIPI_CTRL_STS;
                 } T32avioGbl_MIPI_CTRL_STS;
    typedef union  TavioGbl_AVPLLA_CLK_EN
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_AVPLLA_CLK_EN;
                   };
                 } TavioGbl_AVPLLA_CLK_EN;
    typedef union  TavioGbl_SWPDWN_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_SWPDWN_CTRL;
                   };
                 } TavioGbl_SWPDWN_CTRL;
    typedef union  TavioGbl_RWTC_31to0
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_RWTC_31to0;
                   };
                 } TavioGbl_RWTC_31to0;
    typedef union  TavioGbl_RWTC_57to32
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_RWTC_57to32;
                   };
                 } TavioGbl_RWTC_57to32;
    typedef union  TavioGbl_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_CTRL;
                   };
                 } TavioGbl_CTRL;
    typedef union  TavioGbl_CTRL0
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_CTRL0;
                   };
                 } TavioGbl_CTRL0;
    typedef union  TavioGbl_GIC_INT
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_GIC_INT;
                   };
                 } TavioGbl_GIC_INT;
    typedef union  TavioGbl_INTR_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_INTR_CTRL;
                   };
                 } TavioGbl_INTR_CTRL;
    typedef union  TavioGbl_INTR_STS
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_INTR_STS;
                   };
                 } TavioGbl_INTR_STS;
    typedef union  TavioGbl_DAIF_DRT_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_DAIF_DRT_CTRL;
                   };
                 } TavioGbl_DAIF_DRT_CTRL;
    typedef union  TavioGbl_DCLS_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_DCLS_CTRL;
                   };
                 } TavioGbl_DCLS_CTRL;
    typedef union  TavioGbl_SWRST_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_SWRST_CTRL;
                   };
                 } TavioGbl_SWRST_CTRL;
    typedef union  TavioGbl_LCDC_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_LCDC_CTRL;
                   };
                 } TavioGbl_LCDC_CTRL;
    typedef union  TavioGbl_LCDC2_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_LCDC2_CTRL;
                   };
                 } TavioGbl_LCDC2_CTRL;
    typedef union  TavioGbl_PTRACK_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_PTRACK_CTRL;
                   };
                 } TavioGbl_PTRACK_CTRL;
    typedef union  TavioGbl_MIPI_CTRL
          { UNSG32 u32[2];
            struct {
            struct w32avioGbl_MIPI_CTRL;
            struct w32avioGbl_MIPI_CTRL1;
                   };
                 } TavioGbl_MIPI_CTRL;
    typedef union  TavioGbl_MIPI_CTRL_STS
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_MIPI_CTRL_STS;
                   };
                 } TavioGbl_MIPI_CTRL_STS;
     SIGN32 avioGbl_drvrd(SIE_avioGbl *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 avioGbl_drvwr(SIE_avioGbl *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void avioGbl_reset(SIE_avioGbl *p);
     SIGN32 avioGbl_cmp  (SIE_avioGbl *p, SIE_avioGbl *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define avioGbl_check(p,pie,pfx,hLOG) avioGbl_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define avioGbl_print(p,    pfx,hLOG) avioGbl_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
