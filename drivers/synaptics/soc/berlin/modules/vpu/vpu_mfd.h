/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Synaptics VPU multiple functions selector
 *
 * Copyright (C) 2023, Synaptics Incorporated
 */

#ifndef __SYNA_VPU_MFD_H__
#define __SYNA_VPU_MFD_H__

#include <linux/kernel.h>
#include <linux/device.h>

#define HW_SUPPORT_DEC_MPEG2	BIT(0)
#define HW_SUPPORT_DEC_H263		BIT(1)
#define HW_SUPPORT_DEC_H264		BIT(2)
#define HW_SUPPORT_DEC_H265		BIT(3)
#define HW_SUPPORT_DEC_VP8		BIT(4)
#define HW_SUPPORT_DEC_VP9		BIT(5)
#define HW_SUPPORT_DEC_AV1		BIT(6)

#define HW_SUPPORT_ENC_JPEG		BIT(16)
#define HW_SUPPORT_ENC_H264		BIT(17)
#define HW_SUPPORT_ENC_H265		BIT(18)
#define HW_SUPPORT_ENC_VP8		BIT(19)
#define HW_SUPPORT_ENC_VP9		BIT(20)
#define HW_SUPPORT_ENC_AV1		BIT(21)

#define SYNA_VPU_SUPPORT_DEC_MASK	GENMASK(15, 0)
#define SYNA_VPU_SUPPORT_ENC_MASK	GENMASK(31, 16)

/* From platypus vpu_api.h */

enum VPU_HW_ID {
	VPU_VMETA = 0,
	VPU_V2G = 1,
	VPU_G2 = 2,
	VPU_G1 = 3,
	VPU_H1_0 = 4,
	VPU_H1_1 = 5,
	VPU_VC8000E = 6,
	VPU_HW_IP_NUM,
};

/**
 * @hw_type: Hardware type in the firmware
 * @hw_ver: Generation of the hardware
 * @hw_variant: capabilities of the hardware
 */
struct syna_vpu_variant {
	const u32 hw_type;
	const u32 hw_ver;
	const u32 hw_variant;
};

struct syna_vpu_auxiliary_device {
	struct device dev;
	unsigned int id;
	unsigned int irq;
	const struct syna_vpu_auxiliary_driver *drv;
};

struct syna_vpu_auxiliary_driver {
	int (*probe)(struct syna_vpu_auxiliary_device *auxdev,
		     const struct syna_vpu_variant *variant);
	void (*remove)(struct syna_vpu_auxiliary_device *auxdev);
	int (*connect)(struct syna_vpu_auxiliary_device *auxdev);
	int (*disconnect)(struct syna_vpu_auxiliary_device *auxdev);
};

int syna_mfd_request_connect(struct syna_vpu_auxiliary_device *auxdev);
int syna_mfd_disconnect(struct syna_vpu_auxiliary_device *auxdev);

#if defined(CONFIG_BERLIN_VPU_AMP)
extern const struct syna_vpu_auxiliary_driver syna_vpu_amp_drv;
#endif

#if defined(CONFIG_BERLIN_VPU_V4L2)
extern const struct syna_vpu_auxiliary_driver syna_vpu_v4l2_drv;
int syna_vpu_dh_memdev_alloc(void);
void syna_vpu_dh_memdev_release(void);
#else /* CONFIG_BERLIN_VPU_V4L2 */
static inline int syna_vpu_dh_memdev_alloc(void)
{
	return 0;
}
static inline void syna_vpu_dh_memdev_release(void)
{
}
#endif /* CONFIG_BERLIN_VPU_V4L2 */

#endif
