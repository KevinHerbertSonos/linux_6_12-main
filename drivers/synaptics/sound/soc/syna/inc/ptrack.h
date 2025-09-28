/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2023 Synaptics Incorporated */

#include "dt-bindings/sound/syna_ptrack.h"
/**********************************************************************
 *		 Block definition for block PTRACK_BLK
 **********************************************************************/


/*
 * Enable Phase Tracker
0: Module is disabled
1:  Module is enabled
This field may be changed while module is enabled
 */
#define PTRACK_BUS_IF_CFG_PTRACK_EN_BIT                   (0x0)
#define PTRACK_BUS_IF_CFG_PTRACK_EN_WIDTH                 (0x1)
#define PTRACK_BUS_IF_CFG_PTRACK_EN_MASK                  (0x1)
#define PTRACK_BUS_IF_CFG_PTRACK_EN_SET(x)                (((x) & 0x1) << 0x0)
#define PTRACK_BUS_IF_CFG_PTRACK_EN_CLR                   (~0x1)

/*
 * The source of the data to be stored in BUF I/F FIFO
0 - Source is Rate Ratio (R) + Lock indication
1 - Source is Phase Out (Pout) + Lock indication
2 - Source is Phase In (Pa) + Lock indication
3 - Source is DPLL Error (e) + Lock indication
4 - Source is smoothed DPLL Error (s_error) + Lock indication
 */
#define PTRACK_BUS_IF_CFG_BUS_IF_DAT_SRC_BIT              (0x1)
#define PTRACK_BUS_IF_CFG_BUS_IF_DAT_SRC_WIDTH            (0x3)
#define PTRACK_BUS_IF_CFG_BUS_IF_DAT_SRC_MASK             (0xE)
#define PTRACK_BUS_IF_CFG_BUS_IF_DAT_SRC_SET(x)           (((x) & 0x7) << 0x1)
#define PTRACK_BUS_IF_CFG_BUS_IF_DAT_SRC_CLR              (~0xE)

/*
 * Reset BUF I/F
0 - Bus interface is not in reset state
1 - Bus interface is in reset state
This field may be changed while module is enabled
 */
#define PTRACK_BUS_IF_CFG_BUS_IF_RST_BIT                  (0x4)
#define PTRACK_BUS_IF_CFG_BUS_IF_RST_WIDTH                (0x1)
#define PTRACK_BUS_IF_CFG_BUS_IF_RST_MASK                 (0x10)
#define PTRACK_BUS_IF_CFG_BUS_IF_RST_SET(x)               (((x) & 0x1) << 0x4)
#define PTRACK_BUS_IF_CFG_BUS_IF_RST_CLR                  (~0x10)

/*
 * Bus Interface Clock Enable
0: Clock  is disabled
1:  Clock is enabled
This field may be changed while module is enabled
 */
#define PTRACK_BUS_IF_CFG_BUS_IF_CLK_EN_BIT               (0x5)
#define PTRACK_BUS_IF_CFG_BUS_IF_CLK_EN_WIDTH             (0x1)
#define PTRACK_BUS_IF_CFG_BUS_IF_CLK_EN_MASK              (0x20)
#define PTRACK_BUS_IF_CFG_BUS_IF_CLK_EN_SET(x)            (((x) & 0x1) << 0x5)
#define PTRACK_BUS_IF_CFG_BUS_IF_CLK_EN_CLR               (~0x20)

/*
 * Override FIFO I/F LSB with Lock Indication
0 - Do not Override
1 - Override
 */
#define PTRACK_BUS_IF_CFG_INC_LOCK_IN_FIFO_BIT            (0x6)
#define PTRACK_BUS_IF_CFG_INC_LOCK_IN_FIFO_WIDTH          (0x1)
#define PTRACK_BUS_IF_CFG_INC_LOCK_IN_FIFO_MASK           (0x40)
#define PTRACK_BUS_IF_CFG_INC_LOCK_IN_FIFO_SET(x)         (((x) & 0x1) << 0x6)
#define PTRACK_BUS_IF_CFG_INC_LOCK_IN_FIFO_CLR            (~0x40)

/*
 * Write with 0 for future compatibility
 */

/*
 * Decimation Ratio of BUF I/F FIFO push.
Push to FIFO occurs very (FIFO_PUSH_DECI+1)  arrivels of new data
When FIFO_PUSH_DECI = 0 evey new data is pushed.
 */
#define PTRACK_BUS_IF_CFG_FIFO_PUSH_DECI_BIT              (0x8)
#define PTRACK_BUS_IF_CFG_FIFO_PUSH_DECI_WIDTH            (0x5)
#define PTRACK_BUS_IF_CFG_FIFO_PUSH_DECI_MASK             (0x1F00)
#define PTRACK_BUS_IF_CFG_FIFO_PUSH_DECI_SET(x)           (((x) & 0x1F) << 0x8)
#define PTRACK_BUS_IF_CFG_FIFO_PUSH_DECI_CLR              (~0x1F00)

/*
 * Write with 0 for future compatibility
 */

/*
 * Watermark Level for BUS I/F FIFO.
When the number of unread samples in the output FIFO  is at least  BUF_IF_WM_LEVEL,  a BUFIF_WM_STAT status is set and PTRACK_wm signal becomes active.
Note that  BUF_IF_WM_LEVEL refers to AHB interface
 */
#define PTRACK_BUS_IF_CFG_BUS_IF_WM_LEVEL_BIT             (0x10)
#define PTRACK_BUS_IF_CFG_BUS_IF_WM_LEVEL_WIDTH           (0x7)
#define PTRACK_BUS_IF_CFG_BUS_IF_WM_LEVEL_MASK            (0x7F0000)
#define PTRACK_BUS_IF_CFG_BUS_IF_WM_LEVEL_SET(x)          (((x) & 0x7F) << 0x10)
#define PTRACK_BUS_IF_CFG_BUS_IF_WM_LEVEL_CLR             (~0x7F0000)

/*
 * Write with 0 for future compatibility
 */

/*
 * Number of  current data in Bus I/F FIFO
 */
#define PTRACK_BUS_IF_CFG_FIFO_LEVEL_BIT                  (0x18)
#define PTRACK_BUS_IF_CFG_FIFO_LEVEL_WIDTH                (0x7)
#define PTRACK_BUS_IF_CFG_FIFO_LEVEL_MASK                 (0x7F000000)

/*
 * Write with 0 for future compatibility
 */

/*
 * Tic Generator Mfin division factor.
Actual dibsion by (MFIN_DIV+1).
When MFIN_DIV = 0 divider input is passed unchged to divider output even if if the input is constant.
 */
#define PTRACK_TIC_GEN_CFG_MFIN_DIV_BIT                   (0x0)
#define PTRACK_TIC_GEN_CFG_MFIN_DIV_WIDTH                 (0xA)
#define PTRACK_TIC_GEN_CFG_MFIN_DIV_MASK                  (0x3FF)
#define PTRACK_TIC_GEN_CFG_MFIN_DIV_SET(x)                (((x) & 0x3FF) << 0x0)
#define PTRACK_TIC_GEN_CFG_MFIN_DIV_CLR                   (~0x3FF)

/*
 * Apply  syncronizer to Mfin in tic generator
0 - Synchronizer is not applied
1 - Synchronizer is applied
 */
#define PTRACK_TIC_GEN_CFG_MFIN_SYNC_EN_BIT               (0xA)
#define PTRACK_TIC_GEN_CFG_MFIN_SYNC_EN_WIDTH             (0x1)
#define PTRACK_TIC_GEN_CFG_MFIN_SYNC_EN_MASK              (0x400)
#define PTRACK_TIC_GEN_CFG_MFIN_SYNC_EN_SET(x)            (((x) & 0x1) << 0xA)
#define PTRACK_TIC_GEN_CFG_MFIN_SYNC_EN_CLR               (~0x400)

/*
 * Apply  syncronizer to Nfout in tic generator
0 - Synchronizer is not applied
1 - Synchronizer is applied
 */
#define PTRACK_TIC_GEN_CFG_FOUT_SYNC_EN_BIT               (0xB)
#define PTRACK_TIC_GEN_CFG_FOUT_SYNC_EN_WIDTH             (0x1)
#define PTRACK_TIC_GEN_CFG_FOUT_SYNC_EN_MASK              (0x800)
#define PTRACK_TIC_GEN_CFG_FOUT_SYNC_EN_SET(x)            (((x) & 0x1) << 0xB)
#define PTRACK_TIC_GEN_CFG_FOUT_SYNC_EN_CLR               (~0x800)

/*
 * Fix Mfin to a v alue of 1 - to be used when DMFIN rate is the same as ASRC operational clock.
0 - Mfin is low to high detector output
1 - Mfin=1
 */
#define PTRACK_TIC_GEN_CFG_DMFIN_IS_OP_CLK_BIT            (0xC)
#define PTRACK_TIC_GEN_CFG_DMFIN_IS_OP_CLK_WIDTH          (0x1)
#define PTRACK_TIC_GEN_CFG_DMFIN_IS_OP_CLK_MASK           (0x1000)
#define PTRACK_TIC_GEN_CFG_DMFIN_IS_OP_CLK_SET(x)         (((x) & 0x1) << 0xC)
#define PTRACK_TIC_GEN_CFG_DMFIN_IS_OP_CLK_CLR            (~0x1000)

/*
 * Enable Mfin low to high detection output
0 - Mfin disable
1 - Mfin enabled
 */
#define PTRACK_TIC_GEN_CFG_MFIN_EN_BIT                    (0xD)
#define PTRACK_TIC_GEN_CFG_MFIN_EN_WIDTH                  (0x1)
#define PTRACK_TIC_GEN_CFG_MFIN_EN_MASK                   (0x2000)
#define PTRACK_TIC_GEN_CFG_MFIN_EN_SET(x)                 (((x) & 0x1) << 0xD)
#define PTRACK_TIC_GEN_CFG_MFIN_EN_CLR                    (~0x2000)

/*
 * Enable NFOUT low to high detection output
0 - Nfout disable
1 - Nfout enabled
 */
#define PTRACK_TIC_GEN_CFG_FOUT_EN_BIT                    (0xE)
#define PTRACK_TIC_GEN_CFG_FOUT_EN_WIDTH                  (0x1)
#define PTRACK_TIC_GEN_CFG_FOUT_EN_MASK                   (0x4000)
#define PTRACK_TIC_GEN_CFG_FOUT_EN_SET(x)                 (((x) & 0x1) << 0xE)
#define PTRACK_TIC_GEN_CFG_FOUT_EN_CLR                    (~0x4000)

/*
 * Write with 0 for future compatibility
 */

/*
 * Tic Generator Nfout division factor.
Actual dibsion by (NFOUT_DIV+1).
When NFOUT_DIV = 0 divider input is passed unchged to divider output even if if the input is constant.
 */
#define PTRACK_TIC_GEN_CFG_NFOUT_DIV_BIT                  (0x10)
#define PTRACK_TIC_GEN_CFG_NFOUT_DIV_WIDTH                (0xA)
#define PTRACK_TIC_GEN_CFG_NFOUT_DIV_MASK                 (0x3FF0000)
#define PTRACK_TIC_GEN_CFG_NFOUT_DIV_SET(x)               (((x) & 0x3FF) << 0x10)
#define PTRACK_TIC_GEN_CFG_NFOUT_DIV_CLR                  (~0x3FF0000)

/*
 * Write with 0 for future compatibility
 */

/*
 *
 */
#define PTRACK_PHASE_MEAS_CFG_PHASE_SCALE_BIT             (0x0)
#define PTRACK_PHASE_MEAS_CFG_PHASE_SCALE_WIDTH           (0x4)
#define PTRACK_PHASE_MEAS_CFG_PHASE_SCALE_MASK            (0xF)
#define PTRACK_PHASE_MEAS_CFG_PHASE_SCALE_SET(x)          (((x) & 0xF) << 0x0)
#define PTRACK_PHASE_MEAS_CFG_PHASE_SCALE_CLR             (~0xF)

/*
 *
 */
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_SHIFT_BIT           (0x4)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_SHIFT_WIDTH         (0x4)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_SHIFT_MASK          (0xF0)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_SHIFT_SET(x)        (((x) & 0xF) << 0x4)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_SHIFT_CLR           (~0xF0)

/*
 * Enable fractional phase calculation
0 - Use only integer phase measurement
1 - Use fractional phase calculation
 */
#define PTRACK_PHASE_MEAS_CFG_FRAC_PHASE_CALC_EN_BIT      (0x8)
#define PTRACK_PHASE_MEAS_CFG_FRAC_PHASE_CALC_EN_WIDTH    (0x1)
#define PTRACK_PHASE_MEAS_CFG_FRAC_PHASE_CALC_EN_MASK     (0x100)
#define PTRACK_PHASE_MEAS_CFG_FRAC_PHASE_CALC_EN_SET(x)   (((x) & 0x1) << 0x8)
#define PTRACK_PHASE_MEAS_CFG_FRAC_PHASE_CALC_EN_CLR      (~0x100)

/*
 * Enable Calculation of T_RATIO estimation
0 - T_ratio = INIT_T_RATIO
1 - Estimate T_RATIO
 */
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_CALC_EN_BIT         (0x9)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_CALC_EN_WIDTH       (0x1)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_CALC_EN_MASK        (0x200)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_CALC_EN_SET(x)      (((x) & 0x1) << 0x9)
#define PTRACK_PHASE_MEAS_CFG_T_RATIO_CALC_EN_CLR         (~0x200)

/*
 * Write with 0 for future compatibility
 */

/*
 *
 */
#define PTRACK_DPLL_CFG_S_ERR_SHIFT_BIT                   (0x0)
#define PTRACK_DPLL_CFG_S_ERR_SHIFT_WIDTH                 (0x4)
#define PTRACK_DPLL_CFG_S_ERR_SHIFT_MASK                  (0xF)
#define PTRACK_DPLL_CFG_S_ERR_SHIFT_SET(x)                (((x) & 0xF) << 0x0)
#define PTRACK_DPLL_CFG_S_ERR_SHIFT_CLR                   (~0xF)

/*
 * Enable DPPL 1
0 - Only DPLL 0 is used
1 -Both DPLL0 and DPLL1 are used
 */
#define PTRACK_DPLL_CFG_DPLL_CORE1_EN_BIT                 (0x4)
#define PTRACK_DPLL_CFG_DPLL_CORE1_EN_WIDTH               (0x1)
#define PTRACK_DPLL_CFG_DPLL_CORE1_EN_MASK                (0x10)
#define PTRACK_DPLL_CFG_DPLL_CORE1_EN_SET(x)              (((x) & 0x1) << 0x4)
#define PTRACK_DPLL_CFG_DPLL_CORE1_EN_CLR                 (~0x10)

/*
 * Source of error used by lock monitoring
0 - Error source is DPLL0
1 -Error source is DPLL1
 */
#define PTRACK_DPLL_CFG_ERR_SRC_BIT                       (0x5)
#define PTRACK_DPLL_CFG_ERR_SRC_WIDTH                     (0x1)
#define PTRACK_DPLL_CFG_ERR_SRC_MASK                      (0x20)
#define PTRACK_DPLL_CFG_ERR_SRC_SET(x)                    (((x) & 0x1) << 0x5)
#define PTRACK_DPLL_CFG_ERR_SRC_CLR                       (~0x20)

/*
 * Source of Gear Index
0 - Source is  Gear Control FSM
1 - Source iis regsiter field GEAR_INDEX
 */
#define PTRACK_DPLL_CFG_GEAR_INDEX_SRC_BIT                (0x6)
#define PTRACK_DPLL_CFG_GEAR_INDEX_SRC_WIDTH              (0x1)
#define PTRACK_DPLL_CFG_GEAR_INDEX_SRC_MASK               (0x40)
#define PTRACK_DPLL_CFG_GEAR_INDEX_SRC_SET(x)             (((x) & 0x1) << 0x6)
#define PTRACK_DPLL_CFG_GEAR_INDEX_SRC_CLR                (~0x40)

/*
 *
 */
#define PTRACK_DPLL_CFG_GEAR_INDEX_VALUE_BIT              (0x7)
#define PTRACK_DPLL_CFG_GEAR_INDEX_VALUE_WIDTH            (0x3)
#define PTRACK_DPLL_CFG_GEAR_INDEX_VALUE_MASK             (0x380)
#define PTRACK_DPLL_CFG_GEAR_INDEX_VALUE_SET(x)           (((x) & 0x7) << 0x7)
#define PTRACK_DPLL_CFG_GEAR_INDEX_VALUE_CLR              (~0x380)

/*
 * Lock monitoring method
0 - Declear lock based on phase tracking error
1 - Declear lock based on counter expiration
 */
#define PTRACK_DPLL_CFG_LOCK_ON_CNT_BIT                   (0xA)
#define PTRACK_DPLL_CFG_LOCK_ON_CNT_WIDTH                 (0x1)
#define PTRACK_DPLL_CFG_LOCK_ON_CNT_MASK                  (0x400)
#define PTRACK_DPLL_CFG_LOCK_ON_CNT_SET(x)                (((x) & 0x1) << 0xA)
#define PTRACK_DPLL_CFG_LOCK_ON_CNT_CLR                   (~0x400)

/*
 * Write with 0 for future compatibility
 */

/*
 * Wait period for lock monitoring state machine.in fout tics till error observation
Actual wait value is (LOCK_WAIT*64+2)
 */
#define PTRACK_DPLL_CFG_LOCK_WAIT_BIT                     (0x10)
#define PTRACK_DPLL_CFG_LOCK_WAIT_WIDTH                   (0xC)
#define PTRACK_DPLL_CFG_LOCK_WAIT_MASK                    (0xFFF0000)
#define PTRACK_DPLL_CFG_LOCK_WAIT_SET(x)                  (((x) & 0xFFF) << 0x10)
#define PTRACK_DPLL_CFG_LOCK_WAIT_CLR                     (~0xFFF0000)

/*
 * Write with 0 for future compatibility
 */

/*
 * Low threshold  for DPLL1 error level measurement.
Actual threshold is (16 x LOW_ERR_TH)
 */
#define PTRACK_ERR_TRSH_LOW_ERR_TH_BIT                    (0x0)
#define PTRACK_ERR_TRSH_LOW_ERR_TH_WIDTH                  (0x10)
#define PTRACK_ERR_TRSH_LOW_ERR_TH_MASK                   (0xFFFF)
#define PTRACK_ERR_TRSH_LOW_ERR_TH_SET(x)                 (((x) & 0xFFFF) << 0x0)
#define PTRACK_ERR_TRSH_LOW_ERR_TH_CLR                    (~0xFFFF)

/*
 * High threshold  for DPLL1 error level measurement.
Actual threshold is (16 x HIGH_ERR_TH)
 */
#define PTRACK_ERR_TRSH_HIGH_ERR_TH_BIT                   (0x10)
#define PTRACK_ERR_TRSH_HIGH_ERR_TH_WIDTH                 (0x10)
#define PTRACK_ERR_TRSH_HIGH_ERR_TH_MASK                  (0xFFFF0000)
#define PTRACK_ERR_TRSH_HIGH_ERR_TH_SET(x)                (((x) & 0xFFFF) << 0x10)
#define PTRACK_ERR_TRSH_HIGH_ERR_TH_CLR                   (~0xFFFF0000)

/*
 *
 */
#define PTRACK_GEAR0_KP_SHIFT_BIT                         (0x0)
#define PTRACK_GEAR0_KP_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR0_KP_SHIFT_MASK                        (0x1F)
#define PTRACK_GEAR0_KP_SHIFT_SET(x)                      (((x) & 0x1F) << 0x0)
#define PTRACK_GEAR0_KP_SHIFT_CLR                         (~0x1F)

/*
 *
 */
#define PTRACK_GEAR0_KF_SHIFT_BIT                         (0x5)
#define PTRACK_GEAR0_KF_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR0_KF_SHIFT_MASK                        (0x3E0)
#define PTRACK_GEAR0_KF_SHIFT_SET(x)                      (((x) & 0x1F) << 0x5)
#define PTRACK_GEAR0_KF_SHIFT_CLR                         (~0x3E0)

/*
 * The DPLL Order
0 - 1st order DPLL
1  - 2nd order DPPL
This field may change dynamcially while gear 0 is not selected as the gear used by DPLL
 */
#define PTRACK_GEAR0_ORDER_BIT                            (0xA)
#define PTRACK_GEAR0_ORDER_WIDTH                          (0x1)
#define PTRACK_GEAR0_ORDER_MASK                           (0x400)
#define PTRACK_GEAR0_ORDER_SET(x)                         (((x) & 0x1) << 0xA)
#define PTRACK_GEAR0_ORDER_CLR                            (~0x400)

/*
 * Write with 0 for future compatibility
 */

/*
 * Wait period untill switching to the next gear
Actual wait value is (GEAR_LEN*64)+2 for gear 0 and (GEAR_LEN*64)+1 for other greas.
 */
#define PTRACK_GEAR0_GEAR_LEN_BIT                         (0x10)
#define PTRACK_GEAR0_GEAR_LEN_WIDTH                       (0xE)
#define PTRACK_GEAR0_GEAR_LEN_MASK                        (0x3FFF0000)
#define PTRACK_GEAR0_GEAR_LEN_SET(x)                      (((x) & 0x3FFF) << 0x10)
#define PTRACK_GEAR0_GEAR_LEN_CLR                         (~0x3FFF0000)

/*
 * Write with 0 for future compatibility
 */

/*
 *
 */
#define PTRACK_GEAR1_KP_SHIFT_BIT                         (0x0)
#define PTRACK_GEAR1_KP_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR1_KP_SHIFT_MASK                        (0x1F)
#define PTRACK_GEAR1_KP_SHIFT_SET(x)                      (((x) & 0x1F) << 0x0)
#define PTRACK_GEAR1_KP_SHIFT_CLR                         (~0x1F)

/*
 *
 */
#define PTRACK_GEAR1_KF_SHIFT_BIT                         (0x5)
#define PTRACK_GEAR1_KF_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR1_KF_SHIFT_MASK                        (0x3E0)
#define PTRACK_GEAR1_KF_SHIFT_SET(x)                      (((x) & 0x1F) << 0x5)
#define PTRACK_GEAR1_KF_SHIFT_CLR                         (~0x3E0)

/*
 * The DPLL Order
0 - 1st order DPLL
1  - 2nd order DPPL
This field may change dynamcially while gear 0 is not selected as the gear used by DPLL
 */
#define PTRACK_GEAR1_ORDER_BIT                            (0xA)
#define PTRACK_GEAR1_ORDER_WIDTH                          (0x1)
#define PTRACK_GEAR1_ORDER_MASK                           (0x400)
#define PTRACK_GEAR1_ORDER_SET(x)                         (((x) & 0x1) << 0xA)
#define PTRACK_GEAR1_ORDER_CLR                            (~0x400)

/*
 * Write with 0 for future compatibility
 */

/*
 * Wait period untill switching to the next gear
Actual wait value is (GEAR_LEN*64)+2 for gear 0 and (GEAR_LEN*64)+1 for other greas.
 */
#define PTRACK_GEAR1_GEAR_LEN_BIT                         (0x10)
#define PTRACK_GEAR1_GEAR_LEN_WIDTH                       (0xE)
#define PTRACK_GEAR1_GEAR_LEN_MASK                        (0x3FFF0000)
#define PTRACK_GEAR1_GEAR_LEN_SET(x)                      (((x) & 0x3FFF) << 0x10)
#define PTRACK_GEAR1_GEAR_LEN_CLR                         (~0x3FFF0000)

/*
 * Write with 0 for future compatibility
 */

/*
 *
 */
#define PTRACK_GEAR2_KP_SHIFT_BIT                         (0x0)
#define PTRACK_GEAR2_KP_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR2_KP_SHIFT_MASK                        (0x1F)
#define PTRACK_GEAR2_KP_SHIFT_SET(x)                      (((x) & 0x1F) << 0x0)
#define PTRACK_GEAR2_KP_SHIFT_CLR                         (~0x1F)

/*
 *
 */
#define PTRACK_GEAR2_KF_SHIFT_BIT                         (0x5)
#define PTRACK_GEAR2_KF_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR2_KF_SHIFT_MASK                        (0x3E0)
#define PTRACK_GEAR2_KF_SHIFT_SET(x)                      (((x) & 0x1F) << 0x5)
#define PTRACK_GEAR2_KF_SHIFT_CLR                         (~0x3E0)

/*
 * The DPLL Order
0 - 1st order DPLL
1  - 2nd order DPPL
This field may change dynamcially while gear 0 is not selected as the gear used by DPLL
 */
#define PTRACK_GEAR2_ORDER_BIT                            (0xA)
#define PTRACK_GEAR2_ORDER_WIDTH                          (0x1)
#define PTRACK_GEAR2_ORDER_MASK                           (0x400)
#define PTRACK_GEAR2_ORDER_SET(x)                         (((x) & 0x1) << 0xA)
#define PTRACK_GEAR2_ORDER_CLR                            (~0x400)

/*
 * Write with 0 for future compatibility
 */

/*
 * Wait period untill switching to the next gear
Actual wait value is (GEAR_LEN*64)+2 for gear 0 and (GEAR_LEN*64)+1 for other greas.
 */
#define PTRACK_GEAR2_GEAR_LEN_BIT                         (0x10)
#define PTRACK_GEAR2_GEAR_LEN_WIDTH                       (0xE)
#define PTRACK_GEAR2_GEAR_LEN_MASK                        (0x3FFF0000)
#define PTRACK_GEAR2_GEAR_LEN_SET(x)                      (((x) & 0x3FFF) << 0x10)
#define PTRACK_GEAR2_GEAR_LEN_CLR                         (~0x3FFF0000)

/*
 * Write with 0 for future compatibility
 */

/*
 *
 */
#define PTRACK_GEAR3_KP_SHIFT_BIT                         (0x0)
#define PTRACK_GEAR3_KP_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR3_KP_SHIFT_MASK                        (0x1F)
#define PTRACK_GEAR3_KP_SHIFT_SET(x)                      (((x) & 0x1F) << 0x0)
#define PTRACK_GEAR3_KP_SHIFT_CLR                         (~0x1F)

/*
 *
 */
#define PTRACK_GEAR3_KF_SHIFT_BIT                         (0x5)
#define PTRACK_GEAR3_KF_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR3_KF_SHIFT_MASK                        (0x3E0)
#define PTRACK_GEAR3_KF_SHIFT_SET(x)                      (((x) & 0x1F) << 0x5)
#define PTRACK_GEAR3_KF_SHIFT_CLR                         (~0x3E0)

/*
 * The DPLL Order
0 - 1st order DPLL
1  - 2nd order DPPL
This field may change dynamcially while gear 0 is not selected as the gear used by DPLL
 */
#define PTRACK_GEAR3_ORDER_BIT                            (0xA)
#define PTRACK_GEAR3_ORDER_WIDTH                          (0x1)
#define PTRACK_GEAR3_ORDER_MASK                           (0x400)
#define PTRACK_GEAR3_ORDER_SET(x)                         (((x) & 0x1) << 0xA)
#define PTRACK_GEAR3_ORDER_CLR                            (~0x400)

/*
 * Write with 0 for future compatibility
 */

/*
 * Wait period untill switching to the next gear
Actual wait value is (GEAR_LEN*64)+2 for gear 0 and (GEAR_LEN*64)+1 for other greas.
 */
#define PTRACK_GEAR3_GEAR_LEN_BIT                         (0x10)
#define PTRACK_GEAR3_GEAR_LEN_WIDTH                       (0xE)
#define PTRACK_GEAR3_GEAR_LEN_MASK                        (0x3FFF0000)
#define PTRACK_GEAR3_GEAR_LEN_SET(x)                      (((x) & 0x3FFF) << 0x10)
#define PTRACK_GEAR3_GEAR_LEN_CLR                         (~0x3FFF0000)

/*
 * Write with 0 for future compatibility
 */

/*
 *
 */
#define PTRACK_GEAR4_KP_SHIFT_BIT                         (0x0)
#define PTRACK_GEAR4_KP_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR4_KP_SHIFT_MASK                        (0x1F)
#define PTRACK_GEAR4_KP_SHIFT_SET(x)                      (((x) & 0x1F) << 0x0)
#define PTRACK_GEAR4_KP_SHIFT_CLR                         (~0x1F)

/*
 *
 */
#define PTRACK_GEAR4_KF_SHIFT_BIT                         (0x5)
#define PTRACK_GEAR4_KF_SHIFT_WIDTH                       (0x5)
#define PTRACK_GEAR4_KF_SHIFT_MASK                        (0x3E0)
#define PTRACK_GEAR4_KF_SHIFT_SET(x)                      (((x) & 0x1F) << 0x5)
#define PTRACK_GEAR4_KF_SHIFT_CLR                         (~0x3E0)

/*
 * The DPLL Order
0 - 1st order DPLL
1  - 2nd order DPPL
This field may change dynamcially while gear 0 is not selected as the gear used by DPLL
 */
#define PTRACK_GEAR4_ORDER_BIT                            (0xA)
#define PTRACK_GEAR4_ORDER_WIDTH                          (0x1)
#define PTRACK_GEAR4_ORDER_MASK                           (0x400)
#define PTRACK_GEAR4_ORDER_SET(x)                         (((x) & 0x1) << 0xA)
#define PTRACK_GEAR4_ORDER_CLR                            (~0x400)

/*
 * Write with 0 for future compatibility
 */

/*
 * Estimated value of Rate Ration U.5.26 format
 */
#define PTRACK_EST_R_VALUE_EST_R_VALUE_BIT                (0x0)
#define PTRACK_EST_R_VALUE_EST_R_VALUE_WIDTH              (0x1F)
#define PTRACK_EST_R_VALUE_EST_R_VALUE_MASK               (0x7FFFFFFF)

/*
 * Write with 0 for future compatibility
 */

/*
 * Value of Smoothed error
 */
#define PTRACK_S_ERROR_S_ERROR_BIT                        (0x0)
#define PTRACK_S_ERROR_S_ERROR_WIDTH                      (0x14)
#define PTRACK_S_ERROR_S_ERROR_MASK                       (0xFFFFF)

/*
 * Write with 0 for future compatibility
 */

/*
 * Initial value of T ratio
This field may be changed while module is enabled
 */
#define PTRACK_INIT_T_RATIO_INIT_T_RATIO_BIT              (0x0)
#define PTRACK_INIT_T_RATIO_INIT_T_RATIO_WIDTH            (0x20)
#define PTRACK_INIT_T_RATIO_INIT_T_RATIO_MASK             (0xFFFFFFFF)
#define PTRACK_INIT_T_RATIO_INIT_T_RATIO_SET(x)           (((x) & 0xFFFFFFFF) << 0x0)
#define PTRACK_INIT_T_RATIO_INIT_T_RATIO_CLR              (~0xFFFFFFFF)

/*
 * Value of calculated T_RATIO
 */
#define PTRACK_T_RATIO_T_RATIO_BIT                        (0x0)
#define PTRACK_T_RATIO_T_RATIO_WIDTH                      (0x20)
#define PTRACK_T_RATIO_T_RATIO_MASK                       (0xFFFFFFFF)

/*
 * Status: Indication of FIFO  empty
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_STATUS_AFIFO_EMPTY_STAT_BIT                (0x0)
#define PTRACK_STATUS_AFIFO_EMPTY_STAT_WIDTH              (0x1)
#define PTRACK_STATUS_AFIFO_EMPTY_STAT_MASK               (0x1)

/*
 * Status: Indication of FIFO  full
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_STATUS_AFIFO_FULL_STAT_BIT                 (0x1)
#define PTRACK_STATUS_AFIFO_FULL_STAT_WIDTH               (0x1)
#define PTRACK_STATUS_AFIFO_FULL_STAT_MASK                (0x2)

/*
 * Status: Indication of FIFO overrun
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_STATUS_AFIFO_OVERRUN_STAT_BIT              (0x2)
#define PTRACK_STATUS_AFIFO_OVERRUN_STAT_WIDTH            (0x1)
#define PTRACK_STATUS_AFIFO_OVERRUN_STAT_MASK             (0x4)
#define PTRACK_STATUS_AFIFO_OVERRUN_STAT_SET(x)           (((x) & 0x1) << 0x2)
#define PTRACK_STATUS_AFIFO_OVERRUN_STAT_CLR              (~0x4)

/*
 * Status: Indication of FIFO overrun
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_STATUS_AFIFO_UNDERRUN_STAT_BIT             (0x3)
#define PTRACK_STATUS_AFIFO_UNDERRUN_STAT_WIDTH           (0x1)
#define PTRACK_STATUS_AFIFO_UNDERRUN_STAT_MASK            (0x8)
#define PTRACK_STATUS_AFIFO_UNDERRUN_STAT_SET(x)          (((x) & 0x1) << 0x3)
#define PTRACK_STATUS_AFIFO_UNDERRUN_STAT_CLR             (~0x8)

/*
 * Status:  FIFO contains at least BUF_IF_WM_LEVEL data entries
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_STATUS_AFIFO_WM_STAT_BIT                   (0x4)
#define PTRACK_STATUS_AFIFO_WM_STAT_WIDTH                 (0x1)
#define PTRACK_STATUS_AFIFO_WM_STAT_MASK                  (0x10)
#define PTRACK_STATUS_AFIFO_WM_STAT_SET(x)                (((x) & 0x1) << 0x4)
#define PTRACK_STATUS_AFIFO_WM_STAT_CLR                   (~0x10)

/*
 * Status: DPLL lok indication changed its state from unlocked to locked
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_STATUS_DPLL_LOCK_STAT_BIT                  (0x5)
#define PTRACK_STATUS_DPLL_LOCK_STAT_WIDTH                (0x1)
#define PTRACK_STATUS_DPLL_LOCK_STAT_MASK                 (0x20)
#define PTRACK_STATUS_DPLL_LOCK_STAT_SET(x)               (((x) & 0x1) << 0x5)
#define PTRACK_STATUS_DPLL_LOCK_STAT_CLR                  (~0x20)

/*
 * Status: DPLL lok indication changed its state from locked to unlocked
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_STATUS_DPLL_LOCK_LOSS_STAT_BIT             (0x6)
#define PTRACK_STATUS_DPLL_LOCK_LOSS_STAT_WIDTH           (0x1)
#define PTRACK_STATUS_DPLL_LOCK_LOSS_STAT_MASK            (0x40)
#define PTRACK_STATUS_DPLL_LOCK_LOSS_STAT_SET(x)          (((x) & 0x1) << 0x6)
#define PTRACK_STATUS_DPLL_LOCK_LOSS_STAT_CLR             (~0x40)

/*
 * Write with 0 for future compatibility
 */

/*
 * Write with 0 for future compatibility
 */

/*
 * Interrupt Enable: Indication of FIFO overrun
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_IE_AFIFO_OVERRUN_IE_BIT                    (0x2)
#define PTRACK_IE_AFIFO_OVERRUN_IE_WIDTH                  (0x1)
#define PTRACK_IE_AFIFO_OVERRUN_IE_MASK                   (0x4)
#define PTRACK_IE_AFIFO_OVERRUN_IE_SET(x)                 (((x) & 0x1) << 0x2)
#define PTRACK_IE_AFIFO_OVERRUN_IE_CLR                    (~0x4)

/*
 * Interrupt Enable: Indication of FIFO underrun
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_IE_AFIFO_UNDERRUN_IE_BIT                   (0x3)
#define PTRACK_IE_AFIFO_UNDERRUN_IE_WIDTH                 (0x1)
#define PTRACK_IE_AFIFO_UNDERRUN_IE_MASK                  (0x8)
#define PTRACK_IE_AFIFO_UNDERRUN_IE_SET(x)                (((x) & 0x1) << 0x3)
#define PTRACK_IE_AFIFO_UNDERRUN_IE_CLR                   (~0x8)

/*
 * Interrupt Enable: Indication of FIFO watermark
0  Event didnt occur
1  Event occurred
This field may be cleared while module is enabled
 */
#define PTRACK_IE_AFIFO_WM__IE_BIT                        (0x4)
#define PTRACK_IE_AFIFO_WM__IE_WIDTH                      (0x1)
#define PTRACK_IE_AFIFO_WM__IE_MASK                       (0x10)
#define PTRACK_IE_AFIFO_WM__IE_SET(x)                     (((x) & 0x1) << 0x4)
#define PTRACK_IE_AFIFO_WM__IE_CLR                        (~0x10)

/*
 * Interrupt Enable: Indication of lcok
0  Interrupt Disabled
1  Interrupt Enabled
This field may be changed while module is enabled
 */
#define PTRACK_IE_DPLL_LOCK_IE_BIT                        (0x5)
#define PTRACK_IE_DPLL_LOCK_IE_WIDTH                      (0x1)
#define PTRACK_IE_DPLL_LOCK_IE_MASK                       (0x20)
#define PTRACK_IE_DPLL_LOCK_IE_SET(x)                     (((x) & 0x1) << 0x5)
#define PTRACK_IE_DPLL_LOCK_IE_CLR                        (~0x20)

/*
 * Interrupt Enable: Indication of lcok loss
0  Interrupt Disabled
1  Interrupt Enabled
This field may be changed while module is enabled
 */
#define PTRACK_IE_DPLL_LOCK_LOSS_IE_BIT                   (0x6)
#define PTRACK_IE_DPLL_LOCK_LOSS_IE_WIDTH                 (0x1)
#define PTRACK_IE_DPLL_LOCK_LOSS_IE_MASK                  (0x40)
#define PTRACK_IE_DPLL_LOCK_LOSS_IE_SET(x)                (((x) & 0x1) << 0x6)
#define PTRACK_IE_DPLL_LOCK_LOSS_IE_CLR                   (~0x40)

/*
 * Write with 0 for future compatibility
 */

/*
 * Write with 0 for future compatibility
 */

/*
 * Status: Indication of FIFO overrun
0  Event didnt occur
1  Event occurred
If related interrupt is disabled, status is always zero
 */
#define PTRACK_AI_STATUS_AFIFO_OVERRUN_AI_BIT             (0x2)
#define PTRACK_AI_STATUS_AFIFO_OVERRUN_AI_WIDTH           (0x1)
#define PTRACK_AI_STATUS_AFIFO_OVERRUN_AI_MASK            (0x4)

/*
 * Status: Indication of FIFO underrun
0  Event didnt occur
1  Event occurred
If related interrupt is disabled, status is always zero
 */
#define PTRACK_AI_STATUS_AFIFO_UNDERRUN_AI_BIT            (0x3)
#define PTRACK_AI_STATUS_AFIFO_UNDERRUN_AI_WIDTH          (0x1)
#define PTRACK_AI_STATUS_AFIFO_UNDERRUN_AI_MASK           (0x8)

/*
 * Status: Indication of  FIFO WM
0  Event didnt occur
1  Event occurred
If related interrupt is disabled, status is always zero
 */
#define PTRACK_AI_STATUS_AFIFO_WM__AI_BIT                 (0x4)
#define PTRACK_AI_STATUS_AFIFO_WM__AI_WIDTH               (0x1)
#define PTRACK_AI_STATUS_AFIFO_WM__AI_MASK                (0x10)

/*
 * Status: Indication of DPLL lock
0  Event didnt occur
1  Event occurred
If related interrupt is disabled, status is always zero
 */
#define PTRACK_AI_STATUS_DPLL_LOCK_AI_BIT                 (0x5)
#define PTRACK_AI_STATUS_DPLL_LOCK_AI_WIDTH               (0x1)
#define PTRACK_AI_STATUS_DPLL_LOCK_AI_MASK                (0x20)

/*
 * Status: Indication of DPLL lock loss
0  Event didnt occur
1  Event occurred
If related interrupt is disabled, status is always zero
 */
#define PTRACK_AI_STATUS_DPLL_LOCK_LOSS_AI_BIT            (0x6)
#define PTRACK_AI_STATUS_DPLL_LOCK_LOSS_AI_WIDTH          (0x1)
#define PTRACK_AI_STATUS_DPLL_LOCK_LOSS_AI_MASK           (0x40)

/*
 * Write with 0 for future compatibility
 */

/*Output interface Configuration*/
#define PTRACK_BUS_IF_CFG_OFFS              (0x0)

/*Tic generation configuration*/
#define PTRACK_TIC_GEN_CFG_OFFS             (0x4)

/*Phase measure configuration*/
#define PTRACK_PHASE_MEAS_CFG_OFFS          (0x8)

/*DPLL configuration*/
#define PTRACK_DPLL_CFG_OFFS                (0xC)

/*ERROR threshold configuration*/
#define PTRACK_ERR_TRSH_OFFS                (0x10)

/*GEAR 0 configuration*/
#define PTRACK_GEAR0_OFFS                   (0x14)

/*GEAR 1 configuration*/
#define PTRACK_GEAR1_OFFS                   (0x18)

/*GEAR 2 configuration*/
#define PTRACK_GEAR2_OFFS                   (0x1C)

/*GEAR 3 configuration*/
#define PTRACK_GEAR3_OFFS                   (0x20)

/*GEAR 4 configuration*/
#define PTRACK_GEAR4_OFFS                   (0x24)

/*Estimated Rate Ratio value*/
#define PTRACK_EST_R_VALUE_OFFS             (0x28)

/*Smoothed Error Value*/
#define PTRACK_S_ERROR_OFFS                 (0x2C)

/*Initail T ratio value*/
#define PTRACK_INIT_T_RATIO_OFFS            (0x30)

/*Value of calculated T_RATIO*/
#define PTRACK_T_RATIO_OFFS                 (0x34)

/*PTRACK Status*/
#define PTRACK_STATUS_OFFS                  (0x38)

/*PTRACK Interrupt Enable*/
#define PTRACK_IE_OFFS                      (0x3C)

/*PTRACK Active Interrupt Status*/
#define PTRACK_AI_STATUS_OFFS               (0x40)


/**********************************************************************
 *		 Block end PTRACK_BLK
 **********************************************************************/

// PTRACK_TOP register layout
#define PTRACK_TOP_MEMMAP_OFFS           0x00
#define PTRACK_TOP_MFIN_SRR_OFFS         0x80
#define PTRACK_TOP_NFOUT_SRR_OFFS        0x84
#define PTRACK_TOP_CTRL_OFFS             0x88
#define PTRACK_TOP_FDATA_OFFS            0x8C
