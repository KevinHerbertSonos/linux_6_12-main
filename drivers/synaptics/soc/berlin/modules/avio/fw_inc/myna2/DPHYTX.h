// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Synaptics Incorporated
 */

#ifndef DPHYTX_h
#define DPHYTX_h (){}
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
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
