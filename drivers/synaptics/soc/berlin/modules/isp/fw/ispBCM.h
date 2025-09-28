/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ispBCM_h
#define ispBCM_h (){}
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
#ifndef h_BCM_CMD
#define h_BCM_CMD (){}
#define     RA_BCM_CMD_dummy                               0x0000
	typedef struct SIE_BCM_CMD {
		SIE_oneReg                                       ie_dummy[64];
	} SIE_BCM_CMD;
	SIGN32 BCM_CMD_drvrd(SIE_BCM_CMD *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
	SIGN32 BCM_CMD_drvwr(SIE_BCM_CMD *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
	void BCM_CMD_reset(SIE_BCM_CMD *p);
	SIGN32 BCM_CMD_cmp  (SIE_BCM_CMD *p, SIE_BCM_CMD *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define BCM_CMD_check(p,pie,pfx,hLOG) BCM_CMD_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define BCM_CMD_print(p,    pfx,hLOG) BCM_CMD_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_OCCURENCE
#define h_OCCURENCE (){}
	typedef struct SIE_OCCURENCE {
		UNSG32 u_AUTO_PUSH_CNT                             :  8;
		UNSG32 RSVDx0_b8                                   : 24;
	} SIE_OCCURENCE;
	SIGN32 OCCURENCE_drvrd(SIE_OCCURENCE *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
	SIGN32 OCCURENCE_drvwr(SIE_OCCURENCE *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
	void OCCURENCE_reset(SIE_OCCURENCE *p);
	SIGN32 OCCURENCE_cmp  (SIE_OCCURENCE *p, SIE_OCCURENCE *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define OCCURENCE_check(p,pie,pfx,hLOG) OCCURENCE_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define OCCURENCE_print(p,    pfx,hLOG) OCCURENCE_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ISP_BCM
#define h_ISP_BCM (){}
#define     RA_ISP_BCM_cfgReg                              0x0000
#define     RA_ISP_BCM_BCM_Q0                              0x0100
#define     RA_ISP_BCM_BCM_Q1                              0x0104
#define     RA_ISP_BCM_BCM_Q2                              0x0108
#define     RA_ISP_BCM_BCM_Q3                              0x010C
#define     RA_ISP_BCM_BCM_Q4                              0x0110
#define     RA_ISP_BCM_BCM_Q5                              0x0114
#define     RA_ISP_BCM_BCM_Q6                              0x0118
#define     RA_ISP_BCM_BCM_Q7                              0x011C
#define     RA_ISP_BCM_BCM_Q8                              0x0120
#define     RA_ISP_BCM_BCM_Q9                              0x0124
#define     RA_ISP_BCM_BCM_Q10                             0x0128
#define     RA_ISP_BCM_BCM_Q11                             0x012C
#define     RA_ISP_BCM_BCM_Q14                             0x0130
#define     RA_ISP_BCM_BCM_Q15                             0x0134
#define     RA_ISP_BCM_BCM_Q16                             0x0138
#define     RA_ISP_BCM_BCM_Q17                             0x013C
#define     RA_ISP_BCM_BCM_Q18                             0x0140
#define     RA_ISP_BCM_BCM_FULL_STS                        0x0144
#define     RA_ISP_BCM_BCM_EMP_STS                         0x0148
#define     RA_ISP_BCM_BCM_FLUSH                           0x014C
#define     RA_ISP_BCM_BCM_AUTOPUSH_CNT                    0x0150
#define     RA_ISP_BCM_Q0                                  0x0154
#define     RA_ISP_BCM_Q1                                  0x0158
#define     RA_ISP_BCM_Q2                                  0x015C
#define     RA_ISP_BCM_Q3                                  0x0160
#define     RA_ISP_BCM_Q4                                  0x0164
#define     RA_ISP_BCM_Q5                                  0x0168
#define     RA_ISP_BCM_Q6                                  0x016C
#define     RA_ISP_BCM_Q7                                  0x0170
#define     RA_ISP_BCM_Q8                                  0x0174
#define     RA_ISP_BCM_Q9                                  0x0178
#define     RA_ISP_BCM_Q10                                 0x017C
#define     RA_ISP_BCM_Q11                                 0x0180
#define     RA_ISP_BCM_Q14                                 0x0184
#define     RA_ISP_BCM_Q15                                 0x0188
#define     RA_ISP_BCM_Q16                                 0x018C
#define     RA_ISP_BCM_Q17                                 0x0190
#define     RA_ISP_BCM_Q18                                 0x0194
#define     RA_ISP_BCM_BCM_AUTOPUSH                        0x0198
#define     RA_ISP_BCM_BCM_FULL_STS_STICKY                 0x019C
#define     RA_ISP_BCM_BCM_ERROR                           0x01A0
#define     RA_ISP_BCM_BCM_LOG_ADDR                        0x01A4
#define     RA_ISP_BCM_BCM_ERROR_DATA                      0x01A8
	typedef struct SIE_ISP_BCM {
		SIE_BCM_CMD                                      ie_cfgReg;
#define     w32ISP_BCM_BCM_Q0                              {\
	UNSG32 uBCM_Q0_mux                                 :  5;\
	UNSG32 RSVDx100_b5                                 : 27;\
}
		union { UNSG32 u32ISP_BCM_BCM_Q0;
			struct w32ISP_BCM_BCM_Q0;
		};
#define     w32ISP_BCM_BCM_Q1                              {\
	UNSG32 uBCM_Q1_mux                                 :  5;\
	UNSG32 RSVDx104_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q1;
	struct w32ISP_BCM_BCM_Q1;
};
#define     w32ISP_BCM_BCM_Q2                              {\
	UNSG32 uBCM_Q2_mux                                 :  5;\
	UNSG32 RSVDx108_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q2;
	struct w32ISP_BCM_BCM_Q2;
};
#define     w32ISP_BCM_BCM_Q3                              {\
	UNSG32 uBCM_Q3_mux                                 :  5;\
	UNSG32 RSVDx10C_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q3;
	struct w32ISP_BCM_BCM_Q3;
};
#define     w32ISP_BCM_BCM_Q4                              {\
	UNSG32 uBCM_Q4_mux                                 :  5;\
	UNSG32 RSVDx110_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q4;
	struct w32ISP_BCM_BCM_Q4;
};
#define     w32ISP_BCM_BCM_Q5                              {\
	UNSG32 uBCM_Q5_mux                                 :  5;\
	UNSG32 RSVDx114_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q5;
	struct w32ISP_BCM_BCM_Q5;
};
#define     w32ISP_BCM_BCM_Q6                              {\
	UNSG32 uBCM_Q6_mux                                 :  5;\
	UNSG32 RSVDx118_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q6;
	struct w32ISP_BCM_BCM_Q6;
};
#define     w32ISP_BCM_BCM_Q7                              {\
	UNSG32 uBCM_Q7_mux                                 :  5;\
	UNSG32 RSVDx11C_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q7;
	struct w32ISP_BCM_BCM_Q7;
};
#define     w32ISP_BCM_BCM_Q8                              {\
	UNSG32 uBCM_Q8_mux                                 :  5;\
	UNSG32 RSVDx120_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q8;
	struct w32ISP_BCM_BCM_Q8;
};
#define     w32ISP_BCM_BCM_Q9                              {\
	UNSG32 uBCM_Q9_mux                                 :  5;\
	UNSG32 RSVDx124_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q9;
	struct w32ISP_BCM_BCM_Q9;
};
#define     w32ISP_BCM_BCM_Q10                             {\
	UNSG32 uBCM_Q10_mux                                :  5;\
	UNSG32 RSVDx128_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q10;
	struct w32ISP_BCM_BCM_Q10;
};
#define     w32ISP_BCM_BCM_Q11                             {\
	UNSG32 uBCM_Q11_mux                                :  5;\
	UNSG32 RSVDx12C_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q11;
	struct w32ISP_BCM_BCM_Q11;
};
#define     w32ISP_BCM_BCM_Q14                             {\
	UNSG32 uBCM_Q14_mux                                :  5;\
	UNSG32 RSVDx130_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q14;
	struct w32ISP_BCM_BCM_Q14;
};
#define     w32ISP_BCM_BCM_Q15                             {\
	UNSG32 uBCM_Q15_mux                                :  5;\
	UNSG32 RSVDx134_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q15;
	struct w32ISP_BCM_BCM_Q15;
};
#define     w32ISP_BCM_BCM_Q16                             {\
	UNSG32 uBCM_Q16_mux                                :  5;\
	UNSG32 RSVDx138_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q16;
	struct w32ISP_BCM_BCM_Q16;
};
#define     w32ISP_BCM_BCM_Q17                             {\
	UNSG32 uBCM_Q17_mux                                :  5;\
	UNSG32 RSVDx13C_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q17;
	struct w32ISP_BCM_BCM_Q17;
};
#define     w32ISP_BCM_BCM_Q18                             {\
	UNSG32 uBCM_Q18_mux                                :  5;\
	UNSG32 RSVDx140_b5                                 : 27;\
}
union { UNSG32 u32ISP_BCM_BCM_Q18;
	struct w32ISP_BCM_BCM_Q18;
};
#define     w32ISP_BCM_BCM_FULL_STS                        {\
	UNSG32 uBCM_FULL_STS_Q0                            :  1;\
	UNSG32 uBCM_FULL_STS_Q1                            :  1;\
	UNSG32 uBCM_FULL_STS_Q2                            :  1;\
	UNSG32 uBCM_FULL_STS_Q3                            :  1;\
	UNSG32 uBCM_FULL_STS_Q4                            :  1;\
	UNSG32 uBCM_FULL_STS_Q5                            :  1;\
	UNSG32 uBCM_FULL_STS_Q6                            :  1;\
	UNSG32 uBCM_FULL_STS_Q7                            :  1;\
	UNSG32 uBCM_FULL_STS_Q8                            :  1;\
	UNSG32 uBCM_FULL_STS_Q9                            :  1;\
	UNSG32 uBCM_FULL_STS_Q10                           :  1;\
	UNSG32 uBCM_FULL_STS_Q11                           :  1;\
	UNSG32 uBCM_FULL_STS_Q12                           :  1;\
	UNSG32 uBCM_FULL_STS_Q13                           :  1;\
	UNSG32 uBCM_FULL_STS_Q14                           :  1;\
	UNSG32 uBCM_FULL_STS_Q15                           :  1;\
	UNSG32 uBCM_FULL_STS_Q16                           :  1;\
	UNSG32 uBCM_FULL_STS_Q17                           :  1;\
	UNSG32 uBCM_FULL_STS_Q18                           :  1;\
	UNSG32 uBCM_FULL_STS_Q31                           :  1;\
	UNSG32 RSVDx144_b20                                : 12;\
}
union { UNSG32 u32ISP_BCM_BCM_FULL_STS;
	struct w32ISP_BCM_BCM_FULL_STS;
};
#define     w32ISP_BCM_BCM_EMP_STS                         {\
	UNSG32 uBCM_EMP_STS_Q0                             :  1;\
	UNSG32 uBCM_EMP_STS_Q1                             :  1;\
	UNSG32 uBCM_EMP_STS_Q2                             :  1;\
	UNSG32 uBCM_EMP_STS_Q3                             :  1;\
	UNSG32 uBCM_EMP_STS_Q4                             :  1;\
	UNSG32 uBCM_EMP_STS_Q5                             :  1;\
	UNSG32 uBCM_EMP_STS_Q6                             :  1;\
	UNSG32 uBCM_EMP_STS_Q7                             :  1;\
	UNSG32 uBCM_EMP_STS_Q8                             :  1;\
	UNSG32 uBCM_EMP_STS_Q9                             :  1;\
	UNSG32 uBCM_EMP_STS_Q10                            :  1;\
	UNSG32 uBCM_EMP_STS_Q11                            :  1;\
	UNSG32 uBCM_EMP_STS_Q12                            :  1;\
	UNSG32 uBCM_EMP_STS_Q13                            :  1;\
	UNSG32 uBCM_EMP_STS_Q14                            :  1;\
	UNSG32 uBCM_EMP_STS_Q15                            :  1;\
	UNSG32 uBCM_EMP_STS_Q16                            :  1;\
	UNSG32 uBCM_EMP_STS_Q17                            :  1;\
	UNSG32 uBCM_EMP_STS_Q18                            :  1;\
	UNSG32 uBCM_EMP_STS_Q31                            :  1;\
	UNSG32 RSVDx148_b20                                : 12;\
}
union { UNSG32 u32ISP_BCM_BCM_EMP_STS;
	struct w32ISP_BCM_BCM_EMP_STS;
};
#define     w32ISP_BCM_BCM_FLUSH                           {\
	UNSG32 uBCM_FLUSH_Q0                               :  1;\
	UNSG32 uBCM_FLUSH_Q1                               :  1;\
	UNSG32 uBCM_FLUSH_Q2                               :  1;\
	UNSG32 uBCM_FLUSH_Q3                               :  1;\
	UNSG32 uBCM_FLUSH_Q4                               :  1;\
	UNSG32 uBCM_FLUSH_Q5                               :  1;\
	UNSG32 uBCM_FLUSH_Q6                               :  1;\
	UNSG32 uBCM_FLUSH_Q7                               :  1;\
	UNSG32 uBCM_FLUSH_Q8                               :  1;\
	UNSG32 uBCM_FLUSH_Q9                               :  1;\
	UNSG32 uBCM_FLUSH_Q10                              :  1;\
	UNSG32 uBCM_FLUSH_Q11                              :  1;\
	UNSG32 uBCM_FLUSH_Q12                              :  1;\
	UNSG32 uBCM_FLUSH_Q13                              :  1;\
	UNSG32 uBCM_FLUSH_Q14                              :  1;\
	UNSG32 uBCM_FLUSH_Q15                              :  1;\
	UNSG32 uBCM_FLUSH_Q16                              :  1;\
	UNSG32 uBCM_FLUSH_Q17                              :  1;\
	UNSG32 uBCM_FLUSH_Q18                              :  1;\
	UNSG32 uBCM_FLUSH_Q31                              :  1;\
	UNSG32 RSVDx14C_b20                                : 12;\
}
union { UNSG32 u32ISP_BCM_BCM_FLUSH;
	struct w32ISP_BCM_BCM_FLUSH;
};
#define     w32ISP_BCM_BCM_AUTOPUSH_CNT                    {\
	UNSG32 uBCM_AUTOPUSH_CNT_OCCURENCE                 : 32;\
}
union { UNSG32 u32ISP_BCM_BCM_AUTOPUSH_CNT;
	struct w32ISP_BCM_BCM_AUTOPUSH_CNT;
};
SIE_OCCURENCE                                    ie_Q0;
SIE_OCCURENCE                                    ie_Q1;
SIE_OCCURENCE                                    ie_Q2;
SIE_OCCURENCE                                    ie_Q3;
SIE_OCCURENCE                                    ie_Q4;
SIE_OCCURENCE                                    ie_Q5;
SIE_OCCURENCE                                    ie_Q6;
SIE_OCCURENCE                                    ie_Q7;
SIE_OCCURENCE                                    ie_Q8;
SIE_OCCURENCE                                    ie_Q9;
SIE_OCCURENCE                                    ie_Q10;
SIE_OCCURENCE                                    ie_Q11;
SIE_OCCURENCE                                    ie_Q14;
SIE_OCCURENCE                                    ie_Q15;
SIE_OCCURENCE                                    ie_Q16;
SIE_OCCURENCE                                    ie_Q17;
SIE_OCCURENCE                                    ie_Q18;
#define     w32ISP_BCM_BCM_AUTOPUSH                        {\
	UNSG32 uBCM_AUTOPUSH_Q0                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q1                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q2                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q3                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q4                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q5                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q6                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q7                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q8                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q9                            :  1;\
	UNSG32 uBCM_AUTOPUSH_Q10                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q11                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q12                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q13                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q14                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q15                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q16                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q17                           :  1;\
	UNSG32 uBCM_AUTOPUSH_Q18                           :  1;\
	UNSG32 RSVDx198_b19                                : 13;\
}
union { UNSG32 u32ISP_BCM_BCM_AUTOPUSH;
	struct w32ISP_BCM_BCM_AUTOPUSH;
};
#define     w32ISP_BCM_BCM_FULL_STS_STICKY                 {\
	UNSG32 uBCM_FULL_STS_STICKY_Q0                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q1                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q2                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q3                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q4                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q5                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q6                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q7                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q8                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q9                     :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q10                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q11                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q12                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q13                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q14                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q15                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q16                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q17                    :  1;\
	UNSG32 uBCM_FULL_STS_STICKY_Q18                    :  1;\
	UNSG32 RSVDx19C_b19                                : 13;\
}
union { UNSG32 u32ISP_BCM_BCM_FULL_STS_STICKY;
	struct w32ISP_BCM_BCM_FULL_STS_STICKY;
};
#define     w32ISP_BCM_BCM_ERROR                           {\
	UNSG32 uBCM_ERROR_err                              :  1;\
	UNSG32 RSVDx1A0_b1                                 : 31;\
}
union { UNSG32 u32ISP_BCM_BCM_ERROR;
	struct w32ISP_BCM_BCM_ERROR;
};
#define     w32ISP_BCM_BCM_LOG_ADDR                        {\
	UNSG32 uBCM_LOG_ADDR_addr                          : 32;\
}
union { UNSG32 u32ISP_BCM_BCM_LOG_ADDR;
	struct w32ISP_BCM_BCM_LOG_ADDR;
};
#define     w32ISP_BCM_BCM_ERROR_DATA                      {\
	UNSG32 uBCM_ERROR_DATA_data                        : 32;\
}
union { UNSG32 u32ISP_BCM_BCM_ERROR_DATA;
	struct w32ISP_BCM_BCM_ERROR_DATA;
};
UNSG8 RSVDx1AC                                    [84];
} SIE_ISP_BCM;
typedef union  T32ISP_BCM_BCM_Q0
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q0;
} T32ISP_BCM_BCM_Q0;
typedef union  T32ISP_BCM_BCM_Q1
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q1;
} T32ISP_BCM_BCM_Q1;
typedef union  T32ISP_BCM_BCM_Q2
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q2;
} T32ISP_BCM_BCM_Q2;
typedef union  T32ISP_BCM_BCM_Q3
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q3;
} T32ISP_BCM_BCM_Q3;
typedef union  T32ISP_BCM_BCM_Q4
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q4;
} T32ISP_BCM_BCM_Q4;
typedef union  T32ISP_BCM_BCM_Q5
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q5;
} T32ISP_BCM_BCM_Q5;
typedef union  T32ISP_BCM_BCM_Q6
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q6;
} T32ISP_BCM_BCM_Q6;
typedef union  T32ISP_BCM_BCM_Q7
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q7;
} T32ISP_BCM_BCM_Q7;
typedef union  T32ISP_BCM_BCM_Q8
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q8;
} T32ISP_BCM_BCM_Q8;
typedef union  T32ISP_BCM_BCM_Q9
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q9;
} T32ISP_BCM_BCM_Q9;
typedef union  T32ISP_BCM_BCM_Q10
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q10;
} T32ISP_BCM_BCM_Q10;
typedef union  T32ISP_BCM_BCM_Q11
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q11;
} T32ISP_BCM_BCM_Q11;
typedef union  T32ISP_BCM_BCM_Q14
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q14;
} T32ISP_BCM_BCM_Q14;
typedef union  T32ISP_BCM_BCM_Q15
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q15;
} T32ISP_BCM_BCM_Q15;
typedef union  T32ISP_BCM_BCM_Q16
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q16;
} T32ISP_BCM_BCM_Q16;
typedef union  T32ISP_BCM_BCM_Q17
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q17;
} T32ISP_BCM_BCM_Q17;
typedef union  T32ISP_BCM_BCM_Q18
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_Q18;
} T32ISP_BCM_BCM_Q18;
typedef union  T32ISP_BCM_BCM_FULL_STS
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_FULL_STS;
} T32ISP_BCM_BCM_FULL_STS;
typedef union  T32ISP_BCM_BCM_EMP_STS
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_EMP_STS;
} T32ISP_BCM_BCM_EMP_STS;
typedef union  T32ISP_BCM_BCM_FLUSH
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_FLUSH;
} T32ISP_BCM_BCM_FLUSH;
typedef union  T32ISP_BCM_BCM_AUTOPUSH_CNT
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_AUTOPUSH_CNT;
} T32ISP_BCM_BCM_AUTOPUSH_CNT;
typedef union  T32ISP_BCM_BCM_AUTOPUSH
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_AUTOPUSH;
} T32ISP_BCM_BCM_AUTOPUSH;
typedef union  T32ISP_BCM_BCM_FULL_STS_STICKY
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_FULL_STS_STICKY;
} T32ISP_BCM_BCM_FULL_STS_STICKY;
typedef union  T32ISP_BCM_BCM_ERROR
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_ERROR;
} T32ISP_BCM_BCM_ERROR;
typedef union  T32ISP_BCM_BCM_LOG_ADDR
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_LOG_ADDR;
} T32ISP_BCM_BCM_LOG_ADDR;
typedef union  T32ISP_BCM_BCM_ERROR_DATA
{ UNSG32 u32;
	struct w32ISP_BCM_BCM_ERROR_DATA;
} T32ISP_BCM_BCM_ERROR_DATA;
typedef union  TISP_BCM_BCM_Q0
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q0;
	};
} TISP_BCM_BCM_Q0;
typedef union  TISP_BCM_BCM_Q1
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q1;
	};
} TISP_BCM_BCM_Q1;
typedef union  TISP_BCM_BCM_Q2
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q2;
	};
} TISP_BCM_BCM_Q2;
typedef union  TISP_BCM_BCM_Q3
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q3;
	};
} TISP_BCM_BCM_Q3;
typedef union  TISP_BCM_BCM_Q4
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q4;
	};
} TISP_BCM_BCM_Q4;
typedef union  TISP_BCM_BCM_Q5
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q5;
	};
} TISP_BCM_BCM_Q5;
typedef union  TISP_BCM_BCM_Q6
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q6;
	};
} TISP_BCM_BCM_Q6;
typedef union  TISP_BCM_BCM_Q7
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q7;
	};
} TISP_BCM_BCM_Q7;
typedef union  TISP_BCM_BCM_Q8
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q8;
	};
} TISP_BCM_BCM_Q8;
typedef union  TISP_BCM_BCM_Q9
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q9;
	};
} TISP_BCM_BCM_Q9;
typedef union  TISP_BCM_BCM_Q10
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q10;
	};
} TISP_BCM_BCM_Q10;
typedef union  TISP_BCM_BCM_Q11
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q11;
	};
} TISP_BCM_BCM_Q11;
typedef union  TISP_BCM_BCM_Q14
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q14;
	};
} TISP_BCM_BCM_Q14;
typedef union  TISP_BCM_BCM_Q15
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q15;
	};
} TISP_BCM_BCM_Q15;
typedef union  TISP_BCM_BCM_Q16
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q16;
	};
} TISP_BCM_BCM_Q16;
typedef union  TISP_BCM_BCM_Q17
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q17;
	};
} TISP_BCM_BCM_Q17;
typedef union  TISP_BCM_BCM_Q18
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_Q18;
	};
} TISP_BCM_BCM_Q18;
typedef union  TISP_BCM_BCM_FULL_STS
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_FULL_STS;
	};
} TISP_BCM_BCM_FULL_STS;
typedef union  TISP_BCM_BCM_EMP_STS
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_EMP_STS;
	};
} TISP_BCM_BCM_EMP_STS;
typedef union  TISP_BCM_BCM_FLUSH
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_FLUSH;
	};
} TISP_BCM_BCM_FLUSH;
typedef union  TISP_BCM_BCM_AUTOPUSH_CNT
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_AUTOPUSH_CNT;
	};
} TISP_BCM_BCM_AUTOPUSH_CNT;
typedef union  TISP_BCM_BCM_AUTOPUSH
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_AUTOPUSH;
	};
} TISP_BCM_BCM_AUTOPUSH;
typedef union  TISP_BCM_BCM_FULL_STS_STICKY
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_FULL_STS_STICKY;
	};
} TISP_BCM_BCM_FULL_STS_STICKY;
typedef union  TISP_BCM_BCM_ERROR
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_ERROR;
	};
} TISP_BCM_BCM_ERROR;
typedef union  TISP_BCM_BCM_LOG_ADDR
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_LOG_ADDR;
	};
} TISP_BCM_BCM_LOG_ADDR;
typedef union  TISP_BCM_BCM_ERROR_DATA
{ UNSG32 u32[1];
	struct {
		struct w32ISP_BCM_BCM_ERROR_DATA;
	};
} TISP_BCM_BCM_ERROR_DATA;
SIGN32 ISP_BCM_drvrd(SIE_ISP_BCM *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 ISP_BCM_drvwr(SIE_ISP_BCM *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void ISP_BCM_reset(SIE_ISP_BCM *p);
SIGN32 ISP_BCM_cmp  (SIE_ISP_BCM *p, SIE_ISP_BCM *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define ISP_BCM_check(p,pie,pfx,hLOG) ISP_BCM_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define ISP_BCM_print(p,    pfx,hLOG) ISP_BCM_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ispBCMIntMap
#define h_ispBCMIntMap (){}
#define        ispBCMIntMap_IntrMap_isp2Bcm0Intr                        0x0
#define        ispBCMIntMap_IntrMap_isp2Bcm1Intr                        0x1
#define        ispBCMIntMap_IntrMap_mipi0Intr                           0x2
#define        ispBCMIntMap_IntrMap_mipi1Intr                           0x3
#define        ispBCMIntMap_IntrMap_fdet0Intr                           0x4
#define        ispBCMIntMap_IntrMap_fdet1Intr                           0x5
#define        ispBCMIntMap_IntrMap_scl0Intr                            0x6
#define        ispBCMIntMap_IntrMap_scl1Intr                            0x7
#define        ispBCMIntMap_IntrMap_scl2Intr                            0x8
#define        ispBCMIntMap_IntrMap_dwrpIntr                            0x9
#define        ispBCMIntMap_IntrMap_rotIntr                             0xA
#define        ispBCMIntMap_IntrMap_tilIntr                             0xB
#define        ispBCMIntMap_IntrMap_tsbCh0Iintr                         0xC
#define        ispBCMIntMap_IntrMap_tsbCh1Iintr                         0xD
#define        ispBCMIntMap_IntrMap_tsbCh2Iintr                         0xE
#define        ispBCMIntMap_IntrMap_tsbCh3Iintr                         0xF
#define        ispBCMIntMap_IntrMap_tsbCh4Iintr                         0x10
#define        ispBCMIntMap_IntrMap_tsbCh5Iintr                         0x11
#define        ispBCMIntMap_IntrMap_tsbCh6Iintr                         0x12
#define        ispBCMIntMap_IntrMap_fwrCh0Intr                          0x13
#define        ispBCMIntMap_IntrMap_fwrCh1Intr                          0x14
#define        ispBCMIntMap_IntrMap_fwrCh2Intr                          0x15
#define        ispBCMIntMap_IntrMap_fwrCh3Intr                          0x16
#define        ispBCMIntMap_IntrMap_fwrCh4Intr                          0x17
#define        ispBCMIntMap_IntrMap_fwrCh5Intr                          0x18
#define        ispBCMIntMap_IntrMap_fwrCh6Intr                          0x19
#define        ispBCMIntMap_IntrMap_fwrCh7Intr                          0x1A
#define        ispBCMIntMap_IntrMap_fwrCh8Intr                          0x1B
#define        ispBCMIntMap_IntrMap_fwrCh9Intr                          0x1C
#define        ispBCMIntMap_IntrMap_fwrCh10Intr                         0x1D
#define        ispBCMIntMap_IntrMap_fwrCh11Intr                         0x1E
#define        ispBCMIntMap_IntrMap_isp2Bcm2Intr                        0x1F
typedef struct SIE_ispBCMIntMap {
	UNSG32 u_IntrMap                                   :  5;
	UNSG32 RSVDx0_b5                                   : 27;
} SIE_ispBCMIntMap;
SIGN32 ispBCMIntMap_drvrd(SIE_ispBCMIntMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 ispBCMIntMap_drvwr(SIE_ispBCMIntMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void ispBCMIntMap_reset(SIE_ispBCMIntMap *p);
SIGN32 ispBCMIntMap_cmp  (SIE_ispBCMIntMap *p, SIE_ispBCMIntMap *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define ispBCMIntMap_check(p,pie,pfx,hLOG) ispBCMIntMap_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define ispBCMIntMap_print(p,    pfx,hLOG) ispBCMIntMap_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
}
#endif
#pragma  pack()
#endif
