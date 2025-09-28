/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ovpdnSclWrap_h
#define ovpdnSclWrap_h (){}
#include "com_type.h"
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
#ifndef h_TG_PL
#define h_TG_PL (){}
#define     RA_TG_PL_X                                     0x0000
#define     RA_TG_PL_Y                                     0x0004
	typedef struct SIE_TG_PL {
#define     w32TG_PL_X                                     {\
	UNSG32 uX_start                                    : 13;\
	UNSG32 uX_end                                      : 13;\
	UNSG32 RSVDx0_b26                                  :  6;\
}
	union { UNSG32 u32TG_PL_X;
		struct w32TG_PL_X;
	};
#define     w32TG_PL_Y                                     {\
	UNSG32 uY_start                                    : 12;\
	UNSG32 uY_end                                      : 12;\
	UNSG32 RSVDx4_b24                                  :  8;\
}
union { UNSG32 u32TG_PL_Y;
	struct w32TG_PL_Y;
};
} SIE_TG_PL;
typedef union  T32TG_PL_X
{ UNSG32 u32;
	struct w32TG_PL_X;
} T32TG_PL_X;
typedef union  T32TG_PL_Y
{ UNSG32 u32;
	struct w32TG_PL_Y;
} T32TG_PL_Y;
typedef union  TTG_PL_X
{ UNSG32 u32[1];
	struct {
		struct w32TG_PL_X;
	};
} TTG_PL_X;
typedef union  TTG_PL_Y
{ UNSG32 u32[1];
	struct {
		struct w32TG_PL_Y;
	};
} TTG_PL_Y;
SIGN32 TG_PL_drvrd(SIE_TG_PL *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 TG_PL_drvwr(SIE_TG_PL *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void TG_PL_reset(SIE_TG_PL *p);
SIGN32 TG_PL_cmp  (SIE_TG_PL *p, SIE_TG_PL *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define TG_PL_check(p,pie,pfx,hLOG) TG_PL_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define TG_PL_print(p,    pfx,hLOG) TG_PL_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_TG_PRG
#define h_TG_PRG (){}
#define     RA_TG_PRG_CTRL                                 0x0000
#define     RA_TG_PRG_CTRL1                                0x0004
#define     RA_TG_PRG_Total                                0x0008
#define     RA_TG_PRG_Initial                              0x000C
#define     RA_TG_PRG_HSYNC                                0x0010
#define     RA_TG_PRG_VSYNC                                0x0014
#define     RA_TG_PRG_VS                                   0x0018
#define     RA_TG_PRG_FT                                   0x001C
#define     RA_TG_PRG_VX                                   0x0020
typedef struct SIE_TG_PRG {
#define     w32TG_PRG_CTRL                                 {\
	UNSG32 uCTRL_mode                                  :  2;\
	UNSG32 uCTRL_lwin                                  :  8;\
	UNSG32 uCTRL_frst                                  : 12;\
	UNSG32 uCTRL_freeze                                : 10;\
}
union { UNSG32 u32TG_PRG_CTRL;
	struct w32TG_PRG_CTRL;
};
#define     w32TG_PRG_CTRL1                                {\
	UNSG32 uCTRL1_sync_ctrl                            :  2;\
	UNSG32 uCTRL1_res_change_en                        :  9;\
	UNSG32 RSVDx4_b11                                  : 21;\
}
union { UNSG32 u32TG_PRG_CTRL1;
	struct w32TG_PRG_CTRL1;
};
#define     w32TG_PRG_Total                                {\
	UNSG32 uTotal_vertical                             : 12;\
	UNSG32 uTotal_horizontal                           : 13;\
	UNSG32 RSVDx8_b25                                  :  7;\
}
union { UNSG32 u32TG_PRG_Total;
	struct w32TG_PRG_Total;
};
#define     w32TG_PRG_Initial                              {\
	UNSG32 uInitial_xi                                 : 13;\
	UNSG32 uInitial_yi                                 : 12;\
	UNSG32 RSVDxC_b25                                  :  7;\
}
union { UNSG32 u32TG_PRG_Initial;
	struct w32TG_PRG_Initial;
};
#define     w32TG_PRG_HSYNC                                {\
	UNSG32 uHSYNC_h_start                              : 13;\
	UNSG32 uHSYNC_h_end                                : 13;\
	UNSG32 RSVDx10_b26                                 :  6;\
}
union { UNSG32 u32TG_PRG_HSYNC;
	struct w32TG_PRG_HSYNC;
};
#define     w32TG_PRG_VSYNC                                {\
	UNSG32 uVSYNC_v_start                              : 12;\
	UNSG32 uVSYNC_v_end                                : 12;\
	UNSG32 RSVDx14_b24                                 :  8;\
}
union { UNSG32 u32TG_PRG_VSYNC;
	struct w32TG_PRG_VSYNC;
};
#define     w32TG_PRG_VS                                   {\
	UNSG32 uVS_h_start                                 : 13;\
	UNSG32 uVS_h_end                                   : 13;\
	UNSG32 RSVDx18_b26                                 :  6;\
}
union { UNSG32 u32TG_PRG_VS;
	struct w32TG_PRG_VS;
};
#define     w32TG_PRG_FT                                   {\
	UNSG32 uFT_frame                                   :  8;\
	UNSG32 RSVDx1C_b8                                  : 24;\
}
union { UNSG32 u32TG_PRG_FT;
	struct w32TG_PRG_FT;
};
#define     w32TG_PRG_VX                                   {\
	UNSG32 uVX_vx                                      : 13;\
	UNSG32 RSVDx20_b13                                 : 19;\
}
union { UNSG32 u32TG_PRG_VX;
	struct w32TG_PRG_VX;
};
} SIE_TG_PRG;
typedef union  T32TG_PRG_CTRL
{ UNSG32 u32;
	struct w32TG_PRG_CTRL;
} T32TG_PRG_CTRL;
typedef union  T32TG_PRG_CTRL1
{ UNSG32 u32;
	struct w32TG_PRG_CTRL1;
} T32TG_PRG_CTRL1;
typedef union  T32TG_PRG_Total
{ UNSG32 u32;
	struct w32TG_PRG_Total;
} T32TG_PRG_Total;
typedef union  T32TG_PRG_Initial
{ UNSG32 u32;
	struct w32TG_PRG_Initial;
} T32TG_PRG_Initial;
typedef union  T32TG_PRG_HSYNC
{ UNSG32 u32;
	struct w32TG_PRG_HSYNC;
} T32TG_PRG_HSYNC;
typedef union  T32TG_PRG_VSYNC
{ UNSG32 u32;
	struct w32TG_PRG_VSYNC;
} T32TG_PRG_VSYNC;
typedef union  T32TG_PRG_VS
{ UNSG32 u32;
	struct w32TG_PRG_VS;
} T32TG_PRG_VS;
typedef union  T32TG_PRG_FT
{ UNSG32 u32;
	struct w32TG_PRG_FT;
} T32TG_PRG_FT;
typedef union  T32TG_PRG_VX
{ UNSG32 u32;
	struct w32TG_PRG_VX;
} T32TG_PRG_VX;
typedef union  TTG_PRG_CTRL
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_CTRL;
	};
} TTG_PRG_CTRL;
typedef union  TTG_PRG_CTRL1
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_CTRL1;
	};
} TTG_PRG_CTRL1;
typedef union  TTG_PRG_Total
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_Total;
	};
} TTG_PRG_Total;
typedef union  TTG_PRG_Initial
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_Initial;
	};
} TTG_PRG_Initial;
typedef union  TTG_PRG_HSYNC
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_HSYNC;
	};
} TTG_PRG_HSYNC;
typedef union  TTG_PRG_VSYNC
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_VSYNC;
	};
} TTG_PRG_VSYNC;
typedef union  TTG_PRG_VS
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_VS;
	};
} TTG_PRG_VS;
typedef union  TTG_PRG_FT
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_FT;
	};
} TTG_PRG_FT;
typedef union  TTG_PRG_VX
{ UNSG32 u32[1];
	struct {
		struct w32TG_PRG_VX;
	};
} TTG_PRG_VX;
SIGN32 TG_PRG_drvrd(SIE_TG_PRG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 TG_PRG_drvwr(SIE_TG_PRG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void TG_PRG_reset(SIE_TG_PRG *p);
SIGN32 TG_PRG_cmp  (SIE_TG_PRG *p, SIE_TG_PRG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define TG_PRG_check(p,pie,pfx,hLOG) TG_PRG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define TG_PRG_print(p,    pfx,hLOG) TG_PRG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_TG
#define h_TG (){}
#define     RA_TG_INIT                                     0x0000
#define     RA_TG_SIZE                                     0x0004
#define     RA_TG_HS                                       0x0008
#define     RA_TG_HB                                       0x000C
#define     RA_TG_HB_CR                                    0x0010
#define     RA_TG_HB_CR2                                   0x0014
#define     RA_TG_VS0                                      0x0018
#define     RA_TG_VS1                                      0x001C
#define     RA_TG_VB0                                      0x0020
#define     RA_TG_VB0_CR                                   0x0024
#define     RA_TG_VB0_CR2                                  0x0028
#define     RA_TG_VB1                                      0x002C
#define     RA_TG_SCAN                                     0x0030
#define        TG_SCAN_MODE_PROG                                        0x0
#define        TG_SCAN_MODE_INTER                                       0x1
#define     RA_TG_INTPOS                                   0x0034
#define     RA_TG_MODE                                     0x0038
#define        TG_MODE_EN_MASTER                                        0x0
#define        TG_MODE_EN_SLAVE                                         0x1
#define     RA_TG_HVREF                                    0x003C
#define        TG_HVREF_SEL_SYNC                                        0x0
#define        TG_HVREF_SEL_BLANK                                       0x1
#define        TG_HVREF_POL_NEG_PULSE                                   0x0
#define        TG_HVREF_POL_POS_PULSE                                   0x1
typedef struct SIE_TG {
#define     w32TG_INIT                                     {\
	UNSG32 uINIT_Y                                     : 16;\
	UNSG32 uINIT_X                                     : 16;\
}
union { UNSG32 u32TG_INIT;
	struct w32TG_INIT;
};
#define     w32TG_SIZE                                     {\
	UNSG32 uSIZE_Y                                     : 16;\
	UNSG32 uSIZE_X                                     : 16;\
}
union { UNSG32 u32TG_SIZE;
	struct w32TG_SIZE;
};
#define     w32TG_HS                                       {\
	UNSG32 uHS_FE                                      : 16;\
	UNSG32 uHS_BE                                      : 16;\
}
union { UNSG32 u32TG_HS;
	struct w32TG_HS;
};
#define     w32TG_HB                                       {\
	UNSG32 uHB_FE                                      : 16;\
	UNSG32 uHB_BE                                      : 16;\
}
union { UNSG32 u32TG_HB;
	struct w32TG_HB;
};
#define     w32TG_HB_CR                                    {\
	UNSG32 uHB_CR_FE                                   : 16;\
	UNSG32 uHB_CR_BE                                   : 16;\
}
union { UNSG32 u32TG_HB_CR;
	struct w32TG_HB_CR;
};
#define     w32TG_HB_CR2                                   {\
	UNSG32 uHB_CR2_FE                                  : 16;\
	UNSG32 uHB_CR2_BE                                  : 16;\
}
union { UNSG32 u32TG_HB_CR2;
	struct w32TG_HB_CR2;
};
#define     w32TG_VS0                                      {\
	UNSG32 uVS0_FE                                     : 16;\
	UNSG32 uVS0_BE                                     : 16;\
}
union { UNSG32 u32TG_VS0;
	struct w32TG_VS0;
};
#define     w32TG_VS1                                      {\
	UNSG32 uVS1_FE                                     : 16;\
	UNSG32 uVS1_BE                                     : 16;\
}
union { UNSG32 u32TG_VS1;
	struct w32TG_VS1;
};
#define     w32TG_VB0                                      {\
	UNSG32 uVB0_FE                                     : 16;\
	UNSG32 uVB0_BE                                     : 16;\
}
union { UNSG32 u32TG_VB0;
	struct w32TG_VB0;
};
#define     w32TG_VB0_CR                                   {\
	UNSG32 uVB0_CR_FE                                  : 16;\
	UNSG32 uVB0_CR_BE                                  : 16;\
}
union { UNSG32 u32TG_VB0_CR;
	struct w32TG_VB0_CR;
};
#define     w32TG_VB0_CR2                                  {\
	UNSG32 uVB0_CR2_FE                                 : 16;\
	UNSG32 uVB0_CR2_BE                                 : 16;\
}
union { UNSG32 u32TG_VB0_CR2;
	struct w32TG_VB0_CR2;
};
#define     w32TG_VB1                                      {\
	UNSG32 uVB1_FE                                     : 16;\
	UNSG32 uVB1_BE                                     : 16;\
}
union { UNSG32 u32TG_VB1;
	struct w32TG_VB1;
};
#define     w32TG_SCAN                                     {\
	UNSG32 uSCAN_MODE                                  :  1;\
	UNSG32 RSVDx30_b1                                  : 31;\
}
union { UNSG32 u32TG_SCAN;
	struct w32TG_SCAN;
};
#define     w32TG_INTPOS                                   {\
	UNSG32 uINTPOS_FRAME                               : 16;\
	UNSG32 uINTPOS_FIELD                               : 16;\
}
union { UNSG32 u32TG_INTPOS;
	struct w32TG_INTPOS;
};
#define     w32TG_MODE                                     {\
	UNSG32 uMODE_EN                                    :  1;\
	UNSG32 RSVDx38_b1                                  : 31;\
}
union { UNSG32 u32TG_MODE;
	struct w32TG_MODE;
};
#define     w32TG_HVREF                                    {\
	UNSG32 uHVREF_SEL                                  :  1;\
	UNSG32 uHVREF_POL                                  :  1;\
	UNSG32 RSVDx3C_b2                                  : 30;\
}
union { UNSG32 u32TG_HVREF;
	struct w32TG_HVREF;
};
} SIE_TG;
typedef union  T32TG_INIT
{ UNSG32 u32;
	struct w32TG_INIT;
} T32TG_INIT;
typedef union  T32TG_SIZE
{ UNSG32 u32;
	struct w32TG_SIZE;
} T32TG_SIZE;
typedef union  T32TG_HS
{ UNSG32 u32;
	struct w32TG_HS;
} T32TG_HS;
typedef union  T32TG_HB
{ UNSG32 u32;
	struct w32TG_HB;
} T32TG_HB;
typedef union  T32TG_HB_CR
{ UNSG32 u32;
	struct w32TG_HB_CR;
} T32TG_HB_CR;
typedef union  T32TG_HB_CR2
{ UNSG32 u32;
	struct w32TG_HB_CR2;
} T32TG_HB_CR2;
typedef union  T32TG_VS0
{ UNSG32 u32;
	struct w32TG_VS0;
} T32TG_VS0;
typedef union  T32TG_VS1
{ UNSG32 u32;
	struct w32TG_VS1;
} T32TG_VS1;
typedef union  T32TG_VB0
{ UNSG32 u32;
	struct w32TG_VB0;
} T32TG_VB0;
typedef union  T32TG_VB0_CR
{ UNSG32 u32;
	struct w32TG_VB0_CR;
} T32TG_VB0_CR;
typedef union  T32TG_VB0_CR2
{ UNSG32 u32;
	struct w32TG_VB0_CR2;
} T32TG_VB0_CR2;
typedef union  T32TG_VB1
{ UNSG32 u32;
	struct w32TG_VB1;
} T32TG_VB1;
typedef union  T32TG_SCAN
{ UNSG32 u32;
	struct w32TG_SCAN;
} T32TG_SCAN;
typedef union  T32TG_INTPOS
{ UNSG32 u32;
	struct w32TG_INTPOS;
} T32TG_INTPOS;
typedef union  T32TG_MODE
{ UNSG32 u32;
	struct w32TG_MODE;
} T32TG_MODE;
typedef union  T32TG_HVREF
{ UNSG32 u32;
	struct w32TG_HVREF;
} T32TG_HVREF;
typedef union  TTG_INIT
{ UNSG32 u32[1];
	struct {
		struct w32TG_INIT;
	};
} TTG_INIT;
typedef union  TTG_SIZE
{ UNSG32 u32[1];
	struct {
		struct w32TG_SIZE;
	};
} TTG_SIZE;
typedef union  TTG_HS
{ UNSG32 u32[1];
	struct {
		struct w32TG_HS;
	};
} TTG_HS;
typedef union  TTG_HB
{ UNSG32 u32[1];
	struct {
		struct w32TG_HB;
	};
} TTG_HB;
typedef union  TTG_HB_CR
{ UNSG32 u32[1];
	struct {
		struct w32TG_HB_CR;
	};
} TTG_HB_CR;
typedef union  TTG_HB_CR2
{ UNSG32 u32[1];
	struct {
		struct w32TG_HB_CR2;
	};
} TTG_HB_CR2;
typedef union  TTG_VS0
{ UNSG32 u32[1];
	struct {
		struct w32TG_VS0;
	};
} TTG_VS0;
typedef union  TTG_VS1
{ UNSG32 u32[1];
	struct {
		struct w32TG_VS1;
	};
} TTG_VS1;
typedef union  TTG_VB0
{ UNSG32 u32[1];
	struct {
		struct w32TG_VB0;
	};
} TTG_VB0;
typedef union  TTG_VB0_CR
{ UNSG32 u32[1];
	struct {
		struct w32TG_VB0_CR;
	};
} TTG_VB0_CR;
typedef union  TTG_VB0_CR2
{ UNSG32 u32[1];
	struct {
		struct w32TG_VB0_CR2;
	};
} TTG_VB0_CR2;
typedef union  TTG_VB1
{ UNSG32 u32[1];
	struct {
		struct w32TG_VB1;
	};
} TTG_VB1;
typedef union  TTG_SCAN
{ UNSG32 u32[1];
	struct {
		struct w32TG_SCAN;
	};
} TTG_SCAN;
typedef union  TTG_INTPOS
{ UNSG32 u32[1];
	struct {
		struct w32TG_INTPOS;
	};
} TTG_INTPOS;
typedef union  TTG_MODE
{ UNSG32 u32[1];
	struct {
		struct w32TG_MODE;
	};
} TTG_MODE;
typedef union  TTG_HVREF
{ UNSG32 u32[1];
	struct {
		struct w32TG_HVREF;
	};
} TTG_HVREF;
SIGN32 TG_drvrd(SIE_TG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 TG_drvwr(SIE_TG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void TG_reset(SIE_TG *p);
SIGN32 TG_cmp  (SIE_TG *p, SIE_TG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define TG_check(p,pie,pfx,hLOG) TG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define TG_print(p,    pfx,hLOG) TG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_TG_MAIN
#define h_TG_MAIN (){}
#define     RA_TG_MAIN_INIT                                0x0000
#define     RA_TG_MAIN_SIZE                                0x0004
#define     RA_TG_MAIN_HS                                  0x0008
#define     RA_TG_MAIN_HB                                  0x000C
#define     RA_TG_MAIN_HB_Y                                0x0010
#define     RA_TG_MAIN_HB_C                                0x0014
#define     RA_TG_MAIN_HB_OUT                              0x0018
#define     RA_TG_MAIN_VS0                                 0x001C
#define     RA_TG_MAIN_VS1                                 0x0020
#define     RA_TG_MAIN_VB0                                 0x0024
#define     RA_TG_MAIN_VB0_Y                               0x0028
#define     RA_TG_MAIN_VB0_C                               0x002C
#define     RA_TG_MAIN_VB0_OUT                             0x0030
#define     RA_TG_MAIN_VB1                                 0x0034
#define     RA_TG_MAIN_SCAN                                0x0038
#define        TG_MAIN_SCAN_MODE_PROG                                   0x0
#define        TG_MAIN_SCAN_MODE_INTER                                  0x1
#define     RA_TG_MAIN_INTPOS                              0x003C
#define     RA_TG_MAIN_MODE                                0x0040
#define        TG_MAIN_MODE_EN_MASTER                                   0x0
#define        TG_MAIN_MODE_EN_SLAVE                                    0x1
#define     RA_TG_MAIN_HVREF                               0x0044
#define        TG_MAIN_HVREF_SEL_SYNC                                   0x0
#define        TG_MAIN_HVREF_SEL_BLANK                                  0x1
#define        TG_MAIN_HVREF_POL_NEG_PULSE                              0x0
#define        TG_MAIN_HVREF_POL_POS_PULSE                              0x1
typedef struct SIE_TG_MAIN {
#define     w32TG_MAIN_INIT                                {\
	UNSG32 uINIT_Y                                     : 16;\
	UNSG32 uINIT_X                                     : 16;\
}
union { UNSG32 u32TG_MAIN_INIT;
	struct w32TG_MAIN_INIT;
};
#define     w32TG_MAIN_SIZE                                {\
	UNSG32 uSIZE_Y                                     : 16;\
	UNSG32 uSIZE_X                                     : 16;\
}
union { UNSG32 u32TG_MAIN_SIZE;
	struct w32TG_MAIN_SIZE;
};
#define     w32TG_MAIN_HS                                  {\
	UNSG32 uHS_FE                                      : 16;\
	UNSG32 uHS_BE                                      : 16;\
}
union { UNSG32 u32TG_MAIN_HS;
	struct w32TG_MAIN_HS;
};
#define     w32TG_MAIN_HB                                  {\
	UNSG32 uHB_FE                                      : 16;\
	UNSG32 uHB_BE                                      : 16;\
}
union { UNSG32 u32TG_MAIN_HB;
	struct w32TG_MAIN_HB;
};
#define     w32TG_MAIN_HB_Y                                {\
	UNSG32 uHB_Y_FE                                    : 16;\
	UNSG32 uHB_Y_BE                                    : 16;\
}
union { UNSG32 u32TG_MAIN_HB_Y;
	struct w32TG_MAIN_HB_Y;
};
#define     w32TG_MAIN_HB_C                                {\
	UNSG32 uHB_C_FE                                    : 16;\
	UNSG32 uHB_C_BE                                    : 16;\
}
union { UNSG32 u32TG_MAIN_HB_C;
	struct w32TG_MAIN_HB_C;
};
#define     w32TG_MAIN_HB_OUT                              {\
	UNSG32 uHB_OUT_FE                                  : 16;\
	UNSG32 uHB_OUT_BE                                  : 16;\
}
union { UNSG32 u32TG_MAIN_HB_OUT;
	struct w32TG_MAIN_HB_OUT;
};
#define     w32TG_MAIN_VS0                                 {\
	UNSG32 uVS0_FE                                     : 16;\
	UNSG32 uVS0_BE                                     : 16;\
}
union { UNSG32 u32TG_MAIN_VS0;
	struct w32TG_MAIN_VS0;
};
#define     w32TG_MAIN_VS1                                 {\
	UNSG32 uVS1_FE                                     : 16;\
	UNSG32 uVS1_BE                                     : 16;\
}
union { UNSG32 u32TG_MAIN_VS1;
	struct w32TG_MAIN_VS1;
};
#define     w32TG_MAIN_VB0                                 {\
	UNSG32 uVB0_FE                                     : 16;\
	UNSG32 uVB0_BE                                     : 16;\
}
union { UNSG32 u32TG_MAIN_VB0;
	struct w32TG_MAIN_VB0;
};
#define     w32TG_MAIN_VB0_Y                               {\
	UNSG32 uVB0_Y_FE                                   : 16;\
	UNSG32 uVB0_Y_BE                                   : 16;\
}
union { UNSG32 u32TG_MAIN_VB0_Y;
	struct w32TG_MAIN_VB0_Y;
};
#define     w32TG_MAIN_VB0_C                               {\
	UNSG32 uVB0_C_FE                                   : 16;\
	UNSG32 uVB0_C_BE                                   : 16;\
}
union { UNSG32 u32TG_MAIN_VB0_C;
	struct w32TG_MAIN_VB0_C;
};
#define     w32TG_MAIN_VB0_OUT                             {\
	UNSG32 uVB0_OUT_FE                                 : 16;\
	UNSG32 uVB0_OUT_BE                                 : 16;\
}
union { UNSG32 u32TG_MAIN_VB0_OUT;
	struct w32TG_MAIN_VB0_OUT;
};
#define     w32TG_MAIN_VB1                                 {\
	UNSG32 uVB1_FE                                     : 16;\
	UNSG32 uVB1_BE                                     : 16;\
}
union { UNSG32 u32TG_MAIN_VB1;
	struct w32TG_MAIN_VB1;
};
#define     w32TG_MAIN_SCAN                                {\
	UNSG32 uSCAN_MODE                                  :  1;\
	UNSG32 RSVDx38_b1                                  : 31;\
}
union { UNSG32 u32TG_MAIN_SCAN;
	struct w32TG_MAIN_SCAN;
};
#define     w32TG_MAIN_INTPOS                              {\
	UNSG32 uINTPOS_FRAME                               : 16;\
	UNSG32 uINTPOS_FIELD                               : 16;\
}
union { UNSG32 u32TG_MAIN_INTPOS;
	struct w32TG_MAIN_INTPOS;
};
#define     w32TG_MAIN_MODE                                {\
	UNSG32 uMODE_EN                                    :  1;\
	UNSG32 RSVDx40_b1                                  : 31;\
}
union { UNSG32 u32TG_MAIN_MODE;
	struct w32TG_MAIN_MODE;
};
#define     w32TG_MAIN_HVREF                               {\
	UNSG32 uHVREF_SEL                                  :  1;\
	UNSG32 uHVREF_POL                                  :  1;\
	UNSG32 RSVDx44_b2                                  : 30;\
}
union { UNSG32 u32TG_MAIN_HVREF;
	struct w32TG_MAIN_HVREF;
};
} SIE_TG_MAIN;
typedef union  T32TG_MAIN_INIT
{ UNSG32 u32;
	struct w32TG_MAIN_INIT;
} T32TG_MAIN_INIT;
typedef union  T32TG_MAIN_SIZE
{ UNSG32 u32;
	struct w32TG_MAIN_SIZE;
} T32TG_MAIN_SIZE;
typedef union  T32TG_MAIN_HS
{ UNSG32 u32;
	struct w32TG_MAIN_HS;
} T32TG_MAIN_HS;
typedef union  T32TG_MAIN_HB
{ UNSG32 u32;
	struct w32TG_MAIN_HB;
} T32TG_MAIN_HB;
typedef union  T32TG_MAIN_HB_Y
{ UNSG32 u32;
	struct w32TG_MAIN_HB_Y;
} T32TG_MAIN_HB_Y;
typedef union  T32TG_MAIN_HB_C
{ UNSG32 u32;
	struct w32TG_MAIN_HB_C;
} T32TG_MAIN_HB_C;
typedef union  T32TG_MAIN_HB_OUT
{ UNSG32 u32;
	struct w32TG_MAIN_HB_OUT;
} T32TG_MAIN_HB_OUT;
typedef union  T32TG_MAIN_VS0
{ UNSG32 u32;
	struct w32TG_MAIN_VS0;
} T32TG_MAIN_VS0;
typedef union  T32TG_MAIN_VS1
{ UNSG32 u32;
	struct w32TG_MAIN_VS1;
} T32TG_MAIN_VS1;
typedef union  T32TG_MAIN_VB0
{ UNSG32 u32;
	struct w32TG_MAIN_VB0;
} T32TG_MAIN_VB0;
typedef union  T32TG_MAIN_VB0_Y
{ UNSG32 u32;
	struct w32TG_MAIN_VB0_Y;
} T32TG_MAIN_VB0_Y;
typedef union  T32TG_MAIN_VB0_C
{ UNSG32 u32;
	struct w32TG_MAIN_VB0_C;
} T32TG_MAIN_VB0_C;
typedef union  T32TG_MAIN_VB0_OUT
{ UNSG32 u32;
	struct w32TG_MAIN_VB0_OUT;
} T32TG_MAIN_VB0_OUT;
typedef union  T32TG_MAIN_VB1
{ UNSG32 u32;
	struct w32TG_MAIN_VB1;
} T32TG_MAIN_VB1;
typedef union  T32TG_MAIN_SCAN
{ UNSG32 u32;
	struct w32TG_MAIN_SCAN;
} T32TG_MAIN_SCAN;
typedef union  T32TG_MAIN_INTPOS
{ UNSG32 u32;
	struct w32TG_MAIN_INTPOS;
} T32TG_MAIN_INTPOS;
typedef union  T32TG_MAIN_MODE
{ UNSG32 u32;
	struct w32TG_MAIN_MODE;
} T32TG_MAIN_MODE;
typedef union  T32TG_MAIN_HVREF
{ UNSG32 u32;
	struct w32TG_MAIN_HVREF;
} T32TG_MAIN_HVREF;
typedef union  TTG_MAIN_INIT
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_INIT;
	};
} TTG_MAIN_INIT;
typedef union  TTG_MAIN_SIZE
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_SIZE;
	};
} TTG_MAIN_SIZE;
typedef union  TTG_MAIN_HS
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_HS;
	};
} TTG_MAIN_HS;
typedef union  TTG_MAIN_HB
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_HB;
	};
} TTG_MAIN_HB;
typedef union  TTG_MAIN_HB_Y
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_HB_Y;
	};
} TTG_MAIN_HB_Y;
typedef union  TTG_MAIN_HB_C
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_HB_C;
	};
} TTG_MAIN_HB_C;
typedef union  TTG_MAIN_HB_OUT
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_HB_OUT;
	};
} TTG_MAIN_HB_OUT;
typedef union  TTG_MAIN_VS0
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_VS0;
	};
} TTG_MAIN_VS0;
typedef union  TTG_MAIN_VS1
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_VS1;
	};
} TTG_MAIN_VS1;
typedef union  TTG_MAIN_VB0
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_VB0;
	};
} TTG_MAIN_VB0;
typedef union  TTG_MAIN_VB0_Y
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_VB0_Y;
	};
} TTG_MAIN_VB0_Y;
typedef union  TTG_MAIN_VB0_C
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_VB0_C;
	};
} TTG_MAIN_VB0_C;
typedef union  TTG_MAIN_VB0_OUT
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_VB0_OUT;
	};
} TTG_MAIN_VB0_OUT;
typedef union  TTG_MAIN_VB1
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_VB1;
	};
} TTG_MAIN_VB1;
typedef union  TTG_MAIN_SCAN
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_SCAN;
	};
} TTG_MAIN_SCAN;
typedef union  TTG_MAIN_INTPOS
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_INTPOS;
	};
} TTG_MAIN_INTPOS;
typedef union  TTG_MAIN_MODE
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_MODE;
	};
} TTG_MAIN_MODE;
typedef union  TTG_MAIN_HVREF
{ UNSG32 u32[1];
	struct {
		struct w32TG_MAIN_HVREF;
	};
} TTG_MAIN_HVREF;
SIGN32 TG_MAIN_drvrd(SIE_TG_MAIN *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 TG_MAIN_drvwr(SIE_TG_MAIN *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void TG_MAIN_reset(SIE_TG_MAIN *p);
SIGN32 TG_MAIN_cmp  (SIE_TG_MAIN *p, SIE_TG_MAIN *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define TG_MAIN_check(p,pie,pfx,hLOG) TG_MAIN_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define TG_MAIN_print(p,    pfx,hLOG) TG_MAIN_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_BITMAP40
#define h_BITMAP40 (){}
#define     RA_BITMAP40_SEL                                0x0000
#define     RA_BITMAP40_SEL1                               0x0004
#define     RA_BITMAP40_SEL2                               0x0008
#define     RA_BITMAP40_SEL3                               0x000C
#define     RA_BITMAP40_SEL4                               0x0010
#define     RA_BITMAP40_SEL5                               0x0014
#define     RA_BITMAP40_SEL6                               0x0018
#define     RA_BITMAP40_SEL7                               0x001C
typedef struct SIE_BITMAP40 {
#define     w32BITMAP40_SEL                                {\
	UNSG32 uSEL_BIT_POS0                               :  6;\
	UNSG32 uSEL_BIT_POS1                               :  6;\
	UNSG32 uSEL_BIT_POS2                               :  6;\
	UNSG32 uSEL_BIT_POS3                               :  6;\
	UNSG32 uSEL_BIT_POS4                               :  6;\
	UNSG32 RSVDx0_b30                                  :  2;\
}
union { UNSG32 u32BITMAP40_SEL;
	struct w32BITMAP40_SEL;
};
#define     w32BITMAP40_SEL1                               {\
	UNSG32 uSEL_BIT_POS5                               :  6;\
	UNSG32 uSEL_BIT_POS6                               :  6;\
	UNSG32 uSEL_BIT_POS7                               :  6;\
	UNSG32 uSEL_BIT_POS8                               :  6;\
	UNSG32 uSEL_BIT_POS9                               :  6;\
	UNSG32 RSVDx4_b30                                  :  2;\
}
union { UNSG32 u32BITMAP40_SEL1;
	struct w32BITMAP40_SEL1;
};
#define     w32BITMAP40_SEL2                               {\
	UNSG32 uSEL_BIT_POS10                              :  6;\
	UNSG32 uSEL_BIT_POS11                              :  6;\
	UNSG32 uSEL_BIT_POS12                              :  6;\
	UNSG32 uSEL_BIT_POS13                              :  6;\
	UNSG32 uSEL_BIT_POS14                              :  6;\
	UNSG32 RSVDx8_b30                                  :  2;\
}
union { UNSG32 u32BITMAP40_SEL2;
	struct w32BITMAP40_SEL2;
};
#define     w32BITMAP40_SEL3                               {\
	UNSG32 uSEL_BIT_POS15                              :  6;\
	UNSG32 uSEL_BIT_POS16                              :  6;\
	UNSG32 uSEL_BIT_POS17                              :  6;\
	UNSG32 uSEL_BIT_POS18                              :  6;\
	UNSG32 uSEL_BIT_POS19                              :  6;\
	UNSG32 RSVDxC_b30                                  :  2;\
}
union { UNSG32 u32BITMAP40_SEL3;
	struct w32BITMAP40_SEL3;
};
#define     w32BITMAP40_SEL4                               {\
	UNSG32 uSEL_BIT_POS20                              :  6;\
	UNSG32 uSEL_BIT_POS21                              :  6;\
	UNSG32 uSEL_BIT_POS22                              :  6;\
	UNSG32 uSEL_BIT_POS23                              :  6;\
	UNSG32 uSEL_BIT_POS24                              :  6;\
	UNSG32 RSVDx10_b30                                 :  2;\
}
union { UNSG32 u32BITMAP40_SEL4;
	struct w32BITMAP40_SEL4;
};
#define     w32BITMAP40_SEL5                               {\
	UNSG32 uSEL_BIT_POS25                              :  6;\
	UNSG32 uSEL_BIT_POS26                              :  6;\
	UNSG32 uSEL_BIT_POS27                              :  6;\
	UNSG32 uSEL_BIT_POS28                              :  6;\
	UNSG32 uSEL_BIT_POS29                              :  6;\
	UNSG32 RSVDx14_b30                                 :  2;\
}
union { UNSG32 u32BITMAP40_SEL5;
	struct w32BITMAP40_SEL5;
};
#define     w32BITMAP40_SEL6                               {\
	UNSG32 uSEL_BIT_POS30                              :  6;\
	UNSG32 uSEL_BIT_POS31                              :  6;\
	UNSG32 uSEL_BIT_POS32                              :  6;\
	UNSG32 uSEL_BIT_POS33                              :  6;\
	UNSG32 uSEL_BIT_POS34                              :  6;\
	UNSG32 RSVDx18_b30                                 :  2;\
}
union { UNSG32 u32BITMAP40_SEL6;
	struct w32BITMAP40_SEL6;
};
#define     w32BITMAP40_SEL7                               {\
	UNSG32 uSEL_BIT_POS35                              :  6;\
	UNSG32 uSEL_BIT_POS36                              :  6;\
	UNSG32 uSEL_BIT_POS37                              :  6;\
	UNSG32 uSEL_BIT_POS38                              :  6;\
	UNSG32 uSEL_BIT_POS39                              :  6;\
	UNSG32 RSVDx1C_b30                                 :  2;\
}
union { UNSG32 u32BITMAP40_SEL7;
	struct w32BITMAP40_SEL7;
};
} SIE_BITMAP40;
typedef union  T32BITMAP40_SEL
{ UNSG32 u32;
	struct w32BITMAP40_SEL;
} T32BITMAP40_SEL;
typedef union  T32BITMAP40_SEL1
{ UNSG32 u32;
	struct w32BITMAP40_SEL1;
} T32BITMAP40_SEL1;
typedef union  T32BITMAP40_SEL2
{ UNSG32 u32;
	struct w32BITMAP40_SEL2;
} T32BITMAP40_SEL2;
typedef union  T32BITMAP40_SEL3
{ UNSG32 u32;
	struct w32BITMAP40_SEL3;
} T32BITMAP40_SEL3;
typedef union  T32BITMAP40_SEL4
{ UNSG32 u32;
	struct w32BITMAP40_SEL4;
} T32BITMAP40_SEL4;
typedef union  T32BITMAP40_SEL5
{ UNSG32 u32;
	struct w32BITMAP40_SEL5;
} T32BITMAP40_SEL5;
typedef union  T32BITMAP40_SEL6
{ UNSG32 u32;
	struct w32BITMAP40_SEL6;
} T32BITMAP40_SEL6;
typedef union  T32BITMAP40_SEL7
{ UNSG32 u32;
	struct w32BITMAP40_SEL7;
} T32BITMAP40_SEL7;
typedef union  TBITMAP40_SEL
{ UNSG32 u32[8];
	struct {
		struct w32BITMAP40_SEL;
		struct w32BITMAP40_SEL1;
		struct w32BITMAP40_SEL2;
		struct w32BITMAP40_SEL3;
		struct w32BITMAP40_SEL4;
		struct w32BITMAP40_SEL5;
		struct w32BITMAP40_SEL6;
		struct w32BITMAP40_SEL7;
	};
} TBITMAP40_SEL;
SIGN32 BITMAP40_drvrd(SIE_BITMAP40 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 BITMAP40_drvwr(SIE_BITMAP40 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void BITMAP40_reset(SIE_BITMAP40 *p);
SIGN32 BITMAP40_cmp  (SIE_BITMAP40 *p, SIE_BITMAP40 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define BITMAP40_check(p,pie,pfx,hLOG) BITMAP40_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define BITMAP40_print(p,    pfx,hLOG) BITMAP40_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_BITMAP20
#define h_BITMAP20 (){}
#define     RA_BITMAP20_SEL                                0x0000
#define     RA_BITMAP20_SEL1                               0x0004
#define     RA_BITMAP20_SEL2                               0x0008
#define     RA_BITMAP20_SEL3                               0x000C
typedef struct SIE_BITMAP20 {
#define     w32BITMAP20_SEL                                {\
	UNSG32 uSEL_BIT_POS0                               :  5;\
	UNSG32 uSEL_BIT_POS1                               :  5;\
	UNSG32 uSEL_BIT_POS2                               :  5;\
	UNSG32 uSEL_BIT_POS3                               :  5;\
	UNSG32 uSEL_BIT_POS4                               :  5;\
	UNSG32 uSEL_BIT_POS5                               :  5;\
	UNSG32 RSVDx0_b30                                  :  2;\
}
union { UNSG32 u32BITMAP20_SEL;
	struct w32BITMAP20_SEL;
};
#define     w32BITMAP20_SEL1                               {\
	UNSG32 uSEL_BIT_POS6                               :  5;\
	UNSG32 uSEL_BIT_POS7                               :  5;\
	UNSG32 uSEL_BIT_POS8                               :  5;\
	UNSG32 uSEL_BIT_POS9                               :  5;\
	UNSG32 uSEL_BIT_POS10                              :  5;\
	UNSG32 uSEL_BIT_POS11                              :  5;\
	UNSG32 RSVDx4_b30                                  :  2;\
}
union { UNSG32 u32BITMAP20_SEL1;
	struct w32BITMAP20_SEL1;
};
#define     w32BITMAP20_SEL2                               {\
	UNSG32 uSEL_BIT_POS12                              :  5;\
	UNSG32 uSEL_BIT_POS13                              :  5;\
	UNSG32 uSEL_BIT_POS14                              :  5;\
	UNSG32 uSEL_BIT_POS15                              :  5;\
	UNSG32 uSEL_BIT_POS16                              :  5;\
	UNSG32 uSEL_BIT_POS17                              :  5;\
	UNSG32 RSVDx8_b30                                  :  2;\
}
union { UNSG32 u32BITMAP20_SEL2;
	struct w32BITMAP20_SEL2;
};
#define     w32BITMAP20_SEL3                               {\
	UNSG32 uSEL_BIT_POS18                              :  5;\
	UNSG32 uSEL_BIT_POS19                              :  5;\
	UNSG32 RSVDxC_b10                                  : 22;\
}
union { UNSG32 u32BITMAP20_SEL3;
	struct w32BITMAP20_SEL3;
};
} SIE_BITMAP20;
typedef union  T32BITMAP20_SEL
{ UNSG32 u32;
	struct w32BITMAP20_SEL;
} T32BITMAP20_SEL;
typedef union  T32BITMAP20_SEL1
{ UNSG32 u32;
	struct w32BITMAP20_SEL1;
} T32BITMAP20_SEL1;
typedef union  T32BITMAP20_SEL2
{ UNSG32 u32;
	struct w32BITMAP20_SEL2;
} T32BITMAP20_SEL2;
typedef union  T32BITMAP20_SEL3
{ UNSG32 u32;
	struct w32BITMAP20_SEL3;
} T32BITMAP20_SEL3;
typedef union  TBITMAP20_SEL
{ UNSG32 u32[4];
	struct {
		struct w32BITMAP20_SEL;
		struct w32BITMAP20_SEL1;
		struct w32BITMAP20_SEL2;
		struct w32BITMAP20_SEL3;
	};
} TBITMAP20_SEL;
SIGN32 BITMAP20_drvrd(SIE_BITMAP20 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 BITMAP20_drvwr(SIE_BITMAP20 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void BITMAP20_reset(SIE_BITMAP20 *p);
SIGN32 BITMAP20_cmp  (SIE_BITMAP20 *p, SIE_BITMAP20 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define BITMAP20_check(p,pie,pfx,hLOG) BITMAP20_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define BITMAP20_print(p,    pfx,hLOG) BITMAP20_cmp(p,0,  pfx,(void*)(hLOG),0,0)
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
#ifndef h_BITMAP16
#define h_BITMAP16 (){}
#define     RA_BITMAP16_SEL                                0x0000
#define     RA_BITMAP16_SEL1                               0x0004
typedef struct SIE_BITMAP16 {
#define     w32BITMAP16_SEL                                {\
	UNSG32 uSEL_BIT_POS0                               :  4;\
	UNSG32 uSEL_BIT_POS1                               :  4;\
	UNSG32 uSEL_BIT_POS2                               :  4;\
	UNSG32 uSEL_BIT_POS3                               :  4;\
	UNSG32 uSEL_BIT_POS4                               :  4;\
	UNSG32 uSEL_BIT_POS5                               :  4;\
	UNSG32 uSEL_BIT_POS6                               :  4;\
	UNSG32 uSEL_BIT_POS7                               :  4;\
}
union { UNSG32 u32BITMAP16_SEL;
	struct w32BITMAP16_SEL;
};
#define     w32BITMAP16_SEL1                               {\
	UNSG32 uSEL_BIT_POS8                               :  4;\
	UNSG32 uSEL_BIT_POS9                               :  4;\
	UNSG32 uSEL_BIT_POS10                              :  4;\
	UNSG32 uSEL_BIT_POS11                              :  4;\
	UNSG32 uSEL_BIT_POS12                              :  4;\
	UNSG32 uSEL_BIT_POS13                              :  4;\
	UNSG32 uSEL_BIT_POS14                              :  4;\
	UNSG32 uSEL_BIT_POS15                              :  4;\
}
union { UNSG32 u32BITMAP16_SEL1;
	struct w32BITMAP16_SEL1;
};
} SIE_BITMAP16;
typedef union  T32BITMAP16_SEL
{ UNSG32 u32;
	struct w32BITMAP16_SEL;
} T32BITMAP16_SEL;
typedef union  T32BITMAP16_SEL1
{ UNSG32 u32;
	struct w32BITMAP16_SEL1;
} T32BITMAP16_SEL1;
typedef union  TBITMAP16_SEL
{ UNSG32 u32[2];
	struct {
		struct w32BITMAP16_SEL;
		struct w32BITMAP16_SEL1;
	};
} TBITMAP16_SEL;
SIGN32 BITMAP16_drvrd(SIE_BITMAP16 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 BITMAP16_drvwr(SIE_BITMAP16 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void BITMAP16_reset(SIE_BITMAP16 *p);
SIGN32 BITMAP16_cmp  (SIE_BITMAP16 *p, SIE_BITMAP16 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define BITMAP16_check(p,pie,pfx,hLOG) BITMAP16_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define BITMAP16_print(p,    pfx,hLOG) BITMAP16_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_BITMAP12
#define h_BITMAP12 (){}
#define     RA_BITMAP12_SEL                                0x0000
#define     RA_BITMAP12_SEL1                               0x0004
typedef struct SIE_BITMAP12 {
#define     w32BITMAP12_SEL                                {\
	UNSG32 uSEL_BIT_POS0                               :  4;\
	UNSG32 uSEL_BIT_POS1                               :  4;\
	UNSG32 uSEL_BIT_POS2                               :  4;\
	UNSG32 uSEL_BIT_POS3                               :  4;\
	UNSG32 uSEL_BIT_POS4                               :  4;\
	UNSG32 uSEL_BIT_POS5                               :  4;\
	UNSG32 uSEL_BIT_POS6                               :  4;\
	UNSG32 uSEL_BIT_POS7                               :  4;\
}
union { UNSG32 u32BITMAP12_SEL;
	struct w32BITMAP12_SEL;
};
#define     w32BITMAP12_SEL1                               {\
	UNSG32 uSEL_BIT_POS8                               :  4;\
	UNSG32 uSEL_BIT_POS9                               :  4;\
	UNSG32 uSEL_BIT_POS10                              :  4;\
	UNSG32 uSEL_BIT_POS11                              :  4;\
	UNSG32 RSVDx4_b16                                  : 16;\
}
union { UNSG32 u32BITMAP12_SEL1;
	struct w32BITMAP12_SEL1;
};
} SIE_BITMAP12;
typedef union  T32BITMAP12_SEL
{ UNSG32 u32;
	struct w32BITMAP12_SEL;
} T32BITMAP12_SEL;
typedef union  T32BITMAP12_SEL1
{ UNSG32 u32;
	struct w32BITMAP12_SEL1;
} T32BITMAP12_SEL1;
typedef union  TBITMAP12_SEL
{ UNSG32 u32[2];
	struct {
		struct w32BITMAP12_SEL;
		struct w32BITMAP12_SEL1;
	};
} TBITMAP12_SEL;
SIGN32 BITMAP12_drvrd(SIE_BITMAP12 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 BITMAP12_drvwr(SIE_BITMAP12 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void BITMAP12_reset(SIE_BITMAP12 *p);
SIGN32 BITMAP12_cmp  (SIE_BITMAP12 *p, SIE_BITMAP12 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define BITMAP12_check(p,pie,pfx,hLOG) BITMAP12_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define BITMAP12_print(p,    pfx,hLOG) BITMAP12_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ReadClient
#define h_ReadClient (){}
#define     RA_ReadClient_Rd                               0x0000
#define     RA_ReadClient_Word                             0x0004
#define     RA_ReadClient_NonStdRes                        0x0008
#define     RA_ReadClient_pack                             0x000C
typedef struct SIE_ReadClient {
#define     w32ReadClient_Rd                               {\
	UNSG32 uRd_start                                   :  1;\
	UNSG32 uRd_clear                                   :  1;\
	UNSG32 RSVDx0_b2                                   : 30;\
}
union { UNSG32 u32ReadClient_Rd;
	struct w32ReadClient_Rd;
};
#define     w32ReadClient_Word                             {\
	UNSG32 uWord_tot                                   : 32;\
}
union { UNSG32 u32ReadClient_Word;
	struct w32ReadClient_Word;
};
#define     w32ReadClient_NonStdRes                        {\
	UNSG32 uNonStdRes_enable                           :  1;\
	UNSG32 uNonStdRes_pixlineTot                       : 13;\
	UNSG32 uNonStdRes_flushCnt                         :  4;\
	UNSG32 RSVDx8_b18                                  : 14;\
}
union { UNSG32 u32ReadClient_NonStdRes;
	struct w32ReadClient_NonStdRes;
};
#define     w32ReadClient_pack                             {\
	UNSG32 upack_Sel                                   :  4;\
	UNSG32 RSVDxC_b4                                   : 28;\
}
union { UNSG32 u32ReadClient_pack;
	struct w32ReadClient_pack;
};
} SIE_ReadClient;
typedef union  T32ReadClient_Rd
{ UNSG32 u32;
	struct w32ReadClient_Rd;
} T32ReadClient_Rd;
typedef union  T32ReadClient_Word
{ UNSG32 u32;
	struct w32ReadClient_Word;
} T32ReadClient_Word;
typedef union  T32ReadClient_NonStdRes
{ UNSG32 u32;
	struct w32ReadClient_NonStdRes;
} T32ReadClient_NonStdRes;
typedef union  T32ReadClient_pack
{ UNSG32 u32;
	struct w32ReadClient_pack;
} T32ReadClient_pack;
typedef union  TReadClient_Rd
{ UNSG32 u32[1];
	struct {
		struct w32ReadClient_Rd;
	};
} TReadClient_Rd;
typedef union  TReadClient_Word
{ UNSG32 u32[1];
	struct {
		struct w32ReadClient_Word;
	};
} TReadClient_Word;
typedef union  TReadClient_NonStdRes
{ UNSG32 u32[1];
	struct {
		struct w32ReadClient_NonStdRes;
	};
} TReadClient_NonStdRes;
typedef union  TReadClient_pack
{ UNSG32 u32[1];
	struct {
		struct w32ReadClient_pack;
	};
} TReadClient_pack;
SIGN32 ReadClient_drvrd(SIE_ReadClient *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 ReadClient_drvwr(SIE_ReadClient *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void ReadClient_reset(SIE_ReadClient *p);
SIGN32 ReadClient_cmp  (SIE_ReadClient *p, SIE_ReadClient *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define ReadClient_check(p,pie,pfx,hLOG) ReadClient_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define ReadClient_print(p,    pfx,hLOG) ReadClient_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_WriteClient
#define h_WriteClient (){}
#define     RA_WriteClient_Wr                              0x0000
#define     RA_WriteClient_pix                             0x0004
#define     RA_WriteClient_NonStdRes                       0x0008
#define     RA_WriteClient_pack                            0x000C
typedef struct SIE_WriteClient {
#define     w32WriteClient_Wr                              {\
	UNSG32 uWr_start                                   :  1;\
	UNSG32 uWr_clear                                   :  1;\
	UNSG32 RSVDx0_b2                                   : 30;\
}
union { UNSG32 u32WriteClient_Wr;
	struct w32WriteClient_Wr;
};
#define     w32WriteClient_pix                             {\
	UNSG32 upix_tot                                    : 32;\
}
union { UNSG32 u32WriteClient_pix;
	struct w32WriteClient_pix;
};
#define     w32WriteClient_NonStdRes                       {\
	UNSG32 uNonStdRes_enable                           :  1;\
	UNSG32 uNonStdRes_pixlineTot                       : 13;\
	UNSG32 RSVDx8_b14                                  : 18;\
}
union { UNSG32 u32WriteClient_NonStdRes;
	struct w32WriteClient_NonStdRes;
};
#define     w32WriteClient_pack                            {\
	UNSG32 upack_Sel                                   :  4;\
	UNSG32 RSVDxC_b4                                   : 28;\
}
union { UNSG32 u32WriteClient_pack;
	struct w32WriteClient_pack;
};
} SIE_WriteClient;
typedef union  T32WriteClient_Wr
{ UNSG32 u32;
	struct w32WriteClient_Wr;
} T32WriteClient_Wr;
typedef union  T32WriteClient_pix
{ UNSG32 u32;
	struct w32WriteClient_pix;
} T32WriteClient_pix;
typedef union  T32WriteClient_NonStdRes
{ UNSG32 u32;
	struct w32WriteClient_NonStdRes;
} T32WriteClient_NonStdRes;
typedef union  T32WriteClient_pack
{ UNSG32 u32;
	struct w32WriteClient_pack;
} T32WriteClient_pack;
typedef union  TWriteClient_Wr
{ UNSG32 u32[1];
	struct {
		struct w32WriteClient_Wr;
	};
} TWriteClient_Wr;
typedef union  TWriteClient_pix
{ UNSG32 u32[1];
	struct {
		struct w32WriteClient_pix;
	};
} TWriteClient_pix;
typedef union  TWriteClient_NonStdRes
{ UNSG32 u32[1];
	struct {
		struct w32WriteClient_NonStdRes;
	};
} TWriteClient_NonStdRes;
typedef union  TWriteClient_pack
{ UNSG32 u32[1];
	struct {
		struct w32WriteClient_pack;
	};
} TWriteClient_pack;
SIGN32 WriteClient_drvrd(SIE_WriteClient *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 WriteClient_drvwr(SIE_WriteClient *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void WriteClient_reset(SIE_WriteClient *p);
SIGN32 WriteClient_cmp  (SIE_WriteClient *p, SIE_WriteClient *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define WriteClient_check(p,pie,pfx,hLOG) WriteClient_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define WriteClient_print(p,    pfx,hLOG) WriteClient_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ClientIF
#define h_ClientIF (){}
#define     RA_ClientIF_MR0                                0x0000
#define     RA_ClientIF_MR0_word                           0x0004
#define     RA_ClientIF_MR1                                0x0008
#define     RA_ClientIF_MR1_word                           0x000C
#define     RA_ClientIF_CTRL0                              0x0010
#define     RA_ClientIF_DUMMY                              0x0014
#define     RA_ClientIF_CTRL2                              0x0018
#define     RA_ClientIF_CTRL3                              0x001C
#define     RA_ClientIF_RdClientVmxVm                      0x0020
typedef struct SIE_ClientIF {
#define     w32ClientIF_MR0                                {\
	UNSG32 uMR0_start                                  :  1;\
	UNSG32 uMR0_clear                                  :  1;\
	UNSG32 RSVDx0_b2                                   : 30;\
}
union { UNSG32 u32ClientIF_MR0;
	struct w32ClientIF_MR0;
};
#define     w32ClientIF_MR0_word                           {\
	UNSG32 uMR0_word_tot                               : 32;\
}
union { UNSG32 u32ClientIF_MR0_word;
	struct w32ClientIF_MR0_word;
};
#define     w32ClientIF_MR1                                {\
	UNSG32 uMR1_start                                  :  1;\
	UNSG32 uMR1_clear                                  :  1;\
	UNSG32 RSVDx8_b2                                   : 30;\
}
union { UNSG32 u32ClientIF_MR1;
	struct w32ClientIF_MR1;
};
#define     w32ClientIF_MR1_word                           {\
	UNSG32 uMR1_word_tot                               : 32;\
}
union { UNSG32 u32ClientIF_MR1_word;
	struct w32ClientIF_MR1_word;
};
#define     w32ClientIF_CTRL0                              {\
	UNSG32 uCTRL0_CLKEN_Main0                          :  1;\
	UNSG32 uCTRL0_CLKEN_Main1                          :  1;\
	UNSG32 uCTRL0_rdmain_initval0                      :  1;\
	UNSG32 uCTRL0_rdmain_initval1                      :  1;\
	UNSG32 uCTRL0_rdm_mask_sftrst                      :  1;\
	UNSG32 uCTRL0_packSel_MR0                          :  4;\
	UNSG32 uCTRL0_packSel_MR1                          :  2;\
	UNSG32 uCTRL0_ups420_idataSelM                     :  1;\
	UNSG32 uCTRL0_read_sel_420SP                       :  1;\
	UNSG32 uCTRL0_ups420_idat_ctrl                     :  3;\
	UNSG32 RSVDx10_b16                                 : 16;\
}
union { UNSG32 u32ClientIF_CTRL0;
	struct w32ClientIF_CTRL0;
};
#define     w32ClientIF_DUMMY                              {\
	UNSG32 uDUMMY_dummy                                : 32;\
}
union { UNSG32 u32ClientIF_DUMMY;
	struct w32ClientIF_DUMMY;
};
#define     w32ClientIF_CTRL2                              {\
	UNSG32 uCTRL2_nonStdResEn_MR0                      :  1;\
	UNSG32 uCTRL2_pixlineTot_MR0                       : 13;\
	UNSG32 uCTRL2_flushCnt_MR0                         :  4;\
	UNSG32 RSVDx18_b18                                 : 14;\
}
union { UNSG32 u32ClientIF_CTRL2;
	struct w32ClientIF_CTRL2;
};
#define     w32ClientIF_CTRL3                              {\
	UNSG32 uCTRL3_nonStdResEn_MR1                      :  1;\
	UNSG32 uCTRL3_pixlineTot_MR1                       : 13;\
	UNSG32 uCTRL3_flushCnt_MR1                         :  4;\
	UNSG32 RSVDx1C_b18                                 : 14;\
}
union { UNSG32 u32ClientIF_CTRL3;
	struct w32ClientIF_CTRL3;
};
SIE_ReadClient                                   ie_RdClientVmxVm;
} SIE_ClientIF;
typedef union  T32ClientIF_MR0
{ UNSG32 u32;
	struct w32ClientIF_MR0;
} T32ClientIF_MR0;
typedef union  T32ClientIF_MR0_word
{ UNSG32 u32;
	struct w32ClientIF_MR0_word;
} T32ClientIF_MR0_word;
typedef union  T32ClientIF_MR1
{ UNSG32 u32;
	struct w32ClientIF_MR1;
} T32ClientIF_MR1;
typedef union  T32ClientIF_MR1_word
{ UNSG32 u32;
	struct w32ClientIF_MR1_word;
} T32ClientIF_MR1_word;
typedef union  T32ClientIF_CTRL0
{ UNSG32 u32;
	struct w32ClientIF_CTRL0;
} T32ClientIF_CTRL0;
typedef union  T32ClientIF_DUMMY
{ UNSG32 u32;
	struct w32ClientIF_DUMMY;
} T32ClientIF_DUMMY;
typedef union  T32ClientIF_CTRL2
{ UNSG32 u32;
	struct w32ClientIF_CTRL2;
} T32ClientIF_CTRL2;
typedef union  T32ClientIF_CTRL3
{ UNSG32 u32;
	struct w32ClientIF_CTRL3;
} T32ClientIF_CTRL3;
typedef union  TClientIF_MR0
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_MR0;
	};
} TClientIF_MR0;
typedef union  TClientIF_MR0_word
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_MR0_word;
	};
} TClientIF_MR0_word;
typedef union  TClientIF_MR1
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_MR1;
	};
} TClientIF_MR1;
typedef union  TClientIF_MR1_word
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_MR1_word;
	};
} TClientIF_MR1_word;
typedef union  TClientIF_CTRL0
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_CTRL0;
	};
} TClientIF_CTRL0;
typedef union  TClientIF_DUMMY
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_DUMMY;
	};
} TClientIF_DUMMY;
typedef union  TClientIF_CTRL2
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_CTRL2;
	};
} TClientIF_CTRL2;
typedef union  TClientIF_CTRL3
{ UNSG32 u32[1];
	struct {
		struct w32ClientIF_CTRL3;
	};
} TClientIF_CTRL3;
SIGN32 ClientIF_drvrd(SIE_ClientIF *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 ClientIF_drvwr(SIE_ClientIF *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void ClientIF_reset(SIE_ClientIF *p);
SIGN32 ClientIF_cmp  (SIE_ClientIF *p, SIE_ClientIF *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define ClientIF_check(p,pie,pfx,hLOG) ClientIF_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define ClientIF_print(p,    pfx,hLOG) ClientIF_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_BETG
#define h_BETG (){}
#define     RA_BETG_PL0                                    0x0000
#define     RA_BETG_PL1                                    0x0008
#define     RA_BETG_PL2                                    0x0010
#define     RA_BETG_PL3                                    0x0018
#define     RA_BETG_PL4                                    0x0020
#define     RA_BETG_PL5                                    0x0028
#define     RA_BETG_PL6                                    0x0030
#define     RA_BETG_PL7                                    0x0038
#define     RA_BETG_PL8                                    0x0040
#define     RA_BETG_PL1_CR                                 0x0048
#define     RA_BETG_PL2_CR                                 0x0050
#define     RA_BETG_PL3_CR                                 0x0058
#define     RA_BETG_PL4_CR                                 0x0060
#define     RA_BETG_PL5_CR                                 0x0068
#define     RA_BETG_PL6_CR                                 0x0070
#define     RA_BETG_PL7_CR                                 0x0078
#define     RA_BETG_PL8_CR                                 0x0080
#define     RA_BETG_PL_FLD                                 0x0088
#define     RA_BETG_TG_PRG                                 0x0090
typedef struct SIE_BETG {
	SIE_TG_PL                                        ie_PL0;
	SIE_TG_PL                                        ie_PL1;
	SIE_TG_PL                                        ie_PL2;
	SIE_TG_PL                                        ie_PL3;
	SIE_TG_PL                                        ie_PL4;
	SIE_TG_PL                                        ie_PL5;
	SIE_TG_PL                                        ie_PL6;
	SIE_TG_PL                                        ie_PL7;
	SIE_TG_PL                                        ie_PL8;
	SIE_TG_PL                                        ie_PL1_CR;
	SIE_TG_PL                                        ie_PL2_CR;
	SIE_TG_PL                                        ie_PL3_CR;
	SIE_TG_PL                                        ie_PL4_CR;
	SIE_TG_PL                                        ie_PL5_CR;
	SIE_TG_PL                                        ie_PL6_CR;
	SIE_TG_PL                                        ie_PL7_CR;
	SIE_TG_PL                                        ie_PL8_CR;
	SIE_TG_PL                                        ie_PL_FLD;
	SIE_TG_PRG                                       ie_TG_PRG;
} SIE_BETG;
SIGN32 BETG_drvrd(SIE_BETG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 BETG_drvwr(SIE_BETG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void BETG_reset(SIE_BETG *p);
SIGN32 BETG_cmp  (SIE_BETG *p, SIE_BETG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define BETG_check(p,pie,pfx,hLOG) BETG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define BETG_print(p,    pfx,hLOG) BETG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_CSC_C17O24
#define h_CSC_C17O24 (){}
#define     RA_CSC_C17O24_CFG0                             0x0000
#define     RA_CSC_C17O24_CFG1                             0x0004
#define     RA_CSC_C17O24_CFG2                             0x0008
#define     RA_CSC_C17O24_CFG3                             0x000C
#define     RA_CSC_C17O24_CFG4                             0x0010
#define     RA_CSC_C17O24_CFG5                             0x0014
#define     RA_CSC_C17O24_CFG6                             0x0018
#define     RA_CSC_C17O24_CFG7                             0x001C
#define     RA_CSC_C17O24_CFG8                             0x0020
#define     RA_CSC_C17O24_CFG9                             0x0024
#define     RA_CSC_C17O24_CFG10                            0x0028
#define     RA_CSC_C17O24_CFG11                            0x002C
#define     RA_CSC_C17O24_CFG12                            0x0030
#define     RA_CSC_C17O24_CFG13                            0x0034
#define     RA_CSC_C17O24_CFG14                            0x0038
typedef struct SIE_CSC_C17O24 {
#define     w32CSC_C17O24_CFG0                             {\
	UNSG32 mCFG0_C0                                    : 17;\
	UNSG32 RSVDx0_b17                                  : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG0;
	struct w32CSC_C17O24_CFG0;
};
#define     w32CSC_C17O24_CFG1                             {\
	UNSG32 mCFG1_C1                                    : 17;\
	UNSG32 RSVDx4_b17                                  : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG1;
	struct w32CSC_C17O24_CFG1;
};
#define     w32CSC_C17O24_CFG2                             {\
	UNSG32 mCFG2_C2                                    : 17;\
	UNSG32 RSVDx8_b17                                  : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG2;
	struct w32CSC_C17O24_CFG2;
};
#define     w32CSC_C17O24_CFG3                             {\
	UNSG32 mCFG3_C3                                    : 17;\
	UNSG32 RSVDxC_b17                                  : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG3;
	struct w32CSC_C17O24_CFG3;
};
#define     w32CSC_C17O24_CFG4                             {\
	UNSG32 mCFG4_C4                                    : 17;\
	UNSG32 RSVDx10_b17                                 : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG4;
	struct w32CSC_C17O24_CFG4;
};
#define     w32CSC_C17O24_CFG5                             {\
	UNSG32 mCFG5_C5                                    : 17;\
	UNSG32 RSVDx14_b17                                 : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG5;
	struct w32CSC_C17O24_CFG5;
};
#define     w32CSC_C17O24_CFG6                             {\
	UNSG32 mCFG6_C6                                    : 17;\
	UNSG32 RSVDx18_b17                                 : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG6;
	struct w32CSC_C17O24_CFG6;
};
#define     w32CSC_C17O24_CFG7                             {\
	UNSG32 mCFG7_C7                                    : 17;\
	UNSG32 RSVDx1C_b17                                 : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG7;
	struct w32CSC_C17O24_CFG7;
};
#define     w32CSC_C17O24_CFG8                             {\
	UNSG32 mCFG8_C8                                    : 17;\
	UNSG32 RSVDx20_b17                                 : 15;\
}
union { UNSG32 u32CSC_C17O24_CFG8;
	struct w32CSC_C17O24_CFG8;
};
#define     w32CSC_C17O24_CFG9                             {\
	UNSG32 mCFG9_OFF1                                  : 24;\
	UNSG32 RSVDx24_b24                                 :  8;\
}
union { UNSG32 u32CSC_C17O24_CFG9;
	struct w32CSC_C17O24_CFG9;
};
#define     w32CSC_C17O24_CFG10                            {\
	UNSG32 mCFG10_OFF2                                 : 24;\
	UNSG32 RSVDx28_b24                                 :  8;\
}
union { UNSG32 u32CSC_C17O24_CFG10;
	struct w32CSC_C17O24_CFG10;
};
#define     w32CSC_C17O24_CFG11                            {\
	UNSG32 mCFG11_OFF3                                 : 24;\
	UNSG32 RSVDx2C_b24                                 :  8;\
}
union { UNSG32 u32CSC_C17O24_CFG11;
	struct w32CSC_C17O24_CFG11;
};
#define     w32CSC_C17O24_CFG12                            {\
	UNSG32 uCFG12_CL1MIN                               : 12;\
	UNSG32 uCFG12_CL1MAX                               : 12;\
	UNSG32 RSVDx30_b24                                 :  8;\
}
union { UNSG32 u32CSC_C17O24_CFG12;
	struct w32CSC_C17O24_CFG12;
};
#define     w32CSC_C17O24_CFG13                            {\
	UNSG32 uCFG13_CL2MIN                               : 12;\
	UNSG32 uCFG13_CL2MAX                               : 12;\
	UNSG32 RSVDx34_b24                                 :  8;\
}
union { UNSG32 u32CSC_C17O24_CFG13;
	struct w32CSC_C17O24_CFG13;
};
#define     w32CSC_C17O24_CFG14                            {\
	UNSG32 uCFG14_CL3MIN                               : 12;\
	UNSG32 uCFG14_CL3MAX                               : 12;\
	UNSG32 RSVDx38_b24                                 :  8;\
}
union { UNSG32 u32CSC_C17O24_CFG14;
	struct w32CSC_C17O24_CFG14;
};
} SIE_CSC_C17O24;
typedef union  T32CSC_C17O24_CFG0
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG0;
} T32CSC_C17O24_CFG0;
typedef union  T32CSC_C17O24_CFG1
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG1;
} T32CSC_C17O24_CFG1;
typedef union  T32CSC_C17O24_CFG2
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG2;
} T32CSC_C17O24_CFG2;
typedef union  T32CSC_C17O24_CFG3
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG3;
} T32CSC_C17O24_CFG3;
typedef union  T32CSC_C17O24_CFG4
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG4;
} T32CSC_C17O24_CFG4;
typedef union  T32CSC_C17O24_CFG5
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG5;
} T32CSC_C17O24_CFG5;
typedef union  T32CSC_C17O24_CFG6
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG6;
} T32CSC_C17O24_CFG6;
typedef union  T32CSC_C17O24_CFG7
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG7;
} T32CSC_C17O24_CFG7;
typedef union  T32CSC_C17O24_CFG8
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG8;
} T32CSC_C17O24_CFG8;
typedef union  T32CSC_C17O24_CFG9
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG9;
} T32CSC_C17O24_CFG9;
typedef union  T32CSC_C17O24_CFG10
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG10;
} T32CSC_C17O24_CFG10;
typedef union  T32CSC_C17O24_CFG11
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG11;
} T32CSC_C17O24_CFG11;
typedef union  T32CSC_C17O24_CFG12
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG12;
} T32CSC_C17O24_CFG12;
typedef union  T32CSC_C17O24_CFG13
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG13;
} T32CSC_C17O24_CFG13;
typedef union  T32CSC_C17O24_CFG14
{ UNSG32 u32;
	struct w32CSC_C17O24_CFG14;
} T32CSC_C17O24_CFG14;
typedef union  TCSC_C17O24_CFG0
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG0;
	};
} TCSC_C17O24_CFG0;
typedef union  TCSC_C17O24_CFG1
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG1;
	};
} TCSC_C17O24_CFG1;
typedef union  TCSC_C17O24_CFG2
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG2;
	};
} TCSC_C17O24_CFG2;
typedef union  TCSC_C17O24_CFG3
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG3;
	};
} TCSC_C17O24_CFG3;
typedef union  TCSC_C17O24_CFG4
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG4;
	};
} TCSC_C17O24_CFG4;
typedef union  TCSC_C17O24_CFG5
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG5;
	};
} TCSC_C17O24_CFG5;
typedef union  TCSC_C17O24_CFG6
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG6;
	};
} TCSC_C17O24_CFG6;
typedef union  TCSC_C17O24_CFG7
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG7;
	};
} TCSC_C17O24_CFG7;
typedef union  TCSC_C17O24_CFG8
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG8;
	};
} TCSC_C17O24_CFG8;
typedef union  TCSC_C17O24_CFG9
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG9;
	};
} TCSC_C17O24_CFG9;
typedef union  TCSC_C17O24_CFG10
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG10;
	};
} TCSC_C17O24_CFG10;
typedef union  TCSC_C17O24_CFG11
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG11;
	};
} TCSC_C17O24_CFG11;
typedef union  TCSC_C17O24_CFG12
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG12;
	};
} TCSC_C17O24_CFG12;
typedef union  TCSC_C17O24_CFG13
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG13;
	};
} TCSC_C17O24_CFG13;
typedef union  TCSC_C17O24_CFG14
{ UNSG32 u32[1];
	struct {
		struct w32CSC_C17O24_CFG14;
	};
} TCSC_C17O24_CFG14;
SIGN32 CSC_C17O24_drvrd(SIE_CSC_C17O24 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 CSC_C17O24_drvwr(SIE_CSC_C17O24 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void CSC_C17O24_reset(SIE_CSC_C17O24 *p);
SIGN32 CSC_C17O24_cmp  (SIE_CSC_C17O24 *p, SIE_CSC_C17O24 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define CSC_C17O24_check(p,pie,pfx,hLOG) CSC_C17O24_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define CSC_C17O24_print(p,    pfx,hLOG) CSC_C17O24_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SCL1D_YC
#define h_SCL1D_YC (){}
#define     RA_SCL1D_YC_CFG0                               0x0000
#define     RA_SCL1D_YC_CFG1                               0x0004
#define     RA_SCL1D_YC_CFG2                               0x0008
#define     RA_SCL1D_YC_CFG3                               0x000C
#define     RA_SCL1D_YC_CFG4                               0x0010
#define     RA_SCL1D_YC_CFG5                               0x0014
#define     RA_SCL1D_YC_CFG6                               0x0018
#define     RA_SCL1D_YC_CFG7                               0x001C
#define     RA_SCL1D_YC_CFG8                               0x0020
#define     RA_SCL1D_YC_CFG9                               0x0024
#define     RA_SCL1D_YC_CFG10                              0x0028
#define     RA_SCL1D_YC_CFG11                              0x002C
#define     RA_SCL1D_YC_CFG12                              0x0030
#define     RA_SCL1D_YC_CFG13                              0x0034
#define     RA_SCL1D_YC_CFG14                              0x0038
#define     RA_SCL1D_YC_CFG15                              0x003C
#define     RA_SCL1D_YC_CFG16                              0x0040
#define     RA_SCL1D_YC_CFG17                              0x0044
#define     RA_SCL1D_YC_CFG18                              0x0048
#define     RA_SCL1D_YC_CFG19                              0x004C
#define     RA_SCL1D_YC_CFG20                              0x0050
#define     RA_SCL1D_YC_CFG21                              0x0054
#define     RA_SCL1D_YC_CFG33                              0x0058
typedef struct SIE_SCL1D_YC {
#define     w32SCL1D_YC_CFG0                               {\
	UNSG32 uCFG0_even                                  :  8;\
	UNSG32 uCFG0_odd                                   :  8;\
	UNSG32 uCFG0_hinitph                               :  8;\
	UNSG32 uCFG0_ctrl0                                 :  8;\
}
union { UNSG32 u32SCL1D_YC_CFG0;
	struct w32SCL1D_YC_CFG0;
};
#define     w32SCL1D_YC_CFG1                               {\
	UNSG32 uCFG1_ivres                                 : 12;\
	UNSG32 uCFG1_ovres                                 : 12;\
	UNSG32 RSVDx4_b24                                  :  8;\
}
union { UNSG32 u32SCL1D_YC_CFG1;
	struct w32SCL1D_YC_CFG1;
};
#define     w32SCL1D_YC_CFG2                               {\
	UNSG32 uCFG2_ihres                                 : 13;\
	UNSG32 uCFG2_ohres                                 : 13;\
	UNSG32 RSVDx8_b26                                  :  6;\
}
union { UNSG32 u32SCL1D_YC_CFG2;
	struct w32SCL1D_YC_CFG2;
};
#define     w32SCL1D_YC_CFG3                               {\
	UNSG32 uCFG3_vratio                                : 25;\
	UNSG32 uCFG3_rsv0                                  :  3;\
	UNSG32 uCFG3_mbp_align                             :  1;\
	UNSG32 RSVDxC_b29                                  :  3;\
}
union { UNSG32 u32SCL1D_YC_CFG3;
	struct w32SCL1D_YC_CFG3;
};
#define     w32SCL1D_YC_CFG4                               {\
	UNSG32 uCFG4_hratio                                : 25;\
	UNSG32 uCFG4_rsv0                                  :  3;\
	UNSG32 uCFG4_mrp_align                             :  1;\
	UNSG32 RSVDx10_b29                                 :  3;\
}
union { UNSG32 u32SCL1D_YC_CFG4;
	struct w32SCL1D_YC_CFG4;
};
#define     w32SCL1D_YC_CFG5                               {\
	UNSG32 uCFG5_memsize                               : 11;\
	UNSG32 uCFG5_vwrap                                 : 16;\
	UNSG32 RSVDx14_b27                                 :  5;\
}
union { UNSG32 u32SCL1D_YC_CFG5;
	struct w32SCL1D_YC_CFG5;
};
#define     w32SCL1D_YC_CFG6                               {\
	UNSG32 uCFG6_ups_cswap                             :  1;\
	UNSG32 uCFG6_ups_cshift                            :  1;\
	UNSG32 uCFG6_ups_yshift                            :  1;\
	UNSG32 uCFG6_ups_yblank                            : 12;\
	UNSG32 uCFG6_fstall                                :  8;\
	UNSG32 uCFG6_bstall                                :  8;\
	UNSG32 RSVDx18_b31                                 :  1;\
}
union { UNSG32 u32SCL1D_YC_CFG6;
	struct w32SCL1D_YC_CFG6;
};
#define     w32SCL1D_YC_CFG7                               {\
	UNSG32 uCFG7_ups_cblank                            : 12;\
	UNSG32 uCFG7_htap_offset                           :  6;\
	UNSG32 uCFG7_rsv                                   :  2;\
	UNSG32 uCFG7_vtap_offset                           :  8;\
	UNSG32 RSVDx1C_b28                                 :  4;\
}
union { UNSG32 u32SCL1D_YC_CFG7;
	struct w32SCL1D_YC_CFG7;
};
#define     w32SCL1D_YC_CFG8                               {\
	UNSG32 uCFG8_init_ratio                            : 25;\
	UNSG32 RSVDx20_b25                                 :  7;\
}
union { UNSG32 u32SCL1D_YC_CFG8;
	struct w32SCL1D_YC_CFG8;
};
#define     w32SCL1D_YC_CFG9                               {\
	UNSG32 uCFG9_inc_ratio                             : 25;\
	UNSG32 RSVDx24_b25                                 :  7;\
}
union { UNSG32 u32SCL1D_YC_CFG9;
	struct w32SCL1D_YC_CFG9;
};
#define     w32SCL1D_YC_CFG10                              {\
	UNSG32 uCFG10_nlcres                               : 13;\
	UNSG32 RSVDx28_b13                                 : 19;\
}
union { UNSG32 u32SCL1D_YC_CFG10;
	struct w32SCL1D_YC_CFG10;
};
#define     w32SCL1D_YC_CFG11                              {\
	UNSG32 uCFG11_avg4_coeff0                          : 12;\
	UNSG32 uCFG11_avg4_coeff1                          : 12;\
	UNSG32 uCFG11_dign_dr1                             :  8;\
}
union { UNSG32 u32SCL1D_YC_CFG11;
	struct w32SCL1D_YC_CFG11;
};
#define     w32SCL1D_YC_CFG12                              {\
	UNSG32 uCFG12_avg4_coeff2                          : 12;\
	UNSG32 uCFG12_avg4_coeff3                          : 12;\
	UNSG32 uCFG12_dign_dr2                             :  8;\
}
union { UNSG32 u32SCL1D_YC_CFG12;
	struct w32SCL1D_YC_CFG12;
};
#define     w32SCL1D_YC_CFG13                              {\
	UNSG32 uCFG13_dign_ydiff                           : 12;\
	UNSG32 uCFG13_dign_ddiff                           : 12;\
	UNSG32 uCFG13_dign_dr3                             :  8;\
}
union { UNSG32 u32SCL1D_YC_CFG13;
	struct w32SCL1D_YC_CFG13;
};
#define     w32SCL1D_YC_CFG14                              {\
	UNSG32 uCFG14_hp_offset                            : 12;\
	UNSG32 uCFG14_hp_diff                              : 12;\
	UNSG32 uCFG14_dign_nbr                             :  8;\
}
union { UNSG32 u32SCL1D_YC_CFG14;
	struct w32SCL1D_YC_CFG14;
};
#define     w32SCL1D_YC_CFG15                              {\
	UNSG32 uCFG15_ctrl1                                :  8;\
	UNSG32 uCFG15_hctrl                                :  5;\
	UNSG32 uCFG15_vctrl                                :  5;\
	UNSG32 uCFG15_crop                                 :  4;\
	UNSG32 uCFG15_hscl11                               :  1;\
	UNSG32 uCFG15_vscl11                               :  1;\
	UNSG32 uCFG15_bypass                               :  1;\
	UNSG32 uCFG15_frndsel                              :  1;\
	UNSG32 uCFG15_hvdcp                                :  1;\
	UNSG32 uCFG15_pa_impv_en                           :  1;\
	UNSG32 uCFG15_mask_hvswap_en                       :  4;\
}
union { UNSG32 u32SCL1D_YC_CFG15;
	struct w32SCL1D_YC_CFG15;
};
#define     w32SCL1D_YC_CFG16                              {\
	UNSG32 uCFG16_en_n                                 :  1;\
	UNSG32 uCFG16_xbstall_en                           :  1;\
	UNSG32 uCFG16_xbstall_dly                          : 14;\
	UNSG32 uCFG16_ovf_margin                           :  8;\
	UNSG32 uCFG16_udf_margin                           :  8;\
}
union { UNSG32 u32SCL1D_YC_CFG16;
	struct w32SCL1D_YC_CFG16;
};
#define     w32SCL1D_YC_CFG17                              {\
	UNSG32 uCFG17_lsize                                : 10;\
	UNSG32 uCFG17_lsize_A                              : 10;\
	UNSG32 RSVDx44_b20                                 : 12;\
}
union { UNSG32 u32SCL1D_YC_CFG17;
	struct w32SCL1D_YC_CFG17;
};
#define     w32SCL1D_YC_CFG18                              {\
	UNSG32 uCFG18_fifo_mode                            :  1;\
	UNSG32 uCFG18_fifo_depth                           : 15;\
	UNSG32 uCFG18_fifo_dfst                            : 15;\
	UNSG32 uCFG18_pftch                                :  1;\
}
union { UNSG32 u32SCL1D_YC_CFG18;
	struct w32SCL1D_YC_CFG18;
};
#define     w32SCL1D_YC_CFG19                              {\
	UNSG32 uCFG19_scl_after_crop_h                     :  8;\
	UNSG32 uCFG19_scl_after_crop_v                     :  8;\
	UNSG32 RSVDx4C_b16                                 : 16;\
}
union { UNSG32 u32SCL1D_YC_CFG19;
	struct w32SCL1D_YC_CFG19;
};
#define     w32SCL1D_YC_CFG20                              {\
	UNSG32 uCFG20_hlut_pdwn_regs                       :  1;\
	UNSG32 uCFG20_hlut_pdlvmc_regs                     :  1;\
	UNSG32 uCFG20_hlut_pdfvssm_regs                    :  1;\
	UNSG32 uCFG20_reserved0                            :  1;\
	UNSG32 uCFG20_vlut_pdwn_regs                       :  1;\
	UNSG32 uCFG20_vlut_pdlvmc_regs                     :  1;\
	UNSG32 uCFG20_vlut_pdfvssm_regs                    :  1;\
	UNSG32 uCFG20_reserved1                            :  1;\
	UNSG32 uCFG20_dpwr_regs                            :  3;\
	UNSG32 uCFG20_Reserved2                            :  1;\
	UNSG32 uCFG20_syp_yc420_regs                       :  1;\
	UNSG32 uCFG20_Reserved3                            :  1;\
	UNSG32 uCFG20_ip_type_regs                         :  2;\
	UNSG32 RSVDx50_b16                                 : 16;\
}
union { UNSG32 u32SCL1D_YC_CFG20;
	struct w32SCL1D_YC_CFG20;
};
#define     w32SCL1D_YC_CFG21                              {\
	UNSG32 uCFG21_hinitph_lsb                          : 12;\
	UNSG32 RSVDx54_b12                                 : 20;\
}
union { UNSG32 u32SCL1D_YC_CFG21;
	struct w32SCL1D_YC_CFG21;
};
#define     w32SCL1D_YC_CFG33                              {\
	UNSG32 uCFG33_lb0_pdwn_regs                        :  1;\
	UNSG32 uCFG33_lb0_pdlvmc_regs                      :  1;\
	UNSG32 uCFG33_lb0_pdfvssm_regs                     :  1;\
	UNSG32 uCFG33_reserved0                            :  1;\
	UNSG32 RSVDx58_b4                                  : 28;\
}
union { UNSG32 u32SCL1D_YC_CFG33;
	struct w32SCL1D_YC_CFG33;
};
} SIE_SCL1D_YC;
typedef union  T32SCL1D_YC_CFG0
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG0;
} T32SCL1D_YC_CFG0;
typedef union  T32SCL1D_YC_CFG1
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG1;
} T32SCL1D_YC_CFG1;
typedef union  T32SCL1D_YC_CFG2
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG2;
} T32SCL1D_YC_CFG2;
typedef union  T32SCL1D_YC_CFG3
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG3;
} T32SCL1D_YC_CFG3;
typedef union  T32SCL1D_YC_CFG4
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG4;
} T32SCL1D_YC_CFG4;
typedef union  T32SCL1D_YC_CFG5
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG5;
} T32SCL1D_YC_CFG5;
typedef union  T32SCL1D_YC_CFG6
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG6;
} T32SCL1D_YC_CFG6;
typedef union  T32SCL1D_YC_CFG7
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG7;
} T32SCL1D_YC_CFG7;
typedef union  T32SCL1D_YC_CFG8
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG8;
} T32SCL1D_YC_CFG8;
typedef union  T32SCL1D_YC_CFG9
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG9;
} T32SCL1D_YC_CFG9;
typedef union  T32SCL1D_YC_CFG10
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG10;
} T32SCL1D_YC_CFG10;
typedef union  T32SCL1D_YC_CFG11
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG11;
} T32SCL1D_YC_CFG11;
typedef union  T32SCL1D_YC_CFG12
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG12;
} T32SCL1D_YC_CFG12;
typedef union  T32SCL1D_YC_CFG13
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG13;
} T32SCL1D_YC_CFG13;
typedef union  T32SCL1D_YC_CFG14
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG14;
} T32SCL1D_YC_CFG14;
typedef union  T32SCL1D_YC_CFG15
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG15;
} T32SCL1D_YC_CFG15;
typedef union  T32SCL1D_YC_CFG16
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG16;
} T32SCL1D_YC_CFG16;
typedef union  T32SCL1D_YC_CFG17
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG17;
} T32SCL1D_YC_CFG17;
typedef union  T32SCL1D_YC_CFG18
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG18;
} T32SCL1D_YC_CFG18;
typedef union  T32SCL1D_YC_CFG19
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG19;
} T32SCL1D_YC_CFG19;
typedef union  T32SCL1D_YC_CFG20
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG20;
} T32SCL1D_YC_CFG20;
typedef union  T32SCL1D_YC_CFG21
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG21;
} T32SCL1D_YC_CFG21;
typedef union  T32SCL1D_YC_CFG33
{ UNSG32 u32;
	struct w32SCL1D_YC_CFG33;
} T32SCL1D_YC_CFG33;
typedef union  TSCL1D_YC_CFG0
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG0;
	};
} TSCL1D_YC_CFG0;
typedef union  TSCL1D_YC_CFG1
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG1;
	};
} TSCL1D_YC_CFG1;
typedef union  TSCL1D_YC_CFG2
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG2;
	};
} TSCL1D_YC_CFG2;
typedef union  TSCL1D_YC_CFG3
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG3;
	};
} TSCL1D_YC_CFG3;
typedef union  TSCL1D_YC_CFG4
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG4;
	};
} TSCL1D_YC_CFG4;
typedef union  TSCL1D_YC_CFG5
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG5;
	};
} TSCL1D_YC_CFG5;
typedef union  TSCL1D_YC_CFG6
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG6;
	};
} TSCL1D_YC_CFG6;
typedef union  TSCL1D_YC_CFG7
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG7;
	};
} TSCL1D_YC_CFG7;
typedef union  TSCL1D_YC_CFG8
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG8;
	};
} TSCL1D_YC_CFG8;
typedef union  TSCL1D_YC_CFG9
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG9;
	};
} TSCL1D_YC_CFG9;
typedef union  TSCL1D_YC_CFG10
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG10;
	};
} TSCL1D_YC_CFG10;
typedef union  TSCL1D_YC_CFG11
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG11;
	};
} TSCL1D_YC_CFG11;
typedef union  TSCL1D_YC_CFG12
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG12;
	};
} TSCL1D_YC_CFG12;
typedef union  TSCL1D_YC_CFG13
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG13;
	};
} TSCL1D_YC_CFG13;
typedef union  TSCL1D_YC_CFG14
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG14;
	};
} TSCL1D_YC_CFG14;
typedef union  TSCL1D_YC_CFG15
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG15;
	};
} TSCL1D_YC_CFG15;
typedef union  TSCL1D_YC_CFG16
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG16;
	};
} TSCL1D_YC_CFG16;
typedef union  TSCL1D_YC_CFG17
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG17;
	};
} TSCL1D_YC_CFG17;
typedef union  TSCL1D_YC_CFG18
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG18;
	};
} TSCL1D_YC_CFG18;
typedef union  TSCL1D_YC_CFG19
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG19;
	};
} TSCL1D_YC_CFG19;
typedef union  TSCL1D_YC_CFG20
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG20;
	};
} TSCL1D_YC_CFG20;
typedef union  TSCL1D_YC_CFG21
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG21;
	};
} TSCL1D_YC_CFG21;
typedef union  TSCL1D_YC_CFG33
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_CFG33;
	};
} TSCL1D_YC_CFG33;
SIGN32 SCL1D_YC_drvrd(SIE_SCL1D_YC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 SCL1D_YC_drvwr(SIE_SCL1D_YC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void SCL1D_YC_reset(SIE_SCL1D_YC *p);
SIGN32 SCL1D_YC_cmp  (SIE_SCL1D_YC *p, SIE_SCL1D_YC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define SCL1D_YC_check(p,pie,pfx,hLOG) SCL1D_YC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define SCL1D_YC_print(p,    pfx,hLOG) SCL1D_YC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SCL1D_YC_COEFF
#define h_SCL1D_YC_COEFF (){}
#define     RA_SCL1D_YC_COEFF_CFG0                         0x0000
#define     RA_SCL1D_YC_COEFF_CFG1                         0x0004
#define     RA_SCL1D_YC_COEFF_CFG2                         0x0008
#define     RA_SCL1D_YC_COEFF_CFG3                         0x000C
#define     RA_SCL1D_YC_COEFF_CFG4                         0x0010
typedef struct SIE_SCL1D_YC_COEFF {
#define     w32SCL1D_YC_COEFF_CFG0                         {\
	UNSG32 uCFG0_htap                                  :  4;\
	UNSG32 uCFG0_vtap                                  :  4;\
	UNSG32 uCFG0_coeff_index                           :  5;\
	UNSG32 uCFG0_coeff_hvsel                           :  1;\
	UNSG32 uCFG0_coeffload                             :  1;\
	UNSG32 uCFG0_coeffread                             :  1;\
	UNSG32 RSVDx0_b16                                  : 16;\
}
union { UNSG32 u32SCL1D_YC_COEFF_CFG0;
	struct w32SCL1D_YC_COEFF_CFG0;
};
#define     w32SCL1D_YC_COEFF_CFG1                         {\
	UNSG32 uCFG1_coeff0                                : 12;\
	UNSG32 uCFG1_coeff1                                : 12;\
	UNSG32 RSVDx4_b24                                  :  8;\
}
union { UNSG32 u32SCL1D_YC_COEFF_CFG1;
	struct w32SCL1D_YC_COEFF_CFG1;
};
#define     w32SCL1D_YC_COEFF_CFG2                         {\
	UNSG32 uCFG2_coeff2                                : 12;\
	UNSG32 uCFG2_coeff3                                : 12;\
	UNSG32 RSVDx8_b24                                  :  8;\
}
union { UNSG32 u32SCL1D_YC_COEFF_CFG2;
	struct w32SCL1D_YC_COEFF_CFG2;
};
#define     w32SCL1D_YC_COEFF_CFG3                         {\
	UNSG32 uCFG3_coeff4                                : 12;\
	UNSG32 uCFG3_coeff5                                : 12;\
	UNSG32 RSVDxC_b24                                  :  8;\
}
union { UNSG32 u32SCL1D_YC_COEFF_CFG3;
	struct w32SCL1D_YC_COEFF_CFG3;
};
#define     w32SCL1D_YC_COEFF_CFG4                         {\
	UNSG32 uCFG4_coeff6                                : 12;\
	UNSG32 uCFG4_coeff7                                : 12;\
	UNSG32 RSVDx10_b24                                 :  8;\
}
union { UNSG32 u32SCL1D_YC_COEFF_CFG4;
	struct w32SCL1D_YC_COEFF_CFG4;
};
} SIE_SCL1D_YC_COEFF;
typedef union  T32SCL1D_YC_COEFF_CFG0
{ UNSG32 u32;
	struct w32SCL1D_YC_COEFF_CFG0;
} T32SCL1D_YC_COEFF_CFG0;
typedef union  T32SCL1D_YC_COEFF_CFG1
{ UNSG32 u32;
	struct w32SCL1D_YC_COEFF_CFG1;
} T32SCL1D_YC_COEFF_CFG1;
typedef union  T32SCL1D_YC_COEFF_CFG2
{ UNSG32 u32;
	struct w32SCL1D_YC_COEFF_CFG2;
} T32SCL1D_YC_COEFF_CFG2;
typedef union  T32SCL1D_YC_COEFF_CFG3
{ UNSG32 u32;
	struct w32SCL1D_YC_COEFF_CFG3;
} T32SCL1D_YC_COEFF_CFG3;
typedef union  T32SCL1D_YC_COEFF_CFG4
{ UNSG32 u32;
	struct w32SCL1D_YC_COEFF_CFG4;
} T32SCL1D_YC_COEFF_CFG4;
typedef union  TSCL1D_YC_COEFF_CFG0
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_COEFF_CFG0;
	};
} TSCL1D_YC_COEFF_CFG0;
typedef union  TSCL1D_YC_COEFF_CFG1
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_COEFF_CFG1;
	};
} TSCL1D_YC_COEFF_CFG1;
typedef union  TSCL1D_YC_COEFF_CFG2
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_COEFF_CFG2;
	};
} TSCL1D_YC_COEFF_CFG2;
typedef union  TSCL1D_YC_COEFF_CFG3
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_COEFF_CFG3;
	};
} TSCL1D_YC_COEFF_CFG3;
typedef union  TSCL1D_YC_COEFF_CFG4
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_COEFF_CFG4;
	};
} TSCL1D_YC_COEFF_CFG4;
SIGN32 SCL1D_YC_COEFF_drvrd(SIE_SCL1D_YC_COEFF *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 SCL1D_YC_COEFF_drvwr(SIE_SCL1D_YC_COEFF *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void SCL1D_YC_COEFF_reset(SIE_SCL1D_YC_COEFF *p);
SIGN32 SCL1D_YC_COEFF_cmp  (SIE_SCL1D_YC_COEFF *p, SIE_SCL1D_YC_COEFF *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define SCL1D_YC_COEFF_check(p,pie,pfx,hLOG) SCL1D_YC_COEFF_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define SCL1D_YC_COEFF_print(p,    pfx,hLOG) SCL1D_YC_COEFF_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SCL1D_YC_FRC
#define h_SCL1D_YC_FRC (){}
#define     RA_SCL1D_YC_FRC_CFG0                           0x0000
#define     RA_SCL1D_YC_FRC_CFG1                           0x0004
#define     RA_SCL1D_YC_FRC_CFG2                           0x0008
#define     RA_SCL1D_YC_FRC_CFG3                           0x000C
#define     RA_SCL1D_YC_FRC_CFG4                           0x0010
#define     RA_SCL1D_YC_FRC_CFG5                           0x0014
#define     RA_SCL1D_YC_FRC_CFG6                           0x0018
typedef struct SIE_SCL1D_YC_FRC {
#define     w32SCL1D_YC_FRC_CFG0                           {\
	UNSG32 uCFG0_sclclk_ctrl                           :  8;\
	UNSG32 uCFG0_sclclk_ctrl1                          :  8;\
	UNSG32 uCFG0_ctrl0                                 :  8;\
	UNSG32 uCFG0_ctrl1                                 :  8;\
}
union { UNSG32 u32SCL1D_YC_FRC_CFG0;
	struct w32SCL1D_YC_FRC_CFG0;
};
#define     w32SCL1D_YC_FRC_CFG1                           {\
	UNSG32 uCFG1_ctrl2                                 :  8;\
	UNSG32 uCFG1_dly_frst_de                           :  8;\
	UNSG32 uCFG1_dly_de_lrst                           : 10;\
	UNSG32 uCFG1_auto_lrst                             :  1;\
	UNSG32 RSVDx4_b27                                  :  5;\
}
union { UNSG32 u32SCL1D_YC_FRC_CFG1;
	struct w32SCL1D_YC_FRC_CFG1;
};
#define     w32SCL1D_YC_FRC_CFG2                           {\
	UNSG32 uCFG2_bevres                                : 12;\
	UNSG32 uCFG2_fevres                                : 12;\
	UNSG32 uCFG2_dly_lrst_de                           :  8;\
}
union { UNSG32 u32SCL1D_YC_FRC_CFG2;
	struct w32SCL1D_YC_FRC_CFG2;
};
#define     w32SCL1D_YC_FRC_CFG3                           {\
	UNSG32 uCFG3_behres                                : 13;\
	UNSG32 uCFG3_clnt_ctrl                             :  2;\
	UNSG32 uCFG3_ctrl                                  :  8;\
	UNSG32 uCFG3_rff_ctrl                              :  8;\
	UNSG32 RSVDxC_b31                                  :  1;\
}
union { UNSG32 u32SCL1D_YC_FRC_CFG3;
	struct w32SCL1D_YC_FRC_CFG3;
};
#define     w32SCL1D_YC_FRC_CFG4                           {\
	UNSG32 uCFG4_ols                                   :  1;\
	UNSG32 uCFG4_wclient                               :  1;\
	UNSG32 uCFG4_luma_key_en                           :  1;\
	UNSG32 uCFG4_luma_key_min                          : 10;\
	UNSG32 uCFG4_luma_key_max                          : 10;\
	UNSG32 RSVDx10_b23                                 :  9;\
}
union { UNSG32 u32SCL1D_YC_FRC_CFG4;
	struct w32SCL1D_YC_FRC_CFG4;
};
#define     w32SCL1D_YC_FRC_CFG5                           {\
	UNSG32 uCFG5_al_en_hit_al                          : 10;\
	UNSG32 uCFG5_al_en_non_hit_al                      : 12;\
	UNSG32 RSVDx14_b22                                 : 10;\
}
union { UNSG32 u32SCL1D_YC_FRC_CFG5;
	struct w32SCL1D_YC_FRC_CFG5;
};
#define     w32SCL1D_YC_FRC_CFG6                           {\
	UNSG32 uCFG6_al_out_threshold                      : 10;\
	UNSG32 uCFG6_al_out_zeroluma                       : 10;\
	UNSG32 RSVDx18_b20                                 : 12;\
}
union { UNSG32 u32SCL1D_YC_FRC_CFG6;
	struct w32SCL1D_YC_FRC_CFG6;
};
} SIE_SCL1D_YC_FRC;
typedef union  T32SCL1D_YC_FRC_CFG0
{ UNSG32 u32;
	struct w32SCL1D_YC_FRC_CFG0;
} T32SCL1D_YC_FRC_CFG0;
typedef union  T32SCL1D_YC_FRC_CFG1
{ UNSG32 u32;
	struct w32SCL1D_YC_FRC_CFG1;
} T32SCL1D_YC_FRC_CFG1;
typedef union  T32SCL1D_YC_FRC_CFG2
{ UNSG32 u32;
	struct w32SCL1D_YC_FRC_CFG2;
} T32SCL1D_YC_FRC_CFG2;
typedef union  T32SCL1D_YC_FRC_CFG3
{ UNSG32 u32;
	struct w32SCL1D_YC_FRC_CFG3;
} T32SCL1D_YC_FRC_CFG3;
typedef union  T32SCL1D_YC_FRC_CFG4
{ UNSG32 u32;
	struct w32SCL1D_YC_FRC_CFG4;
} T32SCL1D_YC_FRC_CFG4;
typedef union  T32SCL1D_YC_FRC_CFG5
{ UNSG32 u32;
	struct w32SCL1D_YC_FRC_CFG5;
} T32SCL1D_YC_FRC_CFG5;
typedef union  T32SCL1D_YC_FRC_CFG6
{ UNSG32 u32;
	struct w32SCL1D_YC_FRC_CFG6;
} T32SCL1D_YC_FRC_CFG6;
typedef union  TSCL1D_YC_FRC_CFG0
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_FRC_CFG0;
	};
} TSCL1D_YC_FRC_CFG0;
typedef union  TSCL1D_YC_FRC_CFG1
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_FRC_CFG1;
	};
} TSCL1D_YC_FRC_CFG1;
typedef union  TSCL1D_YC_FRC_CFG2
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_FRC_CFG2;
	};
} TSCL1D_YC_FRC_CFG2;
typedef union  TSCL1D_YC_FRC_CFG3
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_FRC_CFG3;
	};
} TSCL1D_YC_FRC_CFG3;
typedef union  TSCL1D_YC_FRC_CFG4
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_FRC_CFG4;
	};
} TSCL1D_YC_FRC_CFG4;
typedef union  TSCL1D_YC_FRC_CFG5
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_FRC_CFG5;
	};
} TSCL1D_YC_FRC_CFG5;
typedef union  TSCL1D_YC_FRC_CFG6
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_FRC_CFG6;
	};
} TSCL1D_YC_FRC_CFG6;
SIGN32 SCL1D_YC_FRC_drvrd(SIE_SCL1D_YC_FRC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 SCL1D_YC_FRC_drvwr(SIE_SCL1D_YC_FRC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void SCL1D_YC_FRC_reset(SIE_SCL1D_YC_FRC *p);
SIGN32 SCL1D_YC_FRC_cmp  (SIE_SCL1D_YC_FRC *p, SIE_SCL1D_YC_FRC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define SCL1D_YC_FRC_check(p,pie,pfx,hLOG) SCL1D_YC_FRC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define SCL1D_YC_FRC_print(p,    pfx,hLOG) SCL1D_YC_FRC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DITHER
#define h_DITHER (){}
#define     RA_DITHER_CFG0                                 0x0000
typedef struct SIE_DITHER {
#define     w32DITHER_CFG0                                 {\
	UNSG32 uCFG0_mode                                  :  2;\
	UNSG32 uCFG0_ctrl                                  :  2;\
	UNSG32 uCFG0_ycmode                                :  1;\
	UNSG32 RSVDx0_b5                                   : 27;\
}
union { UNSG32 u32DITHER_CFG0;
	struct w32DITHER_CFG0;
};
} SIE_DITHER;
typedef union  T32DITHER_CFG0
{ UNSG32 u32;
	struct w32DITHER_CFG0;
} T32DITHER_CFG0;
typedef union  TDITHER_CFG0
{ UNSG32 u32[1];
	struct {
		struct w32DITHER_CFG0;
	};
} TDITHER_CFG0;
SIGN32 DITHER_drvrd(SIE_DITHER *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 DITHER_drvwr(SIE_DITHER *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void DITHER_reset(SIE_DITHER *p);
SIGN32 DITHER_cmp  (SIE_DITHER *p, SIE_DITHER *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define DITHER_check(p,pie,pfx,hLOG) DITHER_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define DITHER_print(p,    pfx,hLOG) DITHER_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SCL1D_YC_HSQ
#define h_SCL1D_YC_HSQ (){}
#define     RA_SCL1D_YC_HSQ_CFG0                           0x0000
#define     RA_SCL1D_YC_HSQ_CFG1                           0x0004
#define     RA_SCL1D_YC_HSQ_CFG2                           0x0008
#define     RA_SCL1D_YC_HSQ_CFG3                           0x000C
#define     RA_SCL1D_YC_HSQ_CFG4                           0x0010
#define     RA_SCL1D_YC_HSQ_CFG5                           0x0014
#define     RA_SCL1D_YC_HSQ_CFG6                           0x0018
#define     RA_SCL1D_YC_HSQ_CFG7                           0x001C
#define     RA_SCL1D_YC_HSQ_CFG8                           0x0020
#define     RA_SCL1D_YC_HSQ_CFG9                           0x0024
#define     RA_SCL1D_YC_HSQ_CFG10                          0x0028
#define     RA_SCL1D_YC_HSQ_CFG11                          0x002C
typedef struct SIE_SCL1D_YC_HSQ {
#define     w32SCL1D_YC_HSQ_CFG0                           {\
	UNSG32 uCFG0_hsq_mode                              :  1;\
	UNSG32 RSVDx0_b1                                   : 31;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG0;
	struct w32SCL1D_YC_HSQ_CFG0;
};
#define     w32SCL1D_YC_HSQ_CFG1                           {\
	UNSG32 uCFG1_hsq_indx_0                            : 13;\
	UNSG32 uCFG1_rsv0                                  :  3;\
	UNSG32 uCFG1_hsq_indx_1                            : 13;\
	UNSG32 uCFG1_rsv1                                  :  3;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG1;
	struct w32SCL1D_YC_HSQ_CFG1;
};
#define     w32SCL1D_YC_HSQ_CFG2                           {\
	UNSG32 uCFG2_hsq_indx_2                            : 13;\
	UNSG32 uCFG2_rsv2                                  :  3;\
	UNSG32 uCFG2_hsq_indx_3                            : 13;\
	UNSG32 uCFG2_rsv3                                  :  3;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG2;
	struct w32SCL1D_YC_HSQ_CFG2;
};
#define     w32SCL1D_YC_HSQ_CFG3                           {\
	UNSG32 uCFG3_hsq_done                              :  1;\
	UNSG32 RSVDxC_b1                                   : 31;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG3;
	struct w32SCL1D_YC_HSQ_CFG3;
};
#define     w32SCL1D_YC_HSQ_CFG4                           {\
	UNSG32 uCFG4_hsq_frph_0                            : 20;\
	UNSG32 RSVDx10_b20                                 : 12;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG4;
	struct w32SCL1D_YC_HSQ_CFG4;
};
#define     w32SCL1D_YC_HSQ_CFG5                           {\
	UNSG32 uCFG5_hsq_frph_1                            : 20;\
	UNSG32 RSVDx14_b20                                 : 12;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG5;
	struct w32SCL1D_YC_HSQ_CFG5;
};
#define     w32SCL1D_YC_HSQ_CFG6                           {\
	UNSG32 uCFG6_hsq_frph_2                            : 20;\
	UNSG32 RSVDx18_b20                                 : 12;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG6;
	struct w32SCL1D_YC_HSQ_CFG6;
};
#define     w32SCL1D_YC_HSQ_CFG7                           {\
	UNSG32 uCFG7_hsq_frph_3                            : 20;\
	UNSG32 RSVDx1C_b20                                 : 12;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG7;
	struct w32SCL1D_YC_HSQ_CFG7;
};
#define     w32SCL1D_YC_HSQ_CFG8                           {\
	UNSG32 uCFG8_hsq_cidx_0                            : 13;\
	UNSG32 RSVDx20_b13                                 : 19;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG8;
	struct w32SCL1D_YC_HSQ_CFG8;
};
#define     w32SCL1D_YC_HSQ_CFG9                           {\
	UNSG32 uCFG9_hsq_cidx_1                            : 13;\
	UNSG32 RSVDx24_b13                                 : 19;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG9;
	struct w32SCL1D_YC_HSQ_CFG9;
};
#define     w32SCL1D_YC_HSQ_CFG10                          {\
	UNSG32 uCFG10_hsq_cidx_2                           : 13;\
	UNSG32 RSVDx28_b13                                 : 19;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG10;
	struct w32SCL1D_YC_HSQ_CFG10;
};
#define     w32SCL1D_YC_HSQ_CFG11                          {\
	UNSG32 uCFG11_hsq_cidx_3                           : 13;\
	UNSG32 RSVDx2C_b13                                 : 19;\
}
union { UNSG32 u32SCL1D_YC_HSQ_CFG11;
	struct w32SCL1D_YC_HSQ_CFG11;
};
} SIE_SCL1D_YC_HSQ;
typedef union  T32SCL1D_YC_HSQ_CFG0
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG0;
} T32SCL1D_YC_HSQ_CFG0;
typedef union  T32SCL1D_YC_HSQ_CFG1
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG1;
} T32SCL1D_YC_HSQ_CFG1;
typedef union  T32SCL1D_YC_HSQ_CFG2
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG2;
} T32SCL1D_YC_HSQ_CFG2;
typedef union  T32SCL1D_YC_HSQ_CFG3
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG3;
} T32SCL1D_YC_HSQ_CFG3;
typedef union  T32SCL1D_YC_HSQ_CFG4
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG4;
} T32SCL1D_YC_HSQ_CFG4;
typedef union  T32SCL1D_YC_HSQ_CFG5
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG5;
} T32SCL1D_YC_HSQ_CFG5;
typedef union  T32SCL1D_YC_HSQ_CFG6
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG6;
} T32SCL1D_YC_HSQ_CFG6;
typedef union  T32SCL1D_YC_HSQ_CFG7
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG7;
} T32SCL1D_YC_HSQ_CFG7;
typedef union  T32SCL1D_YC_HSQ_CFG8
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG8;
} T32SCL1D_YC_HSQ_CFG8;
typedef union  T32SCL1D_YC_HSQ_CFG9
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG9;
} T32SCL1D_YC_HSQ_CFG9;
typedef union  T32SCL1D_YC_HSQ_CFG10
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG10;
} T32SCL1D_YC_HSQ_CFG10;
typedef union  T32SCL1D_YC_HSQ_CFG11
{ UNSG32 u32;
	struct w32SCL1D_YC_HSQ_CFG11;
} T32SCL1D_YC_HSQ_CFG11;
typedef union  TSCL1D_YC_HSQ_CFG0
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG0;
	};
} TSCL1D_YC_HSQ_CFG0;
typedef union  TSCL1D_YC_HSQ_CFG1
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG1;
	};
} TSCL1D_YC_HSQ_CFG1;
typedef union  TSCL1D_YC_HSQ_CFG2
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG2;
	};
} TSCL1D_YC_HSQ_CFG2;
typedef union  TSCL1D_YC_HSQ_CFG3
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG3;
	};
} TSCL1D_YC_HSQ_CFG3;
typedef union  TSCL1D_YC_HSQ_CFG4
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG4;
	};
} TSCL1D_YC_HSQ_CFG4;
typedef union  TSCL1D_YC_HSQ_CFG5
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG5;
	};
} TSCL1D_YC_HSQ_CFG5;
typedef union  TSCL1D_YC_HSQ_CFG6
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG6;
	};
} TSCL1D_YC_HSQ_CFG6;
typedef union  TSCL1D_YC_HSQ_CFG7
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG7;
	};
} TSCL1D_YC_HSQ_CFG7;
typedef union  TSCL1D_YC_HSQ_CFG8
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG8;
	};
} TSCL1D_YC_HSQ_CFG8;
typedef union  TSCL1D_YC_HSQ_CFG9
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG9;
	};
} TSCL1D_YC_HSQ_CFG9;
typedef union  TSCL1D_YC_HSQ_CFG10
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG10;
	};
} TSCL1D_YC_HSQ_CFG10;
typedef union  TSCL1D_YC_HSQ_CFG11
{ UNSG32 u32[1];
	struct {
		struct w32SCL1D_YC_HSQ_CFG11;
	};
} TSCL1D_YC_HSQ_CFG11;
SIGN32 SCL1D_YC_HSQ_drvrd(SIE_SCL1D_YC_HSQ *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 SCL1D_YC_HSQ_drvwr(SIE_SCL1D_YC_HSQ *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void SCL1D_YC_HSQ_reset(SIE_SCL1D_YC_HSQ *p);
SIGN32 SCL1D_YC_HSQ_cmp  (SIE_SCL1D_YC_HSQ *p, SIE_SCL1D_YC_HSQ *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define SCL1D_YC_HSQ_check(p,pie,pfx,hLOG) SCL1D_YC_HSQ_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define SCL1D_YC_HSQ_print(p,    pfx,hLOG) SCL1D_YC_HSQ_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_OVPSCL
#define h_OVPSCL (){}
#define     RA_OVPSCL_CTRL                                 0x0000
#define     RA_OVPSCL_scl1d_Inpix                          0x0004
#define     RA_OVPSCL_scl1d_Outpix                         0x0008
#define     RA_OVPSCL_SCL1D_YC                             0x000C
#define     RA_OVPSCL_SCL1D_YC_COEFF                       0x0068
#define     RA_OVPSCL_SCL1D_YC_FRC                         0x007C
#define     RA_OVPSCL_DITHER                               0x0098
typedef struct SIE_OVPSCL {
#define     w32OVPSCL_CTRL                                 {\
	UNSG32 uCTRL_SCL_CLKEN_CTRL                        :  1;\
	UNSG32 uCTRL_sb_clkg_en                            :  1;\
	UNSG32 uCTRL_cfg_rst                               :  1;\
	UNSG32 uCTRL_scl1d_sw_rst                          :  1;\
	UNSG32 uCTRL_scl_rst_en0                           :  1;\
	UNSG32 uCTRL_scl_rst_en1                           :  1;\
	UNSG32 uCTRL_scl_rst_en2                           :  1;\
	UNSG32 uCTRL_scl1d_YC_bypass                       :  1;\
	UNSG32 uCTRL_dither_bypass                         :  1;\
	UNSG32 RSVDx0_b9                                   : 23;\
}
union { UNSG32 u32OVPSCL_CTRL;
	struct w32OVPSCL_CTRL;
};
#define     w32OVPSCL_scl1d_Inpix                          {\
	UNSG32 uscl1d_Inpix_tot                            : 32;\
}
union { UNSG32 u32OVPSCL_scl1d_Inpix;
	struct w32OVPSCL_scl1d_Inpix;
};
#define     w32OVPSCL_scl1d_Outpix                         {\
	UNSG32 uscl1d_Outpix_tot                           : 32;\
}
union { UNSG32 u32OVPSCL_scl1d_Outpix;
	struct w32OVPSCL_scl1d_Outpix;
};
SIE_SCL1D_YC                                     ie_SCL1D_YC;
SIE_SCL1D_YC_COEFF                               ie_SCL1D_YC_COEFF;
SIE_SCL1D_YC_FRC                                 ie_SCL1D_YC_FRC;
SIE_DITHER                                       ie_DITHER;
} SIE_OVPSCL;
typedef union  T32OVPSCL_CTRL
{ UNSG32 u32;
	struct w32OVPSCL_CTRL;
} T32OVPSCL_CTRL;
typedef union  T32OVPSCL_scl1d_Inpix
{ UNSG32 u32;
	struct w32OVPSCL_scl1d_Inpix;
} T32OVPSCL_scl1d_Inpix;
typedef union  T32OVPSCL_scl1d_Outpix
{ UNSG32 u32;
	struct w32OVPSCL_scl1d_Outpix;
} T32OVPSCL_scl1d_Outpix;
typedef union  TOVPSCL_CTRL
{ UNSG32 u32[1];
	struct {
		struct w32OVPSCL_CTRL;
	};
} TOVPSCL_CTRL;
typedef union  TOVPSCL_scl1d_Inpix
{ UNSG32 u32[1];
	struct {
		struct w32OVPSCL_scl1d_Inpix;
	};
} TOVPSCL_scl1d_Inpix;
typedef union  TOVPSCL_scl1d_Outpix
{ UNSG32 u32[1];
	struct {
		struct w32OVPSCL_scl1d_Outpix;
	};
} TOVPSCL_scl1d_Outpix;
SIGN32 OVPSCL_drvrd(SIE_OVPSCL *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 OVPSCL_drvwr(SIE_OVPSCL *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void OVPSCL_reset(SIE_OVPSCL *p);
SIGN32 OVPSCL_cmp  (SIE_OVPSCL *p, SIE_OVPSCL *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define OVPSCL_check(p,pie,pfx,hLOG) OVPSCL_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define OVPSCL_print(p,    pfx,hLOG) OVPSCL_cmp(p,0,  pfx,(void*)(hLOG),0,0)
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
#ifndef h_OVPDNSCLWRAP
#define h_OVPDNSCLWRAP (){}
#define     RA_OVPDNSCLWRAP_CFG0                           0x0000
#define     RA_OVPDNSCLWRAP_CFG1                           0x0004
#define     RA_OVPDNSCLWRAP_CTRL                           0x0008
#define     RA_OVPDNSCLWRAP_CFG2                           0x000C
#define     RA_OVPDNSCLWRAP_CTRL0                          0x0010
#define     RA_OVPDNSCLWRAP_CTRL1                          0x0014
#define     RA_OVPDNSCLWRAP_CTRL2                          0x0018
#define     RA_OVPDNSCLWRAP_CTRL3                          0x001C
#define     RA_OVPDNSCLWRAP_CTRL4                          0x0020
#define     RA_OVPDNSCLWRAP_CFG8                           0x0024
#define     RA_OVPDNSCLWRAP_CFG9                           0x0028
#define     RA_OVPDNSCLWRAP_sclOFIFO_YInpix                0x002C
#define     RA_OVPDNSCLWRAP_sclOFIFO_UVInpix               0x0030
#define     RA_OVPDNSCLWRAP_tg_ctrl                        0x0034
#define     RA_OVPDNSCLWRAP_STATUS0                        0x0038
#define     RA_OVPDNSCLWRAP_INTR_EN                        0x003C
#define     RA_OVPDNSCLWRAP_STATUS1                        0x0040
#define     RA_OVPDNSCLWRAP_STATUS2                        0x0044
#define     RA_OVPDNSCLWRAP_OVPSCL_WR0                     0x0048
#define     RA_OVPDNSCLWRAP_OVPSCL_WR1                     0x0058
#define     RA_OVPDNSCLWRAP_OVPSCL_Y                       0x0068
#define     RA_OVPDNSCLWRAP_OVPSCL_UV                      0x0104
#define     RA_OVPDNSCLWRAP_OVPSCL_OTG_Y                   0x01A0
#define     RA_OVPDNSCLWRAP_OVPSCL_OTG_UV                  0x01E0
#define     RA_OVPDNSCLWRAP_BITMAP32                       0x0220
#define     RA_OVPDNSCLWRAP_OTG                            0x0238
#define     RA_OVPDNSCLWRAP_INPTG_OVPDNSCL3                0x0278
#define     RA_OVPDNSCLWRAP_INPTG_OVPDNSCL3_UV             0x02C0
#define     RA_OVPDNSCLWRAP_OVPDNSCL_DITHER                0x0308
#define     RA_OVPDNSCLWRAP_OVPDNSCL3_CSC                  0x030C
#define     RA_OVPDNSCLWRAP_SRAMPWR_Y                      0x0348
#define     RA_OVPDNSCLWRAP_SRAMPWR_UV                     0x034C
#define     RA_OVPDNSCLWRAP_SRAMPWR_Y_HLUT                 0x0350
#define     RA_OVPDNSCLWRAP_SRAMPWR_Y_VLUT                 0x0354
#define     RA_OVPDNSCLWRAP_SRAMPWR_C_HLUT                 0x0358
#define     RA_OVPDNSCLWRAP_SRAMPWR_C_VLUT                 0x035C
typedef struct SIE_OVPDNSCLWRAP {
#define     w32OVPDNSCLWRAP_CFG0                           {\
	UNSG32 uCFG0_bitmap30_en                           :  1;\
	UNSG32 uCFG0_tgEn_rstEn                            :  1;\
	UNSG32 uCFG0_tgEof_en                              :  1;\
	UNSG32 uCFG0_isp_fifo_clear0                       :  1;\
	UNSG32 uCFG0_tg_start                              :  1;\
	UNSG32 uCFG0_tg_clear                              :  1;\
	UNSG32 uCFG0_uv_mask_disable                       :  1;\
	UNSG32 uCFG0_uv_mask_val0                          :  1;\
	UNSG32 uCFG0_uv_mask_val1                          :  1;\
	UNSG32 uCFG0_clken_ctrl0                           :  1;\
	UNSG32 uCFG0_clken_ctrl1                           :  1;\
	UNSG32 uCFG0_clken_ctrl2                           :  1;\
	UNSG32 uCFG0_fifo_full_ctrl1                       :  1;\
	UNSG32 uCFG0_fifo_full_ctrl2                       :  1;\
	UNSG32 uCFG0_fifo_full_ctrlEn                      :  1;\
	UNSG32 uCFG0_read_Sel                              :  1;\
	UNSG32 uCFG0_rd_initval0                           :  1;\
	UNSG32 uCFG0_rd_initval1                           :  1;\
	UNSG32 uCFG0_rd_mask_sftrst                        :  1;\
	UNSG32 uCFG0_hde_mask_initval0                     :  1;\
	UNSG32 uCFG0_hde_mask_initval1                     :  1;\
	UNSG32 uCFG0_crop_de_en                            :  1;\
	UNSG32 uCFG0_idata_Sel                             :  3;\
	UNSG32 uCFG0_vsync_cnt_val                         :  5;\
	UNSG32 uCFG0_start_mask_reset                      :  1;\
	UNSG32 RSVDx0_b31                                  :  1;\
}
union { UNSG32 u32OVPDNSCLWRAP_CFG0;
	struct w32OVPDNSCLWRAP_CFG0;
};
#define     w32OVPDNSCLWRAP_CFG1                           {\
	UNSG32 uCFG1_Inpix_tot                             : 32;\
}
union { UNSG32 u32OVPDNSCLWRAP_CFG1;
	struct w32OVPDNSCLWRAP_CFG1;
};
#define     w32OVPDNSCLWRAP_CTRL                           {\
	UNSG32 uCTRL_sclOfifo_Y_stsCtrl                    :  1;\
	UNSG32 uCTRL_ofifo_sts0_ctrl                       :  1;\
	UNSG32 uCTRL_sclOfifo_UV_stsCtrl                   :  1;\
	UNSG32 uCTRL_ofifo_sts1_ctrl                       :  1;\
	UNSG32 uCTRL_sclOutrdy_sts0_en                     :  1;\
	UNSG32 uCTRL_sclOutrdy_sts1_en                     :  1;\
	UNSG32 uCTRL_luma_data_sel                         :  2;\
	UNSG32 uCTRL_chroma_data_sel                       :  1;\
	UNSG32 uCTRL_420sp_wrbk                            :  1;\
	UNSG32 uCTRL_hde_initval0                          :  1;\
	UNSG32 uCTRL_hde_initval1                          :  1;\
	UNSG32 uCTRL_ovpscl_ictrl_sel                      :  3;\
	UNSG32 uCTRL_idata_frmt_ctrl                       :  1;\
	UNSG32 uCTRL_clk_en_sel                            :  2;\
	UNSG32 uCTRL_hde_msk_en                            :  1;\
	UNSG32 uCTRL_ngptv_bypass                          :  1;\
	UNSG32 uCTRL_ngptv_scl_sel                         :  1;\
	UNSG32 uCTRL_bitmap128_Rd0_en                      :  1;\
	UNSG32 uCTRL_bitmap128_Rd1_en                      :  1;\
	UNSG32 uCTRL_rdsel_420SP                           :  1;\
	UNSG32 uCTRL_rd_mask_sftrst                        :  1;\
	UNSG32 uCTRL_clken_ctrl0                           :  1;\
	UNSG32 uCTRL_clken_ctrl1                           :  1;\
	UNSG32 uCTRL_ups420_idatactrl                      :  1;\
	UNSG32 uCTRL_bitMap32_en                           :  1;\
	UNSG32 uCTRL_rd_initval0                           :  1;\
	UNSG32 uCTRL_rd_initval1                           :  1;\
	UNSG32 RSVDx8_b31                                  :  1;\
}
union { UNSG32 u32OVPDNSCLWRAP_CTRL;
	struct w32OVPDNSCLWRAP_CTRL;
};
#define     w32OVPDNSCLWRAP_CFG2                           {\
	UNSG32 uCFG2_ififo_sts0_ctrl                       :  1;\
	UNSG32 uCFG2_ififo_sts1_ctrl                       :  1;\
	UNSG32 uCFG2_fifo_full_ctrlEn                      :  1;\
	UNSG32 uCFG2_UVPack_sel                            :  1;\
	UNSG32 uCFG2_uvPack_sftrst                         :  1;\
	UNSG32 uCFG2_swap_regs                             :  1;\
	UNSG32 uCFG2_ovpdnsclwrap_rst                      :  1;\
	UNSG32 uCFG2_ovpdnsclwrap_Y_rst                    :  1;\
	UNSG32 uCFG2_ovpdnsclwrap_UV_rst                   :  1;\
	UNSG32 uCFG2_ovpdnsclwrap_Y_memlb_rst              :  1;\
	UNSG32 uCFG2_ovpdnsclwrap_UV_memlb_rst             :  1;\
	UNSG32 uCFG2_ovpdnsclwrap_Y_sbrst                  :  1;\
	UNSG32 uCFG2_ovpdnsclwrap_UV_sbrst                 :  1;\
	UNSG32 RSVDxC_b13                                  : 19;\
}
union { UNSG32 u32OVPDNSCLWRAP_CFG2;
	struct w32OVPDNSCLWRAP_CFG2;
};
#define     w32OVPDNSCLWRAP_CTRL0                          {\
	UNSG32 uCTRL0_ClientR0_start                       :  1;\
	UNSG32 uCTRL0_ClientR1_start                       :  1;\
	UNSG32 uCTRL0_ClientR0_clear                       :  1;\
	UNSG32 uCTRL0_ClientR1_clear                       :  1;\
	UNSG32 RSVDx10_b4                                  : 28;\
}
union { UNSG32 u32OVPDNSCLWRAP_CTRL0;
	struct w32OVPDNSCLWRAP_CTRL0;
};
#define     w32OVPDNSCLWRAP_CTRL1                          {\
	UNSG32 uCTRL1_packSel_R0                           :  3;\
	UNSG32 uCTRL1_pixlineTot_R0                        : 13;\
	UNSG32 uCTRL1_nonStdResEn_R0                       :  1;\
	UNSG32 uCTRL1_flushCnt_R0                          :  4;\
	UNSG32 uCTRL1_ovp_flush_sel                        :  1;\
	UNSG32 uCTRL1_ovp_intr_sel                         :  1;\
	UNSG32 uCTRL1_byteoffsetRGB                        :  2;\
	UNSG32 RSVDx14_b25                                 :  7;\
}
union { UNSG32 u32OVPDNSCLWRAP_CTRL1;
	struct w32OVPDNSCLWRAP_CTRL1;
};
#define     w32OVPDNSCLWRAP_CTRL2                          {\
	UNSG32 uCTRL2_packSel_R1                           :  2;\
	UNSG32 uCTRL2_pixlineTot_R1                        : 13;\
	UNSG32 uCTRL2_nonStdResEn_R1                       :  1;\
	UNSG32 uCTRL2_flushCnt_R1                          :  4;\
	UNSG32 RSVDx18_b20                                 : 12;\
}
union { UNSG32 u32OVPDNSCLWRAP_CTRL2;
	struct w32OVPDNSCLWRAP_CTRL2;
};
#define     w32OVPDNSCLWRAP_CTRL3                          {\
	UNSG32 uCTRL3_wordTot_R0                           : 32;\
}
union { UNSG32 u32OVPDNSCLWRAP_CTRL3;
	struct w32OVPDNSCLWRAP_CTRL3;
};
#define     w32OVPDNSCLWRAP_CTRL4                          {\
	UNSG32 uCTRL4_wordTot_R1                           : 32;\
}
union { UNSG32 u32OVPDNSCLWRAP_CTRL4;
	struct w32OVPDNSCLWRAP_CTRL4;
};
#define     w32OVPDNSCLWRAP_CFG8                           {\
	UNSG32 uCFG8_UV_swap_regs                          :  1;\
	UNSG32 uCFG8_scl1d_bypass                          :  1;\
	UNSG32 uCFG8_uvrd_en_init0                         :  1;\
	UNSG32 uCFG8_uvrd_en_init1                         :  1;\
	UNSG32 uCFG8_uvrd_en_mask                          :  1;\
	UNSG32 uCFG8_uv422_444_sel                         :  1;\
	UNSG32 uCFG8_scl_OFifoY_aEmpCtrl                   :  1;\
	UNSG32 uCFG8_scl_OFifoUV_aEmpCtrl                  :  1;\
	UNSG32 uCFG8_sclOutrdy_sts0_en                     :  1;\
	UNSG32 uCFG8_sclOutrdy_sts1_en                     :  1;\
	UNSG32 uCFG8_csc_bypass                            :  1;\
	UNSG32 uCFG8_dither_bypass                         :  1;\
	UNSG32 uCFG8_sp_wrbk_8b                            :  1;\
	UNSG32 uCFG8_pack_wrbk                             :  2;\
	UNSG32 uCFG8_444_sel                               :  1;\
	UNSG32 uCFG8_scl_OFifoY_aFullCtrl                  :  1;\
	UNSG32 uCFG8_scl_OFifoUV_aFullCtrl                 :  1;\
	UNSG32 RSVDx24_b18                                 : 14;\
}
union { UNSG32 u32OVPDNSCLWRAP_CFG8;
	struct w32OVPDNSCLWRAP_CFG8;
};
#define     w32OVPDNSCLWRAP_CFG9                           {\
	UNSG32 uCFG9_tg_start                              :  1;\
	UNSG32 uCFG9_tg_clear                              :  1;\
	UNSG32 uCFG9_clken_ctrl0_UV                        :  1;\
	UNSG32 uCFG9_clken_ctrl1_UV                        :  1;\
	UNSG32 uCFG9_clken_ctrl2_UV                        :  1;\
	UNSG32 uCFG9_fifo_full_ctrl1_UV                    :  1;\
	UNSG32 uCFG9_fifo_full_ctrl2_UV                    :  1;\
	UNSG32 uCFG9_scl_OFifoY_aFullCtrl_UV               :  1;\
	UNSG32 uCFG9_scl_OFifoUV_aFullCtrl_UV              :  1;\
	UNSG32 uCFG9_tg_sel                                :  1;\
	UNSG32 uCFG9_crop_de_en                            :  1;\
	UNSG32 uCFG9_idata_sel                             :  2;\
	UNSG32 RSVDx28_b13                                 : 19;\
}
union { UNSG32 u32OVPDNSCLWRAP_CFG9;
	struct w32OVPDNSCLWRAP_CFG9;
};
#define     w32OVPDNSCLWRAP_sclOFIFO_YInpix                {\
	UNSG32 usclOFIFO_YInpix_tot                        : 32;\
}
union { UNSG32 u32OVPDNSCLWRAP_sclOFIFO_YInpix;
	struct w32OVPDNSCLWRAP_sclOFIFO_YInpix;
};
#define     w32OVPDNSCLWRAP_sclOFIFO_UVInpix               {\
	UNSG32 usclOFIFO_UVInpix_tot                       : 32;\
}
union { UNSG32 u32OVPDNSCLWRAP_sclOFIFO_UVInpix;
	struct w32OVPDNSCLWRAP_sclOFIFO_UVInpix;
};
#define     w32OVPDNSCLWRAP_tg_ctrl                        {\
	UNSG32 utg_ctrl_start                              :  1;\
	UNSG32 utg_ctrl_clear                              :  1;\
	UNSG32 utg_ctrl_Y_otg_start                        :  1;\
	UNSG32 utg_ctrl_Y_otg_clear                        :  1;\
	UNSG32 utg_ctrl_wr0_start                          :  1;\
	UNSG32 utg_ctrl_wr1_start                          :  1;\
	UNSG32 utg_ctrl_wr0_clear                          :  1;\
	UNSG32 utg_ctrl_wr1_clear                          :  1;\
	UNSG32 utg_ctrl_UV_otg_start                       :  1;\
	UNSG32 utg_ctrl_UV_otg_clear                       :  1;\
	UNSG32 utg_ctrl_420sp_rd_start                     :  1;\
	UNSG32 utg_ctrl_ovp_flush                          :  1;\
	UNSG32 utg_ctrl_Y_Ofifo_clear                      :  1;\
	UNSG32 utg_ctrl_UV_Ofifo_clear                     :  1;\
	UNSG32 utg_ctrl_otg_start                          :  1;\
	UNSG32 utg_ctrl_otg_clear                          :  1;\
	UNSG32 RSVDx34_b16                                 : 16;\
}
union { UNSG32 u32OVPDNSCLWRAP_tg_ctrl;
	struct w32OVPDNSCLWRAP_tg_ctrl;
};
#define     w32OVPDNSCLWRAP_STATUS0                        {\
	UNSG32 uSTATUS0_OVPDNSCL_Y_oTg                     : 16;\
	UNSG32 uSTATUS0_OVPDNSCL_UV_oTg                    : 16;\
}
union { UNSG32 u32OVPDNSCLWRAP_STATUS0;
	struct w32OVPDNSCLWRAP_STATUS0;
};
#define     w32OVPDNSCLWRAP_INTR_EN                        {\
	UNSG32 uINTR_EN_OVPDNSCL_Y_EOF                     :  1;\
	UNSG32 uINTR_EN_OVPDNSCL_UV_EOF                    :  1;\
	UNSG32 uINTR_EN_OVPDNSCL_otg_EOF                   :  1;\
	UNSG32 RSVDx3C_b3                                  : 29;\
}
union { UNSG32 u32OVPDNSCLWRAP_INTR_EN;
	struct w32OVPDNSCLWRAP_INTR_EN;
};
#define     w32OVPDNSCLWRAP_STATUS1                        {\
	UNSG32 uSTATUS1_OVPDNSCL_FeTg                      : 16;\
	UNSG32 uSTATUS1_OVPDNSCL_oTg                       : 16;\
}
union { UNSG32 u32OVPDNSCLWRAP_STATUS1;
	struct w32OVPDNSCLWRAP_STATUS1;
};
#define     w32OVPDNSCLWRAP_STATUS2                        {\
	UNSG32 uSTATUS2_OVPDNSCL_FeTg_UV                   : 16;\
	UNSG32 uSTATUS2_ISP_async_FIFO_OF                  :  1;\
	UNSG32 uSTATUS2_ISP_async_FIFO_UF                  :  1;\
	UNSG32 RSVDx44_b18                                 : 14;\
}
union { UNSG32 u32OVPDNSCLWRAP_STATUS2;
	struct w32OVPDNSCLWRAP_STATUS2;
};
SIE_WriteClient                                  ie_OVPSCL_WR0;
SIE_WriteClient                                  ie_OVPSCL_WR1;
SIE_OVPSCL                                       ie_OVPSCL_Y;
SIE_OVPSCL                                       ie_OVPSCL_UV;
SIE_TG                                           ie_OVPSCL_OTG_Y;
SIE_TG                                           ie_OVPSCL_OTG_UV;
SIE_BITMAP32                                     ie_BITMAP32;
SIE_TG                                           ie_OTG;
SIE_TG_MAIN                                      ie_INPTG_OVPDNSCL3;
SIE_TG_MAIN                                      ie_INPTG_OVPDNSCL3_UV;
SIE_DITHER                                       ie_OVPDNSCL_DITHER;
SIE_CSC_C17O24                                   ie_OVPDNSCL3_CSC;
SIE_SRAMPWR                                      ie_SRAMPWR_Y;
SIE_SRAMPWR                                      ie_SRAMPWR_UV;
SIE_SRAMPWR                                      ie_SRAMPWR_Y_HLUT;
SIE_SRAMPWR                                      ie_SRAMPWR_Y_VLUT;
SIE_SRAMPWR                                      ie_SRAMPWR_C_HLUT;
SIE_SRAMPWR                                      ie_SRAMPWR_C_VLUT;
} SIE_OVPDNSCLWRAP;
typedef union  T32OVPDNSCLWRAP_CFG0
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CFG0;
} T32OVPDNSCLWRAP_CFG0;
typedef union  T32OVPDNSCLWRAP_CFG1
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CFG1;
} T32OVPDNSCLWRAP_CFG1;
typedef union  T32OVPDNSCLWRAP_CTRL
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CTRL;
} T32OVPDNSCLWRAP_CTRL;
typedef union  T32OVPDNSCLWRAP_CFG2
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CFG2;
} T32OVPDNSCLWRAP_CFG2;
typedef union  T32OVPDNSCLWRAP_CTRL0
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CTRL0;
} T32OVPDNSCLWRAP_CTRL0;
typedef union  T32OVPDNSCLWRAP_CTRL1
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CTRL1;
} T32OVPDNSCLWRAP_CTRL1;
typedef union  T32OVPDNSCLWRAP_CTRL2
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CTRL2;
} T32OVPDNSCLWRAP_CTRL2;
typedef union  T32OVPDNSCLWRAP_CTRL3
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CTRL3;
} T32OVPDNSCLWRAP_CTRL3;
typedef union  T32OVPDNSCLWRAP_CTRL4
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CTRL4;
} T32OVPDNSCLWRAP_CTRL4;
typedef union  T32OVPDNSCLWRAP_CFG8
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CFG8;
} T32OVPDNSCLWRAP_CFG8;
typedef union  T32OVPDNSCLWRAP_CFG9
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_CFG9;
} T32OVPDNSCLWRAP_CFG9;
typedef union  T32OVPDNSCLWRAP_sclOFIFO_YInpix
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_sclOFIFO_YInpix;
} T32OVPDNSCLWRAP_sclOFIFO_YInpix;
typedef union  T32OVPDNSCLWRAP_sclOFIFO_UVInpix
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_sclOFIFO_UVInpix;
} T32OVPDNSCLWRAP_sclOFIFO_UVInpix;
typedef union  T32OVPDNSCLWRAP_tg_ctrl
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_tg_ctrl;
} T32OVPDNSCLWRAP_tg_ctrl;
typedef union  T32OVPDNSCLWRAP_STATUS0
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_STATUS0;
} T32OVPDNSCLWRAP_STATUS0;
typedef union  T32OVPDNSCLWRAP_INTR_EN
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_INTR_EN;
} T32OVPDNSCLWRAP_INTR_EN;
typedef union  T32OVPDNSCLWRAP_STATUS1
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_STATUS1;
} T32OVPDNSCLWRAP_STATUS1;
typedef union  T32OVPDNSCLWRAP_STATUS2
{ UNSG32 u32;
	struct w32OVPDNSCLWRAP_STATUS2;
} T32OVPDNSCLWRAP_STATUS2;
typedef union  TOVPDNSCLWRAP_CFG0
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CFG0;
	};
} TOVPDNSCLWRAP_CFG0;
typedef union  TOVPDNSCLWRAP_CFG1
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CFG1;
	};
} TOVPDNSCLWRAP_CFG1;
typedef union  TOVPDNSCLWRAP_CTRL
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CTRL;
	};
} TOVPDNSCLWRAP_CTRL;
typedef union  TOVPDNSCLWRAP_CFG2
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CFG2;
	};
} TOVPDNSCLWRAP_CFG2;
typedef union  TOVPDNSCLWRAP_CTRL0
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CTRL0;
	};
} TOVPDNSCLWRAP_CTRL0;
typedef union  TOVPDNSCLWRAP_CTRL1
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CTRL1;
	};
} TOVPDNSCLWRAP_CTRL1;
typedef union  TOVPDNSCLWRAP_CTRL2
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CTRL2;
	};
} TOVPDNSCLWRAP_CTRL2;
typedef union  TOVPDNSCLWRAP_CTRL3
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CTRL3;
	};
} TOVPDNSCLWRAP_CTRL3;
typedef union  TOVPDNSCLWRAP_CTRL4
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CTRL4;
	};
} TOVPDNSCLWRAP_CTRL4;
typedef union  TOVPDNSCLWRAP_CFG8
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CFG8;
	};
} TOVPDNSCLWRAP_CFG8;
typedef union  TOVPDNSCLWRAP_CFG9
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_CFG9;
	};
} TOVPDNSCLWRAP_CFG9;
typedef union  TOVPDNSCLWRAP_sclOFIFO_YInpix
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_sclOFIFO_YInpix;
	};
} TOVPDNSCLWRAP_sclOFIFO_YInpix;
typedef union  TOVPDNSCLWRAP_sclOFIFO_UVInpix
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_sclOFIFO_UVInpix;
	};
} TOVPDNSCLWRAP_sclOFIFO_UVInpix;
typedef union  TOVPDNSCLWRAP_tg_ctrl
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_tg_ctrl;
	};
} TOVPDNSCLWRAP_tg_ctrl;
typedef union  TOVPDNSCLWRAP_STATUS0
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_STATUS0;
	};
} TOVPDNSCLWRAP_STATUS0;
typedef union  TOVPDNSCLWRAP_INTR_EN
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_INTR_EN;
	};
} TOVPDNSCLWRAP_INTR_EN;
typedef union  TOVPDNSCLWRAP_STATUS1
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_STATUS1;
	};
} TOVPDNSCLWRAP_STATUS1;
typedef union  TOVPDNSCLWRAP_STATUS2
{ UNSG32 u32[1];
	struct {
		struct w32OVPDNSCLWRAP_STATUS2;
	};
} TOVPDNSCLWRAP_STATUS2;
SIGN32 OVPDNSCLWRAP_drvrd(SIE_OVPDNSCLWRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 OVPDNSCLWRAP_drvwr(SIE_OVPDNSCLWRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void OVPDNSCLWRAP_reset(SIE_OVPDNSCLWRAP *p);
SIGN32 OVPDNSCLWRAP_cmp  (SIE_OVPDNSCLWRAP *p, SIE_OVPDNSCLWRAP *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define OVPDNSCLWRAP_check(p,pie,pfx,hLOG) OVPDNSCLWRAP_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define OVPDNSCLWRAP_print(p,    pfx,hLOG) OVPDNSCLWRAP_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
}
#endif
#pragma  pack()
#endif
