HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/iommu/
ifeq ($(ISP_AE_V3), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/ae/ae_v3/vvcam_isp_ae.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/ae/ae_v3/
endif

ifeq ($(ISP_AWB_V3), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/awb/awb_v3/vvcam_isp_awb.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/awb/awb_v3/
endif

ifeq ($(ISP_GC_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/gc/gc_v1/vvcam_isp_gc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/gc/gc_v1/
endif

ifeq ($(ISP_GC_V2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/gc/gc_v2/vvcam_isp_gc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/gc/gc_v2/
endif

ifeq ($(ISP_2DNR_V3), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/2dnr/2dnr_v3/vvcam_isp_2dnr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/2dnr/2dnr_v3/
endif

ifeq ($(ISP_2DNR_V5_2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/2dnr/2dnr_v5_2/vvcam_isp_2dnr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/2dnr/2dnr_v5_2/
endif

ifeq ($(ISP_EE_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/ee/ee_v1/vvcam_isp_ee.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/ee/ee_v1/
endif

ifeq ($(ISP_EE_V2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/ee/ee_v2/vvcam_isp_ee.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/ee/ee_v2/
endif

ifeq ($(ISP_CPROC), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/cproc/cproc/vvcam_isp_cproc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/cproc/cproc/
endif

ifeq ($(ISP_WDR_V3), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/wdr/wdr_v3/vvcam_isp_wdr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/wdr/wdr_v3/
endif

ifeq ($(ISP_WDR_V5_2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/wdr/wdr_v5_2/vvcam_isp_wdr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/wdr/wdr_v5_2/
endif

ifeq ($(ISP_BLS), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/bls/bls/vvcam_isp_bls.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/bls/bls/
endif

ifeq ($(ISP_CCM), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/ccm/ccm/vvcam_isp_ccm.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/ccm/ccm/
endif

ifeq ($(ISP_DPCC), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/dpcc/dpcc/vvcam_isp_dpcc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/dpcc/dpcc/
endif

ifeq ($(ISP_GE), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/ge/ge/vvcam_isp_ge.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/ge/ge/
endif

ifeq ($(ISP_LSC_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/lsc/lsc_v1/vvcam_isp_lsc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/lsc/lsc_v1/
endif

ifeq ($(ISP_LSC_V3), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/lsc/lsc_v3/vvcam_isp_lsc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/lsc/lsc_v3/
endif

ifeq ($(ISP_WB), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/wb/wb/vvcam_isp_wb.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/wb/wb/
endif

ifeq ($(ISP_3DNR_V1_2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/3dnr/3dnr_v1_2/vvcam_isp_3dnr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/3dnr/3dnr_v1_2
endif

ifeq ($(ISP_3DNR_V2_1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/3dnr/3dnr_v2_1/vvcam_isp_3dnr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/3dnr/3dnr_v2_1
endif

ifeq ($(ISP_AF_V3), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/af/af_v3/vvcam_isp_af.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/af/af_v3
endif

ifeq ($(ISP_AFM_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/afm/afm_v1/vvcam_isp_afm.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/afm/afm_v1/
endif

ifeq ($(ISP_DMSC_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/dmsc/dmsc_v1/vvcam_isp_dmsc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/dmsc/dmsc_v1/
endif

ifeq ($(ISP_DMSC_V2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/dmsc/dmsc_v2/vvcam_isp_dmsc.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/dmsc/dmsc_v2/
endif

ifeq ($(ISP_EXP_V2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/exp/exp_v2/vvcam_isp_exp.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/exp/exp_v2/
endif

ifeq ($(ISP_GTM_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/gtm/gtm_v1/vvcam_isp_gtm.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/gtm/gtm_v1
endif

ifeq ($(ISP_HDR_V1_3), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/hdr/hdr_v1_3/vvcam_isp_hdr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/hdr/hdr_v1_3
endif

ifeq ($(ISP_HDR_V3_1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/hdr/hdr_v3_1/vvcam_isp_hdr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/hdr/hdr_v3_1
endif

ifeq ($(ISP_RGBIR_V2), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/rgbir/rgbir_v2/vvcam_isp_rgbir.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/rgbir/rgbir_v2
endif

ifeq ($(ISP_DG), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/dg/dg/vvcam_isp_dg.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/dg/dg
endif

ifeq ($(ISP_CPD), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/cpd/cpd/vvcam_isp_cpd.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/cpd/cpd
endif

ifeq ($(ISP_DPF_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/dpf/dpf_v1/vvcam_isp_dpf.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/dpf/dpf_v1
endif

ifeq ($(ISP_CNR_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/cnr/cnr_v1/vvcam_isp_cnr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/cnr/cnr_v1
endif

ifeq ($(ISP_GWDR_V1), 1)
vvcam_isp_subdev-objs += isp/vvcam/v4l2/isp/isp_ctrl/gwdr/gwdr_v1/vvcam_isp_gwdr.o
HEADER_PATH += -I$(src)/isp/vvcam/v4l2/isp/isp_ctrl/gwdr/gwdr_v1
endif

