// SPDX-License-Identifier: GPL-2.0
// Copyright 2021 Synaptics Incorporated

#include "hrx-drv.h"
#include "hrx-reg.h"

static const char* VIP_IMODE_NAME[] =
{
	"YCBCR8",
	"YCBCR10",
	"YCBCR12",
	"YC8",
	"YC10",
	"YC12",
	"RGB8",
	"RGB10",
	"RGB12",
	"YUV420_8",
	"YUV420_10",
	"YUV420_12"
};

static const char* VIP_OMODE_NAME[] =
{
	"YC8",
	"YC10",
	"YUV420_8",
	"YUV420_10",
	"YUV420_12",
	"YCBCR8",
	"YCBCR10",
	"YCBCR12",
	"8BIT_RGB8",
	"RGB10",
	"RGB12",
	"YC12"
};

extern struct platform_device *pdev_glbl;
static struct dentry *hrx_v4l2_dir = NULL;
static struct dentry *hrx_status_fs = NULL;
static struct dentry *hrx_register_dump_fs = NULL;
int debug;

static int status_show(struct seq_file *s, void *data);
static int register_dump_show(struct seq_file *s, void *data);

DEFINE_SHOW_ATTRIBUTE(status);
DEFINE_SHOW_ATTRIBUTE(register_dump);

static int status_show(struct seq_file *s, void *data)
{
	struct syna_hrx_v4l2_dev *hrx_dev;

	hrx_dev = (struct syna_hrx_v4l2_dev *)dev_get_drvdata(&pdev_glbl->dev);
	seq_printf(
			s,
			"---------------------------------------\n"
	);
	seq_printf(
			s,
			"\t\t HRX STATUS \t\t\n"
	);
	seq_printf(
			s,
			"---------------------------------------\n"
	);

	seq_printf(
			s,
			"Connected \t\t= %s \nScrambled \t\t= %s\nHDMI2 \t\t\t= %s\n",
			hrx_is_5v_connected(hrx_dev) ? "Yes" : "No",
			hrx_dev->is_scrambled ? "Yes" : "No",
			hrx_is_hdmi2(hrx_dev) ? "Yes" : "No"
	);

	seq_printf(
			s,
			"---------------------------------------\n"
	);
	seq_printf(
			s,
			"\t\t Video Status \t\t\n"
	);
	seq_printf(
			s,
			"---------------------------------------\n"
	);

	seq_printf(
			s,
			"VIC \t\t\t= 0x%x\nResolution(Src Total) \t= %dx%d@%d %s\nResolution(Src Active) \t= %dx%d@%d %s\nResolution(VIP Output) \t= %dx%d@%d %s\nResolution(V4L2 Output) = %dx%d@%d %s\n",
			hrx_dev->current_vic,
			hrx_dev->video_params.HrxIpTimingParam.HTotal,
			hrx_dev->video_params.HrxIpTimingParam.VTotal,
			hrx_dev->video_params.FITotal.denominator,
			((hrx_dev->vip_imode < 0) || (hrx_dev->vip_imode >= MAX_NUM_VIP_IMODE)) ? "NULL" : VIP_IMODE_NAME[hrx_dev->vip_imode],
			hrx_dev->video_params.HrxIpTimingParam.HActive,
			hrx_dev->video_params.HrxIpTimingParam.VActive,
			hrx_dev->video_params.FITotal.denominator,
			((hrx_dev->vip_imode < 0) || (hrx_dev->vip_imode >= MAX_NUM_VIP_IMODE)) ? "NULL" : VIP_IMODE_NAME[hrx_dev->vip_imode],
			hrx_dev->scl_res[VIP_FRC_SCL_Y].OHRes,
			hrx_dev->scl_res[VIP_FRC_SCL_Y].OVRes,
			hrx_dev->video_params.FITotal.denominator,
			((hrx_dev->vip_omode < 0) || (hrx_dev->vip_omode >= MAX_NUM_VIP_OMODE)) ? "NULL" : VIP_OMODE_NAME[hrx_dev->vip_omode],
			hrx_dev->format.width,
			hrx_dev->format.height,
			hrx_dev->video_params.FIActive.denominator,
			((hrx_dev->vip_omode < 0) || (hrx_dev->vip_omode >= MAX_NUM_VIP_OMODE)) ? "NULL" : VIP_OMODE_NAME[hrx_dev->vip_omode]
	);

	seq_printf(
			s,
			"HrxState \t\t= %d\n",
			hrx_dev->HrxState
	);

	seq_printf(
			s,
			"VIP Status \t\t= %d\n",
			hrx_dev->vip_status
	);

	seq_printf(
			s,
			"VIP Stable \t\t= %d\n",
			syna_hrx_is_vip_stable(hrx_dev)
	);
	seq_printf(
			s,
			"Frame drop \t\t= %d\n",
			hrx_dev->frame_drop_count
	);

	seq_printf(
			s,
			"---------------------------------------\n"
	);
	seq_printf(
			s,
			"\t\t Audio Status \t\t\n"
	);
	seq_printf(
			s,
			"---------------------------------------\n"
	);

	seq_printf(
			s,
			"N \t\t\t= %d\nCTS \t\t\t= %d\nSampling Frequency \t= %d\nNumber of Channels \t= %d\nSample Size \t\t= %d\nLinear PCM \t\t= %s\nHBR \t\t\t= %s\n",
			hrx_dev->audio_params.N,
			hrx_dev->audio_params.CTS,
			hrx_dev->audio_params.SampFreq,
			hrx_dev->audio_params.NumOfChannels,
			hrx_dev->audio_params.SampleSize,
			hrx_dev->audio_params.IsLinearPCM ? "True" : "False",
			hrx_dev->audio_params.IsHBR ? "True" : "False"
	);

	seq_printf(
			s,
			"---------------------------------------\n"
	);

	return 0;
}

static int register_dump_show(struct seq_file *s, void *data)
{
	hrx_dump_reg(s);
	return 0;
}

int hrx_debug_create(void)
{
	hrx_v4l2_dir = debugfs_create_dir("hrx_v4l2", NULL);
	hrx_status_fs = debugfs_create_file("status", 0644, hrx_v4l2_dir, NULL, &status_fops);
	hrx_register_dump_fs = debugfs_create_file("register_dump", 0644, hrx_v4l2_dir, NULL, &register_dump_fops);

	return 0;
}

void hrx_debug_remove(void)
{
	debugfs_remove_recursive(hrx_v4l2_dir);
}

module_param(debug, int, 0660);
MODULE_PARM_DESC(debug, "Enable logs for HRX");
