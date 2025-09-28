#ifndef aio_h
#define aio_h (){}
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
#ifndef h_PRIAUD
#define h_PRIAUD (){}
    #define     RA_PRIAUD_CLKDIV                               0x0000
    #define        PRIAUD_CLKDIV_SETTING_DIV1                               0x0
    #define        PRIAUD_CLKDIV_SETTING_DIV2                               0x1
    #define        PRIAUD_CLKDIV_SETTING_DIV4                               0x2
    #define        PRIAUD_CLKDIV_SETTING_DIV8                               0x3
    #define        PRIAUD_CLKDIV_SETTING_DIV16                              0x4
    #define        PRIAUD_CLKDIV_SETTING_DIV32                              0x5
    #define        PRIAUD_CLKDIV_SETTING_DIV64                              0x6
    #define        PRIAUD_CLKDIV_SETTING_DIV128                             0x7
    #define        PRIAUD_CLKDIV_SETTING_DIV256                             0x8
    #define        PRIAUD_CLKDIV_SETTING_DIV512                             0x9
    #define        PRIAUD_CLKDIV_SETTING_DIV1024                            0xA
    #define     RA_PRIAUD_CTRL                                 0x0004
    #define        PRIAUD_CTRL_LEFTJFY_LEFT                                 0x0
    #define        PRIAUD_CTRL_LEFTJFY_RIGHT                                0x1
    #define        PRIAUD_CTRL_INVCLK_NORMAL                                0x0
    #define        PRIAUD_CTRL_INVCLK_INVERTED                              0x1
    #define        PRIAUD_CTRL_INVFS_NORMAL                                 0x0
    #define        PRIAUD_CTRL_INVFS_INVERTED                               0x1
    #define        PRIAUD_CTRL_INVCLKMS_NORMAL                              0x0
    #define        PRIAUD_CTRL_INVCLKMS_INVERTED                            0x1
    #define        PRIAUD_CTRL_INVFSMS_NORMAL                               0x0
    #define        PRIAUD_CTRL_INVFSMS_INVERTED                             0x1
    #define        PRIAUD_CTRL_TLSB_MSB_FIRST                               0x0
    #define        PRIAUD_CTRL_TLSB_LSB_FIRST                               0x1
    #define        PRIAUD_CTRL_TDM_16DFM                                    0x0
    #define        PRIAUD_CTRL_TDM_18DFM                                    0x1
    #define        PRIAUD_CTRL_TDM_20DFM                                    0x2
    #define        PRIAUD_CTRL_TDM_24DFM                                    0x3
    #define        PRIAUD_CTRL_TDM_32DFM                                    0x4
    #define        PRIAUD_CTRL_TDM_8DFM                                     0x5
    #define        PRIAUD_CTRL_TCF_16CFM                                    0x0
    #define        PRIAUD_CTRL_TCF_24CFM                                    0x1
    #define        PRIAUD_CTRL_TCF_32CFM                                    0x2
    #define        PRIAUD_CTRL_TCF_8CFM                                     0x3
    #define        PRIAUD_CTRL_TFM_JUSTIFIED                                0x1
    #define        PRIAUD_CTRL_TFM_I2S                                      0x2
    #define        PRIAUD_CTRL_TDMMODE_TDMMODE_OFF                          0x0
    #define        PRIAUD_CTRL_TDMMODE_TDMMODE_ON                           0x1
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_2                             0x0
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_4                             0x1
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_6                             0x2
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_8                             0x3
    #define        PRIAUD_CTRL_TDMCHCNT_Reserved                            0x4
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_16                            0x5
    #define     RA_PRIAUD_CTRL1                                0x0008
    typedef struct SIE_PRIAUD {
    #define     w32PRIAUD_CLKDIV                               {\
            UNSG32 uCLKDIV_SETTING                             :  4;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32PRIAUD_CLKDIV;
            struct w32PRIAUD_CLKDIV;
          };
    #define     w32PRIAUD_CTRL                                 {\
            UNSG32 uCTRL_LEFTJFY                               :  1;\
            UNSG32 uCTRL_INVCLK                                :  1;\
            UNSG32 uCTRL_INVFS                                 :  1;\
            UNSG32 uCTRL_INVCLKMS                              :  1;\
            UNSG32 uCTRL_INVFSMS                               :  1;\
            UNSG32 uCTRL_TLSB                                  :  1;\
            UNSG32 uCTRL_TDM                                   :  3;\
            UNSG32 uCTRL_TCF                                   :  3;\
            UNSG32 uCTRL_TFM                                   :  2;\
            UNSG32 uCTRL_TDMMODE                               :  1;\
            UNSG32 uCTRL_TDMCHCNT                              :  3;\
            UNSG32 uCTRL_TDMWSHIGH                             :  9;\
            UNSG32 RSVDx4_b27                                  :  5;\
          }
    union { UNSG32 u32PRIAUD_CTRL;
            struct w32PRIAUD_CTRL;
          };
    #define     w32PRIAUD_CTRL1                                {\
            UNSG32 uCTRL1_TCF_MANUAL                           :  8;\
            UNSG32 uCTRL1_TCF_MAN_MAR                          :  3;\
            UNSG32 uCTRL1_TDM_MANUAL                           :  8;\
            UNSG32 uCTRL1_PCM_MONO_CH                          :  1;\
            UNSG32 RSVDx8_b20                                  : 12;\
          }
    union { UNSG32 u32PRIAUD_CTRL1;
            struct w32PRIAUD_CTRL1;
          };
    } SIE_PRIAUD;
    typedef union  T32PRIAUD_CLKDIV
          { UNSG32 u32;
            struct w32PRIAUD_CLKDIV;
                 } T32PRIAUD_CLKDIV;
    typedef union  T32PRIAUD_CTRL
          { UNSG32 u32;
            struct w32PRIAUD_CTRL;
                 } T32PRIAUD_CTRL;
    typedef union  T32PRIAUD_CTRL1
          { UNSG32 u32;
            struct w32PRIAUD_CTRL1;
                 } T32PRIAUD_CTRL1;
    typedef union  TPRIAUD_CLKDIV
          { UNSG32 u32[1];
            struct {
            struct w32PRIAUD_CLKDIV;
                   };
                 } TPRIAUD_CLKDIV;
    typedef union  TPRIAUD_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32PRIAUD_CTRL;
                   };
                 } TPRIAUD_CTRL;
    typedef union  TPRIAUD_CTRL1
          { UNSG32 u32[1];
            struct {
            struct w32PRIAUD_CTRL1;
                   };
                 } TPRIAUD_CTRL1;
     SIGN32 PRIAUD_drvrd(SIE_PRIAUD *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 PRIAUD_drvwr(SIE_PRIAUD *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void PRIAUD_reset(SIE_PRIAUD *p);
     SIGN32 PRIAUD_cmp  (SIE_PRIAUD *p, SIE_PRIAUD *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define PRIAUD_check(p,pie,pfx,hLOG) PRIAUD_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define PRIAUD_print(p,    pfx,hLOG) PRIAUD_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AUDCH
#define h_AUDCH (){}
    #define     RA_AUDCH_CTRL                                  0x0000
    #define        AUDCH_CTRL_ENABLE_DISABLE                                0x0
    #define        AUDCH_CTRL_ENABLE_ENABLE                                 0x1
    #define        AUDCH_CTRL_MUTE_MUTE_OFF                                 0x0
    #define        AUDCH_CTRL_MUTE_MUTE_ON                                  0x1
    #define        AUDCH_CTRL_LRSWITCH_SWITCH_OFF                           0x0
    #define        AUDCH_CTRL_LRSWITCH_SWITCH_ON                            0x1
    #define        AUDCH_CTRL_DEBUGEN_DISABLE                               0x0
    #define        AUDCH_CTRL_DEBUGEN_ENABLE                                0x1
    #define        AUDCH_CTRL_FLUSH_ON                                      0x1
    #define        AUDCH_CTRL_FLUSH_OFF                                     0x0
    typedef struct SIE_AUDCH {
    #define     w32AUDCH_CTRL                                  {\
            UNSG32 uCTRL_ENABLE                                :  1;\
            UNSG32 uCTRL_MUTE                                  :  1;\
            UNSG32 uCTRL_LRSWITCH                              :  1;\
            UNSG32 uCTRL_DEBUGEN                               :  1;\
            UNSG32 uCTRL_FLUSH                                 :  1;\
            UNSG32 RSVDx0_b5                                   : 27;\
          }
    union { UNSG32 u32AUDCH_CTRL;
            struct w32AUDCH_CTRL;
          };
    } SIE_AUDCH;
    typedef union  T32AUDCH_CTRL
          { UNSG32 u32;
            struct w32AUDCH_CTRL;
                 } T32AUDCH_CTRL;
    typedef union  TAUDCH_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AUDCH_CTRL;
                   };
                 } TAUDCH_CTRL;
     SIGN32 AUDCH_drvrd(SIE_AUDCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AUDCH_drvwr(SIE_AUDCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AUDCH_reset(SIE_AUDCH *p);
     SIGN32 AUDCH_cmp  (SIE_AUDCH *p, SIE_AUDCH *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AUDCH_check(p,pie,pfx,hLOG) AUDCH_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AUDCH_print(p,    pfx,hLOG) AUDCH_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DBG_TX
#define h_DBG_TX (){}
    #define     RA_DBG_TX_DEBUGHI                              0x0000
    #define     RA_DBG_TX_DEBUGLO                              0x0004
    typedef struct SIE_DBG_TX {
    #define     w32DBG_TX_DEBUGHI                              {\
            UNSG32 uDEBUGHI_DATAHI                             : 32;\
          }
    union { UNSG32 u32DBG_TX_DEBUGHI;
            struct w32DBG_TX_DEBUGHI;
          };
    #define     w32DBG_TX_DEBUGLO                              {\
            UNSG32 uDEBUGLO_DATALO                             : 32;\
          }
    union { UNSG32 u32DBG_TX_DEBUGLO;
            struct w32DBG_TX_DEBUGLO;
          };
    } SIE_DBG_TX;
    typedef union  T32DBG_TX_DEBUGHI
          { UNSG32 u32;
            struct w32DBG_TX_DEBUGHI;
                 } T32DBG_TX_DEBUGHI;
    typedef union  T32DBG_TX_DEBUGLO
          { UNSG32 u32;
            struct w32DBG_TX_DEBUGLO;
                 } T32DBG_TX_DEBUGLO;
    typedef union  TDBG_TX_DEBUGHI
          { UNSG32 u32[1];
            struct {
            struct w32DBG_TX_DEBUGHI;
                   };
                 } TDBG_TX_DEBUGHI;
    typedef union  TDBG_TX_DEBUGLO
          { UNSG32 u32[1];
            struct {
            struct w32DBG_TX_DEBUGLO;
                   };
                 } TDBG_TX_DEBUGLO;
     SIGN32 DBG_TX_drvrd(SIE_DBG_TX *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DBG_TX_drvwr(SIE_DBG_TX *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DBG_TX_reset(SIE_DBG_TX *p);
     SIGN32 DBG_TX_cmp  (SIE_DBG_TX *p, SIE_DBG_TX *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DBG_TX_check(p,pie,pfx,hLOG) DBG_TX_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DBG_TX_print(p,    pfx,hLOG) DBG_TX_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DBG_RX
#define h_DBG_RX (){}
    #define     RA_DBG_RX_DBG                                  0x0000
    #define     RA_DBG_RX_DEBUGHI                              0x0004
    #define     RA_DBG_RX_DEBUGLO                              0x0008
    typedef struct SIE_DBG_RX {
    #define     w32DBG_RX_DBG                                  {\
            UNSG32 uDBG_PORT_SEL                               :  2;\
            UNSG32 RSVDx0_b2                                   : 30;\
          }
    union { UNSG32 u32DBG_RX_DBG;
            struct w32DBG_RX_DBG;
          };
    #define     w32DBG_RX_DEBUGHI                              {\
            UNSG32 uDEBUGHI_DATAHI                             : 32;\
          }
    union { UNSG32 u32DBG_RX_DEBUGHI;
            struct w32DBG_RX_DEBUGHI;
          };
    #define     w32DBG_RX_DEBUGLO                              {\
            UNSG32 uDEBUGLO_DATALO                             : 32;\
          }
    union { UNSG32 u32DBG_RX_DEBUGLO;
            struct w32DBG_RX_DEBUGLO;
          };
    } SIE_DBG_RX;
    typedef union  T32DBG_RX_DBG
          { UNSG32 u32;
            struct w32DBG_RX_DBG;
                 } T32DBG_RX_DBG;
    typedef union  T32DBG_RX_DEBUGHI
          { UNSG32 u32;
            struct w32DBG_RX_DEBUGHI;
                 } T32DBG_RX_DEBUGHI;
    typedef union  T32DBG_RX_DEBUGLO
          { UNSG32 u32;
            struct w32DBG_RX_DEBUGLO;
                 } T32DBG_RX_DEBUGLO;
    typedef union  TDBG_RX_DBG
          { UNSG32 u32[1];
            struct {
            struct w32DBG_RX_DBG;
                   };
                 } TDBG_RX_DBG;
    typedef union  TDBG_RX_DEBUGHI
          { UNSG32 u32[1];
            struct {
            struct w32DBG_RX_DEBUGHI;
                   };
                 } TDBG_RX_DEBUGHI;
    typedef union  TDBG_RX_DEBUGLO
          { UNSG32 u32[1];
            struct {
            struct w32DBG_RX_DEBUGLO;
                   };
                 } TDBG_RX_DEBUGLO;
     SIGN32 DBG_RX_drvrd(SIE_DBG_RX *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DBG_RX_drvwr(SIE_DBG_RX *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DBG_RX_reset(SIE_DBG_RX *p);
     SIGN32 DBG_RX_cmp  (SIE_DBG_RX *p, SIE_DBG_RX *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DBG_RX_check(p,pie,pfx,hLOG) DBG_RX_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DBG_RX_print(p,    pfx,hLOG) DBG_RX_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ACLK
#define h_ACLK (){}
    #define     RA_ACLK_ACLK_CTRL                              0x0000
    #define        ACLK_ACLK_CTRL_clk_Enable_DISABLE                        0x0
    #define        ACLK_ACLK_CTRL_clk_Enable_ENABLE                         0x1
    #define        ACLK_ACLK_CTRL_apll0_clk_sel_PLLOUT0                     0x0
    #define        ACLK_ACLK_CTRL_apll0_clk_sel_PLLOUT0F                    0x1
    #define        ACLK_ACLK_CTRL_apll1_clk_sel_PLLOUT1                     0x0
    #define        ACLK_ACLK_CTRL_apll1_clk_sel_PLLOUT1F                    0x1
    #define        ACLK_ACLK_CTRL_src_sel_AVPLL_A3                          0x0
    #define        ACLK_ACLK_CTRL_src_sel_AVPLL_A4                          0x1
    #define        ACLK_ACLK_CTRL_src_sel_I2S1_MCLK                         0x2
    #define        ACLK_ACLK_CTRL_src_sel_I2S2_MCLK                         0x3
    #define        ACLK_ACLK_CTRL_sw_sync_rst_ASSERT                        0x0
    #define        ACLK_ACLK_CTRL_sw_sync_rst_DEASSERT                      0x1
    typedef struct SIE_ACLK {
    #define     w32ACLK_ACLK_CTRL                              {\
            UNSG32 uACLK_CTRL_clk_Enable                       :  1;\
            UNSG32 uACLK_CTRL_apll0_clk_sel                    :  1;\
            UNSG32 uACLK_CTRL_apll1_clk_sel                    :  1;\
            UNSG32 uACLK_CTRL_src_sel                          :  2;\
            UNSG32 uACLK_CTRL_div_val                          :  6;\
            UNSG32 uACLK_CTRL_sw_sync_rst                      :  1;\
            UNSG32 RSVDx0_b12                                  : 20;\
          }
    union { UNSG32 u32ACLK_ACLK_CTRL;
            struct w32ACLK_ACLK_CTRL;
          };
    } SIE_ACLK;
    typedef union  T32ACLK_ACLK_CTRL
          { UNSG32 u32;
            struct w32ACLK_ACLK_CTRL;
                 } T32ACLK_ACLK_CTRL;
    typedef union  TACLK_ACLK_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32ACLK_ACLK_CTRL;
                   };
                 } TACLK_ACLK_CTRL;
     SIGN32 ACLK_drvrd(SIE_ACLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 ACLK_drvwr(SIE_ACLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void ACLK_reset(SIE_ACLK *p);
     SIGN32 ACLK_cmp  (SIE_ACLK *p, SIE_ACLK *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define ACLK_check(p,pie,pfx,hLOG) ACLK_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define ACLK_print(p,    pfx,hLOG) ACLK_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DMIC_CLK
#define h_DMIC_CLK (){}
    #define     RA_DMIC_CLK_DMIC_Reset                         0x0000
    #define     RA_DMIC_CLK_DMIC_Core_Clock_Config             0x0004
    #define     RA_DMIC_CLK_DMIC_EXTIFABC_Clock_Config         0x0008
    #define     RA_DMIC_CLK_DMIC_EXTIFD_Clock_Config           0x000C
    typedef struct SIE_DMIC_CLK {
    #define     w32DMIC_CLK_DMIC_Reset                         {\
            UNSG32 uDMIC_Reset_Reset                           :  1;\
            UNSG32 RSVDx0_b1                                   : 31;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_Reset;
            struct w32DMIC_CLK_DMIC_Reset;
          };
    #define     w32DMIC_CLK_DMIC_Core_Clock_Config             {\
            UNSG32 uDMIC_Core_Clock_Config_Source_Sel          :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Source_En           :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Divider             : 10;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_A            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_B            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_C            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_D            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_All          :  1;\
            UNSG32 RSVDx4_b17                                  : 15;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_Core_Clock_Config;
            struct w32DMIC_CLK_DMIC_Core_Clock_Config;
          };
    #define     w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config         {\
            UNSG32 uDMIC_EXTIFABC_Clock_Config_DividerM        : 10;\
            UNSG32 uDMIC_EXTIFABC_Clock_Config_DividerN        :  5;\
            UNSG32 uDMIC_EXTIFABC_Clock_Config_DividerP        :  5;\
            UNSG32 RSVDx8_b20                                  : 12;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
            struct w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
          };
    #define     w32DMIC_CLK_DMIC_EXTIFD_Clock_Config           {\
            UNSG32 uDMIC_EXTIFD_Clock_Config_DividerM          : 10;\
            UNSG32 uDMIC_EXTIFD_Clock_Config_DividerN          :  5;\
            UNSG32 uDMIC_EXTIFD_Clock_Config_DividerP          :  5;\
            UNSG32 uDMIC_EXTIFD_Clock_Config_ABCClk_Select     :  1;\
            UNSG32 RSVDxC_b21                                  : 11;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
            struct w32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
          };
    } SIE_DMIC_CLK;
    typedef union  T32DMIC_CLK_DMIC_Reset
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_Reset;
                 } T32DMIC_CLK_DMIC_Reset;
    typedef union  T32DMIC_CLK_DMIC_Core_Clock_Config
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_Core_Clock_Config;
                 } T32DMIC_CLK_DMIC_Core_Clock_Config;
    typedef union  T32DMIC_CLK_DMIC_EXTIFABC_Clock_Config
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
                 } T32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
    typedef union  T32DMIC_CLK_DMIC_EXTIFD_Clock_Config
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
                 } T32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
    typedef union  TDMIC_CLK_DMIC_Reset
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_Reset;
                   };
                 } TDMIC_CLK_DMIC_Reset;
    typedef union  TDMIC_CLK_DMIC_Core_Clock_Config
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_Core_Clock_Config;
                   };
                 } TDMIC_CLK_DMIC_Core_Clock_Config;
    typedef union  TDMIC_CLK_DMIC_EXTIFABC_Clock_Config
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
                   };
                 } TDMIC_CLK_DMIC_EXTIFABC_Clock_Config;
    typedef union  TDMIC_CLK_DMIC_EXTIFD_Clock_Config
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
                   };
                 } TDMIC_CLK_DMIC_EXTIFD_Clock_Config;
     SIGN32 DMIC_CLK_drvrd(SIE_DMIC_CLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DMIC_CLK_drvwr(SIE_DMIC_CLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DMIC_CLK_reset(SIE_DMIC_CLK *p);
     SIGN32 DMIC_CLK_cmp  (SIE_DMIC_CLK *p, SIE_DMIC_CLK *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DMIC_CLK_check(p,pie,pfx,hLOG) DMIC_CLK_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DMIC_CLK_print(p,    pfx,hLOG) DMIC_CLK_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SEC
#define h_SEC (){}
    #define     RA_SEC_SECAUD                                  0x0000
    #define     RA_SEC_TSD0_SEC                                0x000C
    #define     RA_SEC_DBG                                     0x0010
    #define     RA_SEC_SECPORT                                 0x0018
    #define        SEC_SECPORT_ENABLE_DISABLE                               0x0
    #define        SEC_SECPORT_ENABLE_ENABLE                                0x1
    #define     RA_SEC_APRKT                                   0x001C
    #define     RA_SEC_INT                                     0x0020
    #define     RA_SEC_PSC                                     0x0024
    #define     RA_SEC_UFCNT                                   0x0028
    typedef struct SIE_SEC {
              SIE_PRIAUD                                       ie_SECAUD;
              SIE_AUDCH                                        ie_TSD0_SEC;
              SIE_DBG_TX                                       ie_DBG;
    #define     w32SEC_SECPORT                                 {\
            UNSG32 uSECPORT_ENABLE                             :  1;\
            UNSG32 uSECPORT_DHUB_FLS_ON                        :  1;\
            UNSG32 uSECPORT_APRKT_ON                           :  1;\
            UNSG32 uSECPORT_APRKT_ABSO                         :  1;\
            UNSG32 uSECPORT_APRKT_RELA                         :  1;\
            UNSG32 uSECPORT_APRKT_ASAP                         :  1;\
            UNSG32 uSECPORT_MODE                               :  2;\
            UNSG32 uSECPORT_APRPR_ON                           :  1;\
            UNSG32 uSECPORT_FIFO_CG_ON                         :  1;\
            UNSG32 RSVDx18_b10                                 : 22;\
          }
    union { UNSG32 u32SEC_SECPORT;
            struct w32SEC_SECPORT;
          };
    #define     w32SEC_APRKT                                   {\
            UNSG32 uAPRKT_VAL                                  : 32;\
          }
    union { UNSG32 u32SEC_APRKT;
            struct w32SEC_APRKT;
          };
    #define     w32SEC_INT                                     {\
            UNSG32 uINT_SCNT                                   : 32;\
          }
    union { UNSG32 u32SEC_INT;
            struct w32SEC_INT;
          };
    #define     w32SEC_PSC                                     {\
            UNSG32 uPSC_VAL                                    : 32;\
          }
    union { UNSG32 u32SEC_PSC;
            struct w32SEC_PSC;
          };
    #define     w32SEC_UFCNT                                   {\
            UNSG32 uUFCNT_VAL                                  : 16;\
            UNSG32 RSVDx28_b16                                 : 16;\
          }
    union { UNSG32 u32SEC_UFCNT;
            struct w32SEC_UFCNT;
          };
    } SIE_SEC;
    typedef union  T32SEC_SECPORT
          { UNSG32 u32;
            struct w32SEC_SECPORT;
                 } T32SEC_SECPORT;
    typedef union  T32SEC_APRKT
          { UNSG32 u32;
            struct w32SEC_APRKT;
                 } T32SEC_APRKT;
    typedef union  T32SEC_INT
          { UNSG32 u32;
            struct w32SEC_INT;
                 } T32SEC_INT;
    typedef union  T32SEC_PSC
          { UNSG32 u32;
            struct w32SEC_PSC;
                 } T32SEC_PSC;
    typedef union  T32SEC_UFCNT
          { UNSG32 u32;
            struct w32SEC_UFCNT;
                 } T32SEC_UFCNT;
    typedef union  TSEC_SECPORT
          { UNSG32 u32[1];
            struct {
            struct w32SEC_SECPORT;
                   };
                 } TSEC_SECPORT;
    typedef union  TSEC_APRKT
          { UNSG32 u32[1];
            struct {
            struct w32SEC_APRKT;
                   };
                 } TSEC_APRKT;
    typedef union  TSEC_INT
          { UNSG32 u32[1];
            struct {
            struct w32SEC_INT;
                   };
                 } TSEC_INT;
    typedef union  TSEC_PSC
          { UNSG32 u32[1];
            struct {
            struct w32SEC_PSC;
                   };
                 } TSEC_PSC;
    typedef union  TSEC_UFCNT
          { UNSG32 u32[1];
            struct {
            struct w32SEC_UFCNT;
                   };
                 } TSEC_UFCNT;
     SIGN32 SEC_drvrd(SIE_SEC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SEC_drvwr(SIE_SEC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SEC_reset(SIE_SEC *p);
     SIGN32 SEC_cmp  (SIE_SEC *p, SIE_SEC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SEC_check(p,pie,pfx,hLOG) SEC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SEC_print(p,    pfx,hLOG) SEC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MIC2
#define h_MIC2 (){}
    #define     RA_MIC2_MICCTRL                                0x0000
    #define     RA_MIC2_RSD0                                   0x000C
    #define     RA_MIC2_DBG                                    0x0010
    #define     RA_MIC2_MM_MODE                                0x001C
    #define     RA_MIC2_RXPORT                                 0x0020
    #define        MIC2_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC2_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC2_INT                                    0x0024
    #define     RA_MIC2_APRKT                                  0x0028
    #define     RA_MIC2_PSC                                    0x002C
    typedef struct SIE_MIC2 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
    #define     w32MIC2_MM_MODE                                {\
            UNSG32 uMM_MODE_WS_HIGH_PRD                        :  9;\
            UNSG32 uMM_MODE_WS_TOTAL_PRD                       :  9;\
            UNSG32 uMM_MODE_WS_INV                             :  1;\
            UNSG32 RSVDx1C_b19                                 : 13;\
          }
    union { UNSG32 u32MIC2_MM_MODE;
            struct w32MIC2_MM_MODE;
          };
    #define     w32MIC2_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 uRXPORT_APRKT_ON                            :  1;\
            UNSG32 uRXPORT_APRKT_ABSO                          :  1;\
            UNSG32 uRXPORT_APRKT_RELA                          :  1;\
            UNSG32 uRXPORT_APRKT_ASAP                          :  1;\
            UNSG32 uRXPORT_MODE                                :  2;\
            UNSG32 uRXPORT_ILB                                 :  1;\
            UNSG32 uRXPORT_LBD                                 :  2;\
            UNSG32 uRXPORT_APRPR_ON                            :  1;\
            UNSG32 uRXPORT_sw_sync_rst                         :  1;\
            UNSG32 RSVDx20_b12                                 : 20;\
          }
    union { UNSG32 u32MIC2_RXPORT;
            struct w32MIC2_RXPORT;
          };
    #define     w32MIC2_INT                                    {\
            UNSG32 uINT_SCNT                                   : 32;\
          }
    union { UNSG32 u32MIC2_INT;
            struct w32MIC2_INT;
          };
    #define     w32MIC2_APRKT                                  {\
            UNSG32 uAPRKT_VAL                                  : 32;\
          }
    union { UNSG32 u32MIC2_APRKT;
            struct w32MIC2_APRKT;
          };
    #define     w32MIC2_PSC                                    {\
            UNSG32 uPSC_VAL                                    : 32;\
          }
    union { UNSG32 u32MIC2_PSC;
            struct w32MIC2_PSC;
          };
    } SIE_MIC2;
    typedef union  T32MIC2_MM_MODE
          { UNSG32 u32;
            struct w32MIC2_MM_MODE;
                 } T32MIC2_MM_MODE;
    typedef union  T32MIC2_RXPORT
          { UNSG32 u32;
            struct w32MIC2_RXPORT;
                 } T32MIC2_RXPORT;
    typedef union  T32MIC2_INT
          { UNSG32 u32;
            struct w32MIC2_INT;
                 } T32MIC2_INT;
    typedef union  T32MIC2_APRKT
          { UNSG32 u32;
            struct w32MIC2_APRKT;
                 } T32MIC2_APRKT;
    typedef union  T32MIC2_PSC
          { UNSG32 u32;
            struct w32MIC2_PSC;
                 } T32MIC2_PSC;
    typedef union  TMIC2_MM_MODE
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_MM_MODE;
                   };
                 } TMIC2_MM_MODE;
    typedef union  TMIC2_RXPORT
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_RXPORT;
                   };
                 } TMIC2_RXPORT;
    typedef union  TMIC2_INT
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_INT;
                   };
                 } TMIC2_INT;
    typedef union  TMIC2_APRKT
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_APRKT;
                   };
                 } TMIC2_APRKT;
    typedef union  TMIC2_PSC
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_PSC;
                   };
                 } TMIC2_PSC;
     SIGN32 MIC2_drvrd(SIE_MIC2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 MIC2_drvwr(SIE_MIC2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void MIC2_reset(SIE_MIC2 *p);
     SIGN32 MIC2_cmp  (SIE_MIC2 *p, SIE_MIC2 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define MIC2_check(p,pie,pfx,hLOG) MIC2_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define MIC2_print(p,    pfx,hLOG) MIC2_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_PDMCH
#define h_PDMCH (){}
    #define     RA_PDMCH_CTRL                                  0x0000
    #define        PDMCH_CTRL_ENABLE_DISABLE                                0x0
    #define        PDMCH_CTRL_ENABLE_ENABLE                                 0x1
    #define        PDMCH_CTRL_MUTE_MUTE_OFF                                 0x0
    #define        PDMCH_CTRL_MUTE_MUTE_ON                                  0x1
    #define        PDMCH_CTRL_LRSWITCH_SWITCH_OFF                           0x0
    #define        PDMCH_CTRL_LRSWITCH_SWITCH_ON                            0x1
    #define        PDMCH_CTRL_FLUSH_ON                                      0x1
    #define        PDMCH_CTRL_FLUSH_OFF                                     0x0
    #define     RA_PDMCH_CTRL2                                 0x0004
    typedef struct SIE_PDMCH {
    #define     w32PDMCH_CTRL                                  {\
            UNSG32 uCTRL_ENABLE                                :  1;\
            UNSG32 uCTRL_MUTE                                  :  1;\
            UNSG32 uCTRL_LRSWITCH                              :  1;\
            UNSG32 uCTRL_FLUSH                                 :  1;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32PDMCH_CTRL;
            struct w32PDMCH_CTRL;
          };
    #define     w32PDMCH_CTRL2                                 {\
            UNSG32 uCTRL2_RDLT                                 : 16;\
            UNSG32 uCTRL2_FDLT                                 : 16;\
          }
    union { UNSG32 u32PDMCH_CTRL2;
            struct w32PDMCH_CTRL2;
          };
    } SIE_PDMCH;
    typedef union  T32PDMCH_CTRL
          { UNSG32 u32;
            struct w32PDMCH_CTRL;
                 } T32PDMCH_CTRL;
    typedef union  T32PDMCH_CTRL2
          { UNSG32 u32;
            struct w32PDMCH_CTRL2;
                 } T32PDMCH_CTRL2;
    typedef union  TPDMCH_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32PDMCH_CTRL;
                   };
                 } TPDMCH_CTRL;
    typedef union  TPDMCH_CTRL2
          { UNSG32 u32[1];
            struct {
            struct w32PDMCH_CTRL2;
                   };
                 } TPDMCH_CTRL2;
     SIGN32 PDMCH_drvrd(SIE_PDMCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 PDMCH_drvwr(SIE_PDMCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void PDMCH_reset(SIE_PDMCH *p);
     SIGN32 PDMCH_cmp  (SIE_PDMCH *p, SIE_PDMCH *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define PDMCH_check(p,pie,pfx,hLOG) PDMCH_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define PDMCH_print(p,    pfx,hLOG) PDMCH_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_PDM
#define h_PDM (){}
    #define     RA_PDM_CTRL1                                   0x0000
    #define     RA_PDM_APRKT                                   0x0004
    #define     RA_PDM_INT                                     0x0008
    #define     RA_PDM_RXDATA                                  0x000C
    #define     RA_PDM_INTLMODE                                0x0010
    #define     RA_PDM_INTLMAP                                 0x0014
    #define     RA_PDM_PDM0                                    0x0018
    #define     RA_PDM_PDM1                                    0x0020
    #define     RA_PDM_PDM2                                    0x0028
    #define     RA_PDM_PDM3                                    0x0030
    typedef struct SIE_PDM {
    #define     w32PDM_CTRL1                                   {\
            UNSG32 uCTRL1_GENABLE                              :  1;\
            UNSG32 uCTRL1_CLKDIV                               :  4;\
            UNSG32 uCTRL1_INVCLK_OUT                           :  1;\
            UNSG32 uCTRL1_INVCLK_INT                           :  1;\
            UNSG32 uCTRL1_CLKINT_SEL                           :  1;\
            UNSG32 uCTRL1_RLSB                                 :  1;\
            UNSG32 uCTRL1_RDM                                  :  3;\
            UNSG32 uCTRL1_MODE                                 :  1;\
            UNSG32 uCTRL1_SDR_CLKSEL                           :  1;\
            UNSG32 uCTRL1_LATCH_MODE                           :  1;\
            UNSG32 uCTRL1_APRKT_ON                             :  1;\
            UNSG32 uCTRL1_APRKT_ABSO                           :  1;\
            UNSG32 uCTRL1_APRKT_RELA                           :  1;\
            UNSG32 uCTRL1_APRKT_ASAP                           :  1;\
            UNSG32 RSVDx0_b19                                  : 13;\
          }
    union { UNSG32 u32PDM_CTRL1;
            struct w32PDM_CTRL1;
          };
    #define     w32PDM_APRKT                                   {\
            UNSG32 uAPRKT_VAL                                  : 32;\
          }
    union { UNSG32 u32PDM_APRKT;
            struct w32PDM_APRKT;
          };
    #define     w32PDM_INT                                     {\
            UNSG32 uINT_SCNT                                   : 32;\
          }
    union { UNSG32 u32PDM_INT;
            struct w32PDM_INT;
          };
    #define     w32PDM_RXDATA                                  {\
            UNSG32 uRXDATA_INTL                                :  1;\
            UNSG32 RSVDxC_b1                                   : 31;\
          }
    union { UNSG32 u32PDM_RXDATA;
            struct w32PDM_RXDATA;
          };
    #define     w32PDM_INTLMODE                                {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx10_b5                                  : 27;\
          }
    union { UNSG32 u32PDM_INTLMODE;
            struct w32PDM_INTLMODE;
          };
    #define     w32PDM_INTLMAP                                 {\
            UNSG32 uINTLMAP_PORT0                              :  2;\
            UNSG32 uINTLMAP_PORT1                              :  2;\
            UNSG32 uINTLMAP_PORT2                              :  2;\
            UNSG32 uINTLMAP_PORT3                              :  2;\
            UNSG32 RSVDx14_b8                                  : 24;\
          }
    union { UNSG32 u32PDM_INTLMAP;
            struct w32PDM_INTLMAP;
          };
              SIE_PDMCH                                        ie_PDM0;
              SIE_PDMCH                                        ie_PDM1;
              SIE_PDMCH                                        ie_PDM2;
              SIE_PDMCH                                        ie_PDM3;
    } SIE_PDM;
    typedef union  T32PDM_CTRL1
          { UNSG32 u32;
            struct w32PDM_CTRL1;
                 } T32PDM_CTRL1;
    typedef union  T32PDM_APRKT
          { UNSG32 u32;
            struct w32PDM_APRKT;
                 } T32PDM_APRKT;
    typedef union  T32PDM_INT
          { UNSG32 u32;
            struct w32PDM_INT;
                 } T32PDM_INT;
    typedef union  T32PDM_RXDATA
          { UNSG32 u32;
            struct w32PDM_RXDATA;
                 } T32PDM_RXDATA;
    typedef union  T32PDM_INTLMODE
          { UNSG32 u32;
            struct w32PDM_INTLMODE;
                 } T32PDM_INTLMODE;
    typedef union  T32PDM_INTLMAP
          { UNSG32 u32;
            struct w32PDM_INTLMAP;
                 } T32PDM_INTLMAP;
    typedef union  TPDM_CTRL1
          { UNSG32 u32[1];
            struct {
            struct w32PDM_CTRL1;
                   };
                 } TPDM_CTRL1;
    typedef union  TPDM_APRKT
          { UNSG32 u32[1];
            struct {
            struct w32PDM_APRKT;
                   };
                 } TPDM_APRKT;
    typedef union  TPDM_INT
          { UNSG32 u32[1];
            struct {
            struct w32PDM_INT;
                   };
                 } TPDM_INT;
    typedef union  TPDM_RXDATA
          { UNSG32 u32[1];
            struct {
            struct w32PDM_RXDATA;
                   };
                 } TPDM_RXDATA;
    typedef union  TPDM_INTLMODE
          { UNSG32 u32[1];
            struct {
            struct w32PDM_INTLMODE;
                   };
                 } TPDM_INTLMODE;
    typedef union  TPDM_INTLMAP
          { UNSG32 u32[1];
            struct {
            struct w32PDM_INTLMAP;
                   };
                 } TPDM_INTLMAP;
     SIGN32 PDM_drvrd(SIE_PDM *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 PDM_drvwr(SIE_PDM *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void PDM_reset(SIE_PDM *p);
     SIGN32 PDM_cmp  (SIE_PDM *p, SIE_PDM *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define PDM_check(p,pie,pfx,hLOG) PDM_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define PDM_print(p,    pfx,hLOG) PDM_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DMIC
#define h_DMIC (){}
    #define     RA_DMIC_STATUS                                 0x0000
    #define     RA_DMIC_CONTROL                                0x0004
    #define     RA_DMIC_APRKT                                  0x0008
    #define     RA_DMIC_INT                                    0x000C
    #define     RA_DMIC_FLUSH                                  0x0010
    #define     RA_DMIC_DECIMATION_CONTROL                     0x0014
    #define     RA_DMIC_MICROPHONE_CONFIGURATION               0x0018
    #define     RA_DMIC_GAIN_MIC_PAIR_A                        0x001C
    #define     RA_DMIC_GAIN_MIC_PAIR_B                        0x0020
    #define     RA_DMIC_GAIN_MIC_PAIR_C                        0x0024
    #define     RA_DMIC_GAIN_MIC_PAIR_D                        0x0028
    #define     RA_DMIC_GAIN_RAMP_CONTROL                      0x002C
    typedef struct SIE_DMIC {
    #define     w32DMIC_STATUS                                 {\
            UNSG32 uSTATUS_PCM_FIFO_full                       :  1;\
            UNSG32 uSTATUS_PDM_LFIFO_emp                       :  1;\
            UNSG32 uSTATUS_PDM_RFIFO_emp                       :  1;\
            UNSG32 RSVDx0_b3                                   : 29;\
          }
    union { UNSG32 u32DMIC_STATUS;
            struct w32DMIC_STATUS;
          };
    #define     w32DMIC_CONTROL                                {\
            UNSG32 uCONTROL_Enable                             :  1;\
            UNSG32 uCONTROL_Enable_A                           :  1;\
            UNSG32 uCONTROL_Enable_B                           :  1;\
            UNSG32 uCONTROL_Enable_C                           :  1;\
            UNSG32 uCONTROL_Enable_D                           :  1;\
            UNSG32 uCONTROL_Run_A_L                            :  1;\
            UNSG32 uCONTROL_Run_A_R                            :  1;\
            UNSG32 uCONTROL_Run_B_L                            :  1;\
            UNSG32 uCONTROL_Run_B_R                            :  1;\
            UNSG32 uCONTROL_Run_C_L                            :  1;\
            UNSG32 uCONTROL_Run_C_R                            :  1;\
            UNSG32 uCONTROL_Run_D_L                            :  1;\
            UNSG32 uCONTROL_Run_D_R                            :  1;\
            UNSG32 uCONTROL_Wake_On_Sound                      :  4;\
            UNSG32 uCONTROL_Enable_DC_A                        :  1;\
            UNSG32 uCONTROL_Enable_DC_B                        :  1;\
            UNSG32 uCONTROL_Enable_DC_C                        :  1;\
            UNSG32 uCONTROL_Enable_DC_D                        :  1;\
            UNSG32 uCONTROL_Mono_A                             :  1;\
            UNSG32 uCONTROL_Mono_B                             :  1;\
            UNSG32 uCONTROL_Mono_C                             :  1;\
            UNSG32 uCONTROL_Mono_D                             :  1;\
            UNSG32 uCONTROL_PDMIn_D_LRSwp                      :  1;\
            UNSG32 uCONTROL_PDMIn_D_LJn_RJ                     :  1;\
            UNSG32 uCONTROL_PCM_LJn_RJ                         :  1;\
            UNSG32 uCONTROL_APRKT_ON                           :  1;\
            UNSG32 uCONTROL_APRKT_ABSO                         :  1;\
            UNSG32 uCONTROL_APRKT_RELA                         :  1;\
            UNSG32 uCONTROL_APRKT_ASAP                         :  1;\
          }
    union { UNSG32 u32DMIC_CONTROL;
            struct w32DMIC_CONTROL;
          };
    #define     w32DMIC_APRKT                                  {\
            UNSG32 uAPRKT_VAL                                  : 32;\
          }
    union { UNSG32 u32DMIC_APRKT;
            struct w32DMIC_APRKT;
          };
    #define     w32DMIC_INT                                    {\
            UNSG32 uINT_SCNT                                   : 32;\
          }
    union { UNSG32 u32DMIC_INT;
            struct w32DMIC_INT;
          };
    #define     w32DMIC_FLUSH                                  {\
            UNSG32 uFLUSH_PCMWR_FLUSH                          :  1;\
            UNSG32 uFLUSH_PDMRD_FLUSH                          :  1;\
            UNSG32 RSVDx10_b2                                  : 30;\
          }
    union { UNSG32 u32DMIC_FLUSH;
            struct w32DMIC_FLUSH;
          };
    #define     w32DMIC_DECIMATION_CONTROL                     {\
            UNSG32 uDECIMATION_CONTROL_CIC_Ratio_PCM           :  7;\
            UNSG32 uDECIMATION_CONTROL_CIC_Ratio_PCM_D         :  7;\
            UNSG32 uDECIMATION_CONTROL_PDM_Slots_Per_Frame     :  2;\
            UNSG32 uDECIMATION_CONTROL_FIR_Filter_Selection    :  2;\
            UNSG32 uDECIMATION_CONTROL_PDM_Bits_Per_Slot       :  5;\
            UNSG32 RSVDx14_b23                                 :  9;\
          }
    union { UNSG32 u32DMIC_DECIMATION_CONTROL;
            struct w32DMIC_DECIMATION_CONTROL;
          };
    #define     w32DMIC_MICROPHONE_CONFIGURATION               {\
            UNSG32 uMICROPHONE_CONFIGURATION_A_Left_Right_Swap :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_A_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_A_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_B_Left_Right_Swap :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_B_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_B_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_C_Left_Right_Swapz :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_C_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_C_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_Left_Right_Swap :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_PDM_from_ADMA   :  2;\
            UNSG32 RSVDx18_b14                                 : 18;\
          }
    union { UNSG32 u32DMIC_MICROPHONE_CONFIGURATION;
            struct w32DMIC_MICROPHONE_CONFIGURATION;
          };
    #define     w32DMIC_GAIN_MIC_PAIR_A                        {\
            UNSG32 uGAIN_MIC_PAIR_A_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_A_Gain_R                     : 10;\
            UNSG32 RSVDx1C_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_A;
            struct w32DMIC_GAIN_MIC_PAIR_A;
          };
    #define     w32DMIC_GAIN_MIC_PAIR_B                        {\
            UNSG32 uGAIN_MIC_PAIR_B_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_B_Gain_R                     : 10;\
            UNSG32 RSVDx20_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_B;
            struct w32DMIC_GAIN_MIC_PAIR_B;
          };
    #define     w32DMIC_GAIN_MIC_PAIR_C                        {\
            UNSG32 uGAIN_MIC_PAIR_C_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_C_Gain_R                     : 10;\
            UNSG32 RSVDx24_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_C;
            struct w32DMIC_GAIN_MIC_PAIR_C;
          };
    #define     w32DMIC_GAIN_MIC_PAIR_D                        {\
            UNSG32 uGAIN_MIC_PAIR_D_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_D_Gain_R                     : 10;\
            UNSG32 RSVDx28_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_D;
            struct w32DMIC_GAIN_MIC_PAIR_D;
          };
    #define     w32DMIC_GAIN_RAMP_CONTROL                      {\
            UNSG32 uGAIN_RAMP_CONTROL_Step_Size                :  8;\
            UNSG32 uGAIN_RAMP_CONTROL_Step_Rate                :  3;\
            UNSG32 RSVDx2C_b11                                 : 21;\
          }
    union { UNSG32 u32DMIC_GAIN_RAMP_CONTROL;
            struct w32DMIC_GAIN_RAMP_CONTROL;
          };
    } SIE_DMIC;
    typedef union  T32DMIC_STATUS
          { UNSG32 u32;
            struct w32DMIC_STATUS;
                 } T32DMIC_STATUS;
    typedef union  T32DMIC_CONTROL
          { UNSG32 u32;
            struct w32DMIC_CONTROL;
                 } T32DMIC_CONTROL;
    typedef union  T32DMIC_APRKT
          { UNSG32 u32;
            struct w32DMIC_APRKT;
                 } T32DMIC_APRKT;
    typedef union  T32DMIC_INT
          { UNSG32 u32;
            struct w32DMIC_INT;
                 } T32DMIC_INT;
    typedef union  T32DMIC_FLUSH
          { UNSG32 u32;
            struct w32DMIC_FLUSH;
                 } T32DMIC_FLUSH;
    typedef union  T32DMIC_DECIMATION_CONTROL
          { UNSG32 u32;
            struct w32DMIC_DECIMATION_CONTROL;
                 } T32DMIC_DECIMATION_CONTROL;
    typedef union  T32DMIC_MICROPHONE_CONFIGURATION
          { UNSG32 u32;
            struct w32DMIC_MICROPHONE_CONFIGURATION;
                 } T32DMIC_MICROPHONE_CONFIGURATION;
    typedef union  T32DMIC_GAIN_MIC_PAIR_A
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_A;
                 } T32DMIC_GAIN_MIC_PAIR_A;
    typedef union  T32DMIC_GAIN_MIC_PAIR_B
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_B;
                 } T32DMIC_GAIN_MIC_PAIR_B;
    typedef union  T32DMIC_GAIN_MIC_PAIR_C
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_C;
                 } T32DMIC_GAIN_MIC_PAIR_C;
    typedef union  T32DMIC_GAIN_MIC_PAIR_D
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_D;
                 } T32DMIC_GAIN_MIC_PAIR_D;
    typedef union  T32DMIC_GAIN_RAMP_CONTROL
          { UNSG32 u32;
            struct w32DMIC_GAIN_RAMP_CONTROL;
                 } T32DMIC_GAIN_RAMP_CONTROL;
    typedef union  TDMIC_STATUS
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_STATUS;
                   };
                 } TDMIC_STATUS;
    typedef union  TDMIC_CONTROL
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CONTROL;
                   };
                 } TDMIC_CONTROL;
    typedef union  TDMIC_APRKT
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_APRKT;
                   };
                 } TDMIC_APRKT;
    typedef union  TDMIC_INT
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_INT;
                   };
                 } TDMIC_INT;
    typedef union  TDMIC_FLUSH
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_FLUSH;
                   };
                 } TDMIC_FLUSH;
    typedef union  TDMIC_DECIMATION_CONTROL
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_DECIMATION_CONTROL;
                   };
                 } TDMIC_DECIMATION_CONTROL;
    typedef union  TDMIC_MICROPHONE_CONFIGURATION
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_MICROPHONE_CONFIGURATION;
                   };
                 } TDMIC_MICROPHONE_CONFIGURATION;
    typedef union  TDMIC_GAIN_MIC_PAIR_A
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_A;
                   };
                 } TDMIC_GAIN_MIC_PAIR_A;
    typedef union  TDMIC_GAIN_MIC_PAIR_B
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_B;
                   };
                 } TDMIC_GAIN_MIC_PAIR_B;
    typedef union  TDMIC_GAIN_MIC_PAIR_C
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_C;
                   };
                 } TDMIC_GAIN_MIC_PAIR_C;
    typedef union  TDMIC_GAIN_MIC_PAIR_D
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_D;
                   };
                 } TDMIC_GAIN_MIC_PAIR_D;
    typedef union  TDMIC_GAIN_RAMP_CONTROL
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_RAMP_CONTROL;
                   };
                 } TDMIC_GAIN_RAMP_CONTROL;
     SIGN32 DMIC_drvrd(SIE_DMIC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DMIC_drvwr(SIE_DMIC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DMIC_reset(SIE_DMIC *p);
     SIGN32 DMIC_cmp  (SIE_DMIC *p, SIE_DMIC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DMIC_check(p,pie,pfx,hLOG) DMIC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DMIC_print(p,    pfx,hLOG) DMIC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_IOSELTX
#define h_IOSELTX (){}
    #define     RA_IOSELTX_SECBCLK1                            0x0000
    #define     RA_IOSELTX_SECBCLK2                            0x0004
    #define     RA_IOSELTX_SECBCLK3                            0x0008
    #define     RA_IOSELTX_SECBCLK4                            0x000C
    #define     RA_IOSELTX_SECBCLK5                            0x0010
    #define     RA_IOSELTX_SECFSYNC1                           0x0014
    #define     RA_IOSELTX_SECFSYNC2                           0x0018
    #define     RA_IOSELTX_SECFSYNC3                           0x001C
    #define     RA_IOSELTX_SECFSYNC4                           0x0020
    #define     RA_IOSELTX_SECFSYNC5                           0x0024
    typedef struct SIE_IOSELTX {
    #define     w32IOSELTX_SECBCLK1                            {\
            UNSG32 uSECBCLK1_SEL                               :  2;\
            UNSG32 uSECBCLK1_INV                               :  1;\
            UNSG32 RSVDx0_b3                                   : 29;\
          }
    union { UNSG32 u32IOSELTX_SECBCLK1;
            struct w32IOSELTX_SECBCLK1;
          };
    #define     w32IOSELTX_SECBCLK2                            {\
            UNSG32 uSECBCLK2_SEL                               :  2;\
            UNSG32 uSECBCLK2_INV                               :  1;\
            UNSG32 RSVDx4_b3                                   : 29;\
          }
    union { UNSG32 u32IOSELTX_SECBCLK2;
            struct w32IOSELTX_SECBCLK2;
          };
    #define     w32IOSELTX_SECBCLK3                            {\
            UNSG32 uSECBCLK3_SEL                               :  2;\
            UNSG32 uSECBCLK3_INV                               :  1;\
            UNSG32 RSVDx8_b3                                   : 29;\
          }
    union { UNSG32 u32IOSELTX_SECBCLK3;
            struct w32IOSELTX_SECBCLK3;
          };
    #define     w32IOSELTX_SECBCLK4                            {\
            UNSG32 uSECBCLK4_SEL                               :  2;\
            UNSG32 uSECBCLK4_INV                               :  1;\
            UNSG32 RSVDxC_b3                                   : 29;\
          }
    union { UNSG32 u32IOSELTX_SECBCLK4;
            struct w32IOSELTX_SECBCLK4;
          };
    #define     w32IOSELTX_SECBCLK5                            {\
            UNSG32 uSECBCLK5_SEL                               :  2;\
            UNSG32 uSECBCLK5_INV                               :  1;\
            UNSG32 RSVDx10_b3                                  : 29;\
          }
    union { UNSG32 u32IOSELTX_SECBCLK5;
            struct w32IOSELTX_SECBCLK5;
          };
    #define     w32IOSELTX_SECFSYNC1                           {\
            UNSG32 uSECFSYNC1_SEL                              :  1;\
            UNSG32 uSECFSYNC1_INV                              :  1;\
            UNSG32 RSVDx14_b2                                  : 30;\
          }
    union { UNSG32 u32IOSELTX_SECFSYNC1;
            struct w32IOSELTX_SECFSYNC1;
          };
    #define     w32IOSELTX_SECFSYNC2                           {\
            UNSG32 uSECFSYNC2_SEL                              :  1;\
            UNSG32 uSECFSYNC2_INV                              :  1;\
            UNSG32 RSVDx18_b2                                  : 30;\
          }
    union { UNSG32 u32IOSELTX_SECFSYNC2;
            struct w32IOSELTX_SECFSYNC2;
          };
    #define     w32IOSELTX_SECFSYNC3                           {\
            UNSG32 uSECFSYNC3_SEL                              :  1;\
            UNSG32 uSECFSYNC3_INV                              :  1;\
            UNSG32 RSVDx1C_b2                                  : 30;\
          }
    union { UNSG32 u32IOSELTX_SECFSYNC3;
            struct w32IOSELTX_SECFSYNC3;
          };
    #define     w32IOSELTX_SECFSYNC4                           {\
            UNSG32 uSECFSYNC4_SEL                              :  1;\
            UNSG32 uSECFSYNC4_INV                              :  1;\
            UNSG32 RSVDx20_b2                                  : 30;\
          }
    union { UNSG32 u32IOSELTX_SECFSYNC4;
            struct w32IOSELTX_SECFSYNC4;
          };
    #define     w32IOSELTX_SECFSYNC5                           {\
            UNSG32 uSECFSYNC5_SEL                              :  1;\
            UNSG32 uSECFSYNC5_INV                              :  1;\
            UNSG32 RSVDx24_b2                                  : 30;\
          }
    union { UNSG32 u32IOSELTX_SECFSYNC5;
            struct w32IOSELTX_SECFSYNC5;
          };
    } SIE_IOSELTX;
    typedef union  T32IOSELTX_SECBCLK1
          { UNSG32 u32;
            struct w32IOSELTX_SECBCLK1;
                 } T32IOSELTX_SECBCLK1;
    typedef union  T32IOSELTX_SECBCLK2
          { UNSG32 u32;
            struct w32IOSELTX_SECBCLK2;
                 } T32IOSELTX_SECBCLK2;
    typedef union  T32IOSELTX_SECBCLK3
          { UNSG32 u32;
            struct w32IOSELTX_SECBCLK3;
                 } T32IOSELTX_SECBCLK3;
    typedef union  T32IOSELTX_SECBCLK4
          { UNSG32 u32;
            struct w32IOSELTX_SECBCLK4;
                 } T32IOSELTX_SECBCLK4;
    typedef union  T32IOSELTX_SECBCLK5
          { UNSG32 u32;
            struct w32IOSELTX_SECBCLK5;
                 } T32IOSELTX_SECBCLK5;
    typedef union  T32IOSELTX_SECFSYNC1
          { UNSG32 u32;
            struct w32IOSELTX_SECFSYNC1;
                 } T32IOSELTX_SECFSYNC1;
    typedef union  T32IOSELTX_SECFSYNC2
          { UNSG32 u32;
            struct w32IOSELTX_SECFSYNC2;
                 } T32IOSELTX_SECFSYNC2;
    typedef union  T32IOSELTX_SECFSYNC3
          { UNSG32 u32;
            struct w32IOSELTX_SECFSYNC3;
                 } T32IOSELTX_SECFSYNC3;
    typedef union  T32IOSELTX_SECFSYNC4
          { UNSG32 u32;
            struct w32IOSELTX_SECFSYNC4;
                 } T32IOSELTX_SECFSYNC4;
    typedef union  T32IOSELTX_SECFSYNC5
          { UNSG32 u32;
            struct w32IOSELTX_SECFSYNC5;
                 } T32IOSELTX_SECFSYNC5;
    typedef union  TIOSELTX_SECBCLK1
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECBCLK1;
                   };
                 } TIOSELTX_SECBCLK1;
    typedef union  TIOSELTX_SECBCLK2
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECBCLK2;
                   };
                 } TIOSELTX_SECBCLK2;
    typedef union  TIOSELTX_SECBCLK3
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECBCLK3;
                   };
                 } TIOSELTX_SECBCLK3;
    typedef union  TIOSELTX_SECBCLK4
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECBCLK4;
                   };
                 } TIOSELTX_SECBCLK4;
    typedef union  TIOSELTX_SECBCLK5
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECBCLK5;
                   };
                 } TIOSELTX_SECBCLK5;
    typedef union  TIOSELTX_SECFSYNC1
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECFSYNC1;
                   };
                 } TIOSELTX_SECFSYNC1;
    typedef union  TIOSELTX_SECFSYNC2
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECFSYNC2;
                   };
                 } TIOSELTX_SECFSYNC2;
    typedef union  TIOSELTX_SECFSYNC3
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECFSYNC3;
                   };
                 } TIOSELTX_SECFSYNC3;
    typedef union  TIOSELTX_SECFSYNC4
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECFSYNC4;
                   };
                 } TIOSELTX_SECFSYNC4;
    typedef union  TIOSELTX_SECFSYNC5
          { UNSG32 u32[1];
            struct {
            struct w32IOSELTX_SECFSYNC5;
                   };
                 } TIOSELTX_SECFSYNC5;
     SIGN32 IOSELTX_drvrd(SIE_IOSELTX *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 IOSELTX_drvwr(SIE_IOSELTX *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void IOSELTX_reset(SIE_IOSELTX *p);
     SIGN32 IOSELTX_cmp  (SIE_IOSELTX *p, SIE_IOSELTX *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define IOSELTX_check(p,pie,pfx,hLOG) IOSELTX_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define IOSELTX_print(p,    pfx,hLOG) IOSELTX_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AIODRT
#define h_AIODRT (){}
    #define     RA_AIODRT_DA_CTRL                              0x0000
    #define     RA_AIODRT_DA_APRKT                             0x0004
    #define     RA_AIODRT_DA_INT                               0x0008
    #define     RA_AIODRT_AD_CTRL                              0x000C
    #define     RA_AIODRT_AD_APRKT                             0x0010
    #define     RA_AIODRT_AD_INT                               0x0014
    typedef struct SIE_AIODRT {
    #define     w32AIODRT_DA_CTRL                              {\
            UNSG32 uDA_CTRL_APRKT_ON                           :  1;\
            UNSG32 uDA_CTRL_APRKT_ABSO                         :  1;\
            UNSG32 uDA_CTRL_APRKT_RELA                         :  1;\
            UNSG32 uDA_CTRL_APRKT_ASAP                         :  1;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32AIODRT_DA_CTRL;
            struct w32AIODRT_DA_CTRL;
          };
    #define     w32AIODRT_DA_APRKT                             {\
            UNSG32 uDA_APRKT_VAL                               : 32;\
          }
    union { UNSG32 u32AIODRT_DA_APRKT;
            struct w32AIODRT_DA_APRKT;
          };
    #define     w32AIODRT_DA_INT                               {\
            UNSG32 uDA_INT_SCNT                                : 32;\
          }
    union { UNSG32 u32AIODRT_DA_INT;
            struct w32AIODRT_DA_INT;
          };
    #define     w32AIODRT_AD_CTRL                              {\
            UNSG32 uAD_CTRL_APRKT_ON                           :  1;\
            UNSG32 uAD_CTRL_APRKT_ABSO                         :  1;\
            UNSG32 uAD_CTRL_APRKT_RELA                         :  1;\
            UNSG32 uAD_CTRL_APRKT_ASAP                         :  1;\
            UNSG32 RSVDxC_b4                                   : 28;\
          }
    union { UNSG32 u32AIODRT_AD_CTRL;
            struct w32AIODRT_AD_CTRL;
          };
    #define     w32AIODRT_AD_APRKT                             {\
            UNSG32 uAD_APRKT_VAL                               : 32;\
          }
    union { UNSG32 u32AIODRT_AD_APRKT;
            struct w32AIODRT_AD_APRKT;
          };
    #define     w32AIODRT_AD_INT                               {\
            UNSG32 uAD_INT_SCNT                                : 32;\
          }
    union { UNSG32 u32AIODRT_AD_INT;
            struct w32AIODRT_AD_INT;
          };
    } SIE_AIODRT;
    typedef union  T32AIODRT_DA_CTRL
          { UNSG32 u32;
            struct w32AIODRT_DA_CTRL;
                 } T32AIODRT_DA_CTRL;
    typedef union  T32AIODRT_DA_APRKT
          { UNSG32 u32;
            struct w32AIODRT_DA_APRKT;
                 } T32AIODRT_DA_APRKT;
    typedef union  T32AIODRT_DA_INT
          { UNSG32 u32;
            struct w32AIODRT_DA_INT;
                 } T32AIODRT_DA_INT;
    typedef union  T32AIODRT_AD_CTRL
          { UNSG32 u32;
            struct w32AIODRT_AD_CTRL;
                 } T32AIODRT_AD_CTRL;
    typedef union  T32AIODRT_AD_APRKT
          { UNSG32 u32;
            struct w32AIODRT_AD_APRKT;
                 } T32AIODRT_AD_APRKT;
    typedef union  T32AIODRT_AD_INT
          { UNSG32 u32;
            struct w32AIODRT_AD_INT;
                 } T32AIODRT_AD_INT;
    typedef union  TAIODRT_DA_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AIODRT_DA_CTRL;
                   };
                 } TAIODRT_DA_CTRL;
    typedef union  TAIODRT_DA_APRKT
          { UNSG32 u32[1];
            struct {
            struct w32AIODRT_DA_APRKT;
                   };
                 } TAIODRT_DA_APRKT;
    typedef union  TAIODRT_DA_INT
          { UNSG32 u32[1];
            struct {
            struct w32AIODRT_DA_INT;
                   };
                 } TAIODRT_DA_INT;
    typedef union  TAIODRT_AD_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AIODRT_AD_CTRL;
                   };
                 } TAIODRT_AD_CTRL;
    typedef union  TAIODRT_AD_APRKT
          { UNSG32 u32[1];
            struct {
            struct w32AIODRT_AD_APRKT;
                   };
                 } TAIODRT_AD_APRKT;
    typedef union  TAIODRT_AD_INT
          { UNSG32 u32[1];
            struct {
            struct w32AIODRT_AD_INT;
                   };
                 } TAIODRT_AD_INT;
     SIGN32 AIODRT_drvrd(SIE_AIODRT *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AIODRT_drvwr(SIE_AIODRT *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AIODRT_reset(SIE_AIODRT *p);
     SIGN32 AIODRT_cmp  (SIE_AIODRT *p, SIE_AIODRT *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AIODRT_check(p,pie,pfx,hLOG) AIODRT_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AIODRT_print(p,    pfx,hLOG) AIODRT_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AIODCLASS
#define h_AIODCLASS (){}
    #define     RA_AIODCLASS_CTRL                              0x0000
    #define     RA_AIODCLASS_APRKT                             0x0004
    #define     RA_AIODCLASS_INT                               0x0008
    typedef struct SIE_AIODCLASS {
    #define     w32AIODCLASS_CTRL                              {\
            UNSG32 uCTRL_APRKT_ON                              :  1;\
            UNSG32 uCTRL_APRKT_ABSO                            :  1;\
            UNSG32 uCTRL_APRKT_RELA                            :  1;\
            UNSG32 uCTRL_APRKT_ASAP                            :  1;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32AIODCLASS_CTRL;
            struct w32AIODCLASS_CTRL;
          };
    #define     w32AIODCLASS_APRKT                             {\
            UNSG32 uAPRKT_VAL                                  : 32;\
          }
    union { UNSG32 u32AIODCLASS_APRKT;
            struct w32AIODCLASS_APRKT;
          };
    #define     w32AIODCLASS_INT                               {\
            UNSG32 uINT_SCNT                                   : 32;\
          }
    union { UNSG32 u32AIODCLASS_INT;
            struct w32AIODCLASS_INT;
          };
    } SIE_AIODCLASS;
    typedef union  T32AIODCLASS_CTRL
          { UNSG32 u32;
            struct w32AIODCLASS_CTRL;
                 } T32AIODCLASS_CTRL;
    typedef union  T32AIODCLASS_APRKT
          { UNSG32 u32;
            struct w32AIODCLASS_APRKT;
                 } T32AIODCLASS_APRKT;
    typedef union  T32AIODCLASS_INT
          { UNSG32 u32;
            struct w32AIODCLASS_INT;
                 } T32AIODCLASS_INT;
    typedef union  TAIODCLASS_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AIODCLASS_CTRL;
                   };
                 } TAIODCLASS_CTRL;
    typedef union  TAIODCLASS_APRKT
          { UNSG32 u32[1];
            struct {
            struct w32AIODCLASS_APRKT;
                   };
                 } TAIODCLASS_APRKT;
    typedef union  TAIODCLASS_INT
          { UNSG32 u32[1];
            struct {
            struct w32AIODCLASS_INT;
                   };
                 } TAIODCLASS_INT;
     SIGN32 AIODCLASS_drvrd(SIE_AIODCLASS *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AIODCLASS_drvwr(SIE_AIODCLASS *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AIODCLASS_reset(SIE_AIODCLASS *p);
     SIGN32 AIODCLASS_cmp  (SIE_AIODCLASS *p, SIE_AIODCLASS *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AIODCLASS_check(p,pie,pfx,hLOG) AIODCLASS_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AIODCLASS_print(p,    pfx,hLOG) AIODCLASS_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AIO
#define h_AIO (){}
    #define     RA_AIO_SEC1                                    0x0000
    #define     RA_AIO_SEC2                                    0x002C
    #define     RA_AIO_SEC3                                    0x0058
    #define     RA_AIO_SEC4                                    0x0084
    #define     RA_AIO_SEC5                                    0x00B0
    #define     RA_AIO_MIC21                                   0x00DC
    #define     RA_AIO_MIC22                                   0x010C
    #define     RA_AIO_MIC23                                   0x013C
    #define     RA_AIO_MIC24                                   0x016C
    #define     RA_AIO_MIC25                                   0x019C
    #define     RA_AIO_PDM                                     0x01CC
    #define     RA_AIO_DMIC_CLK                                0x0204
    #define     RA_AIO_DMIC                                    0x0214
    #define     RA_AIO_IOSELTX                                 0x0244
    #define     RA_AIO_IRQENABLE                               0x026C
    #define     RA_AIO_IRQENABLE1                              0x0270
    #define     RA_AIO_IRQENABLE2                              0x0274
    #define     RA_AIO_IRQENABLE3                              0x0278
    #define     RA_AIO_IRQENABLE4                              0x027C
    #define     RA_AIO_IRQENABLE5                              0x0280
    #define     RA_AIO_IRQENABLE6                              0x0284
    #define     RA_AIO_IRQENABLE7                              0x0288
    #define     RA_AIO_IRQENABLE8                              0x028C
    #define     RA_AIO_IRQENABLE9                              0x0290
    #define     RA_AIO_IRQENABLE10                             0x0294
    #define     RA_AIO_IRQENABLE11                             0x0298
    #define     RA_AIO_IRQSTS                                  0x029C
    #define     RA_AIO_IRQSTS1                                 0x02A0
    #define     RA_AIO_IRQSTS2                                 0x02A4
    #define     RA_AIO_IRQSTS3                                 0x02A8
    #define     RA_AIO_IRQSTS4                                 0x02AC
    #define     RA_AIO_IRQSTS5                                 0x02B0
    #define     RA_AIO_IRQSTS6                                 0x02B4
    #define     RA_AIO_IRQSTS7                                 0x02B8
    #define     RA_AIO_IRQSTS8                                 0x02BC
    #define     RA_AIO_IRQSTS9                                 0x02C0
    #define     RA_AIO_IRQSTS10                                0x02C4
    #define     RA_AIO_IRQSTS11                                0x02C8
    #define     RA_AIO_MCLKSEC1                                0x02CC
    #define     RA_AIO_MCLKSEC2                                0x02D0
    #define     RA_AIO_MCLKSEC3                                0x02D4
    #define     RA_AIO_MCLKSEC4                                0x02D8
    #define     RA_AIO_MCLKSEC5                                0x02DC
    #define     RA_AIO_MCLKPDM                                 0x02E0
    #define     RA_AIO_SW_RST                                  0x02E4
    #define     RA_AIO_CLK_GATE_EN                             0x02E8
    #define     RA_AIO_SAMP_CTRL                               0x02EC
    #define     RA_AIO_SAMPINFO_REQ                            0x02F0
    #define     RA_AIO_SCR                                     0x02F4
    #define     RA_AIO_SCR1                                    0x02F8
    #define     RA_AIO_SCR2                                    0x02FC
    #define     RA_AIO_SCR3                                    0x0300
    #define     RA_AIO_SCR4                                    0x0304
    #define     RA_AIO_SCR5                                    0x0308
    #define     RA_AIO_SCR6                                    0x030C
    #define     RA_AIO_SCR7                                    0x0310
    #define     RA_AIO_SCR8                                    0x0314
    #define     RA_AIO_SCR9                                    0x0318
    #define     RA_AIO_SCR10                                   0x031C
    #define     RA_AIO_SCR11                                   0x0320
    #define     RA_AIO_SCR12                                   0x0324
    #define     RA_AIO_SCR13                                   0x0328
    #define     RA_AIO_SCR14                                   0x032C
    #define     RA_AIO_STR                                     0x0330
    #define     RA_AIO_STR1                                    0x0334
    #define     RA_AIO_STR2                                    0x0338
    #define     RA_AIO_STR3                                    0x033C
    #define     RA_AIO_STR4                                    0x0340
    #define     RA_AIO_STR5                                    0x0344
    #define     RA_AIO_STR6                                    0x0348
    #define     RA_AIO_STR7                                    0x034C
    #define     RA_AIO_STR8                                    0x0350
    #define     RA_AIO_STR9                                    0x0354
    #define     RA_AIO_STR10                                   0x0358
    #define     RA_AIO_STR11                                   0x035C
    #define     RA_AIO_STR12                                   0x0360
    #define     RA_AIO_STR13                                   0x0364
    #define     RA_AIO_STR14                                   0x0368
    #define     RA_AIO_ATR                                     0x036C
    #define     RA_AIO_XFEED1                                  0x0370
    #define     RA_AIO_XFEED2                                  0x0374
    #define     RA_AIO_DMIC_SRAMPWR                            0x0378
    #define     RA_AIO_DRT                                     0x037C
    #define     RA_AIO_DCLASS                                  0x0394
    #define     RA_AIO_APRKT_EN                                0x03A0
    #define     RA_AIO_STARTED                                 0x03A4
    #define     RA_AIO_APRPR                                   0x03A8
    #define     RA_AIO_GPIO_ETM                                0x03AC
    #define     RA_AIO_GPIO_TRIG                               0x03B0
    #define     RA_AIO_GPIO_TRIG_SEL                           0x03B4
    #define     RA_AIO_GPIO_TRIG_CLR                           0x03B8
    #define     RA_AIO_PAUSED                                  0x03BC
    #define     RA_AIO_INT_SCNT_CLR                            0x03C0
    #define     RA_AIO_FORCEMUTE                               0x03C4
    typedef struct SIE_AIO {
              SIE_SEC                                          ie_SEC1;
              SIE_SEC                                          ie_SEC2;
              SIE_SEC                                          ie_SEC3;
              SIE_SEC                                          ie_SEC4;
              SIE_SEC                                          ie_SEC5;
              SIE_MIC2                                         ie_MIC21;
              SIE_MIC2                                         ie_MIC22;
              SIE_MIC2                                         ie_MIC23;
              SIE_MIC2                                         ie_MIC24;
              SIE_MIC2                                         ie_MIC25;
              SIE_PDM                                          ie_PDM;
              SIE_DMIC_CLK                                     ie_DMIC_CLK;
              SIE_DMIC                                         ie_DMIC;
              SIE_IOSELTX                                      ie_IOSELTX;
    #define     w32AIO_IRQENABLE                               {\
            UNSG32 uIRQENABLE_SEC1IRQ                          :  1;\
            UNSG32 uIRQENABLE_SEC1_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_SEC1_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_SEC2IRQ                          :  1;\
            UNSG32 uIRQENABLE_SEC2_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_SEC2_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_SEC3IRQ                          :  1;\
            UNSG32 uIRQENABLE_SEC3_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_SEC3_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_SEC4IRQ                          :  1;\
            UNSG32 uIRQENABLE_SEC4_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_SEC4_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_SEC5IRQ                          :  1;\
            UNSG32 uIRQENABLE_SEC5_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_SEC5_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_MIC1IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC1_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_MIC1_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_MIC2IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC2_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_MIC2_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_MIC3IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC3_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_MIC3_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_MIC4IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC4_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_MIC4_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_MIC5IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC5_PAUSED                      :  1;\
            UNSG32 uIRQENABLE_MIC5_RESTARTED                   :  1;\
            UNSG32 uIRQENABLE_PDMIRQ                           :  1;\
            UNSG32 uIRQENABLE_DMICIRQ                          :  1;\
          }
    union { UNSG32 u32AIO_IRQENABLE;
            struct w32AIO_IRQENABLE;
          };
    #define     w32AIO_IRQENABLE1                              {\
            UNSG32 uIRQENABLE1_DMIC_WAKE                       :  1;\
            UNSG32 RSVDx270_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQENABLE1;
            struct w32AIO_IRQENABLE1;
          };
    #define     w32AIO_IRQENABLE2                              {\
            UNSG32 uIRQENABLE2_FORCEMUTE_RISE                  :  1;\
            UNSG32 uIRQENABLE2_FORCEMUTE_FALL                  :  1;\
            UNSG32 RSVDx274_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQENABLE2;
            struct w32AIO_IRQENABLE2;
          };
    #define     w32AIO_IRQENABLE3                              {\
            UNSG32 uIRQENABLE3_I2S1_TX_SCNT                    :  1;\
            UNSG32 uIRQENABLE3_I2S1_RX_SCNT                    :  1;\
            UNSG32 RSVDx278_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQENABLE3;
            struct w32AIO_IRQENABLE3;
          };
    #define     w32AIO_IRQENABLE4                              {\
            UNSG32 uIRQENABLE4_I2S2_TX_SCNT                    :  1;\
            UNSG32 uIRQENABLE4_I2S2_RX_SCNT                    :  1;\
            UNSG32 RSVDx27C_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQENABLE4;
            struct w32AIO_IRQENABLE4;
          };
    #define     w32AIO_IRQENABLE5                              {\
            UNSG32 uIRQENABLE5_I2S3_TX_SCNT                    :  1;\
            UNSG32 uIRQENABLE5_I2S3_RX_SCNT                    :  1;\
            UNSG32 RSVDx280_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQENABLE5;
            struct w32AIO_IRQENABLE5;
          };
    #define     w32AIO_IRQENABLE6                              {\
            UNSG32 uIRQENABLE6_I2S4_TX_SCNT                    :  1;\
            UNSG32 uIRQENABLE6_I2S4_RX_SCNT                    :  1;\
            UNSG32 RSVDx284_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQENABLE6;
            struct w32AIO_IRQENABLE6;
          };
    #define     w32AIO_IRQENABLE7                              {\
            UNSG32 uIRQENABLE7_I2S5_TX_SCNT                    :  1;\
            UNSG32 uIRQENABLE7_I2S5_RX_SCNT                    :  1;\
            UNSG32 RSVDx288_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQENABLE7;
            struct w32AIO_IRQENABLE7;
          };
    #define     w32AIO_IRQENABLE8                              {\
            UNSG32 uIRQENABLE8_PDM_SCNT                        :  1;\
            UNSG32 RSVDx28C_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQENABLE8;
            struct w32AIO_IRQENABLE8;
          };
    #define     w32AIO_IRQENABLE9                              {\
            UNSG32 uIRQENABLE9_DMIC_SCNT                       :  1;\
            UNSG32 RSVDx290_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQENABLE9;
            struct w32AIO_IRQENABLE9;
          };
    #define     w32AIO_IRQENABLE10                             {\
            UNSG32 uIRQENABLE10_DRT_DA_SCNT                    :  1;\
            UNSG32 uIRQENABLE10_DRT_AD_SCNT                    :  1;\
            UNSG32 RSVDx294_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQENABLE10;
            struct w32AIO_IRQENABLE10;
          };
    #define     w32AIO_IRQENABLE11                             {\
            UNSG32 uIRQENABLE11_DCLASS_SCNT                    :  1;\
            UNSG32 RSVDx298_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQENABLE11;
            struct w32AIO_IRQENABLE11;
          };
    #define     w32AIO_IRQSTS                                  {\
            UNSG32 uIRQSTS_SEC1STS                             :  1;\
            UNSG32 uIRQSTS_SEC1_PAUSED                         :  1;\
            UNSG32 uIRQSTS_SEC1_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_SEC2STS                             :  1;\
            UNSG32 uIRQSTS_SEC2_PAUSED                         :  1;\
            UNSG32 uIRQSTS_SEC2_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_SEC3STS                             :  1;\
            UNSG32 uIRQSTS_SEC3_PAUSED                         :  1;\
            UNSG32 uIRQSTS_SEC3_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_SEC4STS                             :  1;\
            UNSG32 uIRQSTS_SEC4_PAUSED                         :  1;\
            UNSG32 uIRQSTS_SEC4_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_SEC5STS                             :  1;\
            UNSG32 uIRQSTS_SEC5_PAUSED                         :  1;\
            UNSG32 uIRQSTS_SEC5_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_MIC1STS                             :  1;\
            UNSG32 uIRQSTS_MIC1_PAUSED                         :  1;\
            UNSG32 uIRQSTS_MIC1_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_MIC2STS                             :  1;\
            UNSG32 uIRQSTS_MIC2_PAUSED                         :  1;\
            UNSG32 uIRQSTS_MIC2_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_MIC3STS                             :  1;\
            UNSG32 uIRQSTS_MIC3_PAUSED                         :  1;\
            UNSG32 uIRQSTS_MIC3_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_MIC4STS                             :  1;\
            UNSG32 uIRQSTS_MIC4_PAUSED                         :  1;\
            UNSG32 uIRQSTS_MIC4_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_MIC5STS                             :  1;\
            UNSG32 uIRQSTS_MIC5_PAUSED                         :  1;\
            UNSG32 uIRQSTS_MIC5_RESTARTED                      :  1;\
            UNSG32 uIRQSTS_PDMSTS                              :  1;\
            UNSG32 uIRQSTS_DMICSTS                             :  1;\
          }
    union { UNSG32 u32AIO_IRQSTS;
            struct w32AIO_IRQSTS;
          };
    #define     w32AIO_IRQSTS1                                 {\
            UNSG32 uIRQSTS1_DMIC_WAKE                          :  1;\
            UNSG32 RSVDx2A0_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQSTS1;
            struct w32AIO_IRQSTS1;
          };
    #define     w32AIO_IRQSTS2                                 {\
            UNSG32 uIRQSTS2_FORCEMUTE_RISE                     :  1;\
            UNSG32 uIRQSTS2_FORCEMUTE_FALL                     :  1;\
            UNSG32 RSVDx2A4_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQSTS2;
            struct w32AIO_IRQSTS2;
          };
    #define     w32AIO_IRQSTS3                                 {\
            UNSG32 uIRQSTS3_I2S1_TX_SCNT                       :  1;\
            UNSG32 uIRQSTS3_I2S1_RX_SCNT                       :  1;\
            UNSG32 RSVDx2A8_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQSTS3;
            struct w32AIO_IRQSTS3;
          };
    #define     w32AIO_IRQSTS4                                 {\
            UNSG32 uIRQSTS4_I2S2_TX_SCNT                       :  1;\
            UNSG32 uIRQSTS4_I2S2_RX_SCNT                       :  1;\
            UNSG32 RSVDx2AC_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQSTS4;
            struct w32AIO_IRQSTS4;
          };
    #define     w32AIO_IRQSTS5                                 {\
            UNSG32 uIRQSTS5_I2S3_TX_SCNT                       :  1;\
            UNSG32 uIRQSTS5_I2S3_RX_SCNT                       :  1;\
            UNSG32 RSVDx2B0_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQSTS5;
            struct w32AIO_IRQSTS5;
          };
    #define     w32AIO_IRQSTS6                                 {\
            UNSG32 uIRQSTS6_I2S4_TX_SCNT                       :  1;\
            UNSG32 uIRQSTS6_I2S4_RX_SCNT                       :  1;\
            UNSG32 RSVDx2B4_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQSTS6;
            struct w32AIO_IRQSTS6;
          };
    #define     w32AIO_IRQSTS7                                 {\
            UNSG32 uIRQSTS7_I2S5_TX_SCNT                       :  1;\
            UNSG32 uIRQSTS7_I2S5_RX_SCNT                       :  1;\
            UNSG32 RSVDx2B8_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQSTS7;
            struct w32AIO_IRQSTS7;
          };
    #define     w32AIO_IRQSTS8                                 {\
            UNSG32 uIRQSTS8_PDM_SCNT                           :  1;\
            UNSG32 RSVDx2BC_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQSTS8;
            struct w32AIO_IRQSTS8;
          };
    #define     w32AIO_IRQSTS9                                 {\
            UNSG32 uIRQSTS9_DMIC_SCNT                          :  1;\
            UNSG32 RSVDx2C0_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQSTS9;
            struct w32AIO_IRQSTS9;
          };
    #define     w32AIO_IRQSTS10                                {\
            UNSG32 uIRQSTS10_DRT_DA_SCNT                       :  1;\
            UNSG32 uIRQSTS10_DRT_AD_SCNT                       :  1;\
            UNSG32 RSVDx2C4_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_IRQSTS10;
            struct w32AIO_IRQSTS10;
          };
    #define     w32AIO_IRQSTS11                                {\
            UNSG32 uIRQSTS11_DCLASS_SCNT                       :  1;\
            UNSG32 RSVDx2C8_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_IRQSTS11;
            struct w32AIO_IRQSTS11;
          };
              SIE_ACLK                                         ie_MCLKSEC1;
              SIE_ACLK                                         ie_MCLKSEC2;
              SIE_ACLK                                         ie_MCLKSEC3;
              SIE_ACLK                                         ie_MCLKSEC4;
              SIE_ACLK                                         ie_MCLKSEC5;
              SIE_ACLK                                         ie_MCLKPDM;
    #define     w32AIO_SW_RST                                  {\
            UNSG32 uSW_RST_REFCLK                              :  1;\
            UNSG32 RSVDx2E4_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_SW_RST;
            struct w32AIO_SW_RST;
          };
    #define     w32AIO_CLK_GATE_EN                             {\
            UNSG32 uCLK_GATE_EN_GBL                            :  1;\
            UNSG32 RSVDx2E8_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_CLK_GATE_EN;
            struct w32AIO_CLK_GATE_EN;
          };
    #define     w32AIO_SAMP_CTRL                               {\
            UNSG32 uSAMP_CTRL_EN_I2STX1                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2STX2                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2STX3                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2STX4                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2STX5                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX1                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX2                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX3                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX4                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX5                        :  1;\
            UNSG32 uSAMP_CTRL_EN_AUDTIMER                      :  1;\
            UNSG32 uSAMP_CTRL_EN_PDMRX                         :  1;\
            UNSG32 uSAMP_CTRL_EN_DMICRX                        :  1;\
            UNSG32 uSAMP_CTRL_EN_DRT_DA                        :  1;\
            UNSG32 uSAMP_CTRL_EN_DRT_AD                        :  1;\
            UNSG32 uSAMP_CTRL_EN_DCLASS                        :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2STX1                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2STX2                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2STX3                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2STX4                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2STX5                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2SRX1                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2SRX2                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2SRX3                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2SRX4                     :  1;\
            UNSG32 uSAMP_CTRL_PAUSE_I2SRX5                     :  1;\
            UNSG32 RSVDx2EC_b26                                :  6;\
          }
    union { UNSG32 u32AIO_SAMP_CTRL;
            struct w32AIO_SAMP_CTRL;
          };
    #define     w32AIO_SAMPINFO_REQ                            {\
            UNSG32 uSAMPINFO_REQ_I2STX1                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2STX2                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2STX3                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2STX4                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2STX5                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX1                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX2                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX3                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX4                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX5                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX6                        :  1;\
            UNSG32 uSAMPINFO_REQ_PDMRX                         :  1;\
            UNSG32 uSAMPINFO_REQ_DMICRX                        :  1;\
            UNSG32 uSAMPINFO_REQ_DRT_DA                        :  1;\
            UNSG32 uSAMPINFO_REQ_DRT_AD                        :  1;\
            UNSG32 uSAMPINFO_REQ_DCLASS                        :  1;\
            UNSG32 RSVDx2F0_b16                                : 16;\
          }
    union { UNSG32 u32AIO_SAMPINFO_REQ;
            struct w32AIO_SAMPINFO_REQ;
          };
    #define     w32AIO_SCR                                     {\
            UNSG32 uSCR_I2STX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR;
            struct w32AIO_SCR;
          };
    #define     w32AIO_SCR1                                    {\
            UNSG32 uSCR_I2STX2                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR1;
            struct w32AIO_SCR1;
          };
    #define     w32AIO_SCR2                                    {\
            UNSG32 uSCR_I2STX3                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR2;
            struct w32AIO_SCR2;
          };
    #define     w32AIO_SCR3                                    {\
            UNSG32 uSCR_I2STX4                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR3;
            struct w32AIO_SCR3;
          };
    #define     w32AIO_SCR4                                    {\
            UNSG32 uSCR_I2STX5                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR4;
            struct w32AIO_SCR4;
          };
    #define     w32AIO_SCR5                                    {\
            UNSG32 uSCR_I2SRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR5;
            struct w32AIO_SCR5;
          };
    #define     w32AIO_SCR6                                    {\
            UNSG32 uSCR_I2SRX2                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR6;
            struct w32AIO_SCR6;
          };
    #define     w32AIO_SCR7                                    {\
            UNSG32 uSCR_I2SRX3                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR7;
            struct w32AIO_SCR7;
          };
    #define     w32AIO_SCR8                                    {\
            UNSG32 uSCR_I2SRX4                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR8;
            struct w32AIO_SCR8;
          };
    #define     w32AIO_SCR9                                    {\
            UNSG32 uSCR_I2SRX5                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR9;
            struct w32AIO_SCR9;
          };
    #define     w32AIO_SCR10                                   {\
            UNSG32 uSCR_PDMRX                                  : 32;\
          }
    union { UNSG32 u32AIO_SCR10;
            struct w32AIO_SCR10;
          };
    #define     w32AIO_SCR11                                   {\
            UNSG32 uSCR_DMICRX                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR11;
            struct w32AIO_SCR11;
          };
    #define     w32AIO_SCR12                                   {\
            UNSG32 uSCR_DRT_DA                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR12;
            struct w32AIO_SCR12;
          };
    #define     w32AIO_SCR13                                   {\
            UNSG32 uSCR_DRT_AD                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR13;
            struct w32AIO_SCR13;
          };
    #define     w32AIO_SCR14                                   {\
            UNSG32 uSCR_DCLASS                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR14;
            struct w32AIO_SCR14;
          };
    #define     w32AIO_STR                                     {\
            UNSG32 uSTR_I2STX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR;
            struct w32AIO_STR;
          };
    #define     w32AIO_STR1                                    {\
            UNSG32 uSTR_I2STX2                                 : 32;\
          }
    union { UNSG32 u32AIO_STR1;
            struct w32AIO_STR1;
          };
    #define     w32AIO_STR2                                    {\
            UNSG32 uSTR_I2STX3                                 : 32;\
          }
    union { UNSG32 u32AIO_STR2;
            struct w32AIO_STR2;
          };
    #define     w32AIO_STR3                                    {\
            UNSG32 uSTR_I2STX4                                 : 32;\
          }
    union { UNSG32 u32AIO_STR3;
            struct w32AIO_STR3;
          };
    #define     w32AIO_STR4                                    {\
            UNSG32 uSTR_I2STX5                                 : 32;\
          }
    union { UNSG32 u32AIO_STR4;
            struct w32AIO_STR4;
          };
    #define     w32AIO_STR5                                    {\
            UNSG32 uSTR_I2SRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR5;
            struct w32AIO_STR5;
          };
    #define     w32AIO_STR6                                    {\
            UNSG32 uSTR_I2SRX2                                 : 32;\
          }
    union { UNSG32 u32AIO_STR6;
            struct w32AIO_STR6;
          };
    #define     w32AIO_STR7                                    {\
            UNSG32 uSTR_I2SRX3                                 : 32;\
          }
    union { UNSG32 u32AIO_STR7;
            struct w32AIO_STR7;
          };
    #define     w32AIO_STR8                                    {\
            UNSG32 uSTR_I2SRX4                                 : 32;\
          }
    union { UNSG32 u32AIO_STR8;
            struct w32AIO_STR8;
          };
    #define     w32AIO_STR9                                    {\
            UNSG32 uSTR_I2SRX5                                 : 32;\
          }
    union { UNSG32 u32AIO_STR9;
            struct w32AIO_STR9;
          };
    #define     w32AIO_STR10                                   {\
            UNSG32 uSTR_PDMRX                                  : 32;\
          }
    union { UNSG32 u32AIO_STR10;
            struct w32AIO_STR10;
          };
    #define     w32AIO_STR11                                   {\
            UNSG32 uSTR_DMICRX                                 : 32;\
          }
    union { UNSG32 u32AIO_STR11;
            struct w32AIO_STR11;
          };
    #define     w32AIO_STR12                                   {\
            UNSG32 uSTR_DRT_DA                                 : 32;\
          }
    union { UNSG32 u32AIO_STR12;
            struct w32AIO_STR12;
          };
    #define     w32AIO_STR13                                   {\
            UNSG32 uSTR_DRT_AD                                 : 32;\
          }
    union { UNSG32 u32AIO_STR13;
            struct w32AIO_STR13;
          };
    #define     w32AIO_STR14                                   {\
            UNSG32 uSTR_DCLASS                                 : 32;\
          }
    union { UNSG32 u32AIO_STR14;
            struct w32AIO_STR14;
          };
    #define     w32AIO_ATR                                     {\
            UNSG32 uATR_TIMER                                  : 32;\
          }
    union { UNSG32 u32AIO_ATR;
            struct w32AIO_ATR;
          };
    #define     w32AIO_XFEED1                                  {\
            UNSG32 uXFEED1_I2S1_LRCKIO_MODE                    :  4;\
            UNSG32 uXFEED1_I2S1_BCLKIO_MODE                    :  4;\
            UNSG32 uXFEED1_I2S2_LRCKIO_MODE                    :  4;\
            UNSG32 uXFEED1_I2S2_BCLKIO_MODE                    :  4;\
            UNSG32 uXFEED1_I2S3_LRCKIO_MODE                    :  4;\
            UNSG32 uXFEED1_I2S3_BCLKIO_MODE                    :  4;\
            UNSG32 RSVDx370_b24                                :  8;\
          }
    union { UNSG32 u32AIO_XFEED1;
            struct w32AIO_XFEED1;
          };
    #define     w32AIO_XFEED2                                  {\
            UNSG32 uXFEED2_I2S4_LRCKIO_MODE                    :  4;\
            UNSG32 uXFEED2_I2S4_BCLKIO_MODE                    :  4;\
            UNSG32 uXFEED2_I2S5_LRCKIO_MODE                    :  4;\
            UNSG32 uXFEED2_I2S5_BCLKIO_MODE                    :  4;\
            UNSG32 uXFEED2_PDM_CLK_SEL                         :  3;\
            UNSG32 RSVDx374_b19                                : 13;\
          }
    union { UNSG32 u32AIO_XFEED2;
            struct w32AIO_XFEED2;
          };
              SIE_SRAMPWR                                      ie_DMIC_SRAMPWR;
              SIE_AIODRT                                       ie_DRT;
              SIE_AIODCLASS                                    ie_DCLASS;
    #define     w32AIO_APRKT_EN                                {\
            UNSG32 uAPRKT_EN_SEC1                              :  1;\
            UNSG32 uAPRKT_EN_SEC2                              :  1;\
            UNSG32 uAPRKT_EN_SEC3                              :  1;\
            UNSG32 uAPRKT_EN_SEC4                              :  1;\
            UNSG32 uAPRKT_EN_SEC5                              :  1;\
            UNSG32 uAPRKT_EN_MIC21                             :  1;\
            UNSG32 uAPRKT_EN_MIC22                             :  1;\
            UNSG32 uAPRKT_EN_MIC23                             :  1;\
            UNSG32 uAPRKT_EN_MIC24                             :  1;\
            UNSG32 uAPRKT_EN_MIC25                             :  1;\
            UNSG32 uAPRKT_EN_PDM                               :  1;\
            UNSG32 uAPRKT_EN_DMIC                              :  1;\
            UNSG32 uAPRKT_EN_DRT_DA                            :  1;\
            UNSG32 uAPRKT_EN_DRT_AD                            :  1;\
            UNSG32 uAPRKT_EN_DCLASS                            :  1;\
            UNSG32 RSVDx3A0_b15                                : 17;\
          }
    union { UNSG32 u32AIO_APRKT_EN;
            struct w32AIO_APRKT_EN;
          };
    #define     w32AIO_STARTED                                 {\
            UNSG32 uSTARTED_SEC1                               :  1;\
            UNSG32 uSTARTED_SEC2                               :  1;\
            UNSG32 uSTARTED_SEC3                               :  1;\
            UNSG32 uSTARTED_SEC4                               :  1;\
            UNSG32 uSTARTED_SEC5                               :  1;\
            UNSG32 uSTARTED_MIC21                              :  1;\
            UNSG32 uSTARTED_MIC22                              :  1;\
            UNSG32 uSTARTED_MIC23                              :  1;\
            UNSG32 uSTARTED_MIC24                              :  1;\
            UNSG32 uSTARTED_MIC25                              :  1;\
            UNSG32 uSTARTED_PDM                                :  1;\
            UNSG32 uSTARTED_DMIC                               :  1;\
            UNSG32 uSTARTED_DRT_DA                             :  1;\
            UNSG32 uSTARTED_DRT_AD                             :  1;\
            UNSG32 uSTARTED_DCLASS                             :  1;\
            UNSG32 RSVDx3A4_b15                                : 17;\
          }
    union { UNSG32 u32AIO_STARTED;
            struct w32AIO_STARTED;
          };
    #define     w32AIO_APRPR                                   {\
            UNSG32 uAPRPR_SEC1_PE                              :  1;\
            UNSG32 uAPRPR_SEC2_PE                              :  1;\
            UNSG32 uAPRPR_SEC3_PE                              :  1;\
            UNSG32 uAPRPR_SEC4_PE                              :  1;\
            UNSG32 uAPRPR_SEC5_PE                              :  1;\
            UNSG32 uAPRPR_MIC21_PE                             :  1;\
            UNSG32 uAPRPR_MIC22_PE                             :  1;\
            UNSG32 uAPRPR_MIC23_PE                             :  1;\
            UNSG32 uAPRPR_MIC24_PE                             :  1;\
            UNSG32 uAPRPR_MIC25_PE                             :  1;\
            UNSG32 uAPRPR_SEC1_RSE                             :  1;\
            UNSG32 uAPRPR_SEC2_RSE                             :  1;\
            UNSG32 uAPRPR_SEC3_RSE                             :  1;\
            UNSG32 uAPRPR_SEC4_RSE                             :  1;\
            UNSG32 uAPRPR_SEC5_RSE                             :  1;\
            UNSG32 uAPRPR_MIC21_RSE                            :  1;\
            UNSG32 uAPRPR_MIC22_RSE                            :  1;\
            UNSG32 uAPRPR_MIC23_RSE                            :  1;\
            UNSG32 uAPRPR_MIC24_RSE                            :  1;\
            UNSG32 uAPRPR_MIC25_RSE                            :  1;\
            UNSG32 RSVDx3A8_b20                                : 12;\
          }
    union { UNSG32 u32AIO_APRPR;
            struct w32AIO_APRPR;
          };
    #define     w32AIO_GPIO_ETM                                {\
            UNSG32 uGPIO_ETM_SEC1                              :  1;\
            UNSG32 uGPIO_ETM_SEC2                              :  1;\
            UNSG32 uGPIO_ETM_SEC3                              :  1;\
            UNSG32 uGPIO_ETM_SEC4                              :  1;\
            UNSG32 uGPIO_ETM_SEC5                              :  1;\
            UNSG32 uGPIO_ETM_MIC21                             :  1;\
            UNSG32 uGPIO_ETM_MIC22                             :  1;\
            UNSG32 uGPIO_ETM_MIC23                             :  1;\
            UNSG32 uGPIO_ETM_MIC24                             :  1;\
            UNSG32 uGPIO_ETM_MIC25                             :  1;\
            UNSG32 uGPIO_ETM_PDM                               :  1;\
            UNSG32 uGPIO_ETM_DMIC                              :  1;\
            UNSG32 uGPIO_ETM_DRT_DA                            :  1;\
            UNSG32 uGPIO_ETM_DRT_AD                            :  1;\
            UNSG32 uGPIO_ETM_DCLASS                            :  1;\
            UNSG32 RSVDx3AC_b15                                : 17;\
          }
    union { UNSG32 u32AIO_GPIO_ETM;
            struct w32AIO_GPIO_ETM;
          };
    #define     w32AIO_GPIO_TRIG                               {\
            UNSG32 uGPIO_TRIG_SEC1                             :  1;\
            UNSG32 uGPIO_TRIG_SEC2                             :  1;\
            UNSG32 uGPIO_TRIG_SEC3                             :  1;\
            UNSG32 uGPIO_TRIG_SEC4                             :  1;\
            UNSG32 uGPIO_TRIG_SEC5                             :  1;\
            UNSG32 uGPIO_TRIG_MIC21                            :  1;\
            UNSG32 uGPIO_TRIG_MIC22                            :  1;\
            UNSG32 uGPIO_TRIG_MIC23                            :  1;\
            UNSG32 uGPIO_TRIG_MIC24                            :  1;\
            UNSG32 uGPIO_TRIG_MIC25                            :  1;\
            UNSG32 uGPIO_TRIG_PDM                              :  1;\
            UNSG32 uGPIO_TRIG_DMIC                             :  1;\
            UNSG32 uGPIO_TRIG_DRT_DA                           :  1;\
            UNSG32 uGPIO_TRIG_DRT_AD                           :  1;\
            UNSG32 uGPIO_TRIG_DCLASS                           :  1;\
            UNSG32 RSVDx3B0_b15                                : 17;\
          }
    union { UNSG32 u32AIO_GPIO_TRIG;
            struct w32AIO_GPIO_TRIG;
          };
    #define     w32AIO_GPIO_TRIG_SEL                           {\
            UNSG32 uGPIO_TRIG_SEL_SEC1                         :  2;\
            UNSG32 uGPIO_TRIG_SEL_SEC2                         :  2;\
            UNSG32 uGPIO_TRIG_SEL_SEC3                         :  2;\
            UNSG32 uGPIO_TRIG_SEL_SEC4                         :  2;\
            UNSG32 uGPIO_TRIG_SEL_SEC5                         :  2;\
            UNSG32 uGPIO_TRIG_SEL_MIC21                        :  2;\
            UNSG32 uGPIO_TRIG_SEL_MIC22                        :  2;\
            UNSG32 uGPIO_TRIG_SEL_MIC23                        :  2;\
            UNSG32 uGPIO_TRIG_SEL_MIC24                        :  2;\
            UNSG32 uGPIO_TRIG_SEL_MIC25                        :  2;\
            UNSG32 uGPIO_TRIG_SEL_PDM                          :  2;\
            UNSG32 uGPIO_TRIG_SEL_DMIC                         :  2;\
            UNSG32 uGPIO_TRIG_SEL_DRT_DA                       :  2;\
            UNSG32 uGPIO_TRIG_SEL_DRT_AD                       :  2;\
            UNSG32 uGPIO_TRIG_SEL_DCLASS                       :  2;\
            UNSG32 RSVDx3B4_b30                                :  2;\
          }
    union { UNSG32 u32AIO_GPIO_TRIG_SEL;
            struct w32AIO_GPIO_TRIG_SEL;
          };
    #define     w32AIO_GPIO_TRIG_CLR                           {\
            UNSG32 uGPIO_TRIG_CLR_SEC1                         :  1;\
            UNSG32 uGPIO_TRIG_CLR_SEC2                         :  1;\
            UNSG32 uGPIO_TRIG_CLR_SEC3                         :  1;\
            UNSG32 uGPIO_TRIG_CLR_SEC4                         :  1;\
            UNSG32 uGPIO_TRIG_CLR_SEC5                         :  1;\
            UNSG32 uGPIO_TRIG_CLR_MIC21                        :  1;\
            UNSG32 uGPIO_TRIG_CLR_MIC22                        :  1;\
            UNSG32 uGPIO_TRIG_CLR_MIC23                        :  1;\
            UNSG32 uGPIO_TRIG_CLR_MIC24                        :  1;\
            UNSG32 uGPIO_TRIG_CLR_MIC25                        :  1;\
            UNSG32 uGPIO_TRIG_CLR_PDM                          :  1;\
            UNSG32 uGPIO_TRIG_CLR_DMIC                         :  1;\
            UNSG32 uGPIO_TRIG_CLR_DRT_DA                       :  1;\
            UNSG32 uGPIO_TRIG_CLR_DRT_AD                       :  1;\
            UNSG32 uGPIO_TRIG_CLR_DCLASS                       :  1;\
            UNSG32 RSVDx3B8_b15                                : 17;\
          }
    union { UNSG32 u32AIO_GPIO_TRIG_CLR;
            struct w32AIO_GPIO_TRIG_CLR;
          };
    #define     w32AIO_PAUSED                                  {\
            UNSG32 uPAUSED_SEC1                                :  1;\
            UNSG32 uPAUSED_SEC2                                :  1;\
            UNSG32 uPAUSED_SEC3                                :  1;\
            UNSG32 uPAUSED_SEC4                                :  1;\
            UNSG32 uPAUSED_SEC5                                :  1;\
            UNSG32 uPAUSED_MIC21                               :  1;\
            UNSG32 uPAUSED_MIC22                               :  1;\
            UNSG32 uPAUSED_MIC23                               :  1;\
            UNSG32 uPAUSED_MIC24                               :  1;\
            UNSG32 uPAUSED_MIC25                               :  1;\
            UNSG32 RSVDx3BC_b10                                : 22;\
          }
    union { UNSG32 u32AIO_PAUSED;
            struct w32AIO_PAUSED;
          };
    #define     w32AIO_INT_SCNT_CLR                            {\
            UNSG32 uINT_SCNT_CLR_SEC1                          :  1;\
            UNSG32 uINT_SCNT_CLR_SEC2                          :  1;\
            UNSG32 uINT_SCNT_CLR_SEC3                          :  1;\
            UNSG32 uINT_SCNT_CLR_SEC4                          :  1;\
            UNSG32 uINT_SCNT_CLR_SEC5                          :  1;\
            UNSG32 uINT_SCNT_CLR_MIC21                         :  1;\
            UNSG32 uINT_SCNT_CLR_MIC22                         :  1;\
            UNSG32 uINT_SCNT_CLR_MIC23                         :  1;\
            UNSG32 uINT_SCNT_CLR_MIC24                         :  1;\
            UNSG32 uINT_SCNT_CLR_MIC25                         :  1;\
            UNSG32 uINT_SCNT_CLR_PDM                           :  1;\
            UNSG32 uINT_SCNT_CLR_DMIC                          :  1;\
            UNSG32 uINT_SCNT_CLR_DRT_DA                        :  1;\
            UNSG32 uINT_SCNT_CLR_DRT_AD                        :  1;\
            UNSG32 uINT_SCNT_CLR_DCLASS                        :  1;\
            UNSG32 RSVDx3C0_b15                                : 17;\
          }
    union { UNSG32 u32AIO_INT_SCNT_CLR;
            struct w32AIO_INT_SCNT_CLR;
          };
    #define     w32AIO_FORCEMUTE                               {\
            UNSG32 uFORCEMUTE_DEB_DLY                          :  8;\
            UNSG32 uFORCEMUTE_EFF_DLY                          :  8;\
            UNSG32 uFORCEMUTE_SW_MUTE_I2S1                     :  1;\
            UNSG32 uFORCEMUTE_SW_MUTE_I2S2                     :  1;\
            UNSG32 uFORCEMUTE_SW_MUTE_I2S3                     :  1;\
            UNSG32 uFORCEMUTE_SW_MUTE_I2S4                     :  1;\
            UNSG32 uFORCEMUTE_SW_MUTE_I2S5                     :  1;\
            UNSG32 uFORCEMUTE_SW_MUTE_PDM                      :  4;\
            UNSG32 RSVDx3C4_b25                                :  7;\
          }
    union { UNSG32 u32AIO_FORCEMUTE;
            struct w32AIO_FORCEMUTE;
          };
    } SIE_AIO;
    typedef union  T32AIO_IRQENABLE
          { UNSG32 u32;
            struct w32AIO_IRQENABLE;
                 } T32AIO_IRQENABLE;
    typedef union  T32AIO_IRQENABLE1
          { UNSG32 u32;
            struct w32AIO_IRQENABLE1;
                 } T32AIO_IRQENABLE1;
    typedef union  T32AIO_IRQENABLE2
          { UNSG32 u32;
            struct w32AIO_IRQENABLE2;
                 } T32AIO_IRQENABLE2;
    typedef union  T32AIO_IRQENABLE3
          { UNSG32 u32;
            struct w32AIO_IRQENABLE3;
                 } T32AIO_IRQENABLE3;
    typedef union  T32AIO_IRQENABLE4
          { UNSG32 u32;
            struct w32AIO_IRQENABLE4;
                 } T32AIO_IRQENABLE4;
    typedef union  T32AIO_IRQENABLE5
          { UNSG32 u32;
            struct w32AIO_IRQENABLE5;
                 } T32AIO_IRQENABLE5;
    typedef union  T32AIO_IRQENABLE6
          { UNSG32 u32;
            struct w32AIO_IRQENABLE6;
                 } T32AIO_IRQENABLE6;
    typedef union  T32AIO_IRQENABLE7
          { UNSG32 u32;
            struct w32AIO_IRQENABLE7;
                 } T32AIO_IRQENABLE7;
    typedef union  T32AIO_IRQENABLE8
          { UNSG32 u32;
            struct w32AIO_IRQENABLE8;
                 } T32AIO_IRQENABLE8;
    typedef union  T32AIO_IRQENABLE9
          { UNSG32 u32;
            struct w32AIO_IRQENABLE9;
                 } T32AIO_IRQENABLE9;
    typedef union  T32AIO_IRQENABLE10
          { UNSG32 u32;
            struct w32AIO_IRQENABLE10;
                 } T32AIO_IRQENABLE10;
    typedef union  T32AIO_IRQENABLE11
          { UNSG32 u32;
            struct w32AIO_IRQENABLE11;
                 } T32AIO_IRQENABLE11;
    typedef union  T32AIO_IRQSTS
          { UNSG32 u32;
            struct w32AIO_IRQSTS;
                 } T32AIO_IRQSTS;
    typedef union  T32AIO_IRQSTS1
          { UNSG32 u32;
            struct w32AIO_IRQSTS1;
                 } T32AIO_IRQSTS1;
    typedef union  T32AIO_IRQSTS2
          { UNSG32 u32;
            struct w32AIO_IRQSTS2;
                 } T32AIO_IRQSTS2;
    typedef union  T32AIO_IRQSTS3
          { UNSG32 u32;
            struct w32AIO_IRQSTS3;
                 } T32AIO_IRQSTS3;
    typedef union  T32AIO_IRQSTS4
          { UNSG32 u32;
            struct w32AIO_IRQSTS4;
                 } T32AIO_IRQSTS4;
    typedef union  T32AIO_IRQSTS5
          { UNSG32 u32;
            struct w32AIO_IRQSTS5;
                 } T32AIO_IRQSTS5;
    typedef union  T32AIO_IRQSTS6
          { UNSG32 u32;
            struct w32AIO_IRQSTS6;
                 } T32AIO_IRQSTS6;
    typedef union  T32AIO_IRQSTS7
          { UNSG32 u32;
            struct w32AIO_IRQSTS7;
                 } T32AIO_IRQSTS7;
    typedef union  T32AIO_IRQSTS8
          { UNSG32 u32;
            struct w32AIO_IRQSTS8;
                 } T32AIO_IRQSTS8;
    typedef union  T32AIO_IRQSTS9
          { UNSG32 u32;
            struct w32AIO_IRQSTS9;
                 } T32AIO_IRQSTS9;
    typedef union  T32AIO_IRQSTS10
          { UNSG32 u32;
            struct w32AIO_IRQSTS10;
                 } T32AIO_IRQSTS10;
    typedef union  T32AIO_IRQSTS11
          { UNSG32 u32;
            struct w32AIO_IRQSTS11;
                 } T32AIO_IRQSTS11;
    typedef union  T32AIO_SW_RST
          { UNSG32 u32;
            struct w32AIO_SW_RST;
                 } T32AIO_SW_RST;
    typedef union  T32AIO_CLK_GATE_EN
          { UNSG32 u32;
            struct w32AIO_CLK_GATE_EN;
                 } T32AIO_CLK_GATE_EN;
    typedef union  T32AIO_SAMP_CTRL
          { UNSG32 u32;
            struct w32AIO_SAMP_CTRL;
                 } T32AIO_SAMP_CTRL;
    typedef union  T32AIO_SAMPINFO_REQ
          { UNSG32 u32;
            struct w32AIO_SAMPINFO_REQ;
                 } T32AIO_SAMPINFO_REQ;
    typedef union  T32AIO_SCR
          { UNSG32 u32;
            struct w32AIO_SCR;
                 } T32AIO_SCR;
    typedef union  T32AIO_SCR1
          { UNSG32 u32;
            struct w32AIO_SCR1;
                 } T32AIO_SCR1;
    typedef union  T32AIO_SCR2
          { UNSG32 u32;
            struct w32AIO_SCR2;
                 } T32AIO_SCR2;
    typedef union  T32AIO_SCR3
          { UNSG32 u32;
            struct w32AIO_SCR3;
                 } T32AIO_SCR3;
    typedef union  T32AIO_SCR4
          { UNSG32 u32;
            struct w32AIO_SCR4;
                 } T32AIO_SCR4;
    typedef union  T32AIO_SCR5
          { UNSG32 u32;
            struct w32AIO_SCR5;
                 } T32AIO_SCR5;
    typedef union  T32AIO_SCR6
          { UNSG32 u32;
            struct w32AIO_SCR6;
                 } T32AIO_SCR6;
    typedef union  T32AIO_SCR7
          { UNSG32 u32;
            struct w32AIO_SCR7;
                 } T32AIO_SCR7;
    typedef union  T32AIO_SCR8
          { UNSG32 u32;
            struct w32AIO_SCR8;
                 } T32AIO_SCR8;
    typedef union  T32AIO_SCR9
          { UNSG32 u32;
            struct w32AIO_SCR9;
                 } T32AIO_SCR9;
    typedef union  T32AIO_SCR10
          { UNSG32 u32;
            struct w32AIO_SCR10;
                 } T32AIO_SCR10;
    typedef union  T32AIO_SCR11
          { UNSG32 u32;
            struct w32AIO_SCR11;
                 } T32AIO_SCR11;
    typedef union  T32AIO_SCR12
          { UNSG32 u32;
            struct w32AIO_SCR12;
                 } T32AIO_SCR12;
    typedef union  T32AIO_SCR13
          { UNSG32 u32;
            struct w32AIO_SCR13;
                 } T32AIO_SCR13;
    typedef union  T32AIO_SCR14
          { UNSG32 u32;
            struct w32AIO_SCR14;
                 } T32AIO_SCR14;
    typedef union  T32AIO_STR
          { UNSG32 u32;
            struct w32AIO_STR;
                 } T32AIO_STR;
    typedef union  T32AIO_STR1
          { UNSG32 u32;
            struct w32AIO_STR1;
                 } T32AIO_STR1;
    typedef union  T32AIO_STR2
          { UNSG32 u32;
            struct w32AIO_STR2;
                 } T32AIO_STR2;
    typedef union  T32AIO_STR3
          { UNSG32 u32;
            struct w32AIO_STR3;
                 } T32AIO_STR3;
    typedef union  T32AIO_STR4
          { UNSG32 u32;
            struct w32AIO_STR4;
                 } T32AIO_STR4;
    typedef union  T32AIO_STR5
          { UNSG32 u32;
            struct w32AIO_STR5;
                 } T32AIO_STR5;
    typedef union  T32AIO_STR6
          { UNSG32 u32;
            struct w32AIO_STR6;
                 } T32AIO_STR6;
    typedef union  T32AIO_STR7
          { UNSG32 u32;
            struct w32AIO_STR7;
                 } T32AIO_STR7;
    typedef union  T32AIO_STR8
          { UNSG32 u32;
            struct w32AIO_STR8;
                 } T32AIO_STR8;
    typedef union  T32AIO_STR9
          { UNSG32 u32;
            struct w32AIO_STR9;
                 } T32AIO_STR9;
    typedef union  T32AIO_STR10
          { UNSG32 u32;
            struct w32AIO_STR10;
                 } T32AIO_STR10;
    typedef union  T32AIO_STR11
          { UNSG32 u32;
            struct w32AIO_STR11;
                 } T32AIO_STR11;
    typedef union  T32AIO_STR12
          { UNSG32 u32;
            struct w32AIO_STR12;
                 } T32AIO_STR12;
    typedef union  T32AIO_STR13
          { UNSG32 u32;
            struct w32AIO_STR13;
                 } T32AIO_STR13;
    typedef union  T32AIO_STR14
          { UNSG32 u32;
            struct w32AIO_STR14;
                 } T32AIO_STR14;
    typedef union  T32AIO_ATR
          { UNSG32 u32;
            struct w32AIO_ATR;
                 } T32AIO_ATR;
    typedef union  T32AIO_XFEED1
          { UNSG32 u32;
            struct w32AIO_XFEED1;
                 } T32AIO_XFEED1;
    typedef union  T32AIO_XFEED2
          { UNSG32 u32;
            struct w32AIO_XFEED2;
                 } T32AIO_XFEED2;
    typedef union  T32AIO_APRKT_EN
          { UNSG32 u32;
            struct w32AIO_APRKT_EN;
                 } T32AIO_APRKT_EN;
    typedef union  T32AIO_STARTED
          { UNSG32 u32;
            struct w32AIO_STARTED;
                 } T32AIO_STARTED;
    typedef union  T32AIO_APRPR
          { UNSG32 u32;
            struct w32AIO_APRPR;
                 } T32AIO_APRPR;
    typedef union  T32AIO_GPIO_ETM
          { UNSG32 u32;
            struct w32AIO_GPIO_ETM;
                 } T32AIO_GPIO_ETM;
    typedef union  T32AIO_GPIO_TRIG
          { UNSG32 u32;
            struct w32AIO_GPIO_TRIG;
                 } T32AIO_GPIO_TRIG;
    typedef union  T32AIO_GPIO_TRIG_SEL
          { UNSG32 u32;
            struct w32AIO_GPIO_TRIG_SEL;
                 } T32AIO_GPIO_TRIG_SEL;
    typedef union  T32AIO_GPIO_TRIG_CLR
          { UNSG32 u32;
            struct w32AIO_GPIO_TRIG_CLR;
                 } T32AIO_GPIO_TRIG_CLR;
    typedef union  T32AIO_PAUSED
          { UNSG32 u32;
            struct w32AIO_PAUSED;
                 } T32AIO_PAUSED;
    typedef union  T32AIO_INT_SCNT_CLR
          { UNSG32 u32;
            struct w32AIO_INT_SCNT_CLR;
                 } T32AIO_INT_SCNT_CLR;
    typedef union  T32AIO_FORCEMUTE
          { UNSG32 u32;
            struct w32AIO_FORCEMUTE;
                 } T32AIO_FORCEMUTE;
    typedef union  TAIO_IRQENABLE
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE;
                   };
                 } TAIO_IRQENABLE;
    typedef union  TAIO_IRQENABLE1
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE1;
                   };
                 } TAIO_IRQENABLE1;
    typedef union  TAIO_IRQENABLE2
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE2;
                   };
                 } TAIO_IRQENABLE2;
    typedef union  TAIO_IRQENABLE3
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE3;
                   };
                 } TAIO_IRQENABLE3;
    typedef union  TAIO_IRQENABLE4
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE4;
                   };
                 } TAIO_IRQENABLE4;
    typedef union  TAIO_IRQENABLE5
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE5;
                   };
                 } TAIO_IRQENABLE5;
    typedef union  TAIO_IRQENABLE6
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE6;
                   };
                 } TAIO_IRQENABLE6;
    typedef union  TAIO_IRQENABLE7
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE7;
                   };
                 } TAIO_IRQENABLE7;
    typedef union  TAIO_IRQENABLE8
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE8;
                   };
                 } TAIO_IRQENABLE8;
    typedef union  TAIO_IRQENABLE9
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE9;
                   };
                 } TAIO_IRQENABLE9;
    typedef union  TAIO_IRQENABLE10
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE10;
                   };
                 } TAIO_IRQENABLE10;
    typedef union  TAIO_IRQENABLE11
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE11;
                   };
                 } TAIO_IRQENABLE11;
    typedef union  TAIO_IRQSTS
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS;
                   };
                 } TAIO_IRQSTS;
    typedef union  TAIO_IRQSTS1
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS1;
                   };
                 } TAIO_IRQSTS1;
    typedef union  TAIO_IRQSTS2
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS2;
                   };
                 } TAIO_IRQSTS2;
    typedef union  TAIO_IRQSTS3
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS3;
                   };
                 } TAIO_IRQSTS3;
    typedef union  TAIO_IRQSTS4
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS4;
                   };
                 } TAIO_IRQSTS4;
    typedef union  TAIO_IRQSTS5
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS5;
                   };
                 } TAIO_IRQSTS5;
    typedef union  TAIO_IRQSTS6
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS6;
                   };
                 } TAIO_IRQSTS6;
    typedef union  TAIO_IRQSTS7
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS7;
                   };
                 } TAIO_IRQSTS7;
    typedef union  TAIO_IRQSTS8
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS8;
                   };
                 } TAIO_IRQSTS8;
    typedef union  TAIO_IRQSTS9
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS9;
                   };
                 } TAIO_IRQSTS9;
    typedef union  TAIO_IRQSTS10
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS10;
                   };
                 } TAIO_IRQSTS10;
    typedef union  TAIO_IRQSTS11
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS11;
                   };
                 } TAIO_IRQSTS11;
    typedef union  TAIO_SW_RST
          { UNSG32 u32[1];
            struct {
            struct w32AIO_SW_RST;
                   };
                 } TAIO_SW_RST;
    typedef union  TAIO_CLK_GATE_EN
          { UNSG32 u32[1];
            struct {
            struct w32AIO_CLK_GATE_EN;
                   };
                 } TAIO_CLK_GATE_EN;
    typedef union  TAIO_SAMP_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AIO_SAMP_CTRL;
                   };
                 } TAIO_SAMP_CTRL;
    typedef union  TAIO_SAMPINFO_REQ
          { UNSG32 u32[1];
            struct {
            struct w32AIO_SAMPINFO_REQ;
                   };
                 } TAIO_SAMPINFO_REQ;
    typedef union  TAIO_SCR
          { UNSG32 u32[15];
            struct {
            struct w32AIO_SCR;
            struct w32AIO_SCR1;
            struct w32AIO_SCR2;
            struct w32AIO_SCR3;
            struct w32AIO_SCR4;
            struct w32AIO_SCR5;
            struct w32AIO_SCR6;
            struct w32AIO_SCR7;
            struct w32AIO_SCR8;
            struct w32AIO_SCR9;
            struct w32AIO_SCR10;
            struct w32AIO_SCR11;
            struct w32AIO_SCR12;
            struct w32AIO_SCR13;
            struct w32AIO_SCR14;
                   };
                 } TAIO_SCR;
    typedef union  TAIO_STR
          { UNSG32 u32[15];
            struct {
            struct w32AIO_STR;
            struct w32AIO_STR1;
            struct w32AIO_STR2;
            struct w32AIO_STR3;
            struct w32AIO_STR4;
            struct w32AIO_STR5;
            struct w32AIO_STR6;
            struct w32AIO_STR7;
            struct w32AIO_STR8;
            struct w32AIO_STR9;
            struct w32AIO_STR10;
            struct w32AIO_STR11;
            struct w32AIO_STR12;
            struct w32AIO_STR13;
            struct w32AIO_STR14;
                   };
                 } TAIO_STR;
    typedef union  TAIO_ATR
          { UNSG32 u32[1];
            struct {
            struct w32AIO_ATR;
                   };
                 } TAIO_ATR;
    typedef union  TAIO_XFEED1
          { UNSG32 u32[1];
            struct {
            struct w32AIO_XFEED1;
                   };
                 } TAIO_XFEED1;
    typedef union  TAIO_XFEED2
          { UNSG32 u32[1];
            struct {
            struct w32AIO_XFEED2;
                   };
                 } TAIO_XFEED2;
    typedef union  TAIO_APRKT_EN
          { UNSG32 u32[1];
            struct {
            struct w32AIO_APRKT_EN;
                   };
                 } TAIO_APRKT_EN;
    typedef union  TAIO_STARTED
          { UNSG32 u32[1];
            struct {
            struct w32AIO_STARTED;
                   };
                 } TAIO_STARTED;
    typedef union  TAIO_APRPR
          { UNSG32 u32[1];
            struct {
            struct w32AIO_APRPR;
                   };
                 } TAIO_APRPR;
    typedef union  TAIO_GPIO_ETM
          { UNSG32 u32[1];
            struct {
            struct w32AIO_GPIO_ETM;
                   };
                 } TAIO_GPIO_ETM;
    typedef union  TAIO_GPIO_TRIG
          { UNSG32 u32[1];
            struct {
            struct w32AIO_GPIO_TRIG;
                   };
                 } TAIO_GPIO_TRIG;
    typedef union  TAIO_GPIO_TRIG_SEL
          { UNSG32 u32[1];
            struct {
            struct w32AIO_GPIO_TRIG_SEL;
                   };
                 } TAIO_GPIO_TRIG_SEL;
    typedef union  TAIO_GPIO_TRIG_CLR
          { UNSG32 u32[1];
            struct {
            struct w32AIO_GPIO_TRIG_CLR;
                   };
                 } TAIO_GPIO_TRIG_CLR;
    typedef union  TAIO_PAUSED
          { UNSG32 u32[1];
            struct {
            struct w32AIO_PAUSED;
                   };
                 } TAIO_PAUSED;
    typedef union  TAIO_INT_SCNT_CLR
          { UNSG32 u32[1];
            struct {
            struct w32AIO_INT_SCNT_CLR;
                   };
                 } TAIO_INT_SCNT_CLR;
    typedef union  TAIO_FORCEMUTE
          { UNSG32 u32[1];
            struct {
            struct w32AIO_FORCEMUTE;
                   };
                 } TAIO_FORCEMUTE;
     SIGN32 AIO_drvrd(SIE_AIO *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AIO_drvwr(SIE_AIO *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AIO_reset(SIE_AIO *p);
     SIGN32 AIO_cmp  (SIE_AIO *p, SIE_AIO *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AIO_check(p,pie,pfx,hLOG) AIO_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AIO_print(p,    pfx,hLOG) AIO_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
