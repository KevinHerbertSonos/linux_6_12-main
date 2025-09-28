/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 - 2023 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ispMISC_h
#define ispMISC_h (){}
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
#ifndef h_CLKRST
#define h_CLKRST (){}
#define     RA_CLKRST_AlmCkCtl                             0x0000
#define     RA_CLKRST_MtrCkCtl                             0x0004
#define     RA_CLKRST_TsbCkCtl                             0x0008
#define     RA_CLKRST_FwrCkCtl                             0x000C
#define     RA_CLKRST_BcmCkCtl                             0x0010
#define     RA_CLKRST_CSICkCtl                             0x0014
#define     RA_CLKRST_ISPCkCtl                             0x0018
#define     RA_CLKRST_TileCkCtl                            0x001C
#define     RA_CLKRST_RotCkCtl                             0x0020
#define     RA_CLKRST_DwrpCkCtl                            0x0024
#define     RA_CLKRST_CvpCkCtl                             0x0028
#define     RA_CLKRST_SclCkCtl                             0x002C
#define     RA_CLKRST_FuncReset                            0x0030
#define     RA_CLKRST_BiuReset                             0x0034
#define     RA_CLKRST_AxiIsoEnable                         0x0038
#define     RA_CLKRST_AxiIsoClr                            0x003C
#define     RA_CLKRST_AxiIsoSt                             0x0040
#define     RA_CLKRST_OcpIsoEnable                         0x0044
#define     RA_CLKRST_CfgIsoEnable                         0x0048
#define     RA_CLKRST_CfgIsoClr                            0x004C
#define     RA_CLKRST_CfgIsoSt                             0x0050
typedef struct SIE_CLKRST {
#define     w32CLKRST_AlmCkCtl                             {\
	UNSG32 uAlmCkCtl_almSysclkEn                       :  1;\
	UNSG32 uAlmCkCtl_almCfgclkEn                       :  1;\
	UNSG32 RSVDx0_b2                                   : 30;\
}
union { UNSG32 u32CLKRST_AlmCkCtl;
	struct w32CLKRST_AlmCkCtl;
};
#define     w32CLKRST_MtrCkCtl                             {\
	UNSG32 uMtrCkCtl_mtrSysclkEn                       :  1;\
	UNSG32 uMtrCkCtl_mtrCfgclkEn                       :  1;\
	UNSG32 RSVDx4_b2                                   : 30;\
}
union { UNSG32 u32CLKRST_MtrCkCtl;
	struct w32CLKRST_MtrCkCtl;
};
#define     w32CLKRST_TsbCkCtl                             {\
	UNSG32 uTsbCkCtl_TSBDHubDyCGEn                     :  1;\
	UNSG32 uTsbCkCtl_TSBDHubSwCGEn                     :  1;\
	UNSG32 uTsbCkCtl_TSBDHubCGEn                       :  1;\
	UNSG32 uTsbCkCtl_TSBCfgclkEn                       :  1;\
	UNSG32 RSVDx8_b4                                   : 28;\
}
union { UNSG32 u32CLKRST_TsbCkCtl;
	struct w32CLKRST_TsbCkCtl;
};
#define     w32CLKRST_FwrCkCtl                             {\
	UNSG32 uFwrCkCtl_FWRDHubDyCGEn                     :  1;\
	UNSG32 uFwrCkCtl_FWRDHubSwCGEn                     :  1;\
	UNSG32 uFwrCkCtl_FWRDHubCGEn                       :  1;\
	UNSG32 uFwrCkCtl_FWRCfgclkEn                       :  1;\
	UNSG32 RSVDxC_b4                                   : 28;\
}
union { UNSG32 u32CLKRST_FwrCkCtl;
	struct w32CLKRST_FwrCkCtl;
};
#define     w32CLKRST_BcmCkCtl                             {\
	UNSG32 uBcmCkCtl_bcmCfgclkEn                       :  1;\
	UNSG32 uBcmCkCtl_bcmSysclkEn                       :  1;\
	UNSG32 uBcmCkCtl_bcmCsi0clkEn                      :  1;\
	UNSG32 uBcmCkCtl_bcmCsi1clkEn                      :  1;\
	UNSG32 uBcmCkCtl_bcmIspclkEn                       :  1;\
	UNSG32 uBcmCkCtl_bcmSclclkEn                       :  1;\
	UNSG32 uBcmCkCtl_bcmBeclkEn                        :  1;\
	UNSG32 uBcmCkCtl_dphyCfgclkEn                      :  1;\
	UNSG32 uBcmCkCtl_dphyPhyclkEn                      :  1;\
	UNSG32 RSVDx10_b9                                  : 23;\
}
union { UNSG32 u32CLKRST_BcmCkCtl;
	struct w32CLKRST_BcmCkCtl;
};
#define     w32CLKRST_CSICkCtl                             {\
	UNSG32 uCSICkCtl_csiCfgclkEn                       :  1;\
	UNSG32 uCSICkCtl_csiCsi0clkEn                      :  1;\
	UNSG32 uCSICkCtl_csiCsi1clkEn                      :  1;\
	UNSG32 RSVDx14_b3                                  : 29;\
}
union { UNSG32 u32CLKRST_CSICkCtl;
	struct w32CLKRST_CSICkCtl;
};
#define     w32CLKRST_ISPCkCtl                             {\
	UNSG32 uISPCkCtl_ispCfgclkEn                       :  1;\
	UNSG32 uISPCkCtl_ispSysclkEn                       :  1;\
	UNSG32 uISPCkCtl_ispIspclkEn                       :  1;\
	UNSG32 uISPCkCtl_ispCsi0clkEn                      :  1;\
	UNSG32 uISPCkCtl_ispCsi1clkEn                      :  1;\
	UNSG32 uISPCkCtl_ispMcmclkEn                       :  1;\
	UNSG32 uISPCkCtl_ispBiuclkEn                       :  1;\
	UNSG32 RSVDx18_b7                                  : 25;\
}
union { UNSG32 u32CLKRST_ISPCkCtl;
	struct w32CLKRST_ISPCkCtl;
};
#define     w32CLKRST_TileCkCtl                            {\
	UNSG32 uTileCkCtl_tileCfgclkEn                     :  1;\
	UNSG32 uTileCkCtl_tileIspclkEn                     :  1;\
	UNSG32 uTileCkCtl_tileBeclkEn                      :  1;\
	UNSG32 uTileCkCtl_tileSysclkEn                     :  1;\
	UNSG32 RSVDx1C_b4                                  : 28;\
}
union { UNSG32 u32CLKRST_TileCkCtl;
	struct w32CLKRST_TileCkCtl;
};
#define     w32CLKRST_RotCkCtl                             {\
	UNSG32 uRotCkCtl_rotCfgclkEn                       :  1;\
	UNSG32 uRotCkCtl_rotSysclkEn                       :  1;\
	UNSG32 uRotCkCtl_rotBeclkEn                        :  1;\
	UNSG32 RSVDx20_b3                                  : 29;\
}
union { UNSG32 u32CLKRST_RotCkCtl;
	struct w32CLKRST_RotCkCtl;
};
#define     w32CLKRST_DwrpCkCtl                            {\
	UNSG32 uDwrpCkCtl_dwrpCfgclkEn                     :  1;\
	UNSG32 uDwrpCkCtl_dwrpBeclkEn                      :  1;\
	UNSG32 uDwrpCkCtl_dwrpSysclkEn                     :  1;\
	UNSG32 RSVDx24_b3                                  : 29;\
}
union { UNSG32 u32CLKRST_DwrpCkCtl;
	struct w32CLKRST_DwrpCkCtl;
};
#define     w32CLKRST_CvpCkCtl                             {\
	UNSG32 uCvpCkCtl_cvpCfgclkEn                       :  1;\
	UNSG32 uCvpCkCtl_cvpBeclkEn                        :  1;\
	UNSG32 uCvpCkCtl_cvpSysclkEn                       :  1;\
	UNSG32 RSVDx28_b3                                  : 29;\
}
union { UNSG32 u32CLKRST_CvpCkCtl;
	struct w32CLKRST_CvpCkCtl;
};
#define     w32CLKRST_SclCkCtl                             {\
	UNSG32 uSclCkCtl_sclCfgclkEn                       :  1;\
	UNSG32 uSclCkCtl_sclSclclkEn                       :  1;\
	UNSG32 uSclCkCtl_sclSysclkEn                       :  1;\
	UNSG32 uSclCkCtl_sclIspclkEn                       :  1;\
	UNSG32 RSVDx2C_b4                                  : 28;\
}
union { UNSG32 u32CLKRST_SclCkCtl;
	struct w32CLKRST_SclCkCtl;
};
#define     w32CLKRST_FuncReset                            {\
	UNSG32 uFuncReset_almReset                         :  1;\
	UNSG32 uFuncReset_mtrReset                         :  1;\
	UNSG32 uFuncReset_tsbDhubReset                     :  1;\
	UNSG32 uFuncReset_fwrDhubReset                     :  1;\
	UNSG32 uFuncReset_bcmReset                         :  1;\
	UNSG32 uFuncReset_csi0Reset                        :  1;\
	UNSG32 uFuncReset_csi1Reset                        :  1;\
	UNSG32 uFuncReset_ispReset                         :  1;\
	UNSG32 uFuncReset_tileReset                        :  1;\
	UNSG32 uFuncReset_rotReset                         :  1;\
	UNSG32 uFuncReset_dwrpReset                        :  1;\
	UNSG32 uFuncReset_cvpReset                         :  1;\
	UNSG32 uFuncReset_sclReset                         :  1;\
	UNSG32 RSVDx30_b13                                 : 19;\
}
union { UNSG32 u32CLKRST_FuncReset;
	struct w32CLKRST_FuncReset;
};
#define     w32CLKRST_BiuReset                             {\
	UNSG32 uBiuReset_almReset                          :  1;\
	UNSG32 uBiuReset_mtrReset                          :  1;\
	UNSG32 uBiuReset_tsbDhubReset                      :  1;\
	UNSG32 uBiuReset_fwrDhubReset                      :  1;\
	UNSG32 uBiuReset_bcmReset                          :  1;\
	UNSG32 uBiuReset_feReset                           :  1;\
	UNSG32 uBiuReset_ispReset                          :  1;\
	UNSG32 uBiuReset_tileReset                         :  1;\
	UNSG32 uBiuReset_rotReset                          :  1;\
	UNSG32 uBiuReset_dwrpReset                         :  1;\
	UNSG32 uBiuReset_cvpReset                          :  1;\
	UNSG32 uBiuReset_sclReset                          :  1;\
	UNSG32 RSVDx34_b12                                 : 20;\
}
union { UNSG32 u32CLKRST_BiuReset;
	struct w32CLKRST_BiuReset;
};
#define     w32CLKRST_AxiIsoEnable                         {\
	UNSG32 uAxiIsoEnable_fwrAxiIsoEn                   :  1;\
	UNSG32 uAxiIsoEnable_tsbAxiM1IsoEn                 :  1;\
	UNSG32 uAxiIsoEnable_ispAxiIsoEn                   :  2;\
	UNSG32 uAxiIsoEnable_ixbarM1AxiIsoEn               :  1;\
	UNSG32 uAxiIsoEnable_ispSSAxiIsoEn                 :  1;\
	UNSG32 RSVDx38_b6                                  : 26;\
}
union { UNSG32 u32CLKRST_AxiIsoEnable;
	struct w32CLKRST_AxiIsoEnable;
};
#define     w32CLKRST_AxiIsoClr                            {\
	UNSG32 uAxiIsoClr_fwrAxiIsoClr                     :  1;\
	UNSG32 uAxiIsoClr_tsbAxiM1IsoClr                   :  1;\
	UNSG32 uAxiIsoClr_ispAxiIsoClr                     :  2;\
	UNSG32 uAxiIsoClr_ixbarM1AxiIsoClr                 :  1;\
	UNSG32 uAxiIsoClr_ispSSAxiIsoClr                   :  1;\
	UNSG32 RSVDx3C_b6                                  : 26;\
}
union { UNSG32 u32CLKRST_AxiIsoClr;
	struct w32CLKRST_AxiIsoClr;
};
#define     w32CLKRST_AxiIsoSt                             {\
	UNSG32 uAxiIsoSt_fwrAxiIsoSt                       :  1;\
	UNSG32 uAxiIsoSt_tsbAxiM1IsoSt                     :  1;\
	UNSG32 uAxiIsoSt_ispAxiIsoSt                       :  2;\
	UNSG32 uAxiIsoSt_ixbarM1AxiIsoSt                   :  1;\
	UNSG32 uAxiIsoSt_ispSSAxiIsoSt                     :  1;\
	UNSG32 RSVDx40_b6                                  : 26;\
}
union { UNSG32 u32CLKRST_AxiIsoSt;
	struct w32CLKRST_AxiIsoSt;
};
#define     w32CLKRST_OcpIsoEnable                         {\
	UNSG32 uOcpIsoEnable_tileOcpIsoEn                  :  1;\
	UNSG32 uOcpIsoEnable_scldnOcpIsoEn                 :  1;\
	UNSG32 uOcpIsoEnable_BCMOcpIsoEn                   :  1;\
	UNSG32 uOcpIsoEnable_dwrpOcpIsoEn                  :  1;\
	UNSG32 uOcpIsoEnable_cvpOcpIsoEn                   :  1;\
	UNSG32 uOcpIsoEnable_rotOcpIsoEn                   :  1;\
	UNSG32 RSVDx44_b6                                  : 26;\
}
union { UNSG32 u32CLKRST_OcpIsoEnable;
	struct w32CLKRST_OcpIsoEnable;
};
#define     w32CLKRST_CfgIsoEnable                         {\
	UNSG32 uCfgIsoEnable_mtrBiuIsoEn                   :  1;\
	UNSG32 uCfgIsoEnable_almBiuIsoEn                   :  1;\
	UNSG32 uCfgIsoEnable_tsbBiuIsoEn                   :  1;\
	UNSG32 uCfgIsoEnable_fwrBiuIsoEn                   :  1;\
	UNSG32 uCfgIsoEnable_ispBiuIsoEn                   :  1;\
	UNSG32 uCfgIsoEnable_csiBiuIsoEn                   :  1;\
	UNSG32 uCfgIsoEnable_dphyBiuIsoEn                  :  1;\
	UNSG32 uCfgIsoEnable_tileBiuIsoEn                  :  1;\
	UNSG32 uCfgIsoEnable_scldnBiuIsoEn                 :  1;\
	UNSG32 uCfgIsoEnable_rotBiuIsoEn                   :  1;\
	UNSG32 uCfgIsoEnable_dwrpBiuIsoEn                  :  1;\
	UNSG32 uCfgIsoEnable_cvpBiuIsoEn                   :  1;\
	UNSG32 RSVDx48_b12                                 : 20;\
}
union { UNSG32 u32CLKRST_CfgIsoEnable;
	struct w32CLKRST_CfgIsoEnable;
};
#define     w32CLKRST_CfgIsoClr                            {\
	UNSG32 uCfgIsoClr_mtrBiuIsoClr                     :  1;\
	UNSG32 uCfgIsoClr_almBiuIsoClr                     :  1;\
	UNSG32 uCfgIsoClr_tsbBiuIsoClr                     :  1;\
	UNSG32 uCfgIsoClr_fwrBiuIsoClr                     :  1;\
	UNSG32 uCfgIsoClr_ispBiuIsoClr                     :  1;\
	UNSG32 uCfgIsoClr_csiBiuIsoClr                     :  1;\
	UNSG32 uCfgIsoClr_dphyBiuIsoClr                    :  1;\
	UNSG32 uCfgIsoClr_tileBiuIsoClr                    :  1;\
	UNSG32 uCfgIsoClr_scldnBiuIsoClr                   :  1;\
	UNSG32 uCfgIsoClr_rotBiuIsoClr                     :  1;\
	UNSG32 uCfgIsoClr_dwrpBiuIsoClr                    :  1;\
	UNSG32 uCfgIsoClr_cvpBiuIsoClr                     :  1;\
	UNSG32 RSVDx4C_b12                                 : 20;\
}
union { UNSG32 u32CLKRST_CfgIsoClr;
	struct w32CLKRST_CfgIsoClr;
};
#define     w32CLKRST_CfgIsoSt                             {\
	UNSG32 uCfgIsoSt_mtrBiuIsoSt                       :  1;\
	UNSG32 uCfgIsoSt_almBiuIsoSt                       :  1;\
	UNSG32 uCfgIsoSt_tsbBiuIsoSt                       :  1;\
	UNSG32 uCfgIsoSt_fwrBiuIsoSt                       :  1;\
	UNSG32 uCfgIsoSt_ispBiuIsoSt                       :  1;\
	UNSG32 uCfgIsoSt_csiBiuIsoSt                       :  1;\
	UNSG32 uCfgIsoSt_dphyBiuIsoSt                      :  1;\
	UNSG32 uCfgIsoSt_tileBiuIsoSt                      :  1;\
	UNSG32 uCfgIsoSt_scldnBiuIsoSt                     :  1;\
	UNSG32 uCfgIsoSt_rotBiuIsoSt                       :  1;\
	UNSG32 uCfgIsoSt_dwrpBiuIsoSt                      :  1;\
	UNSG32 uCfgIsoSt_cvpBiuIsoSt                       :  1;\
	UNSG32 RSVDx50_b12                                 : 20;\
}
union { UNSG32 u32CLKRST_CfgIsoSt;
	struct w32CLKRST_CfgIsoSt;
};
} SIE_CLKRST;
typedef union  T32CLKRST_AlmCkCtl
{ UNSG32 u32;
	struct w32CLKRST_AlmCkCtl;
} T32CLKRST_AlmCkCtl;
typedef union  T32CLKRST_MtrCkCtl
{ UNSG32 u32;
	struct w32CLKRST_MtrCkCtl;
} T32CLKRST_MtrCkCtl;
typedef union  T32CLKRST_TsbCkCtl
{ UNSG32 u32;
	struct w32CLKRST_TsbCkCtl;
} T32CLKRST_TsbCkCtl;
typedef union  T32CLKRST_FwrCkCtl
{ UNSG32 u32;
	struct w32CLKRST_FwrCkCtl;
} T32CLKRST_FwrCkCtl;
typedef union  T32CLKRST_BcmCkCtl
{ UNSG32 u32;
	struct w32CLKRST_BcmCkCtl;
} T32CLKRST_BcmCkCtl;
typedef union  T32CLKRST_CSICkCtl
{ UNSG32 u32;
	struct w32CLKRST_CSICkCtl;
} T32CLKRST_CSICkCtl;
typedef union  T32CLKRST_ISPCkCtl
{ UNSG32 u32;
	struct w32CLKRST_ISPCkCtl;
} T32CLKRST_ISPCkCtl;
typedef union  T32CLKRST_TileCkCtl
{ UNSG32 u32;
	struct w32CLKRST_TileCkCtl;
} T32CLKRST_TileCkCtl;
typedef union  T32CLKRST_RotCkCtl
{ UNSG32 u32;
	struct w32CLKRST_RotCkCtl;
} T32CLKRST_RotCkCtl;
typedef union  T32CLKRST_DwrpCkCtl
{ UNSG32 u32;
	struct w32CLKRST_DwrpCkCtl;
} T32CLKRST_DwrpCkCtl;
typedef union  T32CLKRST_CvpCkCtl
{ UNSG32 u32;
	struct w32CLKRST_CvpCkCtl;
} T32CLKRST_CvpCkCtl;
typedef union  T32CLKRST_SclCkCtl
{ UNSG32 u32;
	struct w32CLKRST_SclCkCtl;
} T32CLKRST_SclCkCtl;
typedef union  T32CLKRST_FuncReset
{ UNSG32 u32;
	struct w32CLKRST_FuncReset;
} T32CLKRST_FuncReset;
typedef union  T32CLKRST_BiuReset
{ UNSG32 u32;
	struct w32CLKRST_BiuReset;
} T32CLKRST_BiuReset;
typedef union  T32CLKRST_AxiIsoEnable
{ UNSG32 u32;
	struct w32CLKRST_AxiIsoEnable;
} T32CLKRST_AxiIsoEnable;
typedef union  T32CLKRST_AxiIsoClr
{ UNSG32 u32;
	struct w32CLKRST_AxiIsoClr;
} T32CLKRST_AxiIsoClr;
typedef union  T32CLKRST_AxiIsoSt
{ UNSG32 u32;
	struct w32CLKRST_AxiIsoSt;
} T32CLKRST_AxiIsoSt;
typedef union  T32CLKRST_OcpIsoEnable
{ UNSG32 u32;
	struct w32CLKRST_OcpIsoEnable;
} T32CLKRST_OcpIsoEnable;
typedef union  T32CLKRST_CfgIsoEnable
{ UNSG32 u32;
	struct w32CLKRST_CfgIsoEnable;
} T32CLKRST_CfgIsoEnable;
typedef union  T32CLKRST_CfgIsoClr
{ UNSG32 u32;
	struct w32CLKRST_CfgIsoClr;
} T32CLKRST_CfgIsoClr;
typedef union  T32CLKRST_CfgIsoSt
{ UNSG32 u32;
	struct w32CLKRST_CfgIsoSt;
} T32CLKRST_CfgIsoSt;
typedef union  TCLKRST_AlmCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_AlmCkCtl;
	};
} TCLKRST_AlmCkCtl;
typedef union  TCLKRST_MtrCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_MtrCkCtl;
	};
} TCLKRST_MtrCkCtl;
typedef union  TCLKRST_TsbCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_TsbCkCtl;
	};
} TCLKRST_TsbCkCtl;
typedef union  TCLKRST_FwrCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_FwrCkCtl;
	};
} TCLKRST_FwrCkCtl;
typedef union  TCLKRST_BcmCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_BcmCkCtl;
	};
} TCLKRST_BcmCkCtl;
typedef union  TCLKRST_CSICkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_CSICkCtl;
	};
} TCLKRST_CSICkCtl;
typedef union  TCLKRST_ISPCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_ISPCkCtl;
	};
} TCLKRST_ISPCkCtl;
typedef union  TCLKRST_TileCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_TileCkCtl;
	};
} TCLKRST_TileCkCtl;
typedef union  TCLKRST_RotCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_RotCkCtl;
	};
} TCLKRST_RotCkCtl;
typedef union  TCLKRST_DwrpCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_DwrpCkCtl;
	};
} TCLKRST_DwrpCkCtl;
typedef union  TCLKRST_CvpCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_CvpCkCtl;
	};
} TCLKRST_CvpCkCtl;
typedef union  TCLKRST_SclCkCtl
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_SclCkCtl;
	};
} TCLKRST_SclCkCtl;
typedef union  TCLKRST_FuncReset
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_FuncReset;
	};
} TCLKRST_FuncReset;
typedef union  TCLKRST_BiuReset
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_BiuReset;
	};
} TCLKRST_BiuReset;
typedef union  TCLKRST_AxiIsoEnable
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_AxiIsoEnable;
	};
} TCLKRST_AxiIsoEnable;
typedef union  TCLKRST_AxiIsoClr
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_AxiIsoClr;
	};
} TCLKRST_AxiIsoClr;
typedef union  TCLKRST_AxiIsoSt
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_AxiIsoSt;
	};
} TCLKRST_AxiIsoSt;
typedef union  TCLKRST_OcpIsoEnable
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_OcpIsoEnable;
	};
} TCLKRST_OcpIsoEnable;
typedef union  TCLKRST_CfgIsoEnable
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_CfgIsoEnable;
	};
} TCLKRST_CfgIsoEnable;
typedef union  TCLKRST_CfgIsoClr
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_CfgIsoClr;
	};
} TCLKRST_CfgIsoClr;
typedef union  TCLKRST_CfgIsoSt
{ UNSG32 u32[1];
	struct {
		struct w32CLKRST_CfgIsoSt;
	};
} TCLKRST_CfgIsoSt;
SIGN32 CLKRST_drvrd(SIE_CLKRST *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 CLKRST_drvwr(SIE_CLKRST *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void CLKRST_reset(SIE_CLKRST *p);
SIGN32 CLKRST_cmp  (SIE_CLKRST *p, SIE_CLKRST *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define CLKRST_check(p,pie,pfx,hLOG) CLKRST_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define CLKRST_print(p,    pfx,hLOG) CLKRST_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SecCtrl
#define h_SecCtrl (){}
#define     RA_SecCtrl_Lock                                0x0000
#define     RA_SecCtrl_Security                            0x0004
#define     RA_SecCtrl_SensorId                            0x0008
typedef struct SIE_SecCtrl {
#define     w32SecCtrl_Lock                                {\
	UNSG32 uLock_secLock                               :  1;\
	UNSG32 RSVDx0_b1                                   : 31;\
}
union { UNSG32 u32SecCtrl_Lock;
	struct w32SecCtrl_Lock;
};
#define     w32SecCtrl_Security                            {\
	UNSG32 uSecurity_s0NS                              :  1;\
	UNSG32 uSecurity_s1NS                              :  1;\
	UNSG32 uSecurity_bcmNS                             :  1;\
	UNSG32 RSVDx4_b3                                   : 29;\
}
union { UNSG32 u32SecCtrl_Security;
	struct w32SecCtrl_Security;
};
#define     w32SecCtrl_SensorId                            {\
	UNSG32 uSensorId_SidScld                           :  1;\
	UNSG32 uSensorId_SidTile                           :  1;\
	UNSG32 uSensorId_SidRotate                         :  1;\
	UNSG32 uSensorId_SidDewarp                         :  1;\
	UNSG32 uSensorId_SidCVP                            :  1;\
	UNSG32 RSVDx8_b5                                   : 27;\
}
union { UNSG32 u32SecCtrl_SensorId;
	struct w32SecCtrl_SensorId;
};
} SIE_SecCtrl;
typedef union  T32SecCtrl_Lock
{ UNSG32 u32;
	struct w32SecCtrl_Lock;
} T32SecCtrl_Lock;
typedef union  T32SecCtrl_Security
{ UNSG32 u32;
	struct w32SecCtrl_Security;
} T32SecCtrl_Security;
typedef union  T32SecCtrl_SensorId
{ UNSG32 u32;
	struct w32SecCtrl_SensorId;
} T32SecCtrl_SensorId;
typedef union  TSecCtrl_Lock
{ UNSG32 u32[1];
	struct {
		struct w32SecCtrl_Lock;
	};
} TSecCtrl_Lock;
typedef union  TSecCtrl_Security
{ UNSG32 u32[1];
	struct {
		struct w32SecCtrl_Security;
	};
} TSecCtrl_Security;
typedef union  TSecCtrl_SensorId
{ UNSG32 u32[1];
	struct {
		struct w32SecCtrl_SensorId;
	};
} TSecCtrl_SensorId;
SIGN32 SecCtrl_drvrd(SIE_SecCtrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 SecCtrl_drvwr(SIE_SecCtrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void SecCtrl_reset(SIE_SecCtrl *p);
SIGN32 SecCtrl_cmp  (SIE_SecCtrl *p, SIE_SecCtrl *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define SecCtrl_check(p,pie,pfx,hLOG) SecCtrl_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define SecCtrl_print(p,    pfx,hLOG) SecCtrl_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_TidMapEntry
#define h_TidMapEntry (){}
#define     RA_TidMapEntry_map                             0x0000
typedef struct SIE_TidMapEntry {
#define     w32TidMapEntry_map                             {\
	UNSG32 umap_map                                    :  5;\
	UNSG32 RSVDx0_b5                                   : 27;\
}
union { UNSG32 u32TidMapEntry_map;
	struct w32TidMapEntry_map;
};
} SIE_TidMapEntry;
typedef union  T32TidMapEntry_map
{ UNSG32 u32;
	struct w32TidMapEntry_map;
} T32TidMapEntry_map;
typedef union  TTidMapEntry_map
{ UNSG32 u32[1];
	struct {
		struct w32TidMapEntry_map;
	};
} TTidMapEntry_map;
SIGN32 TidMapEntry_drvrd(SIE_TidMapEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 TidMapEntry_drvwr(SIE_TidMapEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void TidMapEntry_reset(SIE_TidMapEntry *p);
SIGN32 TidMapEntry_cmp  (SIE_TidMapEntry *p, SIE_TidMapEntry *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define TidMapEntry_check(p,pie,pfx,hLOG) TidMapEntry_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define TidMapEntry_print(p,    pfx,hLOG) TidMapEntry_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_TsbMtrFlagEntry
#define h_TsbMtrFlagEntry (){}
#define     RA_TsbMtrFlagEntry_flag                        0x0000
typedef struct SIE_TsbMtrFlagEntry {
#define     w32TsbMtrFlagEntry_flag                        {\
	UNSG32 uflag_flag                                  :  6;\
	UNSG32 RSVDx0_b6                                   : 26;\
}
union { UNSG32 u32TsbMtrFlagEntry_flag;
	struct w32TsbMtrFlagEntry_flag;
};
} SIE_TsbMtrFlagEntry;
typedef union  T32TsbMtrFlagEntry_flag
{ UNSG32 u32;
	struct w32TsbMtrFlagEntry_flag;
} T32TsbMtrFlagEntry_flag;
typedef union  TTsbMtrFlagEntry_flag
{ UNSG32 u32[1];
	struct {
		struct w32TsbMtrFlagEntry_flag;
	};
} TTsbMtrFlagEntry_flag;
SIGN32 TsbMtrFlagEntry_drvrd(SIE_TsbMtrFlagEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 TsbMtrFlagEntry_drvwr(SIE_TsbMtrFlagEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void TsbMtrFlagEntry_reset(SIE_TsbMtrFlagEntry *p);
SIGN32 TsbMtrFlagEntry_cmp  (SIE_TsbMtrFlagEntry *p, SIE_TsbMtrFlagEntry *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define TsbMtrFlagEntry_check(p,pie,pfx,hLOG) TsbMtrFlagEntry_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define TsbMtrFlagEntry_print(p,    pfx,hLOG) TsbMtrFlagEntry_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_IspMtrFlagEntry
#define h_IspMtrFlagEntry (){}
#define     RA_IspMtrFlagEntry_flag                        0x0000
typedef struct SIE_IspMtrFlagEntry {
#define     w32IspMtrFlagEntry_flag                        {\
	UNSG32 uflag_flag                                  : 16;\
	UNSG32 RSVDx0_b16                                  : 16;\
}
union { UNSG32 u32IspMtrFlagEntry_flag;
	struct w32IspMtrFlagEntry_flag;
};
} SIE_IspMtrFlagEntry;
typedef union  T32IspMtrFlagEntry_flag
{ UNSG32 u32;
	struct w32IspMtrFlagEntry_flag;
} T32IspMtrFlagEntry_flag;
typedef union  TIspMtrFlagEntry_flag
{ UNSG32 u32[1];
	struct {
		struct w32IspMtrFlagEntry_flag;
	};
} TIspMtrFlagEntry_flag;
SIGN32 IspMtrFlagEntry_drvrd(SIE_IspMtrFlagEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 IspMtrFlagEntry_drvwr(SIE_IspMtrFlagEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void IspMtrFlagEntry_reset(SIE_IspMtrFlagEntry *p);
SIGN32 IspMtrFlagEntry_cmp  (SIE_IspMtrFlagEntry *p, SIE_IspMtrFlagEntry *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define IspMtrFlagEntry_check(p,pie,pfx,hLOG) IspMtrFlagEntry_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define IspMtrFlagEntry_print(p,    pfx,hLOG) IspMtrFlagEntry_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_FwrMtrFlagEntry
#define h_FwrMtrFlagEntry (){}
#define     RA_FwrMtrFlagEntry_flag                        0x0000
typedef struct SIE_FwrMtrFlagEntry {
#define     w32FwrMtrFlagEntry_flag                        {\
	UNSG32 uflag_flag                                  : 13;\
	UNSG32 RSVDx0_b13                                  : 19;\
}
union { UNSG32 u32FwrMtrFlagEntry_flag;
	struct w32FwrMtrFlagEntry_flag;
};
} SIE_FwrMtrFlagEntry;
typedef union  T32FwrMtrFlagEntry_flag
{ UNSG32 u32;
	struct w32FwrMtrFlagEntry_flag;
} T32FwrMtrFlagEntry_flag;
typedef union  TFwrMtrFlagEntry_flag
{ UNSG32 u32[1];
	struct {
		struct w32FwrMtrFlagEntry_flag;
	};
} TFwrMtrFlagEntry_flag;
SIGN32 FwrMtrFlagEntry_drvrd(SIE_FwrMtrFlagEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 FwrMtrFlagEntry_drvwr(SIE_FwrMtrFlagEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void FwrMtrFlagEntry_reset(SIE_FwrMtrFlagEntry *p);
SIGN32 FwrMtrFlagEntry_cmp  (SIE_FwrMtrFlagEntry *p, SIE_FwrMtrFlagEntry *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define FwrMtrFlagEntry_check(p,pie,pfx,hLOG) FwrMtrFlagEntry_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define FwrMtrFlagEntry_print(p,    pfx,hLOG) FwrMtrFlagEntry_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_Remap
#define h_Remap (){}
#define     RA_Remap_MtrTid                                0x0000
#define     RA_Remap_TsbMtrFlag                            0x0100
#define     RA_Remap_FwrMtrFlag                            0x0108
#define     RA_Remap_Isp0WMtrFlag                          0x0110
#define     RA_Remap_Isp0RMtrFlag                          0x0118
#define     RA_Remap_Isp1WMtrFlag                          0x0120
#define     RA_Remap_Isp1RMtrFlag                          0x0128
#define     RA_Remap_BCMMtrFlag                            0x0130
typedef struct SIE_Remap {
	SIE_TidMapEntry                                  ie_MtrTid[64];
	SIE_TsbMtrFlagEntry                              ie_TsbMtrFlag[2];
	SIE_FwrMtrFlagEntry                              ie_FwrMtrFlag[2];
	SIE_IspMtrFlagEntry                              ie_Isp0WMtrFlag[2];
	SIE_IspMtrFlagEntry                              ie_Isp0RMtrFlag[2];
	SIE_IspMtrFlagEntry                              ie_Isp1WMtrFlag[2];
	SIE_IspMtrFlagEntry                              ie_Isp1RMtrFlag[2];
#define     w32Remap_BCMMtrFlag                            {\
	UNSG32 uBCMMtrFlag_BCMMtrFlag                      :  1;\
	UNSG32 RSVDx130_b1                                 : 31;\
}
	union { UNSG32 u32Remap_BCMMtrFlag;
		struct w32Remap_BCMMtrFlag;
	};
} SIE_Remap;
typedef union  T32Remap_BCMMtrFlag
{ UNSG32 u32;
	struct w32Remap_BCMMtrFlag;
} T32Remap_BCMMtrFlag;
typedef union  TRemap_BCMMtrFlag
{ UNSG32 u32[1];
	struct {
		struct w32Remap_BCMMtrFlag;
	};
} TRemap_BCMMtrFlag;
SIGN32 Remap_drvrd(SIE_Remap *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 Remap_drvwr(SIE_Remap *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void Remap_reset(SIE_Remap *p);
SIGN32 Remap_cmp  (SIE_Remap *p, SIE_Remap *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define Remap_check(p,pie,pfx,hLOG) Remap_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define Remap_print(p,    pfx,hLOG) Remap_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MISC
#define h_MISC (){}
#define     RA_MISC_ALMSel                                 0x0000
typedef struct SIE_MISC {
#define     w32MISC_ALMSel                                 {\
	UNSG32 uALMSel_sel                                 :  3;\
	UNSG32 RSVDx0_b3                                   : 29;\
}
union { UNSG32 u32MISC_ALMSel;
	struct w32MISC_ALMSel;
};
} SIE_MISC;
typedef union  T32MISC_ALMSel
{ UNSG32 u32;
	struct w32MISC_ALMSel;
} T32MISC_ALMSel;
typedef union  TMISC_ALMSel
{ UNSG32 u32[1];
	struct {
		struct w32MISC_ALMSel;
	};
} TMISC_ALMSel;
SIGN32 MISC_drvrd(SIE_MISC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 MISC_drvwr(SIE_MISC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void MISC_reset(SIE_MISC *p);
SIGN32 MISC_cmp  (SIE_MISC *p, SIE_MISC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define MISC_check(p,pie,pfx,hLOG) MISC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define MISC_print(p,    pfx,hLOG) MISC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_Interrupt
#define h_Interrupt (){}
#define     RA_Interrupt_IntStatus                         0x0000
#define     RA_Interrupt_IntEnable                         0x0004
#define     RA_Interrupt_IntMask                           0x0008
#define     RA_Interrupt_IntMaskedStatus                   0x000C
#define     RA_Interrupt_BcmIntEnable                      0x0010
#define     RA_Interrupt_BcmInvalidAddr                    0x0014
#define     RA_Interrupt_BcmInvalidData                    0x0018
#define     RA_Interrupt_TSBIntSrcMask                     0x001C
#define     RA_Interrupt_FWRIntSrcMask                     0x0020
typedef struct SIE_Interrupt {
#define     w32Interrupt_IntStatus                         {\
	UNSG32 uIntStatus_mi_irq                           :  1;\
	UNSG32 uIntStatus_isp_irq                          :  1;\
	UNSG32 uIntStatus_cvp_eof_intr_0                   :  1;\
	UNSG32 uIntStatus_cvp_eof_intr_1                   :  1;\
	UNSG32 uIntStatus_dnscl_eof_intr_0                 :  1;\
	UNSG32 uIntStatus_dnscl_eof_intr_1                 :  1;\
	UNSG32 uIntStatus_dnscl_eof_intr_2                 :  1;\
	UNSG32 uIntStatus_tile_eof_intr                    :  1;\
	UNSG32 uIntStatus_rot_eof_intr                     :  1;\
	UNSG32 uIntStatus_dewarp_eof_intr                  :  1;\
	UNSG32 uIntStatus_bcm_invalid_intr                 :  1;\
	UNSG32 uIntStatus_mipi_intr_0                      :  1;\
	UNSG32 uIntStatus_mipi_intr_1                      :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_0                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_1                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_2                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_3                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_4                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_5                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_6                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_7                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_8                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_9                    :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_10                   :  1;\
	UNSG32 uIntStatus_FWR_ch_intr_11                   :  1;\
	UNSG32 uIntStatus_TSB_ch_intr_0                    :  1;\
	UNSG32 uIntStatus_TSB_ch_intr_1                    :  1;\
	UNSG32 uIntStatus_TSB_ch_intr_2                    :  1;\
	UNSG32 uIntStatus_TSB_ch_intr_3                    :  1;\
	UNSG32 uIntStatus_TSB_ch_intr_4                    :  1;\
	UNSG32 uIntStatus_TSB_ch_intr_5                    :  1;\
	UNSG32 uIntStatus_TSB_ch_intr_6                    :  1;\
}
union { UNSG32 u32Interrupt_IntStatus;
	struct w32Interrupt_IntStatus;
};
#define     w32Interrupt_IntEnable                         {\
	UNSG32 uIntEnable_mi_irq                           :  1;\
	UNSG32 uIntEnable_isp_irq                          :  1;\
	UNSG32 uIntEnable_cvp_eof_intr                     :  2;\
	UNSG32 uIntEnable_dnscl_eof_intr                   :  3;\
	UNSG32 uIntEnable_tile_eof_intr                    :  1;\
	UNSG32 uIntEnable_rot_eof_intr                     :  1;\
	UNSG32 uIntEnable_dewarp_eof_intr                  :  1;\
	UNSG32 uIntEnable_bcm_invalid_intr                 :  1;\
	UNSG32 uIntEnable_mipi_intr                        :  2;\
	UNSG32 uIntEnable_FWR_ch_intr                      : 12;\
	UNSG32 uIntEnable_TSB_ch_intr                      :  7;\
}
union { UNSG32 u32Interrupt_IntEnable;
	struct w32Interrupt_IntEnable;
};
#define     w32Interrupt_IntMask                           {\
	UNSG32 uIntMask_mi_irq                             :  1;\
	UNSG32 uIntMask_isp_irq                            :  1;\
	UNSG32 uIntMask_cvp_eof_intr                       :  2;\
	UNSG32 uIntMask_dnscl_eof_intr                     :  3;\
	UNSG32 uIntMask_tile_eof_intr                      :  1;\
	UNSG32 uIntMask_rot_eof_intr                       :  1;\
	UNSG32 uIntMask_dewarp_eof_intr                    :  1;\
	UNSG32 uIntMask_bcm_invalid_intr                   :  1;\
	UNSG32 uIntMask_mipi_intr                          :  2;\
	UNSG32 uIntMask_FWR_ch_intr                        : 12;\
	UNSG32 uIntMask_TSB_ch_intr                        :  7;\
}
union { UNSG32 u32Interrupt_IntMask;
	struct w32Interrupt_IntMask;
};
#define     w32Interrupt_IntMaskedStatus                   {\
	UNSG32 uIntMaskedStatus_mi_irq                     :  1;\
	UNSG32 uIntMaskedStatus_isp_irq                    :  1;\
	UNSG32 uIntMaskedStatus_cvp_eof_intr               :  2;\
	UNSG32 uIntMaskedStatus_dnscl_eof_intr             :  3;\
	UNSG32 uIntMaskedStatus_tile_eof_intr              :  1;\
	UNSG32 uIntMaskedStatus_rot_eof_intr               :  1;\
	UNSG32 uIntMaskedStatus_dewarp_eof_intr            :  1;\
	UNSG32 uIntMaskedStatus_bcm_invalid_intr           :  1;\
	UNSG32 uIntMaskedStatus_mipi_intr                  :  2;\
	UNSG32 uIntMaskedStatus_FWR_ch_intr                : 12;\
	UNSG32 uIntMaskedStatus_TSB_ch_intr                :  7;\
}
union { UNSG32 u32Interrupt_IntMaskedStatus;
	struct w32Interrupt_IntMaskedStatus;
};
#define     w32Interrupt_BcmIntEnable                      {\
	UNSG32 uBcmIntEnable_mi_irq                        :  1;\
	UNSG32 uBcmIntEnable_isp_irq                       :  1;\
	UNSG32 uBcmIntEnable_cvp_eof_intr                  :  2;\
	UNSG32 uBcmIntEnable_dnscl_eof_intr                :  3;\
	UNSG32 uBcmIntEnable_tile_eof_intr                 :  1;\
	UNSG32 uBcmIntEnable_rot_eof_intr                  :  1;\
	UNSG32 uBcmIntEnable_dewarp_eof_intr               :  1;\
	UNSG32 uBcmIntEnable_mipi_intr                     :  2;\
	UNSG32 uBcmIntEnable_FWR_ch_intr                   : 12;\
	UNSG32 uBcmIntEnable_TSB_ch_intr                   :  7;\
	UNSG32 RSVDx10_b31                                 :  1;\
}
union { UNSG32 u32Interrupt_BcmIntEnable;
	struct w32Interrupt_BcmIntEnable;
};
#define     w32Interrupt_BcmInvalidAddr                    {\
	UNSG32 uBcmInvalidAddr_address                     : 32;\
}
union { UNSG32 u32Interrupt_BcmInvalidAddr;
	struct w32Interrupt_BcmInvalidAddr;
};
#define     w32Interrupt_BcmInvalidData                    {\
	UNSG32 uBcmInvalidData_data                        : 32;\
}
union { UNSG32 u32Interrupt_BcmInvalidData;
	struct w32Interrupt_BcmInvalidData;
};
#define     w32Interrupt_TSBIntSrcMask                     {\
	UNSG32 uTSBIntSrcMask_mask                         : 16;\
	UNSG32 RSVDx1C_b16                                 : 16;\
}
union { UNSG32 u32Interrupt_TSBIntSrcMask;
	struct w32Interrupt_TSBIntSrcMask;
};
#define     w32Interrupt_FWRIntSrcMask                     {\
	UNSG32 uFWRIntSrcMask_mask                         : 16;\
	UNSG32 RSVDx20_b16                                 : 16;\
}
union { UNSG32 u32Interrupt_FWRIntSrcMask;
	struct w32Interrupt_FWRIntSrcMask;
};
} SIE_Interrupt;
typedef union  T32Interrupt_IntStatus
{ UNSG32 u32;
	struct w32Interrupt_IntStatus;
} T32Interrupt_IntStatus;
typedef union  T32Interrupt_IntEnable
{ UNSG32 u32;
	struct w32Interrupt_IntEnable;
} T32Interrupt_IntEnable;
typedef union  T32Interrupt_IntMask
{ UNSG32 u32;
	struct w32Interrupt_IntMask;
} T32Interrupt_IntMask;
typedef union  T32Interrupt_IntMaskedStatus
{ UNSG32 u32;
	struct w32Interrupt_IntMaskedStatus;
} T32Interrupt_IntMaskedStatus;
typedef union  T32Interrupt_BcmIntEnable
{ UNSG32 u32;
	struct w32Interrupt_BcmIntEnable;
} T32Interrupt_BcmIntEnable;
typedef union  T32Interrupt_BcmInvalidAddr
{ UNSG32 u32;
	struct w32Interrupt_BcmInvalidAddr;
} T32Interrupt_BcmInvalidAddr;
typedef union  T32Interrupt_BcmInvalidData
{ UNSG32 u32;
	struct w32Interrupt_BcmInvalidData;
} T32Interrupt_BcmInvalidData;
typedef union  T32Interrupt_TSBIntSrcMask
{ UNSG32 u32;
	struct w32Interrupt_TSBIntSrcMask;
} T32Interrupt_TSBIntSrcMask;
typedef union  T32Interrupt_FWRIntSrcMask
{ UNSG32 u32;
	struct w32Interrupt_FWRIntSrcMask;
} T32Interrupt_FWRIntSrcMask;
typedef union  TInterrupt_IntStatus
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_IntStatus;
	};
} TInterrupt_IntStatus;
typedef union  TInterrupt_IntEnable
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_IntEnable;
	};
} TInterrupt_IntEnable;
typedef union  TInterrupt_IntMask
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_IntMask;
	};
} TInterrupt_IntMask;
typedef union  TInterrupt_IntMaskedStatus
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_IntMaskedStatus;
	};
} TInterrupt_IntMaskedStatus;
typedef union  TInterrupt_BcmIntEnable
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_BcmIntEnable;
	};
} TInterrupt_BcmIntEnable;
typedef union  TInterrupt_BcmInvalidAddr
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_BcmInvalidAddr;
	};
} TInterrupt_BcmInvalidAddr;
typedef union  TInterrupt_BcmInvalidData
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_BcmInvalidData;
	};
} TInterrupt_BcmInvalidData;
typedef union  TInterrupt_TSBIntSrcMask
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_TSBIntSrcMask;
	};
} TInterrupt_TSBIntSrcMask;
typedef union  TInterrupt_FWRIntSrcMask
{ UNSG32 u32[1];
	struct {
		struct w32Interrupt_FWRIntSrcMask;
	};
} TInterrupt_FWRIntSrcMask;
SIGN32 Interrupt_drvrd(SIE_Interrupt *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 Interrupt_drvwr(SIE_Interrupt *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void Interrupt_reset(SIE_Interrupt *p);
SIGN32 Interrupt_cmp  (SIE_Interrupt *p, SIE_Interrupt *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define Interrupt_check(p,pie,pfx,hLOG) Interrupt_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define Interrupt_print(p,    pfx,hLOG) Interrupt_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_TxClkEsc
#define h_TxClkEsc (){}
#define     RA_TxClkEsc_Ctrl                               0x0000
typedef struct SIE_TxClkEsc {
#define     w32TxClkEsc_Ctrl                               {\
	UNSG32 uCtrl_clkEn                                 :  1;\
	UNSG32 uCtrl_divSel                                :  3;\
	UNSG32 RSVDx0_b4                                   : 28;\
}
union { UNSG32 u32TxClkEsc_Ctrl;
	struct w32TxClkEsc_Ctrl;
};
} SIE_TxClkEsc;
typedef union  T32TxClkEsc_Ctrl
{ UNSG32 u32;
	struct w32TxClkEsc_Ctrl;
} T32TxClkEsc_Ctrl;
typedef union  TTxClkEsc_Ctrl
{ UNSG32 u32[1];
	struct {
		struct w32TxClkEsc_Ctrl;
	};
} TTxClkEsc_Ctrl;
SIGN32 TxClkEsc_drvrd(SIE_TxClkEsc *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 TxClkEsc_drvwr(SIE_TxClkEsc *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void TxClkEsc_reset(SIE_TxClkEsc *p);
SIGN32 TxClkEsc_cmp  (SIE_TxClkEsc *p, SIE_TxClkEsc *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define TxClkEsc_check(p,pie,pfx,hLOG) TxClkEsc_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define TxClkEsc_print(p,    pfx,hLOG) TxClkEsc_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ISP2BCMIntCtrl
#define h_ISP2BCMIntCtrl (){}
#define     RA_ISP2BCMIntCtrl_Ctrl                         0x0000
#define     RA_ISP2BCMIntCtrl_Mask0                        0x0004
#define     RA_ISP2BCMIntCtrl_Mask1                        0x0008
#define     RA_ISP2BCMIntCtrl_Mask2                        0x000C
typedef struct SIE_ISP2BCMIntCtrl {
#define     w32ISP2BCMIntCtrl_Ctrl                         {\
	UNSG32 uCtrl_IntEn                                 :  3;\
	UNSG32 uCtrl_IntMode                               :  3;\
	UNSG32 RSVDx0_b6                                   : 26;\
}
union { UNSG32 u32ISP2BCMIntCtrl_Ctrl;
	struct w32ISP2BCMIntCtrl_Ctrl;
};
#define     w32ISP2BCMIntCtrl_Mask0                        {\
	UNSG32 uMask0_mask                                 : 32;\
}
union { UNSG32 u32ISP2BCMIntCtrl_Mask0;
	struct w32ISP2BCMIntCtrl_Mask0;
};
#define     w32ISP2BCMIntCtrl_Mask1                        {\
	UNSG32 uMask1_mask                                 : 32;\
}
union { UNSG32 u32ISP2BCMIntCtrl_Mask1;
	struct w32ISP2BCMIntCtrl_Mask1;
};
#define     w32ISP2BCMIntCtrl_Mask2                        {\
	UNSG32 uMask2_mask                                 : 32;\
}
union { UNSG32 u32ISP2BCMIntCtrl_Mask2;
	struct w32ISP2BCMIntCtrl_Mask2;
};
} SIE_ISP2BCMIntCtrl;
typedef union  T32ISP2BCMIntCtrl_Ctrl
{ UNSG32 u32;
	struct w32ISP2BCMIntCtrl_Ctrl;
} T32ISP2BCMIntCtrl_Ctrl;
typedef union  T32ISP2BCMIntCtrl_Mask0
{ UNSG32 u32;
	struct w32ISP2BCMIntCtrl_Mask0;
} T32ISP2BCMIntCtrl_Mask0;
typedef union  T32ISP2BCMIntCtrl_Mask1
{ UNSG32 u32;
	struct w32ISP2BCMIntCtrl_Mask1;
} T32ISP2BCMIntCtrl_Mask1;
typedef union  T32ISP2BCMIntCtrl_Mask2
{ UNSG32 u32;
	struct w32ISP2BCMIntCtrl_Mask2;
} T32ISP2BCMIntCtrl_Mask2;
typedef union  TISP2BCMIntCtrl_Ctrl
{ UNSG32 u32[1];
	struct {
		struct w32ISP2BCMIntCtrl_Ctrl;
	};
} TISP2BCMIntCtrl_Ctrl;
typedef union  TISP2BCMIntCtrl_Mask0
{ UNSG32 u32[1];
	struct {
		struct w32ISP2BCMIntCtrl_Mask0;
	};
} TISP2BCMIntCtrl_Mask0;
typedef union  TISP2BCMIntCtrl_Mask1
{ UNSG32 u32[1];
	struct {
		struct w32ISP2BCMIntCtrl_Mask1;
	};
} TISP2BCMIntCtrl_Mask1;
typedef union  TISP2BCMIntCtrl_Mask2
{ UNSG32 u32[1];
	struct {
		struct w32ISP2BCMIntCtrl_Mask2;
	};
} TISP2BCMIntCtrl_Mask2;
SIGN32 ISP2BCMIntCtrl_drvrd(SIE_ISP2BCMIntCtrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 ISP2BCMIntCtrl_drvwr(SIE_ISP2BCMIntCtrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void ISP2BCMIntCtrl_reset(SIE_ISP2BCMIntCtrl *p);
SIGN32 ISP2BCMIntCtrl_cmp  (SIE_ISP2BCMIntCtrl *p, SIE_ISP2BCMIntCtrl *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define ISP2BCMIntCtrl_check(p,pie,pfx,hLOG) ISP2BCMIntCtrl_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define ISP2BCMIntCtrl_print(p,    pfx,hLOG) ISP2BCMIntCtrl_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_IspMISC
#define h_IspMISC (){}
#define     RA_IspMISC_security                            0x0000
#define     RA_IspMISC_clkrst                              0x000C
#define     RA_IspMISC_remap                               0x0060
#define     RA_IspMISC_interrupt                           0x0194
#define     RA_IspMISC_misc                                0x01B8
#define     RA_IspMISC_txclk                               0x01BC
#define     RA_IspMISC_ALMPwrCtrl                          0x01C0
#define     RA_IspMISC_MTRPwrCtrl                          0x01C4
#define     RA_IspMISC_TSBPwrCtrl                          0x01C8
#define     RA_IspMISC_FWRPwrCtrl                          0x01CC
#define     RA_IspMISC_CSI0PwrCtrl                         0x01D0
#define     RA_IspMISC_CSI1PwrCtrl                         0x01D4
#define     RA_IspMISC_StitchPwrCtrl                       0x01D8
#define     RA_IspMISC_TdnrPwrCtrl                         0x01DC
#define     RA_IspMISC_IsppipePwrCtrl                      0x01E0
#define     RA_IspMISC_TilePwrCtrl                         0x01E4
#define     RA_IspMISC_DwrpPwrCtrl                         0x01E8
#define     RA_IspMISC_RotPwrCtrl                          0x01EC
#define     RA_IspMISC_DscalPwrCtrl                        0x01F0
#define     RA_IspMISC_CvpPwrCtrl                          0x01F4
#define     RA_IspMISC_rwtc                                0x01F8
#define     RA_IspMISC_Isp2BcmIrq                          0x0204
typedef struct SIE_IspMISC {
	SIE_SecCtrl                                      ie_security;
	SIE_CLKRST                                       ie_clkrst;
	SIE_Remap                                        ie_remap;
	SIE_Interrupt                                    ie_interrupt;
	SIE_MISC                                         ie_misc;
	SIE_TxClkEsc                                     ie_txclk;
	SIE_SRAMPWR                                      ie_ALMPwrCtrl;
	SIE_SRAMPWR                                      ie_MTRPwrCtrl;
	SIE_SRAMPWR                                      ie_TSBPwrCtrl;
	SIE_SRAMPWR                                      ie_FWRPwrCtrl;
	SIE_SRAMPWR                                      ie_CSI0PwrCtrl;
	SIE_SRAMPWR                                      ie_CSI1PwrCtrl;
	SIE_SRAMPWR                                      ie_StitchPwrCtrl;
	SIE_SRAMPWR                                      ie_TdnrPwrCtrl;
	SIE_SRAMPWR                                      ie_IsppipePwrCtrl;
	SIE_SRAMPWR                                      ie_TilePwrCtrl;
	SIE_SRAMPWR                                      ie_DwrpPwrCtrl;
	SIE_SRAMPWR                                      ie_RotPwrCtrl;
	SIE_SRAMPWR                                      ie_DscalPwrCtrl;
	SIE_SRAMPWR                                      ie_CvpPwrCtrl;
	SIE_SRAMRWTC                                     ie_rwtc;
	SIE_ISP2BCMIntCtrl                               ie_Isp2BcmIrq;
} SIE_IspMISC;
SIGN32 IspMISC_drvrd(SIE_IspMISC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
SIGN32 IspMISC_drvwr(SIE_IspMISC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
void IspMISC_reset(SIE_IspMISC *p);
SIGN32 IspMISC_cmp  (SIE_IspMISC *p, SIE_IspMISC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
#define IspMISC_check(p,pie,pfx,hLOG) IspMISC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
#define IspMISC_print(p,    pfx,hLOG) IspMISC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
}
#endif
#pragma  pack()
#endif
