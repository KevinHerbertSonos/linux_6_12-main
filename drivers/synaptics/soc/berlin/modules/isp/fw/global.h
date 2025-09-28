/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef global_h
#define global_h (){}
//#include "ctypes.h"
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
#ifndef h_pwrOff
#define h_pwrOff (){}
#define     RA_pwrOff_ctrl                                 0x0000
#define        pwrOff_ctrl_iso_eN_enable                                0x0
#define        pwrOff_ctrl_iso_eN_disable                               0x1
#define        pwrOff_ctrl_pwrSwitchCtrl_PWROFF                         0x0
#define        pwrOff_ctrl_pwrSwitchCtrl_PWRON                          0x3
#define        pwrOff_ctrl_pwrDomainRstN_enable                         0x0
#define        pwrOff_ctrl_pwrDomainRstN_disable                        0x1
#define     RA_pwrOff_status                               0x0004
typedef struct SIE_pwrOff {
#define     w32pwrOff_ctrl                                 {\
	UNSG32 uctrl_iso_eN                                :  1;\
	UNSG32 uctrl_pwrSwitchCtrl                         :  2;\
	UNSG32 uctrl_pwrDomainRstN                         :  1;\
	UNSG32 RSVDx0_b4                                   : 28;\
}
union { UNSG32 u32pwrOff_ctrl;
	struct w32pwrOff_ctrl;
};
#define     w32pwrOff_status                               {\
	UNSG32 ustatus_pwrStatus                           :  2;\
	UNSG32 RSVDx4_b2                                   : 30;\
}
union { UNSG32 u32pwrOff_status;
	struct w32pwrOff_status;
};
} SIE_pwrOff;
typedef union  T32pwrOff_ctrl
{ UNSG32 u32;
	struct w32pwrOff_ctrl;
} T32pwrOff_ctrl;
typedef union  T32pwrOff_status
{ UNSG32 u32;
	struct w32pwrOff_status;
} T32pwrOff_status;
typedef union  TpwrOff_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32pwrOff_ctrl;
	};
} TpwrOff_ctrl;
typedef union  TpwrOff_status
{ UNSG32 u32[1];
	struct {
		struct w32pwrOff_status;
	};
} TpwrOff_status;
SIGN32 pwrOff_drvrd(SIE_pwrOff *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 pwrOff_drvwr(SIE_pwrOff *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void pwrOff_reset(SIE_pwrOff *p);
SIGN32 pwrOff_cmp  (SIE_pwrOff *p, SIE_pwrOff *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define pwrOff_check(p,pie,pfx,hLOG) pwrOff_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define pwrOff_print(p,    pfx,hLOG) pwrOff_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_pwrOn
#define h_pwrOn (){}
#define     RA_pwrOn_ctrl                                  0x0000
#define        pwrOn_ctrl_iso_eN_enable                                 0x0
#define        pwrOn_ctrl_iso_eN_disable                                0x1
#define     RA_pwrOn_status                                0x0004
typedef struct SIE_pwrOn {
#define     w32pwrOn_ctrl                                  {\
	UNSG32 uctrl_iso_eN                                :  1;\
	UNSG32 uctrl_pwrSwitchCtrl                         :  2;\
	UNSG32 uctrl_pwrDomainRstN                         :  1;\
	UNSG32 RSVDx0_b4                                   : 28;\
}
union { UNSG32 u32pwrOn_ctrl;
	struct w32pwrOn_ctrl;
};
#define     w32pwrOn_status                                {\
	UNSG32 ustatus_pwrStatus                           :  2;\
	UNSG32 RSVDx4_b2                                   : 30;\
}
union { UNSG32 u32pwrOn_status;
	struct w32pwrOn_status;
};
} SIE_pwrOn;
typedef union  T32pwrOn_ctrl
{ UNSG32 u32;
	struct w32pwrOn_ctrl;
} T32pwrOn_ctrl;
typedef union  T32pwrOn_status
{ UNSG32 u32;
	struct w32pwrOn_status;
} T32pwrOn_status;
typedef union  TpwrOn_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32pwrOn_ctrl;
	};
} TpwrOn_ctrl;
typedef union  TpwrOn_status
{ UNSG32 u32[1];
	struct {
		struct w32pwrOn_status;
	};
} TpwrOn_status;
SIGN32 pwrOn_drvrd(SIE_pwrOn *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 pwrOn_drvwr(SIE_pwrOn *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void pwrOn_reset(SIE_pwrOn *p);
SIGN32 pwrOn_cmp  (SIE_pwrOn *p, SIE_pwrOn *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define pwrOn_check(p,pie,pfx,hLOG) pwrOn_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define pwrOn_print(p,    pfx,hLOG) pwrOn_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_pwrOn_iso
#define h_pwrOn_iso (){}
#define     RA_pwrOn_iso_ctrl                              0x0000
#define        pwrOn_iso_ctrl_iso_eN_enable                             0x0
#define        pwrOn_iso_ctrl_iso_eN_disable                            0x1
#define     RA_pwrOn_iso_status                            0x0004
typedef struct SIE_pwrOn_iso {
#define     w32pwrOn_iso_ctrl                              {\
	UNSG32 uctrl_iso_eN                                :  1;\
	UNSG32 uctrl_pwrSwitchCtrl                         :  2;\
	UNSG32 uctrl_pwrDomainRstN                         :  1;\
	UNSG32 RSVDx0_b4                                   : 28;\
}
union { UNSG32 u32pwrOn_iso_ctrl;
	struct w32pwrOn_iso_ctrl;
};
#define     w32pwrOn_iso_status                            {\
	UNSG32 ustatus_pwrStatus                           :  2;\
	UNSG32 ustatus_IP_IDLE                             :  1;\
	UNSG32 RSVDx4_b3                                   : 29;\
}
union { UNSG32 u32pwrOn_iso_status;
	struct w32pwrOn_iso_status;
};
} SIE_pwrOn_iso;
typedef union  T32pwrOn_iso_ctrl
{ UNSG32 u32;
	struct w32pwrOn_iso_ctrl;
} T32pwrOn_iso_ctrl;
typedef union  T32pwrOn_iso_status
{ UNSG32 u32;
	struct w32pwrOn_iso_status;
} T32pwrOn_iso_status;
typedef union  TpwrOn_iso_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32pwrOn_iso_ctrl;
	};
} TpwrOn_iso_ctrl;
typedef union  TpwrOn_iso_status
{ UNSG32 u32[1];
	struct {
		struct w32pwrOn_iso_status;
	};
} TpwrOn_iso_status;
SIGN32 pwrOn_iso_drvrd(SIE_pwrOn_iso *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 pwrOn_iso_drvwr(SIE_pwrOn_iso *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void pwrOn_iso_reset(SIE_pwrOn_iso *p);
SIGN32 pwrOn_iso_cmp  (SIE_pwrOn_iso *p, SIE_pwrOn_iso *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define pwrOn_iso_check(p,pie,pfx,hLOG) pwrOn_iso_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define pwrOn_iso_print(p,    pfx,hLOG) pwrOn_iso_cmp(p,0,  pfx,(void*)(hLOG),0,0)
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
#ifndef h_clkD1
#define h_clkD1 (){}
#define     RA_clkD1_ctrl                                  0x0000
#define        clkD1_ctrl_ClkEn_enable                                  0x1
#define        clkD1_ctrl_ClkEn_disable                                 0x0
#define        clkD1_ctrl_ClkPllSel_CLKSRC0                             0x0
#define        clkD1_ctrl_ClkPllSel_CLKSRC1                             0x1
#define        clkD1_ctrl_ClkPllSel_CLKSRC2                             0x2
#define        clkD1_ctrl_ClkPllSel_CLKSRC3                             0x3
#define        clkD1_ctrl_ClkPllSel_CLKSRC4                             0x4
#define        clkD1_ctrl_ClkPllSwitch_SYSPLL                           0x0
#define        clkD1_ctrl_ClkPllSwitch_ALTPLL                           0x1
#define        clkD1_ctrl_ClkSwitch_SrcClk                              0x0
#define        clkD1_ctrl_ClkSwitch_DivClk                              0x1
#define        clkD1_ctrl_ClkD3Switch_NonDiv3Clk                        0x0
#define        clkD1_ctrl_ClkD3Switch_Div3Clk                           0x1
#define        clkD1_ctrl_ClkSel_d2                                     0x1
#define        clkD1_ctrl_ClkSel_d4                                     0x2
#define        clkD1_ctrl_ClkSel_d6                                     0x3
#define        clkD1_ctrl_ClkSel_d8                                     0x4
#define        clkD1_ctrl_ClkSel_d12                                    0x5
typedef struct SIE_clkD1 {
#define     w32clkD1_ctrl                                  {\
	UNSG32 uctrl_ClkEn                                 :  1;\
	UNSG32 uctrl_ClkPllSel                             :  3;\
	UNSG32 uctrl_ClkPllSwitch                          :  1;\
	UNSG32 uctrl_ClkSwitch                             :  1;\
	UNSG32 uctrl_ClkD3Switch                           :  1;\
	UNSG32 uctrl_ClkSel                                :  3;\
	UNSG32 RSVDx0_b10                                  : 22;\
}
union { UNSG32 u32clkD1_ctrl;
	struct w32clkD1_ctrl;
};
} SIE_clkD1;
typedef union  T32clkD1_ctrl
{ UNSG32 u32;
	struct w32clkD1_ctrl;
} T32clkD1_ctrl;
typedef union  TclkD1_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32clkD1_ctrl;
	};
} TclkD1_ctrl;
SIGN32 clkD1_drvrd(SIE_clkD1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 clkD1_drvwr(SIE_clkD1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void clkD1_reset(SIE_clkD1 *p);
SIGN32 clkD1_cmp  (SIE_clkD1 *p, SIE_clkD1 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define clkD1_check(p,pie,pfx,hLOG) clkD1_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define clkD1_print(p,    pfx,hLOG) clkD1_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_clkD2
#define h_clkD2 (){}
#define     RA_clkD2_ctrl                                  0x0000
#define        clkD2_ctrl_ClkEn_enable                                  0x1
#define        clkD2_ctrl_ClkEn_disable                                 0x0
#define        clkD2_ctrl_ClkPllSel_CLKSRC0                             0x0
#define        clkD2_ctrl_ClkPllSel_CLKSRC1                             0x1
#define        clkD2_ctrl_ClkPllSel_CLKSRC2                             0x2
#define        clkD2_ctrl_ClkPllSel_CLKSRC3                             0x3
#define        clkD2_ctrl_ClkPllSel_CLKSRC4                             0x4
#define        clkD2_ctrl_ClkPllSwitch_SYSPLL                           0x0
#define        clkD2_ctrl_ClkPllSwitch_ALTPLL                           0x1
#define        clkD2_ctrl_ClkSwitch_SrcClk                              0x0
#define        clkD2_ctrl_ClkSwitch_DivClk                              0x1
#define        clkD2_ctrl_ClkD3Switch_NonDiv3Clk                        0x0
#define        clkD2_ctrl_ClkD3Switch_Div3Clk                           0x1
#define        clkD2_ctrl_ClkSel_d2                                     0x1
#define        clkD2_ctrl_ClkSel_d4                                     0x2
#define        clkD2_ctrl_ClkSel_d6                                     0x3
#define        clkD2_ctrl_ClkSel_d8                                     0x4
#define        clkD2_ctrl_ClkSel_d12                                    0x5
#define        clkD2_ctrl_ClkSel_d24                                    0x6
#define        clkD2_ctrl_ClkSel_d48                                    0x7
typedef struct SIE_clkD2 {
#define     w32clkD2_ctrl                                  {\
	UNSG32 uctrl_ClkEn                                 :  1;\
	UNSG32 uctrl_ClkPllSel                             :  3;\
	UNSG32 uctrl_ClkPllSwitch                          :  1;\
	UNSG32 uctrl_ClkSwitch                             :  1;\
	UNSG32 uctrl_ClkD3Switch                           :  1;\
	UNSG32 uctrl_ClkSel                                :  3;\
	UNSG32 RSVDx0_b10                                  : 22;\
}
union { UNSG32 u32clkD2_ctrl;
	struct w32clkD2_ctrl;
};
} SIE_clkD2;
typedef union  T32clkD2_ctrl
{ UNSG32 u32;
	struct w32clkD2_ctrl;
} T32clkD2_ctrl;
typedef union  TclkD2_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32clkD2_ctrl;
	};
} TclkD2_ctrl;
SIGN32 clkD2_drvrd(SIE_clkD2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 clkD2_drvwr(SIE_clkD2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void clkD2_reset(SIE_clkD2 *p);
SIGN32 clkD2_cmp  (SIE_clkD2 *p, SIE_clkD2 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define clkD2_check(p,pie,pfx,hLOG) clkD2_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define clkD2_print(p,    pfx,hLOG) clkD2_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_clkD4
#define h_clkD4 (){}
#define     RA_clkD4_ctrl                                  0x0000
#define        clkD4_ctrl_ClkEn_enable                                  0x1
#define        clkD4_ctrl_ClkEn_disable                                 0x0
#define        clkD4_ctrl_ClkPllSel_CLKSRC0                             0x0
#define        clkD4_ctrl_ClkPllSel_CLKSRC1                             0x1
#define        clkD4_ctrl_ClkPllSel_CLKSRC2                             0x2
#define        clkD4_ctrl_ClkPllSel_CLKSRC3                             0x3
#define        clkD4_ctrl_ClkPllSel_CLKSRC4                             0x4
#define        clkD4_ctrl_ClkPllSwitch_SYSPLL                           0x0
#define        clkD4_ctrl_ClkPllSwitch_ALTPLL                           0x1
#define        clkD4_ctrl_ClkSwitch_SrcClk                              0x0
#define        clkD4_ctrl_ClkSwitch_DivClk                              0x1
#define        clkD4_ctrl_ClkD3Switch_NonDiv3Clk                        0x0
#define        clkD4_ctrl_ClkD3Switch_Div3Clk                           0x1
#define        clkD4_ctrl_ClkSel_d2                                     0x1
#define        clkD4_ctrl_ClkSel_d4                                     0x2
#define        clkD4_ctrl_ClkSel_d6                                     0x3
#define        clkD4_ctrl_ClkSel_d8                                     0x4
#define        clkD4_ctrl_ClkSel_d12                                    0x5
typedef struct SIE_clkD4 {
#define     w32clkD4_ctrl                                  {\
	UNSG32 uctrl_ClkEn                                 :  1;\
	UNSG32 uctrl_ClkPllSel                             :  3;\
	UNSG32 uctrl_ClkPllSwitch                          :  1;\
	UNSG32 uctrl_ClkSwitch                             :  1;\
	UNSG32 uctrl_ClkD3Switch                           :  1;\
	UNSG32 uctrl_ClkSel                                :  3;\
	UNSG32 RSVDx0_b10                                  : 22;\
}
union { UNSG32 u32clkD4_ctrl;
	struct w32clkD4_ctrl;
};
} SIE_clkD4;
typedef union  T32clkD4_ctrl
{ UNSG32 u32;
	struct w32clkD4_ctrl;
} T32clkD4_ctrl;
typedef union  TclkD4_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32clkD4_ctrl;
	};
} TclkD4_ctrl;
SIGN32 clkD4_drvrd(SIE_clkD4 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 clkD4_drvwr(SIE_clkD4 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void clkD4_reset(SIE_clkD4 *p);
SIGN32 clkD4_cmp  (SIE_clkD4 *p, SIE_clkD4 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define clkD4_check(p,pie,pfx,hLOG) clkD4_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define clkD4_print(p,    pfx,hLOG) clkD4_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_clkD6
#define h_clkD6 (){}
#define     RA_clkD6_ctrl                                  0x0000
#define        clkD6_ctrl_ClkEn_enable                                  0x1
#define        clkD6_ctrl_ClkEn_disable                                 0x0
#define        clkD6_ctrl_ClkPllSel_CLKSRC0                             0x0
#define        clkD6_ctrl_ClkPllSel_CLKSRC1                             0x1
#define        clkD6_ctrl_ClkPllSel_CLKSRC2                             0x2
#define        clkD6_ctrl_ClkPllSel_CLKSRC3                             0x3
#define        clkD6_ctrl_ClkPllSel_CLKSRC4                             0x4
#define        clkD6_ctrl_ClkPllSwitch_SYSPLL                           0x0
#define        clkD6_ctrl_ClkPllSwitch_ALTPLL                           0x1
#define        clkD6_ctrl_ClkSwitch_SrcClk                              0x0
#define        clkD6_ctrl_ClkSwitch_DivClk                              0x1
#define        clkD6_ctrl_ClkD3Switch_NonDiv3Clk                        0x0
#define        clkD6_ctrl_ClkD3Switch_Div3Clk                           0x1
#define        clkD6_ctrl_ClkSel_d2                                     0x1
#define        clkD6_ctrl_ClkSel_d4                                     0x2
#define        clkD6_ctrl_ClkSel_d6                                     0x3
#define        clkD6_ctrl_ClkSel_d8                                     0x4
#define        clkD6_ctrl_ClkSel_d12                                    0x5
typedef struct SIE_clkD6 {
#define     w32clkD6_ctrl                                  {\
	UNSG32 uctrl_ClkEn                                 :  1;\
	UNSG32 uctrl_ClkPllSel                             :  3;\
	UNSG32 uctrl_ClkPllSwitch                          :  1;\
	UNSG32 uctrl_ClkSwitch                             :  1;\
	UNSG32 uctrl_ClkD3Switch                           :  1;\
	UNSG32 uctrl_ClkSel                                :  3;\
	UNSG32 RSVDx0_b10                                  : 22;\
}
union { UNSG32 u32clkD6_ctrl;
	struct w32clkD6_ctrl;
};
} SIE_clkD6;
typedef union  T32clkD6_ctrl
{ UNSG32 u32;
	struct w32clkD6_ctrl;
} T32clkD6_ctrl;
typedef union  TclkD6_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32clkD6_ctrl;
	};
} TclkD6_ctrl;
SIGN32 clkD6_drvrd(SIE_clkD6 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 clkD6_drvwr(SIE_clkD6 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void clkD6_reset(SIE_clkD6 *p);
SIGN32 clkD6_cmp  (SIE_clkD6 *p, SIE_clkD6 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define clkD6_check(p,pie,pfx,hLOG) clkD6_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define clkD6_print(p,    pfx,hLOG) clkD6_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_clkD8
#define h_clkD8 (){}
#define     RA_clkD8_ctrl                                  0x0000
#define        clkD8_ctrl_ClkEn_enable                                  0x1
#define        clkD8_ctrl_ClkEn_disable                                 0x0
#define        clkD8_ctrl_ClkPllSel_CLKSRC0                             0x0
#define        clkD8_ctrl_ClkPllSel_CLKSRC1                             0x1
#define        clkD8_ctrl_ClkPllSel_CLKSRC2                             0x2
#define        clkD8_ctrl_ClkPllSel_CLKSRC3                             0x3
#define        clkD8_ctrl_ClkPllSel_CLKSRC4                             0x4
#define        clkD8_ctrl_ClkPllSwitch_SYSPLL                           0x0
#define        clkD8_ctrl_ClkPllSwitch_ALTPLL                           0x1
#define        clkD8_ctrl_ClkSwitch_SrcClk                              0x0
#define        clkD8_ctrl_ClkSwitch_DivClk                              0x1
#define        clkD8_ctrl_ClkD3Switch_NonDiv3Clk                        0x0
#define        clkD8_ctrl_ClkD3Switch_Div3Clk                           0x1
#define        clkD8_ctrl_ClkSel_d2                                     0x1
#define        clkD8_ctrl_ClkSel_d4                                     0x2
#define        clkD8_ctrl_ClkSel_d6                                     0x3
#define        clkD8_ctrl_ClkSel_d8                                     0x4
#define        clkD8_ctrl_ClkSel_d12                                    0x5
typedef struct SIE_clkD8 {
#define     w32clkD8_ctrl                                  {\
	UNSG32 uctrl_ClkEn                                 :  1;\
	UNSG32 uctrl_ClkPllSel                             :  3;\
	UNSG32 uctrl_ClkPllSwitch                          :  1;\
	UNSG32 uctrl_ClkSwitch                             :  1;\
	UNSG32 uctrl_ClkD3Switch                           :  1;\
	UNSG32 uctrl_ClkSel                                :  3;\
	UNSG32 RSVDx0_b10                                  : 22;\
}
union { UNSG32 u32clkD8_ctrl;
	struct w32clkD8_ctrl;
};
} SIE_clkD8;
typedef union  T32clkD8_ctrl
{ UNSG32 u32;
	struct w32clkD8_ctrl;
} T32clkD8_ctrl;
typedef union  TclkD8_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32clkD8_ctrl;
	};
} TclkD8_ctrl;
SIGN32 clkD8_drvrd(SIE_clkD8 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 clkD8_drvwr(SIE_clkD8 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void clkD8_reset(SIE_clkD8 *p);
SIGN32 clkD8_cmp  (SIE_clkD8 *p, SIE_clkD8 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define clkD8_check(p,pie,pfx,hLOG) clkD8_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define clkD8_print(p,    pfx,hLOG) clkD8_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_clkD12
#define h_clkD12 (){}
#define     RA_clkD12_ctrl                                 0x0000
#define        clkD12_ctrl_ClkEn_enable                                 0x1
#define        clkD12_ctrl_ClkEn_disable                                0x0
#define        clkD12_ctrl_ClkPllSel_CLKSRC0                            0x0
#define        clkD12_ctrl_ClkPllSel_CLKSRC1                            0x1
#define        clkD12_ctrl_ClkPllSel_CLKSRC2                            0x2
#define        clkD12_ctrl_ClkPllSel_CLKSRC3                            0x3
#define        clkD12_ctrl_ClkPllSel_CLKSRC4                            0x4
#define        clkD12_ctrl_ClkPllSwitch_SYSPLL                          0x0
#define        clkD12_ctrl_ClkPllSwitch_ALTPLL                          0x1
#define        clkD12_ctrl_ClkSwitch_SrcClk                             0x0
#define        clkD12_ctrl_ClkSwitch_DivClk                             0x1
#define        clkD12_ctrl_ClkD3Switch_NonDiv3Clk                       0x0
#define        clkD12_ctrl_ClkD3Switch_Div3Clk                          0x1
#define        clkD12_ctrl_ClkSel_d2                                    0x1
#define        clkD12_ctrl_ClkSel_d4                                    0x2
#define        clkD12_ctrl_ClkSel_d6                                    0x3
#define        clkD12_ctrl_ClkSel_d8                                    0x4
#define        clkD12_ctrl_ClkSel_d12                                   0x5
typedef struct SIE_clkD12 {
#define     w32clkD12_ctrl                                 {\
	UNSG32 uctrl_ClkEn                                 :  1;\
	UNSG32 uctrl_ClkPllSel                             :  3;\
	UNSG32 uctrl_ClkPllSwitch                          :  1;\
	UNSG32 uctrl_ClkSwitch                             :  1;\
	UNSG32 uctrl_ClkD3Switch                           :  1;\
	UNSG32 uctrl_ClkSel                                :  3;\
	UNSG32 RSVDx0_b10                                  : 22;\
}
union { UNSG32 u32clkD12_ctrl;
	struct w32clkD12_ctrl;
};
} SIE_clkD12;
typedef union  T32clkD12_ctrl
{ UNSG32 u32;
	struct w32clkD12_ctrl;
} T32clkD12_ctrl;
typedef union  TclkD12_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32clkD12_ctrl;
	};
} TclkD12_ctrl;
SIGN32 clkD12_drvrd(SIE_clkD12 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 clkD12_drvwr(SIE_clkD12 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void clkD12_reset(SIE_clkD12 *p);
SIGN32 clkD12_cmp  (SIE_clkD12 *p, SIE_clkD12 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define clkD12_check(p,pie,pfx,hLOG) clkD12_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define clkD12_print(p,    pfx,hLOG) clkD12_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_clkD12_ENOFF
#define h_clkD12_ENOFF (){}
#define     RA_clkD12_ENOFF_ctrl                           0x0000
#define        clkD12_ENOFF_ctrl_ClkEn_enable                           0x1
#define        clkD12_ENOFF_ctrl_ClkEn_disable                          0x0
#define        clkD12_ENOFF_ctrl_ClkPllSel_CLKSRC0                      0x0
#define        clkD12_ENOFF_ctrl_ClkPllSel_CLKSRC1                      0x1
#define        clkD12_ENOFF_ctrl_ClkPllSel_CLKSRC2                      0x2
#define        clkD12_ENOFF_ctrl_ClkPllSel_CLKSRC3                      0x3
#define        clkD12_ENOFF_ctrl_ClkPllSel_CLKSRC4                      0x4
#define        clkD12_ENOFF_ctrl_ClkPllSwitch_SYSPLL                    0x0
#define        clkD12_ENOFF_ctrl_ClkPllSwitch_ALTPLL                    0x1
#define        clkD12_ENOFF_ctrl_ClkSwitch_SrcClk                       0x0
#define        clkD12_ENOFF_ctrl_ClkSwitch_DivClk                       0x1
#define        clkD12_ENOFF_ctrl_ClkD3Switch_NonDiv3Clk                 0x0
#define        clkD12_ENOFF_ctrl_ClkD3Switch_Div3Clk                    0x1
#define        clkD12_ENOFF_ctrl_ClkSel_d2                              0x1
#define        clkD12_ENOFF_ctrl_ClkSel_d4                              0x2
#define        clkD12_ENOFF_ctrl_ClkSel_d6                              0x3
#define        clkD12_ENOFF_ctrl_ClkSel_d8                              0x4
#define        clkD12_ENOFF_ctrl_ClkSel_d12                             0x5
typedef struct SIE_clkD12_ENOFF {
#define     w32clkD12_ENOFF_ctrl                           {\
	UNSG32 uctrl_ClkEn                                 :  1;\
	UNSG32 uctrl_ClkPllSel                             :  3;\
	UNSG32 uctrl_ClkPllSwitch                          :  1;\
	UNSG32 uctrl_ClkSwitch                             :  1;\
	UNSG32 uctrl_ClkD3Switch                           :  1;\
	UNSG32 uctrl_ClkSel                                :  3;\
	UNSG32 RSVDx0_b10                                  : 22;\
}
union { UNSG32 u32clkD12_ENOFF_ctrl;
	struct w32clkD12_ENOFF_ctrl;
};
} SIE_clkD12_ENOFF;
typedef union  T32clkD12_ENOFF_ctrl
{ UNSG32 u32;
	struct w32clkD12_ENOFF_ctrl;
} T32clkD12_ENOFF_ctrl;
typedef union  TclkD12_ENOFF_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32clkD12_ENOFF_ctrl;
	};
} TclkD12_ENOFF_ctrl;
SIGN32 clkD12_ENOFF_drvrd(SIE_clkD12_ENOFF *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 clkD12_ENOFF_drvwr(SIE_clkD12_ENOFF *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void clkD12_ENOFF_reset(SIE_clkD12_ENOFF *p);
SIGN32 clkD12_ENOFF_cmp  (SIE_clkD12_ENOFF *p, SIE_clkD12_ENOFF *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define clkD12_ENOFF_check(p,pie,pfx,hLOG) clkD12_ENOFF_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define clkD12_ENOFF_print(p,    pfx,hLOG) clkD12_ENOFF_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_PERIF
#define h_PERIF (){}
#define     RA_PERIF_PHY_DBG_CTRL                          0x0000
#define     RA_PERIF_IP_CTRL                               0x0004
#define     RA_PERIF_RSERVED_CTRL0                         0x0008
#define     RA_PERIF_SDIO_TST_CTL                          0x000C
#define     RA_PERIF_EMMC_TST_CTL                          0x0010
typedef struct SIE_PERIF {
#define     w32PERIF_PHY_DBG_CTRL                          {\
	UNSG32 uPHY_DBG_CTRL_perif_dbg_sel                 :  3;\
	UNSG32 RSVDx0_b3                                   : 29;\
}
union { UNSG32 u32PERIF_PHY_DBG_CTRL;
	struct w32PERIF_PHY_DBG_CTRL;
};
#define     w32PERIF_IP_CTRL                               {\
	UNSG32 uIP_CTRL_RGMII_TXC_90DEG_SEL                :  1;\
	UNSG32 RSVDx4_b1                                   : 31;\
}
union { UNSG32 u32PERIF_IP_CTRL;
	struct w32PERIF_IP_CTRL;
};
#define     w32PERIF_RSERVED_CTRL0                         {\
	UNSG32 uRSERVED_CTRL0_ctrl                         : 32;\
}
union { UNSG32 u32PERIF_RSERVED_CTRL0;
	struct w32PERIF_RSERVED_CTRL0;
};
#define     w32PERIF_SDIO_TST_CTL                          {\
	UNSG32 uSDIO_TST_CTL_SDIO_PHY_TST_INTF_SEL         :  1;\
	UNSG32 uSDIO_TST_CTL_SDIO_PHY_TST_PAD_MODE         :  1;\
	UNSG32 uSDIO_TST_CTL_SDIO_PHY_TST_DLOUT_EN         :  1;\
	UNSG32 RSVDxC_b3                                   : 29;\
}
union { UNSG32 u32PERIF_SDIO_TST_CTL;
	struct w32PERIF_SDIO_TST_CTL;
};
#define     w32PERIF_EMMC_TST_CTL                          {\
	UNSG32 uEMMC_TST_CTL_EMMC_PHY_TST_INTF_SEL         :  1;\
	UNSG32 uEMMC_TST_CTL_EMMC_PHY_TST_PAD_MODE         :  1;\
	UNSG32 uEMMC_TST_CTL_EMMC_PHY_TST_DLOUT_EN         :  1;\
	UNSG32 uEMMC_TST_CTL_EMMC_PHY_TST_LBT_RESETN       :  1;\
	UNSG32 RSVDx10_b4                                  : 28;\
}
union { UNSG32 u32PERIF_EMMC_TST_CTL;
	struct w32PERIF_EMMC_TST_CTL;
};
} SIE_PERIF;
typedef union  T32PERIF_PHY_DBG_CTRL
{ UNSG32 u32;
	struct w32PERIF_PHY_DBG_CTRL;
} T32PERIF_PHY_DBG_CTRL;
typedef union  T32PERIF_IP_CTRL
{ UNSG32 u32;
	struct w32PERIF_IP_CTRL;
} T32PERIF_IP_CTRL;
typedef union  T32PERIF_RSERVED_CTRL0
{ UNSG32 u32;
	struct w32PERIF_RSERVED_CTRL0;
} T32PERIF_RSERVED_CTRL0;
typedef union  T32PERIF_SDIO_TST_CTL
{ UNSG32 u32;
	struct w32PERIF_SDIO_TST_CTL;
} T32PERIF_SDIO_TST_CTL;
typedef union  T32PERIF_EMMC_TST_CTL
{ UNSG32 u32;
	struct w32PERIF_EMMC_TST_CTL;
} T32PERIF_EMMC_TST_CTL;
typedef union  TPERIF_PHY_DBG_CTRL
{ UNSG32 u32[1];
	struct {
		struct w32PERIF_PHY_DBG_CTRL;
	};
} TPERIF_PHY_DBG_CTRL;
typedef union  TPERIF_IP_CTRL
{ UNSG32 u32[1];
	struct {
		struct w32PERIF_IP_CTRL;
	};
} TPERIF_IP_CTRL;
typedef union  TPERIF_RSERVED_CTRL0
{ UNSG32 u32[1];
	struct {
		struct w32PERIF_RSERVED_CTRL0;
	};
} TPERIF_RSERVED_CTRL0;
typedef union  TPERIF_SDIO_TST_CTL
{ UNSG32 u32[1];
	struct {
		struct w32PERIF_SDIO_TST_CTL;
	};
} TPERIF_SDIO_TST_CTL;
typedef union  TPERIF_EMMC_TST_CTL
{ UNSG32 u32[1];
	struct {
		struct w32PERIF_EMMC_TST_CTL;
	};
} TPERIF_EMMC_TST_CTL;
SIGN32 PERIF_drvrd(SIE_PERIF *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 PERIF_drvwr(SIE_PERIF *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void PERIF_reset(SIE_PERIF *p);
SIGN32 PERIF_cmp  (SIE_PERIF *p, SIE_PERIF *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define PERIF_check(p,pie,pfx,hLOG) PERIF_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define PERIF_print(p,    pfx,hLOG) PERIF_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_Gbl
#define h_Gbl (){}
#define     RA_Gbl_ProductId                               0x0000
#define     RA_Gbl_ProductId_ext                           0x0004
#define     RA_Gbl_INT_ID                                  0x0008
#define     RA_Gbl_bootStrap                               0x000C
#define        Gbl_bootStrap_bootSrc_ROM_SPI_BOOT                       0x0
#define        Gbl_bootStrap_bootSrc_ROM_NAND_BOOT                      0x1
#define        Gbl_bootStrap_bootSrc_ROM_EMMC_BOOT                      0x2
#define        Gbl_bootStrap_bootSrc_ROM_SPI_DIRECT_BOOT                0x3
#define        Gbl_bootStrap_cpuRstByps_CPU_INT_RST_BYPS                0x1
#define        Gbl_bootStrap_cpuRstByps_CPU_INT_RST_EN                  0x0
#define        Gbl_bootStrap_pllPwrDown_PWR_DOWN                        0x1
#define        Gbl_bootStrap_pllPwrDown_PWR_UP                          0x0
#define        Gbl_bootStrap_sysPll_0Byps_PLL_OUT                       0x0
#define        Gbl_bootStrap_sysPll_0Byps_BYPS                          0x1
#define        Gbl_bootStrap_sysPll_1Byps_PLL_OUT                       0x0
#define        Gbl_bootStrap_sysPll_1Byps_BYPS                          0x1
#define        Gbl_bootStrap_sysPll_2Byps_PLL_OUT                       0x0
#define        Gbl_bootStrap_sysPll_2Byps_BYPS                          0x1
#define        Gbl_bootStrap_memPllByps_PLL_OUT                         0x0
#define        Gbl_bootStrap_memPllByps_BYPS                            0x1
#define        Gbl_bootStrap_cpuPllByps_PLL_OUT                         0x0
#define        Gbl_bootStrap_cpuPllByps_BYPS                            0x1
#define        Gbl_bootStrap_emmcV12Enable_V1R8                         0x0
#define        Gbl_bootStrap_emmcV12Enable_V1R2                         0x1
#define        Gbl_bootStrap_ENG_EN_PRODUCTION_MODE                     0x0
#define        Gbl_bootStrap_ENG_EN_DEVELOPE_MODE                       0x1
#define        Gbl_bootStrap_LEGACY_BOOT_FAST_BOOT_MODE                 0x0
#define        Gbl_bootStrap_LEGACY_BOOT_LEGACY_BOOT_MODE               0x1
#define     RA_Gbl_bootStrapEn                             0x0010
#define        Gbl_bootStrapEn_cpuRstBypsEn_ENABLE                      0x1
#define        Gbl_bootStrapEn_cpuRstBypsEn_DISABLE                     0x0
#define        Gbl_bootStrapEn_pllPwrDownEn_ENABLE                      0x1
#define        Gbl_bootStrapEn_pllPwrDownEn_DISABLE                     0x0
#define        Gbl_bootStrapEn_sysPLL_0BypsEn_ENABLE                    0x1
#define        Gbl_bootStrapEn_sysPLL_0BypsEn_DISABLE                   0x0
#define        Gbl_bootStrapEn_memPLLBypsEn_ENABLE                      0x1
#define        Gbl_bootStrapEn_memPLLBypsEn_DISABLE                     0x0
#define        Gbl_bootStrapEn_cpuPLLBypsEn_ENABLE                      0x1
#define        Gbl_bootStrapEn_cpuPLLBypsEn_DISABLE                     0x0
#define        Gbl_bootStrapEn_legacyBootEn_ENABLE                      0x1
#define        Gbl_bootStrapEn_legacyBootEn_DISABLE                     0x0
#define        Gbl_bootStrapEn_sysPLL_1BypsEn_ENABLE                    0x1
#define        Gbl_bootStrapEn_sysPLL_1BypsEn_DISABLE                   0x0
#define        Gbl_bootStrapEn_sysPLL_2BypsEn_ENABLE                    0x1
#define        Gbl_bootStrapEn_sysPLL_2BypsEn_DISABLE                   0x0
#define     RA_Gbl_pkgSel                                  0x0014
#define     RA_Gbl_chipCntl                                0x0040
#define     RA_Gbl_stsCntl                                 0x0044
#define     RA_Gbl_sw_generic0                             0x0080
#define     RA_Gbl_sw_generic1                             0x0084
#define     RA_Gbl_sw_generic2                             0x0088
#define     RA_Gbl_sw_generic3                             0x008C
#define     RA_Gbl_RWTC_SOC                                0x0100
#define     RA_Gbl_SRAM_PWR_EMMC                           0x010C
#define     RA_Gbl_SRAM_PWR_SDIO                           0x0110
#define     RA_Gbl_SRAM_PWR_USB2                           0x0114
#define     RA_Gbl_SRAM_PWR_USB3                           0x0118
#define     RA_Gbl_SRAM_PWR_PCIE                           0x011C
#define     RA_Gbl_SRAM_PWR_GFX3D                          0x0120
#define     RA_Gbl_SRAM_PWR_V4G                            0x0124
#define     RA_Gbl_SRAM_PWR_H1                             0x0128
#define     RA_Gbl_SRAM_PWR_SISS                           0x012C
#define     RA_Gbl_SRAM_PWR_OVP                            0x0130
#define     RA_Gbl_SRAM_PWR_GE                             0x0134
#define     RA_Gbl_SRAM_PWR_PB                             0x0138
#define     RA_Gbl_SRAM_PWR_NPU                            0x013C
#define     RA_Gbl_SRAM_PWR_ISP                            0x0140
#define     RA_Gbl_SRAM_PWR_MC                             0x0144
#define     RA_Gbl_SRAM_PWR_ALM                            0x0148
#define     RA_Gbl_FPGAR                                   0x0198
#define     RA_Gbl_FPGARW                                  0x019C
#define     RA_Gbl_sysPll_0                                0x0200
#define     RA_Gbl_sysPll_1                                0x0220
#define     RA_Gbl_sysPll_2                                0x0240
#define     RA_Gbl_ResetTrigger                            0x0600
#define        Gbl_ResetTrigger_chipReset_assert                        0x1
#define        Gbl_ResetTrigger_chipReset_deassert                      0x0
#define        Gbl_ResetTrigger_socDdrSyncReset_assert                  0x1
#define        Gbl_ResetTrigger_socDdrSyncReset_deassert                0x0
#define     RA_Gbl_ResetStatus                             0x0604
#define        Gbl_ResetStatus_ChipResetStatus_asserted                 0x1
#define        Gbl_ResetStatus_ChipResetStatus_deasserted               0x0
#define        Gbl_ResetStatus_socDdrSyncResetStatus_asserted              0x1
#define        Gbl_ResetStatus_socDdrSyncResetStatus_deasserted              0x0
#define     RA_Gbl_WDTResetStatus                          0x060C
#define        Gbl_WDTResetStatus_wd0Status_asserted                    0x1
#define        Gbl_WDTResetStatus_wd0Status_deasserted                  0x0
#define        Gbl_WDTResetStatus_wd1Status_asserted                    0x1
#define        Gbl_WDTResetStatus_wd1Status_deasserted                  0x0
#define        Gbl_WDTResetStatus_wd2Status_asserted                    0x1
#define        Gbl_WDTResetStatus_wd2Status_deasserted                  0x0
#define     RA_Gbl_avioReset                               0x0620
#define        Gbl_avioReset_SyncReset_assert                           0x1
#define        Gbl_avioReset_SyncReset_deassert                         0x0
#define     RA_Gbl_avioResetStatus                         0x0624
#define        Gbl_avioResetStatus_SyncReset_assert                     0x1
#define        Gbl_avioResetStatus_SyncReset_deassert                   0x0
#define     RA_Gbl_gethRgmiiRstn                           0x0628
#define        Gbl_gethRgmiiRstn_SyncReset_assert                       0x1
#define        Gbl_gethRgmiiRstn_SyncReset_deassert                     0x0
#define     RA_Gbl_gethRgmiiResetStatus                    0x062C
#define        Gbl_gethRgmiiResetStatus_SyncReset_assert                0x1
#define        Gbl_gethRgmiiResetStatus_SyncReset_deassert              0x0
#define     RA_Gbl_gfx3DReset                              0x0630
#define        Gbl_gfx3DReset_SyncReset_assert                          0x1
#define        Gbl_gfx3DReset_SyncReset_deassert                        0x0
#define     RA_Gbl_gfx3DResetStatus                        0x0634
#define        Gbl_gfx3DResetStatus_SyncReset_assert                    0x1
#define        Gbl_gfx3DResetStatus_SyncReset_deassert                  0x0
#define     RA_Gbl_decoderReset                            0x0640
#define        Gbl_decoderReset_SyncReset_assert                        0x1
#define        Gbl_decoderReset_SyncReset_deassert                      0x0
#define     RA_Gbl_decoderResetStatus                      0x0644
#define        Gbl_decoderResetStatus_SyncReset_assert                  0x1
#define        Gbl_decoderResetStatus_SyncReset_deassert                0x0
#define     RA_Gbl_encoderReset                            0x0650
#define        Gbl_encoderReset_SyncReset_assert                        0x1
#define        Gbl_encoderReset_SyncReset_deassert                      0x0
#define     RA_Gbl_encoderResetStatus                      0x0654
#define        Gbl_encoderResetStatus_SyncReset_assert                  0x1
#define        Gbl_encoderResetStatus_SyncReset_deassert                0x0
#define     RA_Gbl_ovpReset                                0x0660
#define        Gbl_ovpReset_SyncReset_assert                            0x1
#define        Gbl_ovpReset_SyncReset_deassert                          0x0
#define     RA_Gbl_ovpResetStatus                          0x0664
#define        Gbl_ovpResetStatus_SyncReset_assert                      0x1
#define        Gbl_ovpResetStatus_SyncReset_deassert                    0x0
#define     RA_Gbl_npuReset                                0x0668
#define        Gbl_npuReset_SyncReset_assert                            0x1
#define        Gbl_npuReset_SyncReset_deassert                          0x0
#define     RA_Gbl_npuResetStatus                          0x066C
#define        Gbl_npuResetStatus_SyncReset_assert                      0x1
#define        Gbl_npuResetStatus_SyncReset_deassert                    0x0
#define     RA_Gbl_almGRstn                                0x0670
#define        Gbl_almGRstn_SyncReset_assert                            0x1
#define        Gbl_almGRstn_SyncReset_deassert                          0x0
#define     RA_Gbl_almResetStatus                          0x0674
#define        Gbl_almResetStatus_SyncReset_assert                      0x1
#define        Gbl_almResetStatus_SyncReset_deassert                    0x0
#define     RA_Gbl_perifReset                              0x0680
#define        Gbl_perifReset_SyncReset_assert                          0x1
#define        Gbl_perifReset_SyncReset_deassert                        0x0
#define        Gbl_perifReset_ahbApbSyncReset_assert                    0x1
#define        Gbl_perifReset_ahbApbSyncReset_deassert                  0x0
#define        Gbl_perifReset_sdioSyncReset_assert                      0x1
#define        Gbl_perifReset_sdioSyncReset_deassert                    0x0
#define        Gbl_perifReset_tspSyncReset_assert                       0x1
#define        Gbl_perifReset_tspSyncReset_deassert                     0x0
#define        Gbl_perifReset_tsSSSyncReset_assert                      0x1
#define        Gbl_perifReset_tsSSSyncReset_deassert                    0x0
#define        Gbl_perifReset_nskSyncReset_assert                       0x1
#define        Gbl_perifReset_nskSyncReset_deassert                     0x0
#define        Gbl_perifReset_nocsSyncReset_assert                      0x1
#define        Gbl_perifReset_nocsSyncReset_deassert                    0x0
#define        Gbl_perifReset_bcmSyncReset_assert                       0x1
#define        Gbl_perifReset_bcmSyncReset_deassert                     0x0
#define        Gbl_perifReset_atbSyncReset_assert                       0x1
#define        Gbl_perifReset_atbSyncReset_deassert                     0x0
#define        Gbl_perifReset_usb0SyncReset_assert                      0x1
#define        Gbl_perifReset_usb0SyncReset_deassert                    0x0
#define        Gbl_perifReset_usb3SyncReset_assert                      0x1
#define        Gbl_perifReset_usb3SyncReset_deassert                    0x0
#define        Gbl_perifReset_emmcSyncReset_assert                      0x1
#define        Gbl_perifReset_emmcSyncReset_deassert                    0x0
#define        Gbl_perifReset_pBridgeSyncReset_assert                   0x1
#define        Gbl_perifReset_pBridgeSyncReset_deassert                 0x0
#define        Gbl_perifReset_ifcpSyncReset_assert                      0x1
#define        Gbl_perifReset_ifcpSyncReset_deassert                    0x0
#define     RA_Gbl_perifResetStatus                        0x0684
#define        Gbl_perifResetStatus_SyncReset_assert                    0x1
#define        Gbl_perifResetStatus_SyncReset_deassert                  0x0
#define        Gbl_perifResetStatus_ahbApbSyncReset_assert              0x1
#define        Gbl_perifResetStatus_ahbApbSyncReset_deassert              0x0
#define        Gbl_perifResetStatus_sdioSyncReset_assert                0x1
#define        Gbl_perifResetStatus_sdioSyncReset_deassert              0x0
#define        Gbl_perifResetStatus_tspSyncReset_assert                 0x1
#define        Gbl_perifResetStatus_tspSyncReset_deassert               0x0
#define        Gbl_perifResetStatus_tsSSSyncReset_assert                0x1
#define        Gbl_perifResetStatus_tsSSSyncReset_deassert              0x0
#define        Gbl_perifResetStatus_nskSyncReset_assert                 0x1
#define        Gbl_perifResetStatus_nskSyncReset_deassert               0x0
#define        Gbl_perifResetStatus_nocsSyncReset_assert                0x1
#define        Gbl_perifResetStatus_nocsSyncReset_deassert              0x0
#define        Gbl_perifResetStatus_bcmSyncReset_assert                 0x1
#define        Gbl_perifResetStatus_bcmSyncReset_deassert               0x0
#define        Gbl_perifResetStatus_atbSyncReset_assert                 0x1
#define        Gbl_perifResetStatus_atbSyncReset_deassert               0x0
#define        Gbl_perifResetStatus_usb0SyncReset_assert                0x1
#define        Gbl_perifResetStatus_usb0SyncReset_deassert              0x0
#define        Gbl_perifResetStatus_usb3SyncReset_assert                0x1
#define        Gbl_perifResetStatus_usb3SyncReset_deassert              0x0
#define        Gbl_perifResetStatus_emmcSyncReset_assert                0x1
#define        Gbl_perifResetStatus_emmcSyncReset_deassert              0x0
#define        Gbl_perifResetStatus_pBridgeSyncReset_assert              0x1
#define        Gbl_perifResetStatus_pBridgeSyncReset_deassert              0x0
#define        Gbl_perifResetStatus_tsioSyncReset_assert                0x1
#define        Gbl_perifResetStatus_tsioSyncReset_deassert              0x0
#define        Gbl_perifResetStatus_ifcpSyncReset_assert                0x1
#define        Gbl_perifResetStatus_ifcpSyncReset_deassert              0x0
#define     RA_Gbl_perifStickyResetN                       0x0688
#define        Gbl_perifStickyResetN_usb3PhyRstn_asserted               0x0
#define        Gbl_perifStickyResetN_usb3PhyRstn_deasserted              0x1
#define        Gbl_perifStickyResetN_pcie0Rstn_asserted                 0x0
#define        Gbl_perifStickyResetN_pcie0Rstn_deasserted               0x1
#define        Gbl_perifStickyResetN_usb0PhyRstn_asserted               0x0
#define        Gbl_perifStickyResetN_usb0PhyRstn_deasserted              0x1
#define        Gbl_perifStickyResetN_usb0CoreRstn_asserted              0x0
#define        Gbl_perifStickyResetN_usb0CoreRstn_deasserted              0x1
#define        Gbl_perifStickyResetN_usb0MahbRstn_asserted              0x0
#define        Gbl_perifStickyResetN_usb0MahbRstn_deasserted              0x1
#define        Gbl_perifStickyResetN_sdioPhyRstn_asserted               0x0
#define        Gbl_perifStickyResetN_sdioPhyRstn_deasserted              0x1
#define        Gbl_perifStickyResetN_emmcPhyRstn_asserted               0x0
#define        Gbl_perifStickyResetN_emmcPhyRstn_deasserted              0x1
#define        Gbl_perifStickyResetN_pciePhyRstn_asserted               0x0
#define        Gbl_perifStickyResetN_pciePhyRstn_deasserted              0x1
#define        Gbl_perifStickyResetN_ispGRstn_asserted                  0x0
#define        Gbl_perifStickyResetN_ispGRstn_deasserted                0x1
#define     RA_Gbl_apbPerifResetTrigger                    0x068C
#define        Gbl_apbPerifResetTrigger_uart0SyncReset_assert              0x1
#define        Gbl_apbPerifResetTrigger_uart0SyncReset_deassert              0x0
#define        Gbl_apbPerifResetTrigger_uart1SyncReset_assert              0x1
#define        Gbl_apbPerifResetTrigger_uart1SyncReset_deassert              0x0
#define        Gbl_apbPerifResetTrigger_i2c0SyncReset_assert              0x1
#define        Gbl_apbPerifResetTrigger_i2c0SyncReset_deassert              0x0
#define        Gbl_apbPerifResetTrigger_i2c1SyncReset_assert              0x1
#define        Gbl_apbPerifResetTrigger_i2c1SyncReset_deassert              0x0
#define        Gbl_apbPerifResetTrigger_spiSyncReset_assert              0x1
#define        Gbl_apbPerifResetTrigger_spiSyncReset_deassert              0x0
#define        Gbl_apbPerifResetTrigger_timer0SyncReset_assert              0x1
#define        Gbl_apbPerifResetTrigger_timer0SyncReset_deassert              0x0
#define        Gbl_apbPerifResetTrigger_timer1SyncReset_assert              0x1
#define        Gbl_apbPerifResetTrigger_timer1SyncReset_deassert              0x0
#define     RA_Gbl_apbPerifResetStatus                     0x0690
#define        Gbl_apbPerifResetStatus_uart0SyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_uart0SyncResetStatus_deassert              0x0
#define        Gbl_apbPerifResetStatus_uart1SyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_uart1SyncResetStatus_deassert              0x0
#define        Gbl_apbPerifResetStatus_i2c0SyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_i2c0SyncResetStatus_deassert              0x0
#define        Gbl_apbPerifResetStatus_i2c1SyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_i2c1SyncResetStatus_deassert              0x0
#define        Gbl_apbPerifResetStatus_i2c2SyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_i2c2SyncResetStatus_deassert              0x0
#define        Gbl_apbPerifResetStatus_spiSyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_spiSyncResetStatus_deassert              0x0
#define        Gbl_apbPerifResetStatus_timer0SyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_timer0SyncResetStatus_deassert              0x0
#define        Gbl_apbPerifResetStatus_timer1SyncResetStatus_assert              0x1
#define        Gbl_apbPerifResetStatus_timer1SyncResetStatus_deassert              0x0
#define     RA_Gbl_clkEnable                               0x0700
#define        Gbl_clkEnable_usb0CoreClkEn_enable                       0x1
#define        Gbl_clkEnable_usb0CoreClkEn_disable                      0x0
#define        Gbl_clkEnable_sdioSysClkEn_enable                        0x1
#define        Gbl_clkEnable_sdioSysClkEn_disable                       0x0
#define        Gbl_clkEnable_pcie0SysClkEn_enable                       0x1
#define        Gbl_clkEnable_pcie0SysClkEn_disable                      0x0
#define        Gbl_clkEnable_emmcSysClkEn_enable                        0x1
#define        Gbl_clkEnable_emmcSysClkEn_disable                       0x0
#define        Gbl_clkEnable_pBridgeCoreClkEn_enable                    0x1
#define        Gbl_clkEnable_pBridgeCoreClkEn_disable                   0x0
#define        Gbl_clkEnable_npuAxiClkEn_enable                         0x1
#define        Gbl_clkEnable_npuAxiClkEn_disable                        0x0
#define        Gbl_clkEnable_gethRgmiiSysClkEn_enable                   0x1
#define        Gbl_clkEnable_gethRgmiiSysClkEn_disable                  0x0
#define     RA_Gbl_ClkSwitch                               0x0710
#define        Gbl_ClkSwitch_sysPLL_0SWBypass_refClk                    0x1
#define        Gbl_ClkSwitch_sysPLL_0SWBypass_pllClk                    0x0
#define        Gbl_ClkSwitch_sysPLL_1SWBypass_refClk                    0x1
#define        Gbl_ClkSwitch_sysPLL_1SWBypass_pllClk                    0x0
#define        Gbl_ClkSwitch_sysPLL_2SWBypass_refClk                    0x1
#define        Gbl_ClkSwitch_sysPLL_2SWBypass_pllClk                    0x0
#define        Gbl_ClkSwitch_memPLLSWBypass_refClk                      0x1
#define        Gbl_ClkSwitch_memPLLSWBypass_pllClk                      0x0
#define        Gbl_ClkSwitch_cpuPLLSWBypass_refClk                      0x1
#define        Gbl_ClkSwitch_cpuPLLSWBypass_pllClk                      0x0
#define     RA_Gbl_cpufastRefClk                           0x0720
#define     RA_Gbl_memfastRefClk                           0x0724
#define     RA_Gbl_cfgClk                                  0x0740
#define     RA_Gbl_perifSysClk                             0x0744
#define     RA_Gbl_atbClk                                  0x0748
#define     RA_Gbl_decoderClk                              0x074C
#define     RA_Gbl_encoderClk                              0x0754
#define     RA_Gbl_ovpCoreClk                              0x0758
#define     RA_Gbl_gfx3DCoreClk                            0x0760
#define     RA_Gbl_gfx3DSysClk                             0x0764
#define     RA_Gbl_avioSysClk                              0x0770
#define     RA_Gbl_vppSysClk                               0x0774
#define     RA_Gbl_hpiClk                                  0x0778
#define     RA_Gbl_avioBiuClk                              0x077C
#define     RA_Gbl_vipPipeClk                              0x0780
#define     RA_Gbl_avioFpll400_clk                         0x0784
#define     RA_Gbl_tspClk                                  0x0790
#define     RA_Gbl_tspRefClk                               0x0794
#define     RA_Gbl_nocsClk                                 0x079C
#define     RA_Gbl_apbCoreClk                              0x07A0
#define     RA_Gbl_emmcClk                                 0x07B0
#define     RA_Gbl_sd0Clk                                  0x07B4
#define     RA_Gbl_gethRgmiiClk                            0x07C0
#define     RA_Gbl_avioOvpClk                              0x07C4
#define     RA_Gbl_perifTestClk125mGroup0                  0x07E0
#define     RA_Gbl_usb2TestClk                             0x07F0
#define     RA_Gbl_perifTestClk250mGroup0                  0x07F4
#define     RA_Gbl_usb3CoreClk                             0x07F8
#define     RA_Gbl_bcmClk                                  0x0810
#define     RA_Gbl_vxSysClk                                0x0814
#define     RA_Gbl_npuClk                                  0x0818
#define     RA_Gbl_sissSysClk                              0x081C
#define     RA_Gbl_ifcpClk                                 0x0820
#define     RA_Gbl_issSysClk                               0x0824
#define     RA_Gbl_ispClk                                  0x0828
#define     RA_Gbl_ispBeClk                                0x082C
#define     RA_Gbl_ispDscClk                               0x0830
#define     RA_Gbl_ispCsi0Clk                              0x0834
#define     RA_Gbl_ispCsi1Clk                              0x0838
#define     RA_Gbl_hdmiRxrefClk                            0x083C
#define     RA_Gbl_mipiRxScanByteClk                       0x0840
#define     RA_Gbl_usb2TestClk480mGroup0                   0x0844
#define     RA_Gbl_usb2TestClk480mGroup1                   0x0848
#define     RA_Gbl_usb2TestClk480mGroup2                   0x084C
#define     RA_Gbl_usb2TestClk100mGroup0                   0x0850
#define     RA_Gbl_usb2TestClk100mGroup1                   0x0854
#define     RA_Gbl_usb2TestClk100mGroup2                   0x0858
#define     RA_Gbl_usb2TestClk100mGroup3                   0x085C
#define     RA_Gbl_usb2TestClk100mGroup4                   0x0860
#define     RA_Gbl_perifTestClk200mGroup0                  0x0864
#define     RA_Gbl_perifTestClk200mGroup1                  0x0868
#define     RA_Gbl_perifTestClk500mGroup0                  0x086C
#define     RA_Gbl_txClkEsc                                0x0870
#define     RA_Gbl_aioSysClk                               0x0874
#define     RA_Gbl_SECURE_SCAN_EN                          0x0900
#define     RA_Gbl_gfx3DDisRamClkGate                      0x0918
#define        Gbl_gfx3DDisRamClkGate_drcg_drcgActive                   0x1
#define        Gbl_gfx3DDisRamClkGate_drcg_drcgInactive                 0x0
#define     RA_Gbl_gic400_ctrl                             0x0960
#define     RA_Gbl_GFX_CTRL_0                              0x0970
#define     RA_Gbl_GFX_CTRL_1                              0x0974
#define     RA_Gbl_GFX_CTRL_2                              0x0978
#define     RA_Gbl_GFX_CTRL_3                              0x097C
#define     RA_Gbl_GFX_TIMER_VALUE                         0x0980
#define     RA_Gbl_GFX_TIMER_VALUE1                        0x0984
#define     RA_Gbl_GFX_STATUS                              0x098C
#define     RA_Gbl_USBOTG_REFCLK_CTRL0                     0x0990
#define     RA_Gbl_USBOTG_REFCLK_CTRL1                     0x0994
#define     RA_Gbl_PERIF                                   0x0A00
#define     RA_Gbl_pinMuxCntlBus                           0x8000
#define        Gbl_pinMuxCntlBus_SDIO_CDn_MODE_0                        0x0
#define        Gbl_pinMuxCntlBus_SDIO_CDn_MODE_1                        0x1
#define        Gbl_pinMuxCntlBus_SDIO_CDn_MODE_2                        0x2
#define        Gbl_pinMuxCntlBus_SDIO_WP_MODE_0                         0x0
#define        Gbl_pinMuxCntlBus_SDIO_WP_MODE_1                         0x1
#define        Gbl_pinMuxCntlBus_SDIO_WP_MODE_2                         0x2
#define        Gbl_pinMuxCntlBus_SPI1_SS0n_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_SPI1_SS0n_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_SPI1_SS1n_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_SPI1_SS1n_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_SPI1_SS1n_MODE_2                       0x2
#define        Gbl_pinMuxCntlBus_SPI1_SS1n_MODE_4                       0x4
#define        Gbl_pinMuxCntlBus_SPI1_SS1n_MODE_7                       0x7
#define        Gbl_pinMuxCntlBus_SPI1_SS2n_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_SPI1_SS2n_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_SPI1_SS2n_MODE_2                       0x2
#define        Gbl_pinMuxCntlBus_SPI1_SS2n_MODE_3                       0x3
#define        Gbl_pinMuxCntlBus_SPI1_SS2n_MODE_4                       0x4
#define        Gbl_pinMuxCntlBus_SPI1_SS2n_MODE_7                       0x7
#define        Gbl_pinMuxCntlBus_SPI1_SS3n_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_SPI1_SS3n_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_SPI1_SS3n_MODE_2                       0x2
#define        Gbl_pinMuxCntlBus_SPI1_SS3n_MODE_3                       0x3
#define        Gbl_pinMuxCntlBus_SPI1_SS3n_MODE_7                       0x7
#define        Gbl_pinMuxCntlBus_SPI1_SDO_MODE_0                        0x0
#define        Gbl_pinMuxCntlBus_SPI1_SDO_MODE_1                        0x1
#define        Gbl_pinMuxCntlBus_SPI1_SCLK_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_SPI1_SCLK_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_SPI1_SCLK_MODE_7                       0x7
#define        Gbl_pinMuxCntlBus_SPI1_SDI_MODE_0                        0x0
#define        Gbl_pinMuxCntlBus_SPI1_SDI_MODE_1                        0x1
#define        Gbl_pinMuxCntlBus_TW0_SCL_MODE_0                         0x0
#define        Gbl_pinMuxCntlBus_TW0_SCL_MODE_1                         0x1
#define        Gbl_pinMuxCntlBus_TW0_SCL_MODE_7                         0x7
#define     RA_Gbl_pinMuxCntlBus1                          0x8004
#define        Gbl_pinMuxCntlBus_TW0_SDA_MODE_0                         0x0
#define        Gbl_pinMuxCntlBus_TW0_SDA_MODE_1                         0x1
#define        Gbl_pinMuxCntlBus_TW0_SDA_MODE_7                         0x7
#define        Gbl_pinMuxCntlBus_STS0_CLK_MODE_0                        0x0
#define        Gbl_pinMuxCntlBus_STS0_CLK_MODE_1                        0x1
#define        Gbl_pinMuxCntlBus_STS0_CLK_MODE_2                        0x2
#define        Gbl_pinMuxCntlBus_STS0_CLK_MODE_4                        0x4
#define        Gbl_pinMuxCntlBus_STS0_CLK_MODE_7                        0x7
#define        Gbl_pinMuxCntlBus_STS0_SOP_MODE_0                        0x0
#define        Gbl_pinMuxCntlBus_STS0_SOP_MODE_1                        0x1
#define        Gbl_pinMuxCntlBus_STS0_SOP_MODE_2                        0x2
#define        Gbl_pinMuxCntlBus_STS0_SOP_MODE_3                        0x3
#define        Gbl_pinMuxCntlBus_STS0_SOP_MODE_4                        0x4
#define        Gbl_pinMuxCntlBus_STS0_SOP_MODE_7                        0x7
#define        Gbl_pinMuxCntlBus_STS0_SD_MODE_0                         0x0
#define        Gbl_pinMuxCntlBus_STS0_SD_MODE_1                         0x1
#define        Gbl_pinMuxCntlBus_STS0_SD_MODE_2                         0x2
#define        Gbl_pinMuxCntlBus_STS0_SD_MODE_4                         0x4
#define        Gbl_pinMuxCntlBus_STS0_SD_MODE_7                         0x7
#define        Gbl_pinMuxCntlBus_STS0_VALD_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_STS0_VALD_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_STS0_VALD_MODE_3                       0x3
#define        Gbl_pinMuxCntlBus_STS0_VALD_MODE_4                       0x4
#define        Gbl_pinMuxCntlBus_STS0_VALD_MODE_7                       0x7
#define        Gbl_pinMuxCntlBus_STS1_CLK_MODE_0                        0x0
#define        Gbl_pinMuxCntlBus_STS1_CLK_MODE_1                        0x1
#define        Gbl_pinMuxCntlBus_STS1_CLK_MODE_2                        0x2
#define        Gbl_pinMuxCntlBus_STS1_CLK_MODE_7                        0x7
#define        Gbl_pinMuxCntlBus_STS1_SOP_MODE_0                        0x0
#define        Gbl_pinMuxCntlBus_STS1_SOP_MODE_1                        0x1
#define        Gbl_pinMuxCntlBus_STS1_SOP_MODE_2                        0x2
#define        Gbl_pinMuxCntlBus_STS1_SOP_MODE_3                        0x3
#define        Gbl_pinMuxCntlBus_STS1_SOP_MODE_7                        0x7
#define        Gbl_pinMuxCntlBus_STS1_SD_MODE_0                         0x0
#define        Gbl_pinMuxCntlBus_STS1_SD_MODE_1                         0x1
#define        Gbl_pinMuxCntlBus_STS1_SD_MODE_2                         0x2
#define        Gbl_pinMuxCntlBus_STS1_SD_MODE_7                         0x7
#define        Gbl_pinMuxCntlBus_STS1_VALD_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_STS1_VALD_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_STS1_VALD_MODE_2                       0x2
#define        Gbl_pinMuxCntlBus_STS1_VALD_MODE_3                       0x3
#define        Gbl_pinMuxCntlBus_STS1_VALD_MODE_7                       0x7
#define        Gbl_pinMuxCntlBus_USB2_DRV_VBUS_MODE_0                   0x0
#define        Gbl_pinMuxCntlBus_USB2_DRV_VBUS_MODE_1                   0x1
#define     RA_Gbl_pinMuxCntlBus2                          0x8008
#define        Gbl_pinMuxCntlBus_RGMII_MDC_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_RGMII_MDC_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_RGMII_MDC_MODE_7                       0x7
#define        Gbl_pinMuxCntlBus_RGMII_MDIO_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_MDIO_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_MDIO_MODE_7                      0x7
#define        Gbl_pinMuxCntlBus_RGMII_TXC_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_RGMII_TXC_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_RGMII_TXD0_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_TXD0_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_TXD1_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_TXD1_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_TXD1_MODE_7                      0x7
#define        Gbl_pinMuxCntlBus_RGMII_TXD2_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_TXD2_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_TXD3_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_TXD3_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_TXCTL_MODE_0                     0x0
#define        Gbl_pinMuxCntlBus_RGMII_TXCTL_MODE_1                     0x1
#define        Gbl_pinMuxCntlBus_RGMII_RXC_MODE_0                       0x0
#define        Gbl_pinMuxCntlBus_RGMII_RXC_MODE_1                       0x1
#define        Gbl_pinMuxCntlBus_RGMII_RXD0_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_RXD0_MODE_1                      0x1
#define     RA_Gbl_pinMuxCntlBus3                          0x800C
#define        Gbl_pinMuxCntlBus_RGMII_RXD1_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_RXD1_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_RXD2_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_RXD2_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_RXD3_MODE_0                      0x0
#define        Gbl_pinMuxCntlBus_RGMII_RXD3_MODE_1                      0x1
#define        Gbl_pinMuxCntlBus_RGMII_RXCTL_MODE_0                     0x0
#define        Gbl_pinMuxCntlBus_RGMII_RXCTL_MODE_1                     0x1
#define     RA_Gbl_avioPinMuxCntlBus                       0x8400
#define        Gbl_avioPinMuxCntlBus_I2S1_DO0_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S1_DO0_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S1_DO0_MODE_7                    0x7
#define        Gbl_avioPinMuxCntlBus_I2S1_DO1_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S1_DO1_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S1_DO1_MODE_3                    0x3
#define        Gbl_avioPinMuxCntlBus_I2S1_DO1_MODE_7                    0x7
#define        Gbl_avioPinMuxCntlBus_I2S1_DO2_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S1_DO2_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S1_DO2_MODE_2                    0x2
#define        Gbl_avioPinMuxCntlBus_I2S1_DO2_MODE_3                    0x3
#define        Gbl_avioPinMuxCntlBus_I2S1_DO2_MODE_4                    0x4
#define        Gbl_avioPinMuxCntlBus_I2S1_DO2_MODE_7                    0x7
#define        Gbl_avioPinMuxCntlBus_I2S1_DO3_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S1_DO3_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S1_DO3_MODE_2                    0x2
#define        Gbl_avioPinMuxCntlBus_I2S1_DO3_MODE_3                    0x3
#define        Gbl_avioPinMuxCntlBus_I2S1_DO3_MODE_4                    0x4
#define        Gbl_avioPinMuxCntlBus_I2S1_DO3_MODE_7                    0x7
#define        Gbl_avioPinMuxCntlBus_I2S1_LRCKIO_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_I2S1_LRCKIO_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_I2S1_LRCKIO_MODE_2                 0x2
#define        Gbl_avioPinMuxCntlBus_I2S1_LRCKIO_MODE_3                 0x3
#define        Gbl_avioPinMuxCntlBus_I2S1_LRCKIO_MODE_7                 0x7
#define        Gbl_avioPinMuxCntlBus_I2S1_BCLKIO_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_I2S1_BCLKIO_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_I2S1_BCLKIO_MODE_2                 0x2
#define        Gbl_avioPinMuxCntlBus_I2S1_BCLKIO_MODE_7                 0x7
#define        Gbl_avioPinMuxCntlBus_SPDIFO_MODE_0                      0x0
#define        Gbl_avioPinMuxCntlBus_SPDIFO_MODE_1                      0x1
#define        Gbl_avioPinMuxCntlBus_SPDIFO_MODE_4                      0x4
#define        Gbl_avioPinMuxCntlBus_SPDIFI_MODE_0                      0x0
#define        Gbl_avioPinMuxCntlBus_SPDIFI_MODE_1                      0x1
#define        Gbl_avioPinMuxCntlBus_SPDIFI_MODE_2                      0x2
#define        Gbl_avioPinMuxCntlBus_I2S2_LRCKIO_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_I2S2_LRCKIO_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_I2S2_BCLKIO_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_I2S2_BCLKIO_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_I2S2_BCLKIO_MODE_2                 0x2
#define     RA_Gbl_avioPinMuxCntlBus1                      0x8404
#define        Gbl_avioPinMuxCntlBus_I2S2_DI0_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S2_DI0_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S2_DI0_MODE_2                    0x2
#define        Gbl_avioPinMuxCntlBus_I2S2_DI1_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S2_DI1_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S2_DI1_MODE_2                    0x2
#define        Gbl_avioPinMuxCntlBus_I2S2_DI1_MODE_3                    0x3
#define        Gbl_avioPinMuxCntlBus_I2S2_DI2_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S2_DI2_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S2_DI2_MODE_2                    0x2
#define        Gbl_avioPinMuxCntlBus_I2S2_DI2_MODE_3                    0x3
#define        Gbl_avioPinMuxCntlBus_I2S2_DI3_MODE_0                    0x0
#define        Gbl_avioPinMuxCntlBus_I2S2_DI3_MODE_1                    0x1
#define        Gbl_avioPinMuxCntlBus_I2S2_DI3_MODE_2                    0x2
#define        Gbl_avioPinMuxCntlBus_I2S2_DI3_MODE_3                    0x3
#define        Gbl_avioPinMuxCntlBus_I2S1_MCLK_MODE_0                   0x0
#define        Gbl_avioPinMuxCntlBus_I2S1_MCLK_MODE_1                   0x1
#define        Gbl_avioPinMuxCntlBus_I2S1_MCLK_MODE_3                   0x3
#define        Gbl_avioPinMuxCntlBus_I2S1_MCLK_MODE_7                   0x7
#define        Gbl_avioPinMuxCntlBus_I2S2_MCLK_MODE_0                   0x0
#define        Gbl_avioPinMuxCntlBus_I2S2_MCLK_MODE_1                   0x1
#define        Gbl_avioPinMuxCntlBus_I2S2_MCLK_MODE_2                   0x2
#define        Gbl_avioPinMuxCntlBus_I2S2_MCLK_MODE_4                   0x4
#define        Gbl_avioPinMuxCntlBus_TX_EDDC_SCL_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_TX_EDDC_SCL_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_TX_EDDC_SDA_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_TX_EDDC_SDA_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_I2S3_DO_MODE_0                     0x0
#define        Gbl_avioPinMuxCntlBus_I2S3_DO_MODE_1                     0x1
#define        Gbl_avioPinMuxCntlBus_I2S3_DO_MODE_3                     0x3
#define        Gbl_avioPinMuxCntlBus_I2S3_DO_MODE_7                     0x7
#define        Gbl_avioPinMuxCntlBus_I2S3_LRCKIO_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_I2S3_LRCKIO_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_I2S3_LRCKIO_MODE_3                 0x3
#define     RA_Gbl_avioPinMuxCntlBus2                      0x8408
#define        Gbl_avioPinMuxCntlBus_I2S3_BCLKIO_MODE_0                 0x0
#define        Gbl_avioPinMuxCntlBus_I2S3_BCLKIO_MODE_1                 0x1
#define        Gbl_avioPinMuxCntlBus_I2S3_BCLKIO_MODE_3                 0x3
#define        Gbl_avioPinMuxCntlBus_I2S3_DI_MODE_0                     0x0
#define        Gbl_avioPinMuxCntlBus_I2S3_DI_MODE_1                     0x1
#define        Gbl_avioPinMuxCntlBus_I2S3_DI_MODE_3                     0x3
#define     RA_Gbl_SDIO_CDnCntl                            0x8800
#define     RA_Gbl_SDIO_WPCntl                             0x8804
#define     RA_Gbl_SPI1_SS0nCntl                           0x8808
#define     RA_Gbl_SPI1_SS1nCntl                           0x880C
#define     RA_Gbl_SPI1_SS2nCntl                           0x8810
#define     RA_Gbl_SPI1_SS3nCntl                           0x8814
#define     RA_Gbl_SPI1_SDOCntl                            0x8818
#define     RA_Gbl_SPI1_SCLKCntl                           0x881C
#define     RA_Gbl_SPI1_SDICntl                            0x8820
#define     RA_Gbl_TW0_SCLCntl                             0x8824
#define     RA_Gbl_TW0_SDACntl                             0x8828
#define     RA_Gbl_STS0_CLKCntl                            0x882C
#define     RA_Gbl_STS0_SOPCntl                            0x8830
#define     RA_Gbl_STS0_SDCntl                             0x8834
#define     RA_Gbl_STS0_VALDCntl                           0x8838
#define     RA_Gbl_STS1_CLKCntl                            0x883C
#define     RA_Gbl_STS1_SOPCntl                            0x8840
#define     RA_Gbl_STS1_SDCntl                             0x8844
#define     RA_Gbl_STS1_VALDCntl                           0x8848
#define     RA_Gbl_USB2_DRV_VBUSCntl                       0x884C
#define     RA_Gbl_RGMII_MDCCntl                           0x8850
#define     RA_Gbl_RGMII_MDIOCntl                          0x8854
#define     RA_Gbl_RGMII_TXCCntl                           0x8858
#define     RA_Gbl_RGMII_TXD0Cntl                          0x885C
#define     RA_Gbl_RGMII_TXD1Cntl                          0x8860
#define     RA_Gbl_RGMII_TXD2Cntl                          0x8864
#define     RA_Gbl_RGMII_TXD3Cntl                          0x8868
#define     RA_Gbl_RGMII_TXCTLCntl                         0x886C
#define     RA_Gbl_RGMII_RXCCntl                           0x8870
#define     RA_Gbl_RGMII_RXD0Cntl                          0x8874
#define     RA_Gbl_RGMII_RXD1Cntl                          0x8878
#define     RA_Gbl_RGMII_RXD2Cntl                          0x887C
#define     RA_Gbl_RGMII_RXD3Cntl                          0x8880
#define     RA_Gbl_RGMII_RXCTLCntl                         0x8884
#define     RA_Gbl_I2S1_DO0Cntl                            0x8C00
#define     RA_Gbl_I2S1_DO1Cntl                            0x8C04
#define     RA_Gbl_I2S1_DO2Cntl                            0x8C08
#define     RA_Gbl_I2S1_DO3Cntl                            0x8C0C
#define     RA_Gbl_I2S1_LRCKIOCntl                         0x8C10
#define     RA_Gbl_I2S1_BCLKIOCntl                         0x8C14
#define     RA_Gbl_SPDIFOCntl                              0x8C18
#define     RA_Gbl_SPDIFICntl                              0x8C1C
#define     RA_Gbl_I2S2_LRCKIOCntl                         0x8C20
#define     RA_Gbl_I2S2_BCLKIOCntl                         0x8C24
#define     RA_Gbl_I2S2_DI0Cntl                            0x8C28
#define     RA_Gbl_I2S2_DI1Cntl                            0x8C2C
#define     RA_Gbl_I2S2_DI2Cntl                            0x8C30
#define     RA_Gbl_I2S2_DI3Cntl                            0x8C34
#define     RA_Gbl_I2S1_MCLKCntl                           0x8C38
#define     RA_Gbl_I2S2_MCLKCntl                           0x8C3C
#define     RA_Gbl_TX_EDDC_SCLCntl                         0x8C40
#define     RA_Gbl_TX_EDDC_SDACntl                         0x8C44
#define     RA_Gbl_I2S3_DOCntl                             0x8C48
#define     RA_Gbl_I2S3_LRCKIOCntl                         0x8C4C
#define     RA_Gbl_I2S3_BCLKIOCntl                         0x8C50
#define     RA_Gbl_I2S3_DICntl                             0x8C54
typedef struct SIE_Gbl {
#define     w32Gbl_ProductId                               {\
	UNSG32 uProductId_Id                               : 32;\
}
union { UNSG32 u32Gbl_ProductId;
	struct w32Gbl_ProductId;
};
#define     w32Gbl_ProductId_ext                           {\
	UNSG32 uProductId_ext_ID_EXT                       :  8;\
	UNSG32 RSVDx4_b8                                   : 24;\
}
union { UNSG32 u32Gbl_ProductId_ext;
	struct w32Gbl_ProductId_ext;
};
#define     w32Gbl_INT_ID                                  {\
	UNSG32 uINT_ID_VALUE                               :  8;\
	UNSG32 RSVDx8_b8                                   : 24;\
}
union { UNSG32 u32Gbl_INT_ID;
	struct w32Gbl_INT_ID;
};
#define     w32Gbl_bootStrap                               {\
	UNSG32 ubootStrap_softwareStrap                    : 16;\
	UNSG32 ubootStrap_bootSrc                          :  2;\
	UNSG32 ubootStrap_cpuRstByps                       :  1;\
	UNSG32 ubootStrap_pllPwrDown                       :  1;\
	UNSG32 ubootStrap_sysPll_0Byps                     :  1;\
	UNSG32 ubootStrap_sysPll_1Byps                     :  1;\
	UNSG32 ubootStrap_sysPll_2Byps                     :  1;\
	UNSG32 ubootStrap_memPllByps                       :  1;\
	UNSG32 ubootStrap_cpuPllByps                       :  1;\
	UNSG32 ubootStrap_emmcV12Enable                    :  1;\
	UNSG32 ubootStrap_ENG_EN                           :  1;\
	UNSG32 ubootStrap_LEGACY_BOOT                      :  1;\
	UNSG32 RSVDxC_b28                                  :  4;\
}
union { UNSG32 u32Gbl_bootStrap;
	struct w32Gbl_bootStrap;
};
#define     w32Gbl_bootStrapEn                             {\
	UNSG32 ubootStrapEn_cpuRstBypsEn                   :  1;\
	UNSG32 ubootStrapEn_pllPwrDownEn                   :  1;\
	UNSG32 ubootStrapEn_sysPLL_0BypsEn                 :  1;\
	UNSG32 ubootStrapEn_memPLLBypsEn                   :  1;\
	UNSG32 ubootStrapEn_cpuPLLBypsEn                   :  1;\
	UNSG32 ubootStrapEn_legacyBootEn                   :  1;\
	UNSG32 ubootStrapEn_sysPLL_1BypsEn                 :  1;\
	UNSG32 ubootStrapEn_sysPLL_2BypsEn                 :  1;\
	UNSG32 RSVDx10_b8                                  : 24;\
}
union { UNSG32 u32Gbl_bootStrapEn;
	struct w32Gbl_bootStrapEn;
};
#define     w32Gbl_pkgSel                                  {\
	UNSG32 upkgSel_DDR32bit                            :  1;\
	UNSG32 upkgSel_reserved                            :  1;\
	UNSG32 RSVDx14_b2                                  : 30;\
}
union { UNSG32 u32Gbl_pkgSel;
	struct w32Gbl_pkgSel;
};
UNSG8 RSVDx18                                     [40];
#define     w32Gbl_chipCntl                                {\
	UNSG32 uchipCntl_DBG_SEL                           :  3;\
	UNSG32 uchipCntl_PLL_PADDBG_SEL                    :  2;\
	UNSG32 uchipCntl_TW1_SEL                           :  1;\
	UNSG32 uchipCntl_USBOTG_CLK_PLL_SEL                :  1;\
	UNSG32 RSVDx40_b7                                  : 25;\
}
union { UNSG32 u32Gbl_chipCntl;
	struct w32Gbl_chipCntl;
};
#define     w32Gbl_stsCntl                                 {\
	UNSG32 ustsCntl_sts0TwoWire                        :  1;\
	UNSG32 ustsCntl_sts1TwoWire                        :  1;\
	UNSG32 ustsCntl_sts2TwoWire                        :  1;\
	UNSG32 ustsCntl_sts3TwoWire                        :  1;\
	UNSG32 ustsCntl_sts4TwoWire                        :  1;\
	UNSG32 ustsCntl_sts5TwoWire                        :  1;\
	UNSG32 ustsCntl_sts6TwoWire                        :  1;\
	UNSG32 ustsCntl_sts7TwoWire                        :  1;\
	UNSG32 RSVDx44_b8                                  : 24;\
}
union { UNSG32 u32Gbl_stsCntl;
	struct w32Gbl_stsCntl;
};
UNSG8 RSVDx48                                     [56];
#define     w32Gbl_sw_generic0                             {\
	UNSG32 usw_generic0_swReg0                         : 32;\
}
union { UNSG32 u32Gbl_sw_generic0;
	struct w32Gbl_sw_generic0;
};
#define     w32Gbl_sw_generic1                             {\
	UNSG32 usw_generic1_swReg1                         : 32;\
}
union { UNSG32 u32Gbl_sw_generic1;
	struct w32Gbl_sw_generic1;
};
#define     w32Gbl_sw_generic2                             {\
	UNSG32 usw_generic2_swReg2                         : 32;\
}
union { UNSG32 u32Gbl_sw_generic2;
	struct w32Gbl_sw_generic2;
};
#define     w32Gbl_sw_generic3                             {\
	UNSG32 usw_generic3_swReg3                         : 32;\
}
union { UNSG32 u32Gbl_sw_generic3;
	struct w32Gbl_sw_generic3;
};
UNSG8 RSVDx90                                     [112];
SIE_SRAMRWTC                                     ie_RWTC_SOC;
SIE_SRAMPWR                                      ie_SRAM_PWR_EMMC;
SIE_SRAMPWR                                      ie_SRAM_PWR_SDIO;
SIE_SRAMPWR                                      ie_SRAM_PWR_USB2;
SIE_SRAMPWR                                      ie_SRAM_PWR_USB3;
SIE_SRAMPWR                                      ie_SRAM_PWR_PCIE;
SIE_SRAMPWR                                      ie_SRAM_PWR_GFX3D;
SIE_SRAMPWR                                      ie_SRAM_PWR_V4G;
SIE_SRAMPWR                                      ie_SRAM_PWR_H1;
SIE_SRAMPWR                                      ie_SRAM_PWR_SISS;
SIE_SRAMPWR                                      ie_SRAM_PWR_OVP;
SIE_SRAMPWR                                      ie_SRAM_PWR_GE;
SIE_SRAMPWR                                      ie_SRAM_PWR_PB;
SIE_SRAMPWR                                      ie_SRAM_PWR_NPU;
SIE_SRAMPWR                                      ie_SRAM_PWR_ISP;
SIE_SRAMPWR                                      ie_SRAM_PWR_MC;
SIE_SRAMPWR                                      ie_SRAM_PWR_ALM;
UNSG8 RSVDx14C                                    [76];
#define     w32Gbl_FPGAR                                   {\
	UNSG32 uFPGAR_FPGAR                                : 32;\
}
union { UNSG32 u32Gbl_FPGAR;
	struct w32Gbl_FPGAR;
};
#define     w32Gbl_FPGARW                                  {\
	UNSG32 uFPGARW_FPGARW                              : 32;\
}
union { UNSG32 u32Gbl_FPGARW;
	struct w32Gbl_FPGARW;
};
UNSG8 RSVDx1A0                                    [96];
SIE_abipll                                       ie_sysPll_0;
SIE_abipll                                       ie_sysPll_1;
SIE_abipll                                       ie_sysPll_2;
UNSG8 RSVDx260                                    [928];
#define     w32Gbl_ResetTrigger                            {\
	UNSG32 uResetTrigger_chipReset                     :  1;\
	UNSG32 uResetTrigger_socDdrSyncReset               :  1;\
	UNSG32 RSVDx600_b2                                 : 30;\
}
union { UNSG32 u32Gbl_ResetTrigger;
	struct w32Gbl_ResetTrigger;
};
#define     w32Gbl_ResetStatus                             {\
	UNSG32 uResetStatus_ChipResetStatus                :  1;\
	UNSG32 uResetStatus_socDdrSyncResetStatus          :  1;\
	UNSG32 RSVDx604_b2                                 : 30;\
}
union { UNSG32 u32Gbl_ResetStatus;
	struct w32Gbl_ResetStatus;
};
UNSG8 RSVDx608                                    [4];
#define     w32Gbl_WDTResetStatus                          {\
	UNSG32 uWDTResetStatus_wd0Status                   :  1;\
	UNSG32 uWDTResetStatus_wd1Status                   :  1;\
	UNSG32 uWDTResetStatus_wd2Status                   :  1;\
	UNSG32 RSVDx60C_b3                                 : 29;\
}
union { UNSG32 u32Gbl_WDTResetStatus;
	struct w32Gbl_WDTResetStatus;
};
UNSG8 RSVDx610                                    [16];
#define     w32Gbl_avioReset                               {\
	UNSG32 uavioReset_SyncReset                        :  1;\
	UNSG32 RSVDx620_b1                                 : 31;\
}
union { UNSG32 u32Gbl_avioReset;
	struct w32Gbl_avioReset;
};
#define     w32Gbl_avioResetStatus                         {\
	UNSG32 uavioResetStatus_SyncReset                  :  1;\
	UNSG32 RSVDx624_b1                                 : 31;\
}
union { UNSG32 u32Gbl_avioResetStatus;
	struct w32Gbl_avioResetStatus;
};
#define     w32Gbl_gethRgmiiRstn                           {\
	UNSG32 ugethRgmiiRstn_SyncReset                    :  1;\
	UNSG32 RSVDx628_b1                                 : 31;\
}
union { UNSG32 u32Gbl_gethRgmiiRstn;
	struct w32Gbl_gethRgmiiRstn;
};
#define     w32Gbl_gethRgmiiResetStatus                    {\
	UNSG32 ugethRgmiiResetStatus_SyncReset             :  1;\
	UNSG32 RSVDx62C_b1                                 : 31;\
}
union { UNSG32 u32Gbl_gethRgmiiResetStatus;
	struct w32Gbl_gethRgmiiResetStatus;
};
#define     w32Gbl_gfx3DReset                              {\
	UNSG32 ugfx3DReset_SyncReset                       :  1;\
	UNSG32 RSVDx630_b1                                 : 31;\
}
union { UNSG32 u32Gbl_gfx3DReset;
	struct w32Gbl_gfx3DReset;
};
#define     w32Gbl_gfx3DResetStatus                        {\
	UNSG32 ugfx3DResetStatus_SyncReset                 :  1;\
	UNSG32 RSVDx634_b1                                 : 31;\
}
union { UNSG32 u32Gbl_gfx3DResetStatus;
	struct w32Gbl_gfx3DResetStatus;
};
UNSG8 RSVDx638                                    [8];
#define     w32Gbl_decoderReset                            {\
	UNSG32 udecoderReset_SyncReset                     :  1;\
	UNSG32 RSVDx640_b1                                 : 31;\
}
union { UNSG32 u32Gbl_decoderReset;
	struct w32Gbl_decoderReset;
};
#define     w32Gbl_decoderResetStatus                      {\
	UNSG32 udecoderResetStatus_SyncReset               :  1;\
	UNSG32 RSVDx644_b1                                 : 31;\
}
union { UNSG32 u32Gbl_decoderResetStatus;
	struct w32Gbl_decoderResetStatus;
};
UNSG8 RSVDx648                                    [8];
#define     w32Gbl_encoderReset                            {\
	UNSG32 uencoderReset_SyncReset                     :  1;\
	UNSG32 RSVDx650_b1                                 : 31;\
}
union { UNSG32 u32Gbl_encoderReset;
	struct w32Gbl_encoderReset;
};
#define     w32Gbl_encoderResetStatus                      {\
	UNSG32 uencoderResetStatus_SyncReset               :  1;\
	UNSG32 RSVDx654_b1                                 : 31;\
}
union { UNSG32 u32Gbl_encoderResetStatus;
	struct w32Gbl_encoderResetStatus;
};
UNSG8 RSVDx658                                    [8];
#define     w32Gbl_ovpReset                                {\
	UNSG32 uovpReset_SyncReset                         :  1;\
	UNSG32 RSVDx660_b1                                 : 31;\
}
union { UNSG32 u32Gbl_ovpReset;
	struct w32Gbl_ovpReset;
};
#define     w32Gbl_ovpResetStatus                          {\
	UNSG32 uovpResetStatus_SyncReset                   :  1;\
	UNSG32 RSVDx664_b1                                 : 31;\
}
union { UNSG32 u32Gbl_ovpResetStatus;
	struct w32Gbl_ovpResetStatus;
};
#define     w32Gbl_npuReset                                {\
	UNSG32 unpuReset_SyncReset                         :  1;\
	UNSG32 RSVDx668_b1                                 : 31;\
}
union { UNSG32 u32Gbl_npuReset;
	struct w32Gbl_npuReset;
};
#define     w32Gbl_npuResetStatus                          {\
	UNSG32 unpuResetStatus_SyncReset                   :  1;\
	UNSG32 RSVDx66C_b1                                 : 31;\
}
union { UNSG32 u32Gbl_npuResetStatus;
	struct w32Gbl_npuResetStatus;
};
#define     w32Gbl_almGRstn                                {\
	UNSG32 ualmGRstn_SyncReset                         :  1;\
	UNSG32 RSVDx670_b1                                 : 31;\
}
union { UNSG32 u32Gbl_almGRstn;
	struct w32Gbl_almGRstn;
};
#define     w32Gbl_almResetStatus                          {\
	UNSG32 ualmResetStatus_SyncReset                   :  1;\
	UNSG32 RSVDx674_b1                                 : 31;\
}
union { UNSG32 u32Gbl_almResetStatus;
	struct w32Gbl_almResetStatus;
};
UNSG8 RSVDx678                                    [8];
#define     w32Gbl_perifReset                              {\
	UNSG32 uperifReset_SyncReset                       :  1;\
	UNSG32 uperifReset_ahbApbSyncReset                 :  1;\
	UNSG32 uperifReset_sdioSyncReset                   :  1;\
	UNSG32 uperifReset_tspSyncReset                    :  1;\
	UNSG32 uperifReset_tsSSSyncReset                   :  1;\
	UNSG32 uperifReset_nskSyncReset                    :  1;\
	UNSG32 uperifReset_nocsSyncReset                   :  1;\
	UNSG32 uperifReset_bcmSyncReset                    :  1;\
	UNSG32 uperifReset_atbSyncReset                    :  1;\
	UNSG32 uperifReset_usb0SyncReset                   :  1;\
	UNSG32 uperifReset_usb3SyncReset                   :  1;\
	UNSG32 uperifReset_emmcSyncReset                   :  1;\
	UNSG32 uperifReset_pBridgeSyncReset                :  1;\
	UNSG32 uperifReset_ifcpSyncReset                   :  1;\
	UNSG32 RSVDx680_b14                                : 18;\
}
union { UNSG32 u32Gbl_perifReset;
	struct w32Gbl_perifReset;
};
#define     w32Gbl_perifResetStatus                        {\
	UNSG32 uperifResetStatus_SyncReset                 :  1;\
	UNSG32 uperifResetStatus_ahbApbSyncReset           :  1;\
	UNSG32 uperifResetStatus_sdioSyncReset             :  1;\
	UNSG32 uperifResetStatus_tspSyncReset              :  1;\
	UNSG32 uperifResetStatus_tsSSSyncReset             :  1;\
	UNSG32 uperifResetStatus_nskSyncReset              :  1;\
	UNSG32 uperifResetStatus_nocsSyncReset             :  1;\
	UNSG32 uperifResetStatus_bcmSyncReset              :  1;\
	UNSG32 uperifResetStatus_atbSyncReset              :  1;\
	UNSG32 uperifResetStatus_usb0SyncReset             :  1;\
	UNSG32 uperifResetStatus_usb3SyncReset             :  1;\
	UNSG32 uperifResetStatus_emmcSyncReset             :  1;\
	UNSG32 uperifResetStatus_pBridgeSyncReset          :  1;\
	UNSG32 uperifResetStatus_tsioSyncReset             :  1;\
	UNSG32 uperifResetStatus_ifcpSyncReset             :  1;\
	UNSG32 RSVDx684_b15                                : 17;\
}
union { UNSG32 u32Gbl_perifResetStatus;
	struct w32Gbl_perifResetStatus;
};
#define     w32Gbl_perifStickyResetN                       {\
	UNSG32 uperifStickyResetN_usb3PhyRstn              :  1;\
	UNSG32 uperifStickyResetN_pcie0Rstn                :  1;\
	UNSG32 uperifStickyResetN_usb0PhyRstn              :  1;\
	UNSG32 uperifStickyResetN_usb0CoreRstn             :  1;\
	UNSG32 uperifStickyResetN_usb0MahbRstn             :  1;\
	UNSG32 uperifStickyResetN_sdioPhyRstn              :  1;\
	UNSG32 uperifStickyResetN_emmcPhyRstn              :  1;\
	UNSG32 uperifStickyResetN_pciePhyRstn              :  1;\
	UNSG32 uperifStickyResetN_ispGRstn                 :  1;\
	UNSG32 RSVDx688_b9                                 : 23;\
}
union { UNSG32 u32Gbl_perifStickyResetN;
	struct w32Gbl_perifStickyResetN;
};
#define     w32Gbl_apbPerifResetTrigger                    {\
	UNSG32 uapbPerifResetTrigger_uart0SyncReset        :  1;\
	UNSG32 uapbPerifResetTrigger_uart1SyncReset        :  1;\
	UNSG32 uapbPerifResetTrigger_i2c0SyncReset         :  1;\
	UNSG32 uapbPerifResetTrigger_i2c1SyncReset         :  1;\
	UNSG32 uapbPerifResetTrigger_spiSyncReset          :  1;\
	UNSG32 uapbPerifResetTrigger_timer0SyncReset       :  1;\
	UNSG32 uapbPerifResetTrigger_timer1SyncReset       :  1;\
	UNSG32 RSVDx68C_b7                                 : 25;\
}
union { UNSG32 u32Gbl_apbPerifResetTrigger;
	struct w32Gbl_apbPerifResetTrigger;
};
#define     w32Gbl_apbPerifResetStatus                     {\
	UNSG32 uapbPerifResetStatus_uart0SyncResetStatus   :  1;\
	UNSG32 uapbPerifResetStatus_uart1SyncResetStatus   :  1;\
	UNSG32 uapbPerifResetStatus_i2c0SyncResetStatus    :  1;\
	UNSG32 uapbPerifResetStatus_i2c1SyncResetStatus    :  1;\
	UNSG32 uapbPerifResetStatus_i2c2SyncResetStatus    :  1;\
	UNSG32 uapbPerifResetStatus_spiSyncResetStatus     :  1;\
	UNSG32 uapbPerifResetStatus_timer0SyncResetStatus  :  1;\
	UNSG32 uapbPerifResetStatus_timer1SyncResetStatus  :  1;\
	UNSG32 RSVDx690_b8                                 : 24;\
}
union { UNSG32 u32Gbl_apbPerifResetStatus;
	struct w32Gbl_apbPerifResetStatus;
};
UNSG8 RSVDx694                                    [108];
#define     w32Gbl_clkEnable                               {\
	UNSG32 uclkEnable_usb0CoreClkEn                    :  1;\
	UNSG32 uclkEnable_sdioSysClkEn                     :  1;\
	UNSG32 uclkEnable_pcie0SysClkEn                    :  1;\
	UNSG32 uclkEnable_emmcSysClkEn                     :  1;\
	UNSG32 uclkEnable_pBridgeCoreClkEn                 :  1;\
	UNSG32 uclkEnable_npuAxiClkEn                      :  1;\
	UNSG32 uclkEnable_gethRgmiiSysClkEn                :  1;\
	UNSG32 RSVDx700_b7                                 : 25;\
}
union { UNSG32 u32Gbl_clkEnable;
	struct w32Gbl_clkEnable;
};
UNSG8 RSVDx704                                    [12];
#define     w32Gbl_ClkSwitch                               {\
	UNSG32 uClkSwitch_sysPLL_0SWBypass                 :  1;\
	UNSG32 uClkSwitch_sysPLL_1SWBypass                 :  1;\
	UNSG32 uClkSwitch_sysPLL_2SWBypass                 :  1;\
	UNSG32 uClkSwitch_memPLLSWBypass                   :  1;\
	UNSG32 uClkSwitch_cpuPLLSWBypass                   :  1;\
	UNSG32 RSVDx710_b5                                 : 27;\
}
union { UNSG32 u32Gbl_ClkSwitch;
	struct w32Gbl_ClkSwitch;
};
UNSG8 RSVDx714                                    [12];
SIE_clkD2                                        ie_cpufastRefClk;
SIE_clkD2                                        ie_memfastRefClk;
UNSG8 RSVDx728                                    [24];
SIE_clkD8                                        ie_cfgClk;
SIE_clkD2                                        ie_perifSysClk;
SIE_clkD8                                        ie_atbClk;
SIE_clkD2                                        ie_decoderClk;
UNSG8 RSVDx750                                    [4];
SIE_clkD4                                        ie_encoderClk;
SIE_clkD4                                        ie_ovpCoreClk;
UNSG8 RSVDx75C                                    [4];
SIE_clkD2                                        ie_gfx3DCoreClk;
SIE_clkD2                                        ie_gfx3DSysClk;
UNSG8 RSVDx768                                    [8];
SIE_clkD2                                        ie_avioSysClk;
SIE_clkD2                                        ie_vppSysClk;
SIE_clkD8                                        ie_hpiClk;
SIE_clkD8                                        ie_avioBiuClk;
SIE_clkD2                                        ie_vipPipeClk;
SIE_clkD2                                        ie_avioFpll400_clk;
UNSG8 RSVDx788                                    [8];
SIE_clkD2                                        ie_tspClk;
SIE_clkD4                                        ie_tspRefClk;
UNSG8 RSVDx798                                    [4];
SIE_clkD4                                        ie_nocsClk;
SIE_clkD4                                        ie_apbCoreClk;
UNSG8 RSVDx7A4                                    [12];
SIE_clkD4                                        ie_emmcClk;
SIE_clkD4                                        ie_sd0Clk;
UNSG8 RSVDx7B8                                    [8];
SIE_clkD8                                        ie_gethRgmiiClk;
SIE_clkD2                                        ie_avioOvpClk;
UNSG8 RSVDx7C8                                    [24];
SIE_clkD12_ENOFF                                 ie_perifTestClk125mGroup0;
UNSG8 RSVDx7E4                                    [12];
SIE_clkD12_ENOFF                                 ie_usb2TestClk;
SIE_clkD12_ENOFF                                 ie_perifTestClk250mGroup0;
SIE_clkD4                                        ie_usb3CoreClk;
UNSG8 RSVDx7FC                                    [20];
SIE_clkD4                                        ie_bcmClk;
SIE_clkD2                                        ie_vxSysClk;
SIE_clkD8                                        ie_npuClk;
SIE_clkD2                                        ie_sissSysClk;
SIE_clkD8                                        ie_ifcpClk;
SIE_clkD2                                        ie_issSysClk;
SIE_clkD2                                        ie_ispClk;
SIE_clkD2                                        ie_ispBeClk;
SIE_clkD4                                        ie_ispDscClk;
SIE_clkD2                                        ie_ispCsi0Clk;
SIE_clkD8                                        ie_ispCsi1Clk;
SIE_clkD8                                        ie_hdmiRxrefClk;
SIE_clkD12_ENOFF                                 ie_mipiRxScanByteClk;
SIE_clkD12_ENOFF                                 ie_usb2TestClk480mGroup0;
SIE_clkD12_ENOFF                                 ie_usb2TestClk480mGroup1;
SIE_clkD12_ENOFF                                 ie_usb2TestClk480mGroup2;
SIE_clkD12_ENOFF                                 ie_usb2TestClk100mGroup0;
SIE_clkD12_ENOFF                                 ie_usb2TestClk100mGroup1;
SIE_clkD12_ENOFF                                 ie_usb2TestClk100mGroup2;
SIE_clkD12_ENOFF                                 ie_usb2TestClk100mGroup3;
SIE_clkD12_ENOFF                                 ie_usb2TestClk100mGroup4;
SIE_clkD12_ENOFF                                 ie_perifTestClk200mGroup0;
SIE_clkD12_ENOFF                                 ie_perifTestClk200mGroup1;
SIE_clkD12_ENOFF                                 ie_perifTestClk500mGroup0;
SIE_clkD12_ENOFF                                 ie_txClkEsc;
SIE_clkD4                                        ie_aioSysClk;
UNSG8 RSVDx878                                    [136];
#define     w32Gbl_SECURE_SCAN_EN                          {\
	UNSG32 uSECURE_SCAN_EN_SET                         :  1;\
	UNSG32 RSVDx900_b1                                 : 31;\
}
union { UNSG32 u32Gbl_SECURE_SCAN_EN;
	struct w32Gbl_SECURE_SCAN_EN;
};
UNSG8 RSVDx904                                    [20];
#define     w32Gbl_gfx3DDisRamClkGate                      {\
	UNSG32 ugfx3DDisRamClkGate_drcg                    :  1;\
	UNSG32 RSVDx918_b1                                 : 31;\
}
union { UNSG32 u32Gbl_gfx3DDisRamClkGate;
	struct w32Gbl_gfx3DDisRamClkGate;
};
UNSG8 RSVDx91C                                    [68];
#define     w32Gbl_gic400_ctrl                             {\
	UNSG32 ugic400_ctrl_cgfsdisable                    :  1;\
	UNSG32 RSVDx960_b1                                 : 31;\
}
union { UNSG32 u32Gbl_gic400_ctrl;
	struct w32Gbl_gic400_ctrl;
};
UNSG8 RSVDx964                                    [12];
#define     w32Gbl_GFX_CTRL_0                              {\
	UNSG32 uGFX_CTRL_0_timer_enable                    :  1;\
	UNSG32 RSVDx970_b1                                 : 31;\
}
union { UNSG32 u32Gbl_GFX_CTRL_0;
	struct w32Gbl_GFX_CTRL_0;
};
#define     w32Gbl_GFX_CTRL_1                              {\
	UNSG32 uGFX_CTRL_1_timer_load                      :  1;\
	UNSG32 uGFX_CTRL_1_timer_halt                      :  1;\
	UNSG32 RSVDx974_b2                                 : 30;\
}
union { UNSG32 u32Gbl_GFX_CTRL_1;
	struct w32Gbl_GFX_CTRL_1;
};
#define     w32Gbl_GFX_CTRL_2                              {\
	UNSG32 uGFX_CTRL_2_timer_value_lsb                 : 32;\
}
union { UNSG32 u32Gbl_GFX_CTRL_2;
	struct w32Gbl_GFX_CTRL_2;
};
#define     w32Gbl_GFX_CTRL_3                              {\
	UNSG32 uGFX_CTRL_3_timer_value_msb                 : 32;\
}
union { UNSG32 u32Gbl_GFX_CTRL_3;
	struct w32Gbl_GFX_CTRL_3;
};
#define     w32Gbl_GFX_TIMER_VALUE                         {\
	UNSG32 uGFX_TIMER_VALUE_timer_binary_value_lsb     : 32;\
}
union { UNSG32 u32Gbl_GFX_TIMER_VALUE;
	struct w32Gbl_GFX_TIMER_VALUE;
};
#define     w32Gbl_GFX_TIMER_VALUE1                        {\
	UNSG32 uGFX_TIMER_VALUE_timer_binary_value_msb     : 32;\
}
union { UNSG32 u32Gbl_GFX_TIMER_VALUE1;
	struct w32Gbl_GFX_TIMER_VALUE1;
};
UNSG8 RSVDx988                                    [4];
#define     w32Gbl_GFX_STATUS                              {\
	UNSG32 uGFX_STATUS_gfx3d_idle                      :  1;\
	UNSG32 RSVDx98C_b1                                 : 31;\
}
union { UNSG32 u32Gbl_GFX_STATUS;
	struct w32Gbl_GFX_STATUS;
};
#define     w32Gbl_USBOTG_REFCLK_CTRL0                     {\
	UNSG32 uUSBOTG_REFCLK_CTRL0_divval                 : 32;\
}
union { UNSG32 u32Gbl_USBOTG_REFCLK_CTRL0;
	struct w32Gbl_USBOTG_REFCLK_CTRL0;
};
#define     w32Gbl_USBOTG_REFCLK_CTRL1                     {\
	UNSG32 uUSBOTG_REFCLK_CTRL1_lowcnt                 : 32;\
}
union { UNSG32 u32Gbl_USBOTG_REFCLK_CTRL1;
	struct w32Gbl_USBOTG_REFCLK_CTRL1;
};
UNSG8 RSVDx998                                    [104];
SIE_PERIF                                        ie_PERIF;
UNSG8 RSVDxA14                                    [30188];
#define     w32Gbl_pinMuxCntlBus                           {\
	UNSG32 upinMuxCntlBus_SDIO_CDn                     :  3;\
	UNSG32 upinMuxCntlBus_SDIO_WP                      :  3;\
	UNSG32 upinMuxCntlBus_SPI1_SS0n                    :  3;\
	UNSG32 upinMuxCntlBus_SPI1_SS1n                    :  3;\
	UNSG32 upinMuxCntlBus_SPI1_SS2n                    :  3;\
	UNSG32 upinMuxCntlBus_SPI1_SS3n                    :  3;\
	UNSG32 upinMuxCntlBus_SPI1_SDO                     :  3;\
	UNSG32 upinMuxCntlBus_SPI1_SCLK                    :  3;\
	UNSG32 upinMuxCntlBus_SPI1_SDI                     :  3;\
	UNSG32 upinMuxCntlBus_TW0_SCL                      :  3;\
	UNSG32 RSVDx8000_b30                               :  2;\
}
union { UNSG32 u32Gbl_pinMuxCntlBus;
	struct w32Gbl_pinMuxCntlBus;
};
#define     w32Gbl_pinMuxCntlBus1                          {\
	UNSG32 upinMuxCntlBus_TW0_SDA                      :  3;\
	UNSG32 upinMuxCntlBus_STS0_CLK                     :  3;\
	UNSG32 upinMuxCntlBus_STS0_SOP                     :  3;\
	UNSG32 upinMuxCntlBus_STS0_SD                      :  3;\
	UNSG32 upinMuxCntlBus_STS0_VALD                    :  3;\
	UNSG32 upinMuxCntlBus_STS1_CLK                     :  3;\
	UNSG32 upinMuxCntlBus_STS1_SOP                     :  3;\
	UNSG32 upinMuxCntlBus_STS1_SD                      :  3;\
	UNSG32 upinMuxCntlBus_STS1_VALD                    :  3;\
	UNSG32 upinMuxCntlBus_USB2_DRV_VBUS                :  3;\
	UNSG32 RSVDx8004_b30                               :  2;\
}
union { UNSG32 u32Gbl_pinMuxCntlBus1;
	struct w32Gbl_pinMuxCntlBus1;
};
#define     w32Gbl_pinMuxCntlBus2                          {\
	UNSG32 upinMuxCntlBus_RGMII_MDC                    :  3;\
	UNSG32 upinMuxCntlBus_RGMII_MDIO                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_TXC                    :  3;\
	UNSG32 upinMuxCntlBus_RGMII_TXD0                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_TXD1                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_TXD2                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_TXD3                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_TXCTL                  :  3;\
	UNSG32 upinMuxCntlBus_RGMII_RXC                    :  3;\
	UNSG32 upinMuxCntlBus_RGMII_RXD0                   :  3;\
	UNSG32 RSVDx8008_b30                               :  2;\
}
union { UNSG32 u32Gbl_pinMuxCntlBus2;
	struct w32Gbl_pinMuxCntlBus2;
};
#define     w32Gbl_pinMuxCntlBus3                          {\
	UNSG32 upinMuxCntlBus_RGMII_RXD1                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_RXD2                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_RXD3                   :  3;\
	UNSG32 upinMuxCntlBus_RGMII_RXCTL                  :  3;\
	UNSG32 RSVDx800C_b12                               : 20;\
}
union { UNSG32 u32Gbl_pinMuxCntlBus3;
	struct w32Gbl_pinMuxCntlBus3;
};
UNSG8 RSVDx8010                                   [1008];
#define     w32Gbl_avioPinMuxCntlBus                       {\
	UNSG32 uavioPinMuxCntlBus_I2S1_DO0                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S1_DO1                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S1_DO2                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S1_DO3                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S1_LRCKIO              :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S1_BCLKIO              :  3;\
	UNSG32 uavioPinMuxCntlBus_SPDIFO                   :  3;\
	UNSG32 uavioPinMuxCntlBus_SPDIFI                   :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S2_LRCKIO              :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S2_BCLKIO              :  3;\
	UNSG32 RSVDx8400_b30                               :  2;\
}
union { UNSG32 u32Gbl_avioPinMuxCntlBus;
	struct w32Gbl_avioPinMuxCntlBus;
};
#define     w32Gbl_avioPinMuxCntlBus1                      {\
	UNSG32 uavioPinMuxCntlBus_I2S2_DI0                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S2_DI1                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S2_DI2                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S2_DI3                 :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S1_MCLK                :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S2_MCLK                :  3;\
	UNSG32 uavioPinMuxCntlBus_TX_EDDC_SCL              :  3;\
	UNSG32 uavioPinMuxCntlBus_TX_EDDC_SDA              :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S3_DO                  :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S3_LRCKIO              :  3;\
	UNSG32 RSVDx8404_b30                               :  2;\
}
union { UNSG32 u32Gbl_avioPinMuxCntlBus1;
	struct w32Gbl_avioPinMuxCntlBus1;
};
#define     w32Gbl_avioPinMuxCntlBus2                      {\
	UNSG32 uavioPinMuxCntlBus_I2S3_BCLKIO              :  3;\
	UNSG32 uavioPinMuxCntlBus_I2S3_DI                  :  3;\
	UNSG32 RSVDx8408_b6                                : 26;\
}
union { UNSG32 u32Gbl_avioPinMuxCntlBus2;
	struct w32Gbl_avioPinMuxCntlBus2;
};
UNSG8 RSVDx840C                                   [1012];
#define     w32Gbl_SDIO_CDnCntl                            {\
	UNSG32 uSDIO_CDnCntl_DS0                           :  1;\
	UNSG32 uSDIO_CDnCntl_DS1                           :  1;\
	UNSG32 uSDIO_CDnCntl_DS2                           :  1;\
	UNSG32 uSDIO_CDnCntl_DS3                           :  1;\
	UNSG32 uSDIO_CDnCntl_IE                            :  1;\
	UNSG32 uSDIO_CDnCntl_PE                            :  1;\
	UNSG32 uSDIO_CDnCntl_PS                            :  1;\
	UNSG32 uSDIO_CDnCntl_SL                            :  1;\
	UNSG32 uSDIO_CDnCntl_SPU                           :  1;\
	UNSG32 uSDIO_CDnCntl_ST                            :  1;\
	UNSG32 RSVDx8800_b10                               : 22;\
}
union { UNSG32 u32Gbl_SDIO_CDnCntl;
	struct w32Gbl_SDIO_CDnCntl;
};
#define     w32Gbl_SDIO_WPCntl                             {\
	UNSG32 uSDIO_WPCntl_DS0                            :  1;\
	UNSG32 uSDIO_WPCntl_DS1                            :  1;\
	UNSG32 uSDIO_WPCntl_DS2                            :  1;\
	UNSG32 uSDIO_WPCntl_DS3                            :  1;\
	UNSG32 uSDIO_WPCntl_IE                             :  1;\
	UNSG32 uSDIO_WPCntl_PE                             :  1;\
	UNSG32 uSDIO_WPCntl_PS                             :  1;\
	UNSG32 uSDIO_WPCntl_SL                             :  1;\
	UNSG32 uSDIO_WPCntl_SPU                            :  1;\
	UNSG32 uSDIO_WPCntl_ST                             :  1;\
	UNSG32 RSVDx8804_b10                               : 22;\
}
union { UNSG32 u32Gbl_SDIO_WPCntl;
	struct w32Gbl_SDIO_WPCntl;
};
#define     w32Gbl_SPI1_SS0nCntl                           {\
	UNSG32 uSPI1_SS0nCntl_DS0                          :  1;\
	UNSG32 uSPI1_SS0nCntl_DS1                          :  1;\
	UNSG32 uSPI1_SS0nCntl_DS2                          :  1;\
	UNSG32 uSPI1_SS0nCntl_IE                           :  1;\
	UNSG32 uSPI1_SS0nCntl_PD                           :  1;\
	UNSG32 uSPI1_SS0nCntl_PU                           :  1;\
	UNSG32 uSPI1_SS0nCntl_SL                           :  1;\
	UNSG32 uSPI1_SS0nCntl_ST                           :  1;\
	UNSG32 RSVDx8808_b8                                : 24;\
}
union { UNSG32 u32Gbl_SPI1_SS0nCntl;
	struct w32Gbl_SPI1_SS0nCntl;
};
#define     w32Gbl_SPI1_SS1nCntl                           {\
	UNSG32 uSPI1_SS1nCntl_DS0                          :  1;\
	UNSG32 uSPI1_SS1nCntl_DS1                          :  1;\
	UNSG32 uSPI1_SS1nCntl_DS2                          :  1;\
	UNSG32 uSPI1_SS1nCntl_IE                           :  1;\
	UNSG32 uSPI1_SS1nCntl_PD                           :  1;\
	UNSG32 uSPI1_SS1nCntl_PU                           :  1;\
	UNSG32 uSPI1_SS1nCntl_SL                           :  1;\
	UNSG32 uSPI1_SS1nCntl_ST                           :  1;\
	UNSG32 RSVDx880C_b8                                : 24;\
}
union { UNSG32 u32Gbl_SPI1_SS1nCntl;
	struct w32Gbl_SPI1_SS1nCntl;
};
#define     w32Gbl_SPI1_SS2nCntl                           {\
	UNSG32 uSPI1_SS2nCntl_DS0                          :  1;\
	UNSG32 uSPI1_SS2nCntl_DS1                          :  1;\
	UNSG32 uSPI1_SS2nCntl_DS2                          :  1;\
	UNSG32 uSPI1_SS2nCntl_DS3                          :  1;\
	UNSG32 uSPI1_SS2nCntl_IE                           :  1;\
	UNSG32 uSPI1_SS2nCntl_PE                           :  1;\
	UNSG32 uSPI1_SS2nCntl_PS                           :  1;\
	UNSG32 uSPI1_SS2nCntl_SL                           :  1;\
	UNSG32 uSPI1_SS2nCntl_SPU                          :  1;\
	UNSG32 uSPI1_SS2nCntl_ST                           :  1;\
	UNSG32 RSVDx8810_b10                               : 22;\
}
union { UNSG32 u32Gbl_SPI1_SS2nCntl;
	struct w32Gbl_SPI1_SS2nCntl;
};
#define     w32Gbl_SPI1_SS3nCntl                           {\
	UNSG32 uSPI1_SS3nCntl_DS0                          :  1;\
	UNSG32 uSPI1_SS3nCntl_DS1                          :  1;\
	UNSG32 uSPI1_SS3nCntl_DS2                          :  1;\
	UNSG32 uSPI1_SS3nCntl_DS3                          :  1;\
	UNSG32 uSPI1_SS3nCntl_IE                           :  1;\
	UNSG32 uSPI1_SS3nCntl_PE                           :  1;\
	UNSG32 uSPI1_SS3nCntl_PS                           :  1;\
	UNSG32 uSPI1_SS3nCntl_SL                           :  1;\
	UNSG32 uSPI1_SS3nCntl_SPU                          :  1;\
	UNSG32 uSPI1_SS3nCntl_ST                           :  1;\
	UNSG32 RSVDx8814_b10                               : 22;\
}
union { UNSG32 u32Gbl_SPI1_SS3nCntl;
	struct w32Gbl_SPI1_SS3nCntl;
};
#define     w32Gbl_SPI1_SDOCntl                            {\
	UNSG32 uSPI1_SDOCntl_DS0                           :  1;\
	UNSG32 uSPI1_SDOCntl_DS1                           :  1;\
	UNSG32 uSPI1_SDOCntl_DS2                           :  1;\
	UNSG32 uSPI1_SDOCntl_IE                            :  1;\
	UNSG32 uSPI1_SDOCntl_PD                            :  1;\
	UNSG32 uSPI1_SDOCntl_PU                            :  1;\
	UNSG32 uSPI1_SDOCntl_SL                            :  1;\
	UNSG32 uSPI1_SDOCntl_ST                            :  1;\
	UNSG32 RSVDx8818_b8                                : 24;\
}
union { UNSG32 u32Gbl_SPI1_SDOCntl;
	struct w32Gbl_SPI1_SDOCntl;
};
#define     w32Gbl_SPI1_SCLKCntl                           {\
	UNSG32 uSPI1_SCLKCntl_DS0                          :  1;\
	UNSG32 uSPI1_SCLKCntl_DS1                          :  1;\
	UNSG32 uSPI1_SCLKCntl_DS2                          :  1;\
	UNSG32 uSPI1_SCLKCntl_IE                           :  1;\
	UNSG32 uSPI1_SCLKCntl_PD                           :  1;\
	UNSG32 uSPI1_SCLKCntl_PU                           :  1;\
	UNSG32 uSPI1_SCLKCntl_SL                           :  1;\
	UNSG32 uSPI1_SCLKCntl_ST                           :  1;\
	UNSG32 RSVDx881C_b8                                : 24;\
}
union { UNSG32 u32Gbl_SPI1_SCLKCntl;
	struct w32Gbl_SPI1_SCLKCntl;
};
#define     w32Gbl_SPI1_SDICntl                            {\
	UNSG32 uSPI1_SDICntl_DS0                           :  1;\
	UNSG32 uSPI1_SDICntl_DS1                           :  1;\
	UNSG32 uSPI1_SDICntl_DS2                           :  1;\
	UNSG32 uSPI1_SDICntl_IE                            :  1;\
	UNSG32 uSPI1_SDICntl_PD                            :  1;\
	UNSG32 uSPI1_SDICntl_PU                            :  1;\
	UNSG32 uSPI1_SDICntl_SL                            :  1;\
	UNSG32 uSPI1_SDICntl_ST                            :  1;\
	UNSG32 RSVDx8820_b8                                : 24;\
}
union { UNSG32 u32Gbl_SPI1_SDICntl;
	struct w32Gbl_SPI1_SDICntl;
};
#define     w32Gbl_TW0_SCLCntl                             {\
	UNSG32 uTW0_SCLCntl_DS0                            :  1;\
	UNSG32 uTW0_SCLCntl_DS1                            :  1;\
	UNSG32 uTW0_SCLCntl_DS2                            :  1;\
	UNSG32 uTW0_SCLCntl_DS3                            :  1;\
	UNSG32 uTW0_SCLCntl_IE                             :  1;\
	UNSG32 uTW0_SCLCntl_PE                             :  1;\
	UNSG32 uTW0_SCLCntl_PS                             :  1;\
	UNSG32 uTW0_SCLCntl_SL                             :  1;\
	UNSG32 uTW0_SCLCntl_SPU                            :  1;\
	UNSG32 uTW0_SCLCntl_ST                             :  1;\
	UNSG32 RSVDx8824_b10                               : 22;\
}
union { UNSG32 u32Gbl_TW0_SCLCntl;
	struct w32Gbl_TW0_SCLCntl;
};
#define     w32Gbl_TW0_SDACntl                             {\
	UNSG32 uTW0_SDACntl_DS0                            :  1;\
	UNSG32 uTW0_SDACntl_DS1                            :  1;\
	UNSG32 uTW0_SDACntl_DS2                            :  1;\
	UNSG32 uTW0_SDACntl_DS3                            :  1;\
	UNSG32 uTW0_SDACntl_IE                             :  1;\
	UNSG32 uTW0_SDACntl_PE                             :  1;\
	UNSG32 uTW0_SDACntl_PS                             :  1;\
	UNSG32 uTW0_SDACntl_SL                             :  1;\
	UNSG32 uTW0_SDACntl_SPU                            :  1;\
	UNSG32 uTW0_SDACntl_ST                             :  1;\
	UNSG32 RSVDx8828_b10                               : 22;\
}
union { UNSG32 u32Gbl_TW0_SDACntl;
	struct w32Gbl_TW0_SDACntl;
};
#define     w32Gbl_STS0_CLKCntl                            {\
	UNSG32 uSTS0_CLKCntl_DS0                           :  1;\
	UNSG32 uSTS0_CLKCntl_DS1                           :  1;\
	UNSG32 uSTS0_CLKCntl_DS2                           :  1;\
	UNSG32 uSTS0_CLKCntl_IE                            :  1;\
	UNSG32 uSTS0_CLKCntl_PD                            :  1;\
	UNSG32 uSTS0_CLKCntl_PU                            :  1;\
	UNSG32 uSTS0_CLKCntl_SL                            :  1;\
	UNSG32 uSTS0_CLKCntl_ST                            :  1;\
	UNSG32 RSVDx882C_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS0_CLKCntl;
	struct w32Gbl_STS0_CLKCntl;
};
#define     w32Gbl_STS0_SOPCntl                            {\
	UNSG32 uSTS0_SOPCntl_DS0                           :  1;\
	UNSG32 uSTS0_SOPCntl_DS1                           :  1;\
	UNSG32 uSTS0_SOPCntl_DS2                           :  1;\
	UNSG32 uSTS0_SOPCntl_IE                            :  1;\
	UNSG32 uSTS0_SOPCntl_PD                            :  1;\
	UNSG32 uSTS0_SOPCntl_PU                            :  1;\
	UNSG32 uSTS0_SOPCntl_SL                            :  1;\
	UNSG32 uSTS0_SOPCntl_ST                            :  1;\
	UNSG32 RSVDx8830_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS0_SOPCntl;
	struct w32Gbl_STS0_SOPCntl;
};
#define     w32Gbl_STS0_SDCntl                             {\
	UNSG32 uSTS0_SDCntl_DS0                            :  1;\
	UNSG32 uSTS0_SDCntl_DS1                            :  1;\
	UNSG32 uSTS0_SDCntl_DS2                            :  1;\
	UNSG32 uSTS0_SDCntl_IE                             :  1;\
	UNSG32 uSTS0_SDCntl_PD                             :  1;\
	UNSG32 uSTS0_SDCntl_PU                             :  1;\
	UNSG32 uSTS0_SDCntl_SL                             :  1;\
	UNSG32 uSTS0_SDCntl_ST                             :  1;\
	UNSG32 RSVDx8834_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS0_SDCntl;
	struct w32Gbl_STS0_SDCntl;
};
#define     w32Gbl_STS0_VALDCntl                           {\
	UNSG32 uSTS0_VALDCntl_DS0                          :  1;\
	UNSG32 uSTS0_VALDCntl_DS1                          :  1;\
	UNSG32 uSTS0_VALDCntl_DS2                          :  1;\
	UNSG32 uSTS0_VALDCntl_IE                           :  1;\
	UNSG32 uSTS0_VALDCntl_PD                           :  1;\
	UNSG32 uSTS0_VALDCntl_PU                           :  1;\
	UNSG32 uSTS0_VALDCntl_SL                           :  1;\
	UNSG32 uSTS0_VALDCntl_ST                           :  1;\
	UNSG32 RSVDx8838_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS0_VALDCntl;
	struct w32Gbl_STS0_VALDCntl;
};
#define     w32Gbl_STS1_CLKCntl                            {\
	UNSG32 uSTS1_CLKCntl_DS0                           :  1;\
	UNSG32 uSTS1_CLKCntl_DS1                           :  1;\
	UNSG32 uSTS1_CLKCntl_DS2                           :  1;\
	UNSG32 uSTS1_CLKCntl_IE                            :  1;\
	UNSG32 uSTS1_CLKCntl_PD                            :  1;\
	UNSG32 uSTS1_CLKCntl_PU                            :  1;\
	UNSG32 uSTS1_CLKCntl_SL                            :  1;\
	UNSG32 uSTS1_CLKCntl_ST                            :  1;\
	UNSG32 RSVDx883C_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS1_CLKCntl;
	struct w32Gbl_STS1_CLKCntl;
};
#define     w32Gbl_STS1_SOPCntl                            {\
	UNSG32 uSTS1_SOPCntl_DS0                           :  1;\
	UNSG32 uSTS1_SOPCntl_DS1                           :  1;\
	UNSG32 uSTS1_SOPCntl_DS2                           :  1;\
	UNSG32 uSTS1_SOPCntl_IE                            :  1;\
	UNSG32 uSTS1_SOPCntl_PD                            :  1;\
	UNSG32 uSTS1_SOPCntl_PU                            :  1;\
	UNSG32 uSTS1_SOPCntl_SL                            :  1;\
	UNSG32 uSTS1_SOPCntl_ST                            :  1;\
	UNSG32 RSVDx8840_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS1_SOPCntl;
	struct w32Gbl_STS1_SOPCntl;
};
#define     w32Gbl_STS1_SDCntl                             {\
	UNSG32 uSTS1_SDCntl_DS0                            :  1;\
	UNSG32 uSTS1_SDCntl_DS1                            :  1;\
	UNSG32 uSTS1_SDCntl_DS2                            :  1;\
	UNSG32 uSTS1_SDCntl_IE                             :  1;\
	UNSG32 uSTS1_SDCntl_PD                             :  1;\
	UNSG32 uSTS1_SDCntl_PU                             :  1;\
	UNSG32 uSTS1_SDCntl_SL                             :  1;\
	UNSG32 uSTS1_SDCntl_ST                             :  1;\
	UNSG32 RSVDx8844_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS1_SDCntl;
	struct w32Gbl_STS1_SDCntl;
};
#define     w32Gbl_STS1_VALDCntl                           {\
	UNSG32 uSTS1_VALDCntl_DS0                          :  1;\
	UNSG32 uSTS1_VALDCntl_DS1                          :  1;\
	UNSG32 uSTS1_VALDCntl_DS2                          :  1;\
	UNSG32 uSTS1_VALDCntl_IE                           :  1;\
	UNSG32 uSTS1_VALDCntl_PD                           :  1;\
	UNSG32 uSTS1_VALDCntl_PU                           :  1;\
	UNSG32 uSTS1_VALDCntl_SL                           :  1;\
	UNSG32 uSTS1_VALDCntl_ST                           :  1;\
	UNSG32 RSVDx8848_b8                                : 24;\
}
union { UNSG32 u32Gbl_STS1_VALDCntl;
	struct w32Gbl_STS1_VALDCntl;
};
#define     w32Gbl_USB2_DRV_VBUSCntl                       {\
	UNSG32 uUSB2_DRV_VBUSCntl_DS0                      :  1;\
	UNSG32 uUSB2_DRV_VBUSCntl_DS1                      :  1;\
	UNSG32 uUSB2_DRV_VBUSCntl_DS2                      :  1;\
	UNSG32 uUSB2_DRV_VBUSCntl_IE                       :  1;\
	UNSG32 uUSB2_DRV_VBUSCntl_PD                       :  1;\
	UNSG32 uUSB2_DRV_VBUSCntl_PU                       :  1;\
	UNSG32 uUSB2_DRV_VBUSCntl_SL                       :  1;\
	UNSG32 uUSB2_DRV_VBUSCntl_ST                       :  1;\
	UNSG32 RSVDx884C_b8                                : 24;\
}
union { UNSG32 u32Gbl_USB2_DRV_VBUSCntl;
	struct w32Gbl_USB2_DRV_VBUSCntl;
};
#define     w32Gbl_RGMII_MDCCntl                           {\
	UNSG32 uRGMII_MDCCntl_DS0                          :  1;\
	UNSG32 uRGMII_MDCCntl_DS1                          :  1;\
	UNSG32 uRGMII_MDCCntl_DS2                          :  1;\
	UNSG32 uRGMII_MDCCntl_IE                           :  1;\
	UNSG32 uRGMII_MDCCntl_PD                           :  1;\
	UNSG32 uRGMII_MDCCntl_PU                           :  1;\
	UNSG32 uRGMII_MDCCntl_SL                           :  1;\
	UNSG32 uRGMII_MDCCntl_ST                           :  1;\
	UNSG32 RSVDx8850_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_MDCCntl;
	struct w32Gbl_RGMII_MDCCntl;
};
#define     w32Gbl_RGMII_MDIOCntl                          {\
	UNSG32 uRGMII_MDIOCntl_DS0                         :  1;\
	UNSG32 uRGMII_MDIOCntl_DS1                         :  1;\
	UNSG32 uRGMII_MDIOCntl_DS2                         :  1;\
	UNSG32 uRGMII_MDIOCntl_IE                          :  1;\
	UNSG32 uRGMII_MDIOCntl_PD                          :  1;\
	UNSG32 uRGMII_MDIOCntl_PU                          :  1;\
	UNSG32 uRGMII_MDIOCntl_SL                          :  1;\
	UNSG32 uRGMII_MDIOCntl_ST                          :  1;\
	UNSG32 RSVDx8854_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_MDIOCntl;
	struct w32Gbl_RGMII_MDIOCntl;
};
#define     w32Gbl_RGMII_TXCCntl                           {\
	UNSG32 uRGMII_TXCCntl_DS0                          :  1;\
	UNSG32 uRGMII_TXCCntl_DS1                          :  1;\
	UNSG32 uRGMII_TXCCntl_DS2                          :  1;\
	UNSG32 uRGMII_TXCCntl_IE                           :  1;\
	UNSG32 uRGMII_TXCCntl_PD                           :  1;\
	UNSG32 uRGMII_TXCCntl_PU                           :  1;\
	UNSG32 uRGMII_TXCCntl_SL                           :  1;\
	UNSG32 uRGMII_TXCCntl_ST                           :  1;\
	UNSG32 RSVDx8858_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_TXCCntl;
	struct w32Gbl_RGMII_TXCCntl;
};
#define     w32Gbl_RGMII_TXD0Cntl                          {\
	UNSG32 uRGMII_TXD0Cntl_DS0                         :  1;\
	UNSG32 uRGMII_TXD0Cntl_DS1                         :  1;\
	UNSG32 uRGMII_TXD0Cntl_DS2                         :  1;\
	UNSG32 uRGMII_TXD0Cntl_IE                          :  1;\
	UNSG32 uRGMII_TXD0Cntl_PD                          :  1;\
	UNSG32 uRGMII_TXD0Cntl_PU                          :  1;\
	UNSG32 uRGMII_TXD0Cntl_SL                          :  1;\
	UNSG32 uRGMII_TXD0Cntl_ST                          :  1;\
	UNSG32 RSVDx885C_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_TXD0Cntl;
	struct w32Gbl_RGMII_TXD0Cntl;
};
#define     w32Gbl_RGMII_TXD1Cntl                          {\
	UNSG32 uRGMII_TXD1Cntl_DS0                         :  1;\
	UNSG32 uRGMII_TXD1Cntl_DS1                         :  1;\
	UNSG32 uRGMII_TXD1Cntl_DS2                         :  1;\
	UNSG32 uRGMII_TXD1Cntl_IE                          :  1;\
	UNSG32 uRGMII_TXD1Cntl_PD                          :  1;\
	UNSG32 uRGMII_TXD1Cntl_PU                          :  1;\
	UNSG32 uRGMII_TXD1Cntl_SL                          :  1;\
	UNSG32 uRGMII_TXD1Cntl_ST                          :  1;\
	UNSG32 RSVDx8860_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_TXD1Cntl;
	struct w32Gbl_RGMII_TXD1Cntl;
};
#define     w32Gbl_RGMII_TXD2Cntl                          {\
	UNSG32 uRGMII_TXD2Cntl_DS0                         :  1;\
	UNSG32 uRGMII_TXD2Cntl_DS1                         :  1;\
	UNSG32 uRGMII_TXD2Cntl_DS2                         :  1;\
	UNSG32 uRGMII_TXD2Cntl_IE                          :  1;\
	UNSG32 uRGMII_TXD2Cntl_PD                          :  1;\
	UNSG32 uRGMII_TXD2Cntl_PU                          :  1;\
	UNSG32 uRGMII_TXD2Cntl_SL                          :  1;\
	UNSG32 uRGMII_TXD2Cntl_ST                          :  1;\
	UNSG32 RSVDx8864_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_TXD2Cntl;
	struct w32Gbl_RGMII_TXD2Cntl;
};
#define     w32Gbl_RGMII_TXD3Cntl                          {\
	UNSG32 uRGMII_TXD3Cntl_DS0                         :  1;\
	UNSG32 uRGMII_TXD3Cntl_DS1                         :  1;\
	UNSG32 uRGMII_TXD3Cntl_DS2                         :  1;\
	UNSG32 uRGMII_TXD3Cntl_IE                          :  1;\
	UNSG32 uRGMII_TXD3Cntl_PD                          :  1;\
	UNSG32 uRGMII_TXD3Cntl_PU                          :  1;\
	UNSG32 uRGMII_TXD3Cntl_SL                          :  1;\
	UNSG32 uRGMII_TXD3Cntl_ST                          :  1;\
	UNSG32 RSVDx8868_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_TXD3Cntl;
	struct w32Gbl_RGMII_TXD3Cntl;
};
#define     w32Gbl_RGMII_TXCTLCntl                         {\
	UNSG32 uRGMII_TXCTLCntl_DS0                        :  1;\
	UNSG32 uRGMII_TXCTLCntl_DS1                        :  1;\
	UNSG32 uRGMII_TXCTLCntl_DS2                        :  1;\
	UNSG32 uRGMII_TXCTLCntl_IE                         :  1;\
	UNSG32 uRGMII_TXCTLCntl_PD                         :  1;\
	UNSG32 uRGMII_TXCTLCntl_PU                         :  1;\
	UNSG32 uRGMII_TXCTLCntl_SL                         :  1;\
	UNSG32 uRGMII_TXCTLCntl_ST                         :  1;\
	UNSG32 RSVDx886C_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_TXCTLCntl;
	struct w32Gbl_RGMII_TXCTLCntl;
};
#define     w32Gbl_RGMII_RXCCntl                           {\
	UNSG32 uRGMII_RXCCntl_DS0                          :  1;\
	UNSG32 uRGMII_RXCCntl_DS1                          :  1;\
	UNSG32 uRGMII_RXCCntl_DS2                          :  1;\
	UNSG32 uRGMII_RXCCntl_IE                           :  1;\
	UNSG32 uRGMII_RXCCntl_PD                           :  1;\
	UNSG32 uRGMII_RXCCntl_PU                           :  1;\
	UNSG32 uRGMII_RXCCntl_SL                           :  1;\
	UNSG32 uRGMII_RXCCntl_ST                           :  1;\
	UNSG32 RSVDx8870_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_RXCCntl;
	struct w32Gbl_RGMII_RXCCntl;
};
#define     w32Gbl_RGMII_RXD0Cntl                          {\
	UNSG32 uRGMII_RXD0Cntl_DS0                         :  1;\
	UNSG32 uRGMII_RXD0Cntl_DS1                         :  1;\
	UNSG32 uRGMII_RXD0Cntl_DS2                         :  1;\
	UNSG32 uRGMII_RXD0Cntl_IE                          :  1;\
	UNSG32 uRGMII_RXD0Cntl_PD                          :  1;\
	UNSG32 uRGMII_RXD0Cntl_PU                          :  1;\
	UNSG32 uRGMII_RXD0Cntl_SL                          :  1;\
	UNSG32 uRGMII_RXD0Cntl_ST                          :  1;\
	UNSG32 RSVDx8874_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_RXD0Cntl;
	struct w32Gbl_RGMII_RXD0Cntl;
};
#define     w32Gbl_RGMII_RXD1Cntl                          {\
	UNSG32 uRGMII_RXD1Cntl_DS0                         :  1;\
	UNSG32 uRGMII_RXD1Cntl_DS1                         :  1;\
	UNSG32 uRGMII_RXD1Cntl_DS2                         :  1;\
	UNSG32 uRGMII_RXD1Cntl_IE                          :  1;\
	UNSG32 uRGMII_RXD1Cntl_PD                          :  1;\
	UNSG32 uRGMII_RXD1Cntl_PU                          :  1;\
	UNSG32 uRGMII_RXD1Cntl_SL                          :  1;\
	UNSG32 uRGMII_RXD1Cntl_ST                          :  1;\
	UNSG32 RSVDx8878_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_RXD1Cntl;
	struct w32Gbl_RGMII_RXD1Cntl;
};
#define     w32Gbl_RGMII_RXD2Cntl                          {\
	UNSG32 uRGMII_RXD2Cntl_DS0                         :  1;\
	UNSG32 uRGMII_RXD2Cntl_DS1                         :  1;\
	UNSG32 uRGMII_RXD2Cntl_DS2                         :  1;\
	UNSG32 uRGMII_RXD2Cntl_IE                          :  1;\
	UNSG32 uRGMII_RXD2Cntl_PD                          :  1;\
	UNSG32 uRGMII_RXD2Cntl_PU                          :  1;\
	UNSG32 uRGMII_RXD2Cntl_SL                          :  1;\
	UNSG32 uRGMII_RXD2Cntl_ST                          :  1;\
	UNSG32 RSVDx887C_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_RXD2Cntl;
	struct w32Gbl_RGMII_RXD2Cntl;
};
#define     w32Gbl_RGMII_RXD3Cntl                          {\
	UNSG32 uRGMII_RXD3Cntl_DS0                         :  1;\
	UNSG32 uRGMII_RXD3Cntl_DS1                         :  1;\
	UNSG32 uRGMII_RXD3Cntl_DS2                         :  1;\
	UNSG32 uRGMII_RXD3Cntl_IE                          :  1;\
	UNSG32 uRGMII_RXD3Cntl_PD                          :  1;\
	UNSG32 uRGMII_RXD3Cntl_PU                          :  1;\
	UNSG32 uRGMII_RXD3Cntl_SL                          :  1;\
	UNSG32 uRGMII_RXD3Cntl_ST                          :  1;\
	UNSG32 RSVDx8880_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_RXD3Cntl;
	struct w32Gbl_RGMII_RXD3Cntl;
};
#define     w32Gbl_RGMII_RXCTLCntl                         {\
	UNSG32 uRGMII_RXCTLCntl_DS0                        :  1;\
	UNSG32 uRGMII_RXCTLCntl_DS1                        :  1;\
	UNSG32 uRGMII_RXCTLCntl_DS2                        :  1;\
	UNSG32 uRGMII_RXCTLCntl_IE                         :  1;\
	UNSG32 uRGMII_RXCTLCntl_PD                         :  1;\
	UNSG32 uRGMII_RXCTLCntl_PU                         :  1;\
	UNSG32 uRGMII_RXCTLCntl_SL                         :  1;\
	UNSG32 uRGMII_RXCTLCntl_ST                         :  1;\
	UNSG32 RSVDx8884_b8                                : 24;\
}
union { UNSG32 u32Gbl_RGMII_RXCTLCntl;
	struct w32Gbl_RGMII_RXCTLCntl;
};
UNSG8 RSVDx8888                                   [888];
#define     w32Gbl_I2S1_DO0Cntl                            {\
	UNSG32 uI2S1_DO0Cntl_DS0                           :  1;\
	UNSG32 uI2S1_DO0Cntl_DS1                           :  1;\
	UNSG32 uI2S1_DO0Cntl_DS2                           :  1;\
	UNSG32 uI2S1_DO0Cntl_IE                            :  1;\
	UNSG32 uI2S1_DO0Cntl_PD                            :  1;\
	UNSG32 uI2S1_DO0Cntl_PU                            :  1;\
	UNSG32 uI2S1_DO0Cntl_SL                            :  1;\
	UNSG32 uI2S1_DO0Cntl_ST                            :  1;\
	UNSG32 RSVDx8C00_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S1_DO0Cntl;
	struct w32Gbl_I2S1_DO0Cntl;
};
#define     w32Gbl_I2S1_DO1Cntl                            {\
	UNSG32 uI2S1_DO1Cntl_DS0                           :  1;\
	UNSG32 uI2S1_DO1Cntl_DS1                           :  1;\
	UNSG32 uI2S1_DO1Cntl_DS2                           :  1;\
	UNSG32 uI2S1_DO1Cntl_IE                            :  1;\
	UNSG32 uI2S1_DO1Cntl_PD                            :  1;\
	UNSG32 uI2S1_DO1Cntl_PU                            :  1;\
	UNSG32 uI2S1_DO1Cntl_SL                            :  1;\
	UNSG32 uI2S1_DO1Cntl_ST                            :  1;\
	UNSG32 RSVDx8C04_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S1_DO1Cntl;
	struct w32Gbl_I2S1_DO1Cntl;
};
#define     w32Gbl_I2S1_DO2Cntl                            {\
	UNSG32 uI2S1_DO2Cntl_DS0                           :  1;\
	UNSG32 uI2S1_DO2Cntl_DS1                           :  1;\
	UNSG32 uI2S1_DO2Cntl_DS2                           :  1;\
	UNSG32 uI2S1_DO2Cntl_IE                            :  1;\
	UNSG32 uI2S1_DO2Cntl_PD                            :  1;\
	UNSG32 uI2S1_DO2Cntl_PU                            :  1;\
	UNSG32 uI2S1_DO2Cntl_SL                            :  1;\
	UNSG32 uI2S1_DO2Cntl_ST                            :  1;\
	UNSG32 RSVDx8C08_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S1_DO2Cntl;
	struct w32Gbl_I2S1_DO2Cntl;
};
#define     w32Gbl_I2S1_DO3Cntl                            {\
	UNSG32 uI2S1_DO3Cntl_DS0                           :  1;\
	UNSG32 uI2S1_DO3Cntl_DS1                           :  1;\
	UNSG32 uI2S1_DO3Cntl_DS2                           :  1;\
	UNSG32 uI2S1_DO3Cntl_IE                            :  1;\
	UNSG32 uI2S1_DO3Cntl_PD                            :  1;\
	UNSG32 uI2S1_DO3Cntl_PU                            :  1;\
	UNSG32 uI2S1_DO3Cntl_SL                            :  1;\
	UNSG32 uI2S1_DO3Cntl_ST                            :  1;\
	UNSG32 RSVDx8C0C_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S1_DO3Cntl;
	struct w32Gbl_I2S1_DO3Cntl;
};
#define     w32Gbl_I2S1_LRCKIOCntl                         {\
	UNSG32 uI2S1_LRCKIOCntl_DS0                        :  1;\
	UNSG32 uI2S1_LRCKIOCntl_DS1                        :  1;\
	UNSG32 uI2S1_LRCKIOCntl_DS2                        :  1;\
	UNSG32 uI2S1_LRCKIOCntl_IE                         :  1;\
	UNSG32 uI2S1_LRCKIOCntl_PD                         :  1;\
	UNSG32 uI2S1_LRCKIOCntl_PU                         :  1;\
	UNSG32 uI2S1_LRCKIOCntl_SL                         :  1;\
	UNSG32 uI2S1_LRCKIOCntl_ST                         :  1;\
	UNSG32 RSVDx8C10_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S1_LRCKIOCntl;
	struct w32Gbl_I2S1_LRCKIOCntl;
};
#define     w32Gbl_I2S1_BCLKIOCntl                         {\
	UNSG32 uI2S1_BCLKIOCntl_DS0                        :  1;\
	UNSG32 uI2S1_BCLKIOCntl_DS1                        :  1;\
	UNSG32 uI2S1_BCLKIOCntl_DS2                        :  1;\
	UNSG32 uI2S1_BCLKIOCntl_IE                         :  1;\
	UNSG32 uI2S1_BCLKIOCntl_PD                         :  1;\
	UNSG32 uI2S1_BCLKIOCntl_PU                         :  1;\
	UNSG32 uI2S1_BCLKIOCntl_SL                         :  1;\
	UNSG32 uI2S1_BCLKIOCntl_ST                         :  1;\
	UNSG32 RSVDx8C14_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S1_BCLKIOCntl;
	struct w32Gbl_I2S1_BCLKIOCntl;
};
#define     w32Gbl_SPDIFOCntl                              {\
	UNSG32 uSPDIFOCntl_DS0                             :  1;\
	UNSG32 uSPDIFOCntl_DS1                             :  1;\
	UNSG32 uSPDIFOCntl_DS2                             :  1;\
	UNSG32 uSPDIFOCntl_IE                              :  1;\
	UNSG32 uSPDIFOCntl_PD                              :  1;\
	UNSG32 uSPDIFOCntl_PU                              :  1;\
	UNSG32 uSPDIFOCntl_SL                              :  1;\
	UNSG32 uSPDIFOCntl_ST                              :  1;\
	UNSG32 RSVDx8C18_b8                                : 24;\
}
union { UNSG32 u32Gbl_SPDIFOCntl;
	struct w32Gbl_SPDIFOCntl;
};
#define     w32Gbl_SPDIFICntl                              {\
	UNSG32 uSPDIFICntl_DS0                             :  1;\
	UNSG32 uSPDIFICntl_DS1                             :  1;\
	UNSG32 uSPDIFICntl_DS2                             :  1;\
	UNSG32 uSPDIFICntl_IE                              :  1;\
	UNSG32 uSPDIFICntl_PD                              :  1;\
	UNSG32 uSPDIFICntl_PU                              :  1;\
	UNSG32 uSPDIFICntl_SL                              :  1;\
	UNSG32 uSPDIFICntl_ST                              :  1;\
	UNSG32 RSVDx8C1C_b8                                : 24;\
}
union { UNSG32 u32Gbl_SPDIFICntl;
	struct w32Gbl_SPDIFICntl;
};
#define     w32Gbl_I2S2_LRCKIOCntl                         {\
	UNSG32 uI2S2_LRCKIOCntl_DS0                        :  1;\
	UNSG32 uI2S2_LRCKIOCntl_DS1                        :  1;\
	UNSG32 uI2S2_LRCKIOCntl_DS2                        :  1;\
	UNSG32 uI2S2_LRCKIOCntl_IE                         :  1;\
	UNSG32 uI2S2_LRCKIOCntl_PD                         :  1;\
	UNSG32 uI2S2_LRCKIOCntl_PU                         :  1;\
	UNSG32 uI2S2_LRCKIOCntl_SL                         :  1;\
	UNSG32 uI2S2_LRCKIOCntl_ST                         :  1;\
	UNSG32 RSVDx8C20_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S2_LRCKIOCntl;
	struct w32Gbl_I2S2_LRCKIOCntl;
};
#define     w32Gbl_I2S2_BCLKIOCntl                         {\
	UNSG32 uI2S2_BCLKIOCntl_DS0                        :  1;\
	UNSG32 uI2S2_BCLKIOCntl_DS1                        :  1;\
	UNSG32 uI2S2_BCLKIOCntl_DS2                        :  1;\
	UNSG32 uI2S2_BCLKIOCntl_IE                         :  1;\
	UNSG32 uI2S2_BCLKIOCntl_PD                         :  1;\
	UNSG32 uI2S2_BCLKIOCntl_PU                         :  1;\
	UNSG32 uI2S2_BCLKIOCntl_SL                         :  1;\
	UNSG32 uI2S2_BCLKIOCntl_ST                         :  1;\
	UNSG32 RSVDx8C24_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S2_BCLKIOCntl;
	struct w32Gbl_I2S2_BCLKIOCntl;
};
#define     w32Gbl_I2S2_DI0Cntl                            {\
	UNSG32 uI2S2_DI0Cntl_DS0                           :  1;\
	UNSG32 uI2S2_DI0Cntl_DS1                           :  1;\
	UNSG32 uI2S2_DI0Cntl_DS2                           :  1;\
	UNSG32 uI2S2_DI0Cntl_IE                            :  1;\
	UNSG32 uI2S2_DI0Cntl_PD                            :  1;\
	UNSG32 uI2S2_DI0Cntl_PU                            :  1;\
	UNSG32 uI2S2_DI0Cntl_SL                            :  1;\
	UNSG32 uI2S2_DI0Cntl_ST                            :  1;\
	UNSG32 RSVDx8C28_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S2_DI0Cntl;
	struct w32Gbl_I2S2_DI0Cntl;
};
#define     w32Gbl_I2S2_DI1Cntl                            {\
	UNSG32 uI2S2_DI1Cntl_DS0                           :  1;\
	UNSG32 uI2S2_DI1Cntl_DS1                           :  1;\
	UNSG32 uI2S2_DI1Cntl_DS2                           :  1;\
	UNSG32 uI2S2_DI1Cntl_IE                            :  1;\
	UNSG32 uI2S2_DI1Cntl_PD                            :  1;\
	UNSG32 uI2S2_DI1Cntl_PU                            :  1;\
	UNSG32 uI2S2_DI1Cntl_SL                            :  1;\
	UNSG32 uI2S2_DI1Cntl_ST                            :  1;\
	UNSG32 RSVDx8C2C_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S2_DI1Cntl;
	struct w32Gbl_I2S2_DI1Cntl;
};
#define     w32Gbl_I2S2_DI2Cntl                            {\
	UNSG32 uI2S2_DI2Cntl_DS0                           :  1;\
	UNSG32 uI2S2_DI2Cntl_DS1                           :  1;\
	UNSG32 uI2S2_DI2Cntl_DS2                           :  1;\
	UNSG32 uI2S2_DI2Cntl_IE                            :  1;\
	UNSG32 uI2S2_DI2Cntl_PD                            :  1;\
	UNSG32 uI2S2_DI2Cntl_PU                            :  1;\
	UNSG32 uI2S2_DI2Cntl_SL                            :  1;\
	UNSG32 uI2S2_DI2Cntl_ST                            :  1;\
	UNSG32 RSVDx8C30_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S2_DI2Cntl;
	struct w32Gbl_I2S2_DI2Cntl;
};
#define     w32Gbl_I2S2_DI3Cntl                            {\
	UNSG32 uI2S2_DI3Cntl_DS0                           :  1;\
	UNSG32 uI2S2_DI3Cntl_DS1                           :  1;\
	UNSG32 uI2S2_DI3Cntl_DS2                           :  1;\
	UNSG32 uI2S2_DI3Cntl_IE                            :  1;\
	UNSG32 uI2S2_DI3Cntl_PD                            :  1;\
	UNSG32 uI2S2_DI3Cntl_PU                            :  1;\
	UNSG32 uI2S2_DI3Cntl_SL                            :  1;\
	UNSG32 uI2S2_DI3Cntl_ST                            :  1;\
	UNSG32 RSVDx8C34_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S2_DI3Cntl;
	struct w32Gbl_I2S2_DI3Cntl;
};
#define     w32Gbl_I2S1_MCLKCntl                           {\
	UNSG32 uI2S1_MCLKCntl_DS0                          :  1;\
	UNSG32 uI2S1_MCLKCntl_DS1                          :  1;\
	UNSG32 uI2S1_MCLKCntl_DS2                          :  1;\
	UNSG32 uI2S1_MCLKCntl_IE                           :  1;\
	UNSG32 uI2S1_MCLKCntl_PD                           :  1;\
	UNSG32 uI2S1_MCLKCntl_PU                           :  1;\
	UNSG32 uI2S1_MCLKCntl_SL                           :  1;\
	UNSG32 uI2S1_MCLKCntl_ST                           :  1;\
	UNSG32 RSVDx8C38_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S1_MCLKCntl;
	struct w32Gbl_I2S1_MCLKCntl;
};
#define     w32Gbl_I2S2_MCLKCntl                           {\
	UNSG32 uI2S2_MCLKCntl_DS0                          :  1;\
	UNSG32 uI2S2_MCLKCntl_DS1                          :  1;\
	UNSG32 uI2S2_MCLKCntl_DS2                          :  1;\
	UNSG32 uI2S2_MCLKCntl_IE                           :  1;\
	UNSG32 uI2S2_MCLKCntl_PD                           :  1;\
	UNSG32 uI2S2_MCLKCntl_PU                           :  1;\
	UNSG32 uI2S2_MCLKCntl_SL                           :  1;\
	UNSG32 uI2S2_MCLKCntl_ST                           :  1;\
	UNSG32 RSVDx8C3C_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S2_MCLKCntl;
	struct w32Gbl_I2S2_MCLKCntl;
};
#define     w32Gbl_TX_EDDC_SCLCntl                         {\
	UNSG32 uTX_EDDC_SCLCntl_DS0                        :  1;\
	UNSG32 uTX_EDDC_SCLCntl_DS1                        :  1;\
	UNSG32 uTX_EDDC_SCLCntl_DS2                        :  1;\
	UNSG32 uTX_EDDC_SCLCntl_DS3                        :  1;\
	UNSG32 uTX_EDDC_SCLCntl_IE                         :  1;\
	UNSG32 uTX_EDDC_SCLCntl_PE                         :  1;\
	UNSG32 uTX_EDDC_SCLCntl_PS                         :  1;\
	UNSG32 uTX_EDDC_SCLCntl_SL                         :  1;\
	UNSG32 uTX_EDDC_SCLCntl_SPU                        :  1;\
	UNSG32 uTX_EDDC_SCLCntl_ST                         :  1;\
	UNSG32 RSVDx8C40_b10                               : 22;\
}
union { UNSG32 u32Gbl_TX_EDDC_SCLCntl;
	struct w32Gbl_TX_EDDC_SCLCntl;
};
#define     w32Gbl_TX_EDDC_SDACntl                         {\
	UNSG32 uTX_EDDC_SDACntl_DS0                        :  1;\
	UNSG32 uTX_EDDC_SDACntl_DS1                        :  1;\
	UNSG32 uTX_EDDC_SDACntl_DS2                        :  1;\
	UNSG32 uTX_EDDC_SDACntl_DS3                        :  1;\
	UNSG32 uTX_EDDC_SDACntl_IE                         :  1;\
	UNSG32 uTX_EDDC_SDACntl_PE                         :  1;\
	UNSG32 uTX_EDDC_SDACntl_PS                         :  1;\
	UNSG32 uTX_EDDC_SDACntl_SL                         :  1;\
	UNSG32 uTX_EDDC_SDACntl_SPU                        :  1;\
	UNSG32 uTX_EDDC_SDACntl_ST                         :  1;\
	UNSG32 RSVDx8C44_b10                               : 22;\
}
union { UNSG32 u32Gbl_TX_EDDC_SDACntl;
	struct w32Gbl_TX_EDDC_SDACntl;
};
#define     w32Gbl_I2S3_DOCntl                             {\
	UNSG32 uI2S3_DOCntl_DS0                            :  1;\
	UNSG32 uI2S3_DOCntl_DS1                            :  1;\
	UNSG32 uI2S3_DOCntl_DS2                            :  1;\
	UNSG32 uI2S3_DOCntl_IE                             :  1;\
	UNSG32 uI2S3_DOCntl_PD                             :  1;\
	UNSG32 uI2S3_DOCntl_PU                             :  1;\
	UNSG32 uI2S3_DOCntl_SL                             :  1;\
	UNSG32 uI2S3_DOCntl_ST                             :  1;\
	UNSG32 RSVDx8C48_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S3_DOCntl;
	struct w32Gbl_I2S3_DOCntl;
};
#define     w32Gbl_I2S3_LRCKIOCntl                         {\
	UNSG32 uI2S3_LRCKIOCntl_DS0                        :  1;\
	UNSG32 uI2S3_LRCKIOCntl_DS1                        :  1;\
	UNSG32 uI2S3_LRCKIOCntl_DS2                        :  1;\
	UNSG32 uI2S3_LRCKIOCntl_IE                         :  1;\
	UNSG32 uI2S3_LRCKIOCntl_PD                         :  1;\
	UNSG32 uI2S3_LRCKIOCntl_PU                         :  1;\
	UNSG32 uI2S3_LRCKIOCntl_SL                         :  1;\
	UNSG32 uI2S3_LRCKIOCntl_ST                         :  1;\
	UNSG32 RSVDx8C4C_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S3_LRCKIOCntl;
	struct w32Gbl_I2S3_LRCKIOCntl;
};
#define     w32Gbl_I2S3_BCLKIOCntl                         {\
	UNSG32 uI2S3_BCLKIOCntl_DS0                        :  1;\
	UNSG32 uI2S3_BCLKIOCntl_DS1                        :  1;\
	UNSG32 uI2S3_BCLKIOCntl_DS2                        :  1;\
	UNSG32 uI2S3_BCLKIOCntl_IE                         :  1;\
	UNSG32 uI2S3_BCLKIOCntl_PD                         :  1;\
	UNSG32 uI2S3_BCLKIOCntl_PU                         :  1;\
	UNSG32 uI2S3_BCLKIOCntl_SL                         :  1;\
	UNSG32 uI2S3_BCLKIOCntl_ST                         :  1;\
	UNSG32 RSVDx8C50_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S3_BCLKIOCntl;
	struct w32Gbl_I2S3_BCLKIOCntl;
};
#define     w32Gbl_I2S3_DICntl                             {\
	UNSG32 uI2S3_DICntl_DS0                            :  1;\
	UNSG32 uI2S3_DICntl_DS1                            :  1;\
	UNSG32 uI2S3_DICntl_DS2                            :  1;\
	UNSG32 uI2S3_DICntl_IE                             :  1;\
	UNSG32 uI2S3_DICntl_PD                             :  1;\
	UNSG32 uI2S3_DICntl_PU                             :  1;\
	UNSG32 uI2S3_DICntl_SL                             :  1;\
	UNSG32 uI2S3_DICntl_ST                             :  1;\
	UNSG32 RSVDx8C54_b8                                : 24;\
}
union { UNSG32 u32Gbl_I2S3_DICntl;
	struct w32Gbl_I2S3_DICntl;
};
} SIE_Gbl;
typedef union  T32Gbl_ProductId
{ UNSG32 u32;
	struct w32Gbl_ProductId;
} T32Gbl_ProductId;
typedef union  T32Gbl_ProductId_ext
{ UNSG32 u32;
	struct w32Gbl_ProductId_ext;
} T32Gbl_ProductId_ext;
typedef union  T32Gbl_INT_ID
{ UNSG32 u32;
	struct w32Gbl_INT_ID;
} T32Gbl_INT_ID;
typedef union  T32Gbl_bootStrap
{ UNSG32 u32;
	struct w32Gbl_bootStrap;
} T32Gbl_bootStrap;
typedef union  T32Gbl_bootStrapEn
{ UNSG32 u32;
	struct w32Gbl_bootStrapEn;
} T32Gbl_bootStrapEn;
typedef union  T32Gbl_pkgSel
{ UNSG32 u32;
	struct w32Gbl_pkgSel;
} T32Gbl_pkgSel;
typedef union  T32Gbl_chipCntl
{ UNSG32 u32;
	struct w32Gbl_chipCntl;
} T32Gbl_chipCntl;
typedef union  T32Gbl_stsCntl
{ UNSG32 u32;
	struct w32Gbl_stsCntl;
} T32Gbl_stsCntl;
typedef union  T32Gbl_sw_generic0
{ UNSG32 u32;
	struct w32Gbl_sw_generic0;
} T32Gbl_sw_generic0;
typedef union  T32Gbl_sw_generic1
{ UNSG32 u32;
	struct w32Gbl_sw_generic1;
} T32Gbl_sw_generic1;
typedef union  T32Gbl_sw_generic2
{ UNSG32 u32;
	struct w32Gbl_sw_generic2;
} T32Gbl_sw_generic2;
typedef union  T32Gbl_sw_generic3
{ UNSG32 u32;
	struct w32Gbl_sw_generic3;
} T32Gbl_sw_generic3;
typedef union  T32Gbl_FPGAR
{ UNSG32 u32;
	struct w32Gbl_FPGAR;
} T32Gbl_FPGAR;
typedef union  T32Gbl_FPGARW
{ UNSG32 u32;
	struct w32Gbl_FPGARW;
} T32Gbl_FPGARW;
typedef union  T32Gbl_ResetTrigger
{ UNSG32 u32;
	struct w32Gbl_ResetTrigger;
} T32Gbl_ResetTrigger;
typedef union  T32Gbl_ResetStatus
{ UNSG32 u32;
	struct w32Gbl_ResetStatus;
} T32Gbl_ResetStatus;
typedef union  T32Gbl_WDTResetStatus
{ UNSG32 u32;
	struct w32Gbl_WDTResetStatus;
} T32Gbl_WDTResetStatus;
typedef union  T32Gbl_avioReset
{ UNSG32 u32;
	struct w32Gbl_avioReset;
} T32Gbl_avioReset;
typedef union  T32Gbl_avioResetStatus
{ UNSG32 u32;
	struct w32Gbl_avioResetStatus;
} T32Gbl_avioResetStatus;
typedef union  T32Gbl_gethRgmiiRstn
{ UNSG32 u32;
	struct w32Gbl_gethRgmiiRstn;
} T32Gbl_gethRgmiiRstn;
typedef union  T32Gbl_gethRgmiiResetStatus
{ UNSG32 u32;
	struct w32Gbl_gethRgmiiResetStatus;
} T32Gbl_gethRgmiiResetStatus;
typedef union  T32Gbl_gfx3DReset
{ UNSG32 u32;
	struct w32Gbl_gfx3DReset;
} T32Gbl_gfx3DReset;
typedef union  T32Gbl_gfx3DResetStatus
{ UNSG32 u32;
	struct w32Gbl_gfx3DResetStatus;
} T32Gbl_gfx3DResetStatus;
typedef union  T32Gbl_decoderReset
{ UNSG32 u32;
	struct w32Gbl_decoderReset;
} T32Gbl_decoderReset;
typedef union  T32Gbl_decoderResetStatus
{ UNSG32 u32;
	struct w32Gbl_decoderResetStatus;
} T32Gbl_decoderResetStatus;
typedef union  T32Gbl_encoderReset
{ UNSG32 u32;
	struct w32Gbl_encoderReset;
} T32Gbl_encoderReset;
typedef union  T32Gbl_encoderResetStatus
{ UNSG32 u32;
	struct w32Gbl_encoderResetStatus;
} T32Gbl_encoderResetStatus;
typedef union  T32Gbl_ovpReset
{ UNSG32 u32;
	struct w32Gbl_ovpReset;
} T32Gbl_ovpReset;
typedef union  T32Gbl_ovpResetStatus
{ UNSG32 u32;
	struct w32Gbl_ovpResetStatus;
} T32Gbl_ovpResetStatus;
typedef union  T32Gbl_npuReset
{ UNSG32 u32;
	struct w32Gbl_npuReset;
} T32Gbl_npuReset;
typedef union  T32Gbl_npuResetStatus
{ UNSG32 u32;
	struct w32Gbl_npuResetStatus;
} T32Gbl_npuResetStatus;
typedef union  T32Gbl_almGRstn
{ UNSG32 u32;
	struct w32Gbl_almGRstn;
} T32Gbl_almGRstn;
typedef union  T32Gbl_almResetStatus
{ UNSG32 u32;
	struct w32Gbl_almResetStatus;
} T32Gbl_almResetStatus;
typedef union  T32Gbl_perifReset
{ UNSG32 u32;
	struct w32Gbl_perifReset;
} T32Gbl_perifReset;
typedef union  T32Gbl_perifResetStatus
{ UNSG32 u32;
	struct w32Gbl_perifResetStatus;
} T32Gbl_perifResetStatus;
typedef union  T32Gbl_perifStickyResetN
{ UNSG32 u32;
	struct w32Gbl_perifStickyResetN;
} T32Gbl_perifStickyResetN;
typedef union  T32Gbl_apbPerifResetTrigger
{ UNSG32 u32;
	struct w32Gbl_apbPerifResetTrigger;
} T32Gbl_apbPerifResetTrigger;
typedef union  T32Gbl_apbPerifResetStatus
{ UNSG32 u32;
	struct w32Gbl_apbPerifResetStatus;
} T32Gbl_apbPerifResetStatus;
typedef union  T32Gbl_clkEnable
{ UNSG32 u32;
	struct w32Gbl_clkEnable;
} T32Gbl_clkEnable;
typedef union  T32Gbl_ClkSwitch
{ UNSG32 u32;
	struct w32Gbl_ClkSwitch;
} T32Gbl_ClkSwitch;
typedef union  T32Gbl_SECURE_SCAN_EN
{ UNSG32 u32;
	struct w32Gbl_SECURE_SCAN_EN;
} T32Gbl_SECURE_SCAN_EN;
typedef union  T32Gbl_gfx3DDisRamClkGate
{ UNSG32 u32;
	struct w32Gbl_gfx3DDisRamClkGate;
} T32Gbl_gfx3DDisRamClkGate;
typedef union  T32Gbl_gic400_ctrl
{ UNSG32 u32;
	struct w32Gbl_gic400_ctrl;
} T32Gbl_gic400_ctrl;
typedef union  T32Gbl_GFX_CTRL_0
{ UNSG32 u32;
	struct w32Gbl_GFX_CTRL_0;
} T32Gbl_GFX_CTRL_0;
typedef union  T32Gbl_GFX_CTRL_1
{ UNSG32 u32;
	struct w32Gbl_GFX_CTRL_1;
} T32Gbl_GFX_CTRL_1;
typedef union  T32Gbl_GFX_CTRL_2
{ UNSG32 u32;
	struct w32Gbl_GFX_CTRL_2;
} T32Gbl_GFX_CTRL_2;
typedef union  T32Gbl_GFX_CTRL_3
{ UNSG32 u32;
	struct w32Gbl_GFX_CTRL_3;
} T32Gbl_GFX_CTRL_3;
typedef union  T32Gbl_GFX_TIMER_VALUE
{ UNSG32 u32;
	struct w32Gbl_GFX_TIMER_VALUE;
} T32Gbl_GFX_TIMER_VALUE;
typedef union  T32Gbl_GFX_TIMER_VALUE1
{ UNSG32 u32;
	struct w32Gbl_GFX_TIMER_VALUE1;
} T32Gbl_GFX_TIMER_VALUE1;
typedef union  T32Gbl_GFX_STATUS
{ UNSG32 u32;
	struct w32Gbl_GFX_STATUS;
} T32Gbl_GFX_STATUS;
typedef union  T32Gbl_USBOTG_REFCLK_CTRL0
{ UNSG32 u32;
	struct w32Gbl_USBOTG_REFCLK_CTRL0;
} T32Gbl_USBOTG_REFCLK_CTRL0;
typedef union  T32Gbl_USBOTG_REFCLK_CTRL1
{ UNSG32 u32;
	struct w32Gbl_USBOTG_REFCLK_CTRL1;
} T32Gbl_USBOTG_REFCLK_CTRL1;
typedef union  T32Gbl_pinMuxCntlBus
{ UNSG32 u32;
	struct w32Gbl_pinMuxCntlBus;
} T32Gbl_pinMuxCntlBus;
typedef union  T32Gbl_pinMuxCntlBus1
{ UNSG32 u32;
	struct w32Gbl_pinMuxCntlBus1;
} T32Gbl_pinMuxCntlBus1;
typedef union  T32Gbl_pinMuxCntlBus2
{ UNSG32 u32;
	struct w32Gbl_pinMuxCntlBus2;
} T32Gbl_pinMuxCntlBus2;
typedef union  T32Gbl_pinMuxCntlBus3
{ UNSG32 u32;
	struct w32Gbl_pinMuxCntlBus3;
} T32Gbl_pinMuxCntlBus3;
typedef union  T32Gbl_avioPinMuxCntlBus
{ UNSG32 u32;
	struct w32Gbl_avioPinMuxCntlBus;
} T32Gbl_avioPinMuxCntlBus;
typedef union  T32Gbl_avioPinMuxCntlBus1
{ UNSG32 u32;
	struct w32Gbl_avioPinMuxCntlBus1;
} T32Gbl_avioPinMuxCntlBus1;
typedef union  T32Gbl_avioPinMuxCntlBus2
{ UNSG32 u32;
	struct w32Gbl_avioPinMuxCntlBus2;
} T32Gbl_avioPinMuxCntlBus2;
typedef union  T32Gbl_SDIO_CDnCntl
{ UNSG32 u32;
	struct w32Gbl_SDIO_CDnCntl;
} T32Gbl_SDIO_CDnCntl;
typedef union  T32Gbl_SDIO_WPCntl
{ UNSG32 u32;
	struct w32Gbl_SDIO_WPCntl;
} T32Gbl_SDIO_WPCntl;
typedef union  T32Gbl_SPI1_SS0nCntl
{ UNSG32 u32;
	struct w32Gbl_SPI1_SS0nCntl;
} T32Gbl_SPI1_SS0nCntl;
typedef union  T32Gbl_SPI1_SS1nCntl
{ UNSG32 u32;
	struct w32Gbl_SPI1_SS1nCntl;
} T32Gbl_SPI1_SS1nCntl;
typedef union  T32Gbl_SPI1_SS2nCntl
{ UNSG32 u32;
	struct w32Gbl_SPI1_SS2nCntl;
} T32Gbl_SPI1_SS2nCntl;
typedef union  T32Gbl_SPI1_SS3nCntl
{ UNSG32 u32;
	struct w32Gbl_SPI1_SS3nCntl;
} T32Gbl_SPI1_SS3nCntl;
typedef union  T32Gbl_SPI1_SDOCntl
{ UNSG32 u32;
	struct w32Gbl_SPI1_SDOCntl;
} T32Gbl_SPI1_SDOCntl;
typedef union  T32Gbl_SPI1_SCLKCntl
{ UNSG32 u32;
	struct w32Gbl_SPI1_SCLKCntl;
} T32Gbl_SPI1_SCLKCntl;
typedef union  T32Gbl_SPI1_SDICntl
{ UNSG32 u32;
	struct w32Gbl_SPI1_SDICntl;
} T32Gbl_SPI1_SDICntl;
typedef union  T32Gbl_TW0_SCLCntl
{ UNSG32 u32;
	struct w32Gbl_TW0_SCLCntl;
} T32Gbl_TW0_SCLCntl;
typedef union  T32Gbl_TW0_SDACntl
{ UNSG32 u32;
	struct w32Gbl_TW0_SDACntl;
} T32Gbl_TW0_SDACntl;
typedef union  T32Gbl_STS0_CLKCntl
{ UNSG32 u32;
	struct w32Gbl_STS0_CLKCntl;
} T32Gbl_STS0_CLKCntl;
typedef union  T32Gbl_STS0_SOPCntl
{ UNSG32 u32;
	struct w32Gbl_STS0_SOPCntl;
} T32Gbl_STS0_SOPCntl;
typedef union  T32Gbl_STS0_SDCntl
{ UNSG32 u32;
	struct w32Gbl_STS0_SDCntl;
} T32Gbl_STS0_SDCntl;
typedef union  T32Gbl_STS0_VALDCntl
{ UNSG32 u32;
	struct w32Gbl_STS0_VALDCntl;
} T32Gbl_STS0_VALDCntl;
typedef union  T32Gbl_STS1_CLKCntl
{ UNSG32 u32;
	struct w32Gbl_STS1_CLKCntl;
} T32Gbl_STS1_CLKCntl;
typedef union  T32Gbl_STS1_SOPCntl
{ UNSG32 u32;
	struct w32Gbl_STS1_SOPCntl;
} T32Gbl_STS1_SOPCntl;
typedef union  T32Gbl_STS1_SDCntl
{ UNSG32 u32;
	struct w32Gbl_STS1_SDCntl;
} T32Gbl_STS1_SDCntl;
typedef union  T32Gbl_STS1_VALDCntl
{ UNSG32 u32;
	struct w32Gbl_STS1_VALDCntl;
} T32Gbl_STS1_VALDCntl;
typedef union  T32Gbl_USB2_DRV_VBUSCntl
{ UNSG32 u32;
	struct w32Gbl_USB2_DRV_VBUSCntl;
} T32Gbl_USB2_DRV_VBUSCntl;
typedef union  T32Gbl_RGMII_MDCCntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_MDCCntl;
} T32Gbl_RGMII_MDCCntl;
typedef union  T32Gbl_RGMII_MDIOCntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_MDIOCntl;
} T32Gbl_RGMII_MDIOCntl;
typedef union  T32Gbl_RGMII_TXCCntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_TXCCntl;
} T32Gbl_RGMII_TXCCntl;
typedef union  T32Gbl_RGMII_TXD0Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_TXD0Cntl;
} T32Gbl_RGMII_TXD0Cntl;
typedef union  T32Gbl_RGMII_TXD1Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_TXD1Cntl;
} T32Gbl_RGMII_TXD1Cntl;
typedef union  T32Gbl_RGMII_TXD2Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_TXD2Cntl;
} T32Gbl_RGMII_TXD2Cntl;
typedef union  T32Gbl_RGMII_TXD3Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_TXD3Cntl;
} T32Gbl_RGMII_TXD3Cntl;
typedef union  T32Gbl_RGMII_TXCTLCntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_TXCTLCntl;
} T32Gbl_RGMII_TXCTLCntl;
typedef union  T32Gbl_RGMII_RXCCntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_RXCCntl;
} T32Gbl_RGMII_RXCCntl;
typedef union  T32Gbl_RGMII_RXD0Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_RXD0Cntl;
} T32Gbl_RGMII_RXD0Cntl;
typedef union  T32Gbl_RGMII_RXD1Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_RXD1Cntl;
} T32Gbl_RGMII_RXD1Cntl;
typedef union  T32Gbl_RGMII_RXD2Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_RXD2Cntl;
} T32Gbl_RGMII_RXD2Cntl;
typedef union  T32Gbl_RGMII_RXD3Cntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_RXD3Cntl;
} T32Gbl_RGMII_RXD3Cntl;
typedef union  T32Gbl_RGMII_RXCTLCntl
{ UNSG32 u32;
	struct w32Gbl_RGMII_RXCTLCntl;
} T32Gbl_RGMII_RXCTLCntl;
typedef union  T32Gbl_I2S1_DO0Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S1_DO0Cntl;
} T32Gbl_I2S1_DO0Cntl;
typedef union  T32Gbl_I2S1_DO1Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S1_DO1Cntl;
} T32Gbl_I2S1_DO1Cntl;
typedef union  T32Gbl_I2S1_DO2Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S1_DO2Cntl;
} T32Gbl_I2S1_DO2Cntl;
typedef union  T32Gbl_I2S1_DO3Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S1_DO3Cntl;
} T32Gbl_I2S1_DO3Cntl;
typedef union  T32Gbl_I2S1_LRCKIOCntl
{ UNSG32 u32;
	struct w32Gbl_I2S1_LRCKIOCntl;
} T32Gbl_I2S1_LRCKIOCntl;
typedef union  T32Gbl_I2S1_BCLKIOCntl
{ UNSG32 u32;
	struct w32Gbl_I2S1_BCLKIOCntl;
} T32Gbl_I2S1_BCLKIOCntl;
typedef union  T32Gbl_SPDIFOCntl
{ UNSG32 u32;
	struct w32Gbl_SPDIFOCntl;
} T32Gbl_SPDIFOCntl;
typedef union  T32Gbl_SPDIFICntl
{ UNSG32 u32;
	struct w32Gbl_SPDIFICntl;
} T32Gbl_SPDIFICntl;
typedef union  T32Gbl_I2S2_LRCKIOCntl
{ UNSG32 u32;
	struct w32Gbl_I2S2_LRCKIOCntl;
} T32Gbl_I2S2_LRCKIOCntl;
typedef union  T32Gbl_I2S2_BCLKIOCntl
{ UNSG32 u32;
	struct w32Gbl_I2S2_BCLKIOCntl;
} T32Gbl_I2S2_BCLKIOCntl;
typedef union  T32Gbl_I2S2_DI0Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S2_DI0Cntl;
} T32Gbl_I2S2_DI0Cntl;
typedef union  T32Gbl_I2S2_DI1Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S2_DI1Cntl;
} T32Gbl_I2S2_DI1Cntl;
typedef union  T32Gbl_I2S2_DI2Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S2_DI2Cntl;
} T32Gbl_I2S2_DI2Cntl;
typedef union  T32Gbl_I2S2_DI3Cntl
{ UNSG32 u32;
	struct w32Gbl_I2S2_DI3Cntl;
} T32Gbl_I2S2_DI3Cntl;
typedef union  T32Gbl_I2S1_MCLKCntl
{ UNSG32 u32;
	struct w32Gbl_I2S1_MCLKCntl;
} T32Gbl_I2S1_MCLKCntl;
typedef union  T32Gbl_I2S2_MCLKCntl
{ UNSG32 u32;
	struct w32Gbl_I2S2_MCLKCntl;
} T32Gbl_I2S2_MCLKCntl;
typedef union  T32Gbl_TX_EDDC_SCLCntl
{ UNSG32 u32;
	struct w32Gbl_TX_EDDC_SCLCntl;
} T32Gbl_TX_EDDC_SCLCntl;
typedef union  T32Gbl_TX_EDDC_SDACntl
{ UNSG32 u32;
	struct w32Gbl_TX_EDDC_SDACntl;
} T32Gbl_TX_EDDC_SDACntl;
typedef union  T32Gbl_I2S3_DOCntl
{ UNSG32 u32;
	struct w32Gbl_I2S3_DOCntl;
} T32Gbl_I2S3_DOCntl;
typedef union  T32Gbl_I2S3_LRCKIOCntl
{ UNSG32 u32;
	struct w32Gbl_I2S3_LRCKIOCntl;
} T32Gbl_I2S3_LRCKIOCntl;
typedef union  T32Gbl_I2S3_BCLKIOCntl
{ UNSG32 u32;
	struct w32Gbl_I2S3_BCLKIOCntl;
} T32Gbl_I2S3_BCLKIOCntl;
typedef union  T32Gbl_I2S3_DICntl
{ UNSG32 u32;
	struct w32Gbl_I2S3_DICntl;
} T32Gbl_I2S3_DICntl;
typedef union  TGbl_ProductId
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_ProductId;
	};
} TGbl_ProductId;
typedef union  TGbl_ProductId_ext
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_ProductId_ext;
	};
} TGbl_ProductId_ext;
typedef union  TGbl_INT_ID
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_INT_ID;
	};
} TGbl_INT_ID;
typedef union  TGbl_bootStrap
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_bootStrap;
	};
} TGbl_bootStrap;
typedef union  TGbl_bootStrapEn
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_bootStrapEn;
	};
} TGbl_bootStrapEn;
typedef union  TGbl_pkgSel
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_pkgSel;
	};
} TGbl_pkgSel;
typedef union  TGbl_chipCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_chipCntl;
	};
} TGbl_chipCntl;
typedef union  TGbl_stsCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_stsCntl;
	};
} TGbl_stsCntl;
typedef union  TGbl_sw_generic0
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_sw_generic0;
	};
} TGbl_sw_generic0;
typedef union  TGbl_sw_generic1
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_sw_generic1;
	};
} TGbl_sw_generic1;
typedef union  TGbl_sw_generic2
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_sw_generic2;
	};
} TGbl_sw_generic2;
typedef union  TGbl_sw_generic3
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_sw_generic3;
	};
} TGbl_sw_generic3;
typedef union  TGbl_FPGAR
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_FPGAR;
	};
} TGbl_FPGAR;
typedef union  TGbl_FPGARW
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_FPGARW;
	};
} TGbl_FPGARW;
typedef union  TGbl_ResetTrigger
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_ResetTrigger;
	};
} TGbl_ResetTrigger;
typedef union  TGbl_ResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_ResetStatus;
	};
} TGbl_ResetStatus;
typedef union  TGbl_WDTResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_WDTResetStatus;
	};
} TGbl_WDTResetStatus;
typedef union  TGbl_avioReset
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_avioReset;
	};
} TGbl_avioReset;
typedef union  TGbl_avioResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_avioResetStatus;
	};
} TGbl_avioResetStatus;
typedef union  TGbl_gethRgmiiRstn
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_gethRgmiiRstn;
	};
} TGbl_gethRgmiiRstn;
typedef union  TGbl_gethRgmiiResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_gethRgmiiResetStatus;
	};
} TGbl_gethRgmiiResetStatus;
typedef union  TGbl_gfx3DReset
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_gfx3DReset;
	};
} TGbl_gfx3DReset;
typedef union  TGbl_gfx3DResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_gfx3DResetStatus;
	};
} TGbl_gfx3DResetStatus;
typedef union  TGbl_decoderReset
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_decoderReset;
	};
} TGbl_decoderReset;
typedef union  TGbl_decoderResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_decoderResetStatus;
	};
} TGbl_decoderResetStatus;
typedef union  TGbl_encoderReset
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_encoderReset;
	};
} TGbl_encoderReset;
typedef union  TGbl_encoderResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_encoderResetStatus;
	};
} TGbl_encoderResetStatus;
typedef union  TGbl_ovpReset
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_ovpReset;
	};
} TGbl_ovpReset;
typedef union  TGbl_ovpResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_ovpResetStatus;
	};
} TGbl_ovpResetStatus;
typedef union  TGbl_npuReset
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_npuReset;
	};
} TGbl_npuReset;
typedef union  TGbl_npuResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_npuResetStatus;
	};
} TGbl_npuResetStatus;
typedef union  TGbl_almGRstn
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_almGRstn;
	};
} TGbl_almGRstn;
typedef union  TGbl_almResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_almResetStatus;
	};
} TGbl_almResetStatus;
typedef union  TGbl_perifReset
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_perifReset;
	};
} TGbl_perifReset;
typedef union  TGbl_perifResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_perifResetStatus;
	};
} TGbl_perifResetStatus;
typedef union  TGbl_perifStickyResetN
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_perifStickyResetN;
	};
} TGbl_perifStickyResetN;
typedef union  TGbl_apbPerifResetTrigger
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_apbPerifResetTrigger;
	};
} TGbl_apbPerifResetTrigger;
typedef union  TGbl_apbPerifResetStatus
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_apbPerifResetStatus;
	};
} TGbl_apbPerifResetStatus;
typedef union  TGbl_clkEnable
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_clkEnable;
	};
} TGbl_clkEnable;
typedef union  TGbl_ClkSwitch
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_ClkSwitch;
	};
} TGbl_ClkSwitch;
typedef union  TGbl_SECURE_SCAN_EN
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SECURE_SCAN_EN;
	};
} TGbl_SECURE_SCAN_EN;
typedef union  TGbl_gfx3DDisRamClkGate
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_gfx3DDisRamClkGate;
	};
} TGbl_gfx3DDisRamClkGate;
typedef union  TGbl_gic400_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_gic400_ctrl;
	};
} TGbl_gic400_ctrl;
typedef union  TGbl_GFX_CTRL_0
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_GFX_CTRL_0;
	};
} TGbl_GFX_CTRL_0;
typedef union  TGbl_GFX_CTRL_1
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_GFX_CTRL_1;
	};
} TGbl_GFX_CTRL_1;
typedef union  TGbl_GFX_CTRL_2
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_GFX_CTRL_2;
	};
} TGbl_GFX_CTRL_2;
typedef union  TGbl_GFX_CTRL_3
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_GFX_CTRL_3;
	};
} TGbl_GFX_CTRL_3;
typedef union  TGbl_GFX_TIMER_VALUE
{ UNSG32 u32[2];
	struct {
		struct w32Gbl_GFX_TIMER_VALUE;
		struct w32Gbl_GFX_TIMER_VALUE1;
	};
} TGbl_GFX_TIMER_VALUE;
typedef union  TGbl_GFX_STATUS
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_GFX_STATUS;
	};
} TGbl_GFX_STATUS;
typedef union  TGbl_USBOTG_REFCLK_CTRL0
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_USBOTG_REFCLK_CTRL0;
	};
} TGbl_USBOTG_REFCLK_CTRL0;
typedef union  TGbl_USBOTG_REFCLK_CTRL1
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_USBOTG_REFCLK_CTRL1;
	};
} TGbl_USBOTG_REFCLK_CTRL1;
typedef union  TGbl_pinMuxCntlBus
{ UNSG32 u32[4];
	struct {
		struct w32Gbl_pinMuxCntlBus;
		struct w32Gbl_pinMuxCntlBus1;
		struct w32Gbl_pinMuxCntlBus2;
		struct w32Gbl_pinMuxCntlBus3;
	};
} TGbl_pinMuxCntlBus;
typedef union  TGbl_avioPinMuxCntlBus
{ UNSG32 u32[3];
	struct {
		struct w32Gbl_avioPinMuxCntlBus;
		struct w32Gbl_avioPinMuxCntlBus1;
		struct w32Gbl_avioPinMuxCntlBus2;
	};
} TGbl_avioPinMuxCntlBus;
typedef union  TGbl_SDIO_CDnCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SDIO_CDnCntl;
	};
} TGbl_SDIO_CDnCntl;
typedef union  TGbl_SDIO_WPCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SDIO_WPCntl;
	};
} TGbl_SDIO_WPCntl;
typedef union  TGbl_SPI1_SS0nCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPI1_SS0nCntl;
	};
} TGbl_SPI1_SS0nCntl;
typedef union  TGbl_SPI1_SS1nCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPI1_SS1nCntl;
	};
} TGbl_SPI1_SS1nCntl;
typedef union  TGbl_SPI1_SS2nCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPI1_SS2nCntl;
	};
} TGbl_SPI1_SS2nCntl;
typedef union  TGbl_SPI1_SS3nCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPI1_SS3nCntl;
	};
} TGbl_SPI1_SS3nCntl;
typedef union  TGbl_SPI1_SDOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPI1_SDOCntl;
	};
} TGbl_SPI1_SDOCntl;
typedef union  TGbl_SPI1_SCLKCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPI1_SCLKCntl;
	};
} TGbl_SPI1_SCLKCntl;
typedef union  TGbl_SPI1_SDICntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPI1_SDICntl;
	};
} TGbl_SPI1_SDICntl;
typedef union  TGbl_TW0_SCLCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_TW0_SCLCntl;
	};
} TGbl_TW0_SCLCntl;
typedef union  TGbl_TW0_SDACntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_TW0_SDACntl;
	};
} TGbl_TW0_SDACntl;
typedef union  TGbl_STS0_CLKCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS0_CLKCntl;
	};
} TGbl_STS0_CLKCntl;
typedef union  TGbl_STS0_SOPCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS0_SOPCntl;
	};
} TGbl_STS0_SOPCntl;
typedef union  TGbl_STS0_SDCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS0_SDCntl;
	};
} TGbl_STS0_SDCntl;
typedef union  TGbl_STS0_VALDCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS0_VALDCntl;
	};
} TGbl_STS0_VALDCntl;
typedef union  TGbl_STS1_CLKCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS1_CLKCntl;
	};
} TGbl_STS1_CLKCntl;
typedef union  TGbl_STS1_SOPCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS1_SOPCntl;
	};
} TGbl_STS1_SOPCntl;
typedef union  TGbl_STS1_SDCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS1_SDCntl;
	};
} TGbl_STS1_SDCntl;
typedef union  TGbl_STS1_VALDCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_STS1_VALDCntl;
	};
} TGbl_STS1_VALDCntl;
typedef union  TGbl_USB2_DRV_VBUSCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_USB2_DRV_VBUSCntl;
	};
} TGbl_USB2_DRV_VBUSCntl;
typedef union  TGbl_RGMII_MDCCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_MDCCntl;
	};
} TGbl_RGMII_MDCCntl;
typedef union  TGbl_RGMII_MDIOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_MDIOCntl;
	};
} TGbl_RGMII_MDIOCntl;
typedef union  TGbl_RGMII_TXCCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_TXCCntl;
	};
} TGbl_RGMII_TXCCntl;
typedef union  TGbl_RGMII_TXD0Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_TXD0Cntl;
	};
} TGbl_RGMII_TXD0Cntl;
typedef union  TGbl_RGMII_TXD1Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_TXD1Cntl;
	};
} TGbl_RGMII_TXD1Cntl;
typedef union  TGbl_RGMII_TXD2Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_TXD2Cntl;
	};
} TGbl_RGMII_TXD2Cntl;
typedef union  TGbl_RGMII_TXD3Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_TXD3Cntl;
	};
} TGbl_RGMII_TXD3Cntl;
typedef union  TGbl_RGMII_TXCTLCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_TXCTLCntl;
	};
} TGbl_RGMII_TXCTLCntl;
typedef union  TGbl_RGMII_RXCCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_RXCCntl;
	};
} TGbl_RGMII_RXCCntl;
typedef union  TGbl_RGMII_RXD0Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_RXD0Cntl;
	};
} TGbl_RGMII_RXD0Cntl;
typedef union  TGbl_RGMII_RXD1Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_RXD1Cntl;
	};
} TGbl_RGMII_RXD1Cntl;
typedef union  TGbl_RGMII_RXD2Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_RXD2Cntl;
	};
} TGbl_RGMII_RXD2Cntl;
typedef union  TGbl_RGMII_RXD3Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_RXD3Cntl;
	};
} TGbl_RGMII_RXD3Cntl;
typedef union  TGbl_RGMII_RXCTLCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_RGMII_RXCTLCntl;
	};
} TGbl_RGMII_RXCTLCntl;
typedef union  TGbl_I2S1_DO0Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S1_DO0Cntl;
	};
} TGbl_I2S1_DO0Cntl;
typedef union  TGbl_I2S1_DO1Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S1_DO1Cntl;
	};
} TGbl_I2S1_DO1Cntl;
typedef union  TGbl_I2S1_DO2Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S1_DO2Cntl;
	};
} TGbl_I2S1_DO2Cntl;
typedef union  TGbl_I2S1_DO3Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S1_DO3Cntl;
	};
} TGbl_I2S1_DO3Cntl;
typedef union  TGbl_I2S1_LRCKIOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S1_LRCKIOCntl;
	};
} TGbl_I2S1_LRCKIOCntl;
typedef union  TGbl_I2S1_BCLKIOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S1_BCLKIOCntl;
	};
} TGbl_I2S1_BCLKIOCntl;
typedef union  TGbl_SPDIFOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPDIFOCntl;
	};
} TGbl_SPDIFOCntl;
typedef union  TGbl_SPDIFICntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_SPDIFICntl;
	};
} TGbl_SPDIFICntl;
typedef union  TGbl_I2S2_LRCKIOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S2_LRCKIOCntl;
	};
} TGbl_I2S2_LRCKIOCntl;
typedef union  TGbl_I2S2_BCLKIOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S2_BCLKIOCntl;
	};
} TGbl_I2S2_BCLKIOCntl;
typedef union  TGbl_I2S2_DI0Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S2_DI0Cntl;
	};
} TGbl_I2S2_DI0Cntl;
typedef union  TGbl_I2S2_DI1Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S2_DI1Cntl;
	};
} TGbl_I2S2_DI1Cntl;
typedef union  TGbl_I2S2_DI2Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S2_DI2Cntl;
	};
} TGbl_I2S2_DI2Cntl;
typedef union  TGbl_I2S2_DI3Cntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S2_DI3Cntl;
	};
} TGbl_I2S2_DI3Cntl;
typedef union  TGbl_I2S1_MCLKCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S1_MCLKCntl;
	};
} TGbl_I2S1_MCLKCntl;
typedef union  TGbl_I2S2_MCLKCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S2_MCLKCntl;
	};
} TGbl_I2S2_MCLKCntl;
typedef union  TGbl_TX_EDDC_SCLCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_TX_EDDC_SCLCntl;
	};
} TGbl_TX_EDDC_SCLCntl;
typedef union  TGbl_TX_EDDC_SDACntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_TX_EDDC_SDACntl;
	};
} TGbl_TX_EDDC_SDACntl;
typedef union  TGbl_I2S3_DOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S3_DOCntl;
	};
} TGbl_I2S3_DOCntl;
typedef union  TGbl_I2S3_LRCKIOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S3_LRCKIOCntl;
	};
} TGbl_I2S3_LRCKIOCntl;
typedef union  TGbl_I2S3_BCLKIOCntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S3_BCLKIOCntl;
	};
} TGbl_I2S3_BCLKIOCntl;
typedef union  TGbl_I2S3_DICntl
{ UNSG32 u32[1];
	struct {
		struct w32Gbl_I2S3_DICntl;
	};
} TGbl_I2S3_DICntl;
SIGN32 Gbl_drvrd(SIE_Gbl *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 Gbl_drvwr(SIE_Gbl *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void Gbl_reset(SIE_Gbl *p);
SIGN32 Gbl_cmp  (SIE_Gbl *p, SIE_Gbl *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define Gbl_check(p,pie,pfx,hLOG) Gbl_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define Gbl_print(p,    pfx,hLOG) Gbl_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
}
#endif
#pragma  pack()
#endif
