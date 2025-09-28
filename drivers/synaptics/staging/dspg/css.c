/**
 * linux/drivers/staging/dspg/css.c
 *
 *  Copyright (C) 2012, 2013, 2016 DSP Group
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
#include <linux/clk.h>
#include <linux/coma/cfifo.h>
#include <linux/coma/coma.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sec.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/reset.h>

#include <mach/platform.h>

#define CSS_MAGIC			0xDADADADA
#define CSS_IMG_VERSION			0x0102
#define CSS_ABI_VERSION			0x0209

static int ignore_abi_mismatch = 0;
module_param(ignore_abi_mismatch, int, 0644);
MODULE_PARM_DESC(ignore_abi_mismatch, "Ignore ABI mismatch");

enum firmware_state {
	CSS_FIRMWARE_NA,
	CSS_FIRMWARE_PENDING,
	CSS_FIRMWARE_LOADED,
	CSS_FIRMWARE_PANIC,
	CSS_FIRMWARE_SUSPENDED,
};

static const char * const firmware_state_names[] = {
	[CSS_FIRMWARE_NA]	= "none",
	[CSS_FIRMWARE_PENDING]	= "pending",
	[CSS_FIRMWARE_LOADED]	= "loaded",
	[CSS_FIRMWARE_PANIC]	= "panic",
	[CSS_FIRMWARE_SUSPENDED]= "suspended",
};

struct boot_config {
	unsigned long dram_addr;
	unsigned long dram_size;
	int uart;
	unsigned long cfifo_l2c;
	unsigned long cfifo_c2l;
};

struct css_img_hdr {
	unsigned long magic;
	unsigned long parts;
	unsigned long dc_addr; /* unused */
	unsigned long boot_config;
	unsigned long elf_md5_addr;
	u16 abi_version;
	u16 img_version;
	char info[52];
} __attribute__((__packed__));

struct css_part_hdr {
	unsigned long size;
	unsigned long rsize;
	unsigned long dest_addr;
	unsigned char md5[16];
} __attribute__((__packed__));

#define CSS_CLK_REQ_ENABLE		0
#define CSS_CLK_REQ_DISABLE		1
#define CSS_CLK_REQ_SET_RATE		2
#define CSS_CLK_REQ_GET_RATE		3
#define CSS_CLK_REQ_ROUND_RATE		4

enum css_dev_clocks {
	CSS_CLK_ADPCM,
	CSS_CLK_BMP,
	CSS_CLK_DRT,
	CSS_CLK_RFAPU,
	CSS_CLK_GDMAC,
	CSS_CLK_TIMER1,
	CSS_CLK_TIMER2,
	CSS_CLK_UART,
	CSS_CLK_RTC,
	CSS_CLK_TUNE,
	CSS_CLK_DCLASS,
	CSS_CLK_TDM1,
	CSS_CLK_TDM2,
	CSS_CLK_APU,
	CSS_CLK_PMU,
	CSS_CLK_TDM3,
	CSS_CLK_APP_CPU,
	_CSS_CLK_LAST,
};

static char *dev_clock_names[_CSS_CLK_LAST] = {
	"adpcm_clk",
	"bmp_clk",
	"drt_clk",
	"rfapu_clk",
	"gdmac_clk",
	"timer1_clk",
	"timer2_clk",
	"uart?_clk",
	"rtc_clk",
	"tune_clk",
	"dclass_clk",
	"tdm1_clk",
	"tdm2_clk",
	"apu_clk",
	"pmu_clk",
	"tdm3_clk",
	"app_cpu_clk",
};

#define CSS_RST_REQ_ENABLE		0
#define CSS_RST_REQ_DISABLE		1

enum css_dev_resets {
	CSS_RST_ADPCM,
	CSS_RST_BMP,
	CSS_RST_DRT,
	CSS_RST_RFAPU,
	CSS_RST_GDMAC,
	CSS_RST_TIMERS,
	CSS_RST_UART,
	CSS_RST_DCLASS,
	CSS_RST_TDM1,
	CSS_RST_TDM2,
	CSS_RST_TDM3,
	_CSS_RST_LAST,
};

static char *dev_reset_names[_CSS_RST_LAST] = {
	"adpcm",
	"bmp",
	"drt",
	"rfapu",
	"gdmac",
	"timers",
	"uart?",
	"dclass",
	"tdm1",
	"tdm2",
	"tdm3",
};

struct memory {
	void __iomem *data;
	struct resource *res;
};

struct phys_memory {
	void *data;
	unsigned int size;
	unsigned int dt_size;
	dma_addr_t phys;
};

struct loader_private {
	struct memory dtcm;
	struct memory itcm;
	struct memory ahbram;
	struct phys_memory dram;
#ifdef CONFIG_DEBUG_FS
	struct debugfs_blob_wrapper dram_blob;
#endif
	struct clk *clk_css;
	struct clk *clk_css_parent;
	struct clk *clk_css_arm;
	struct clk *clk_css_etm;
	struct device *dev;
	struct dentry *pdentry;
	struct sec_msg *panic;
	struct sec_msg *reload;
	struct sec_msg *counter;
	struct sec_msg *pm_clk_cmd;
	struct sec_msg *pm_clk_arg;
	struct sec_msg *pm_rst_cmd;
	enum firmware_state state;
	struct clk *clk_css_devices[_CSS_CLK_LAST];
	int clk_css_dev_refcnt[_CSS_CLK_LAST];
	struct reset_control *rst_css_devices[_CSS_RST_LAST];
	struct work_struct panic_work;
	struct work_struct pm_clk_work;
	unsigned long pm_clk_data;
	struct mutex lock;
	struct reset_control *rst_css_arm;
	struct reset_control *rst_css;
	struct reset_control *rst_css_etm;
	int debug_disabled;
	unsigned rf_reset;
	int uart;
	unsigned int tdm_ownership_mask;
	int is_dvf97;
};

static inline void css_rf_reset_release(struct loader_private *p)
{
	/* release RF from reset */
	if (gpio_is_valid(p->rf_reset))
		gpio_set_value(p->rf_reset, 1);
}

static inline void css_rf_reset_set(struct loader_private *p)
{
	/* put RF into reset */
	if (gpio_is_valid(p->rf_reset))
		gpio_set_value(p->rf_reset, 0);
}

#ifdef CONFIG_DEBUG_FS
struct css_mem_file {
	void *buf;
	size_t size;
};

static int css_debug_common_open(struct loader_private *p, struct file *f,
				 struct memory *mem)
{
	struct css_mem_file *mf;
	size_t size = resource_size(mem->res);

	mf = kmalloc(sizeof(struct css_mem_file), GFP_KERNEL);
	if (!mf)
		return -ENOMEM;

	mf->size = size;
	mf->buf = vmalloc(size);
	if (!mf->buf) {
		kfree(mf);
		return -ENOMEM;
	}

	memcpy_fromio(mf->buf, mem->data, size);
	f->private_data = mf;

	return 0;
}

static int css_debug_ahb_open(struct inode *inode, struct file *f)
{
	struct loader_private *p = inode->i_private;
	int ret = -EACCES;

	mutex_lock(&p->lock);

	if (p->state != CSS_FIRMWARE_LOADED && p->state != CSS_FIRMWARE_PANIC)
		goto out;

	ret = css_debug_common_open(p, f, &p->ahbram);

out:
	mutex_unlock(&p->lock);
	return ret;
}

static int css_debug_dtcm_open(struct inode *inode, struct file *f)
{
	struct loader_private *p = inode->i_private;
	int ret = -EACCES;

	mutex_lock(&p->lock);

	if (p->state != CSS_FIRMWARE_LOADED && p->state != CSS_FIRMWARE_PANIC)
		goto out;

	ret = css_debug_common_open(p, f, &p->dtcm);

out:
	mutex_unlock(&p->lock);
	return ret;
}

static int css_debug_itcm_open(struct inode *inode, struct file *f)
{
	struct loader_private *p = inode->i_private;
	int ret = -EACCES;

	mutex_lock(&p->lock);

	if (p->state != CSS_FIRMWARE_PANIC)
		goto out;

	/* Put ARM into reset to gain access to ITCM */
	clk_disable(p->clk_css_arm);
	ret = reset_control_assert(p->rst_css_arm);
	if (ret)
		panic("unable to assert reset");
	clk_enable(p->clk_css_arm);
	mdelay(2);

	ret = css_debug_common_open(p, f, &p->itcm);

out:
	mutex_unlock(&p->lock);
	return ret;
}

static ssize_t css_debug_read(struct file *f, char __user *u, size_t size,
			      loff_t *off)
{
	struct css_mem_file *mf = f->private_data;

	if (*off >= mf->size)
		return 0;
	if (*off + size > mf->size)
		size = mf->size - *off;

	if (copy_to_user(u, mf->buf + *off, size))
		return -EFAULT;

	*off += size;

	return size;
}

static int css_debug_common_release(struct inode *inode, struct file *f)
{
	struct css_mem_file *mf = f->private_data;

	vfree(mf->buf);
	kfree(mf);

	return 0;
}

static struct file_operations debug_ahb_fops = {
	.open		= css_debug_ahb_open,
	.read		= css_debug_read,
	.release	= css_debug_common_release,
};

static struct file_operations debug_dtcm_fops = {
	.open		= css_debug_dtcm_open,
	.read		= css_debug_read,
	.release	= css_debug_common_release,
};

static struct file_operations debug_itcm_fops = {
	.open		= css_debug_itcm_open,
	.read		= css_debug_read,
	.release	= css_debug_common_release,
};

static void css_create_debugfs_files(struct loader_private *p)
{
	struct dentry *pdentry;

	pdentry = debugfs_create_dir("css", NULL);
	p->pdentry = pdentry;

	p->dram_blob.data = p->dram.data;
	p->dram_blob.size = p->dram.dt_size;

	if (p->ahbram.data)
		debugfs_create_file("ahb-ram", S_IRUSR, pdentry, p, &debug_ahb_fops);
	debugfs_create_file("itcm", S_IRUSR, pdentry, p, &debug_itcm_fops);
	debugfs_create_file("dtcm", S_IRUSR, pdentry, p, &debug_dtcm_fops);
	debugfs_create_blob("dram", S_IRUSR, pdentry, &p->dram_blob);
}

static void css_remove_debugfs_files(struct loader_private *p)
{
	if (p->pdentry)
		debugfs_remove_recursive(p->pdentry);
}
#else
#define css_create_debugfs_files(p)
#define css_remove_debugfs_files(p)
#endif

static void css_panic_handler(int id, unsigned long data, void *context)
{
	struct loader_private *p = context;
	char event[11];
	char *envp[] = { event, NULL };

	sprintf(event, "SEC=PANIC");

	kobject_uevent_env(&p->dev->kobj, KOBJ_CHANGE, envp);

	schedule_work(&p->panic_work);
}

static void css_reload_handler(int id, unsigned long data, void *context)
{
	struct loader_private *p = context;
	char event[11];
	char *envp[] = { event, NULL };

	sprintf(event, "SEC=RELOAD");

	kobject_uevent_env(&p->dev->kobj, KOBJ_CHANGE, envp);
}

static void css_pm_clk_work(struct work_struct *work)
{
	struct loader_private *p = container_of(work, struct loader_private,
		pm_clk_work);
	unsigned long data = p->pm_clk_data;
	unsigned int clk_num = (data >> 8) & 0xff;
	int ret = -EINVAL;

	switch (data & 0xff) {
	case CSS_CLK_REQ_SET_RATE:
		ret = clk_set_rate(p->clk_css_devices[clk_num],
				   sec_msg_read(p->pm_clk_arg));
		dev_dbg(p->dev, "css_clk_set_rate(%s) %lu returns %d\n",
			dev_clock_names[clk_num], sec_msg_read(p->pm_clk_arg),
			ret);
		break;
	case CSS_CLK_REQ_GET_RATE:
		ret = clk_get_rate(p->clk_css_devices[clk_num]);
		break;
	case CSS_CLK_REQ_ROUND_RATE:
		ret = clk_round_rate(p->clk_css_devices[clk_num],
				     sec_msg_read(p->pm_clk_arg));
		dev_dbg(p->dev, "css_clk_round_rate(%s) %lu returns %d\n",
			dev_clock_names[clk_num], sec_msg_read(p->pm_clk_arg),
			ret);
		break;
	}

	sec_msg_trigger(p->pm_clk_cmd, ret);
}

static void css_pm_clk_handler(int id, unsigned long data, void *context)
{
	struct loader_private *p = context;
	unsigned int clk_num = (data >> 8) & 0xff;
	int ret;

	if ((clk_num >= _CSS_CLK_LAST) ||
	    !p->clk_css_devices[clk_num]) {
		sec_msg_trigger(p->pm_clk_cmd, -EINVAL);
		return;
	}

	switch (data & 0xff) {
	case CSS_CLK_REQ_ENABLE:
		dev_dbg(p->dev, "css_clk_req_enable(%s)\n",
			dev_clock_names[clk_num]);
		ret = clk_enable(p->clk_css_devices[clk_num]);
		p->clk_css_dev_refcnt[clk_num]++;
		sec_msg_trigger(p->pm_clk_cmd, ret);
		break;
	case CSS_CLK_REQ_DISABLE:
		dev_dbg(p->dev, "css_clk_req_disable(%s)\n",
			dev_clock_names[clk_num]);
		clk_disable(p->clk_css_devices[clk_num]);
		p->clk_css_dev_refcnt[clk_num]--;
		ret = 0;
		sec_msg_trigger(p->pm_clk_cmd, ret);
		break;
	default:
		p->pm_clk_data = data;
		ret = !schedule_work(&p->pm_clk_work);
		break;
	}

	WARN_ON(ret); /* complain if multiple calls are started in parallel */
}

static void css_pm_rst_handler(int id, unsigned long data, void *context)
{
	struct loader_private *p = (struct loader_private *)context;
	int ret;
	unsigned rst_num = (data >> 8) & 0xff;

	if (rst_num >= _CSS_RST_LAST)
		return;
	if (p->rst_css_devices[rst_num] == NULL)
		return;

	switch (data & 0xff) {
	case CSS_RST_REQ_ENABLE:
		ret = reset_control_assert(p->rst_css_devices[rst_num]);
		dev_dbg(p->dev, "reset_control_assert(%s)\n",
			dev_reset_names[rst_num]);
		if (ret)
			dev_err(p->dev, "reset failed\n");
		break;
	case CSS_RST_REQ_DISABLE:
		ret = reset_control_deassert(p->rst_css_devices[rst_num]);
		dev_dbg(p->dev, "reset_control_deassert(%s)\n",
			dev_reset_names[rst_num]);
		if (ret)
			dev_err(p->dev, "reset failed\n");
		break;
	}
}

static int css_power_up(struct loader_private *p)
{
	int ret;

	p->panic = sec_msg_register(css_panic_handler, SEC_MSG_CSS_PANIC, 0, p);
	if (IS_ERR(p->panic)) {
		dev_err(p->dev, "failed to register SEC handler\n");
		return PTR_ERR(p->panic);
	}

	p->reload = sec_msg_register(css_reload_handler, SEC_MSG_CSS_RELOAD, 0,
				     p);
	if (IS_ERR(p->reload)) {
		dev_err(p->dev, "failed to register SEC handler\n");
		return PTR_ERR(p->reload);
	}

	p->counter = sec_msg_register(NULL, SEC_MSG_CSS_COUNTER, SEC_OVERWRITE,
				      p);
	if (IS_ERR(p->counter)) {
		dev_err(p->dev, "failed to register SEC handler\n");
		return PTR_ERR(p->counter);
	}

	p->pm_clk_cmd = sec_msg_register(css_pm_clk_handler, SEC_MSG_CLK_CMD,
					 SEC_FIQ, p);
	if (IS_ERR(p->pm_clk_cmd)) {
		dev_err(p->dev, "failed to register clock handler\n");
		return PTR_ERR(p->pm_clk_cmd);
	}

	p->pm_clk_arg = sec_msg_register(NULL, SEC_MSG_CLK_ARG, SEC_OVERWRITE,
					 p);
	if (IS_ERR(p->pm_clk_arg)) {
		dev_err(p->dev, "failed to register clock arg handler\n");
		return PTR_ERR(p->pm_clk_arg);
	}

	p->pm_rst_cmd = sec_msg_register(css_pm_rst_handler, SEC_MSG_RST_CMD,
					 SEC_FIQ, p);
	if (IS_ERR(p->pm_rst_cmd)) {
		dev_err(p->dev, "failed to register reset handler\n");
		return PTR_ERR(p->pm_rst_cmd);
	}

	ret = reset_control_deassert(p->rst_css);
	if (ret)
		goto err_power_down;

	ret = clk_enable(p->clk_css);
	if (ret)
		goto err_power_down;

	ret = reset_control_deassert(p->rst_css_etm);
	if (ret)
		goto err_disable_css;

	if (!p->debug_disabled) {
		ret = clk_enable(p->clk_css_etm);
		if (ret)
			goto err_disable_css;
	}

	ret = clk_enable(p->clk_css_arm);
	if (ret)
		goto err_disable_etm;

	css_rf_reset_release(p);

	return 0;

err_disable_etm:
	if (!p->debug_disabled)
		clk_disable(p->clk_css_etm);
err_disable_css:
	ret = reset_control_assert(p->rst_css_etm);
	if (ret)
		panic("unable to assert reset");
	clk_disable(p->clk_css);
err_power_down:
	ret = reset_control_assert(p->rst_css_etm);
	if (ret)
		panic("unable to assert reset");
	return ret;
}

static void css_power_down(struct loader_private *p)
{
	int ret, i;

	/* Shut down execution on the CSS */
	clk_disable(p->clk_css_arm);
	if (!p->debug_disabled)
		clk_disable(p->clk_css_etm);
	ret = reset_control_assert(p->rst_css_arm);
	if (ret)
		panic("unable to assert reset");
	ret = reset_control_assert(p->rst_css_etm);
	if (ret)
		panic("unable to assert reset");
	clk_enable(p->clk_css_etm);
	clk_enable(p->clk_css_arm);
	mdelay(1);
	clk_disable(p->clk_css_arm);
	clk_disable(p->clk_css_etm);

	/* Disable all device clocks which were left enabled */
	for (i = 0; i < _CSS_CLK_LAST; i++) {
		while (p->clk_css_dev_refcnt[i] > 0) {
			clk_disable(p->clk_css_devices[i]);
			p->clk_css_dev_refcnt[i]--;
		}
	}

	/* Put all devices back into reset */
	if (p->rst_css_devices[CSS_RST_ADPCM])
		reset_control_assert(p->rst_css_devices[CSS_RST_ADPCM]);
	reset_control_assert(p->rst_css_devices[CSS_RST_BMP]);
	if (p->rst_css_devices[CSS_RST_GDMAC])
		reset_control_assert(p->rst_css_devices[CSS_RST_GDMAC]);
	reset_control_assert(p->rst_css_devices[CSS_RST_TIMERS]);
	if (p->rst_css_devices[CSS_RST_UART])
		reset_control_assert(p->rst_css_devices[CSS_RST_UART]);
	reset_control_assert(p->rst_css_devices[CSS_RST_DCLASS]);
	if (p->rst_css_devices[CSS_RST_TDM1])
		reset_control_assert(p->rst_css_devices[CSS_RST_TDM1]);
	if (p->rst_css_devices[CSS_RST_TDM2])
		reset_control_assert(p->rst_css_devices[CSS_RST_TDM2]);
	if (p->rst_css_devices[CSS_RST_TDM3])
		reset_control_assert(p->rst_css_devices[CSS_RST_TDM3]);

	/* correctly reset BMP, ADPCMs, DRT (need clock during reset) */
	if (dev_clock_names[CSS_CLK_ADPCM])
		clk_enable(p->clk_css_devices[CSS_CLK_ADPCM]);
	clk_enable(p->clk_css_devices[CSS_CLK_BMP]);
	clk_enable(p->clk_css_devices[CSS_CLK_DRT]);
	mdelay(1);
	if (dev_clock_names[CSS_CLK_ADPCM])
		clk_disable(p->clk_css_devices[CSS_CLK_ADPCM]);
	clk_disable(p->clk_css_devices[CSS_CLK_BMP]);
	clk_disable(p->clk_css_devices[CSS_CLK_DRT]);

	css_rf_reset_set(p);

	/* Shutdown rest of the CSS */
	clk_disable(p->clk_css);
	ret = reset_control_assert(p->rst_css);
	if (ret)
		panic("unable to assert reset");
	clk_enable(p->clk_css);
	mdelay(2);
	clk_disable(p->clk_css);

	sec_msg_deregister(p->pm_rst_cmd);
	sec_msg_deregister(p->pm_clk_arg);
	sec_msg_deregister(p->pm_clk_cmd);
	sec_msg_deregister(p->panic);
	sec_msg_deregister(p->reload);
	sec_msg_deregister(p->counter);
}

static int css_realloc_dma_mem(struct loader_private *p,
			       struct css_part_hdr *dram_hdr)
{
	/* Reallocate DRAM section only if it has grown */
	if (p->dram.data && dram_hdr->rsize > p->dram.size) {
		dma_free_coherent(p->dev, p->dram.size, p->dram.data, p->dram.phys);
		memset(&p->dram, 0, sizeof(p->dram));
	}

	if (!p->dram.data) {
		p->dram.data = dma_alloc_coherent(p->dev,
						  dram_hdr->rsize,
						  &p->dram.phys,
						  GFP_KERNEL);
		if (!p->dram.data)
			return -ENOMEM;

		p->dram.size = dram_hdr->rsize;
		/* wipe whole reserved dram area */
		memset_io(p->dram.data, 0, p->dram.dt_size);
	}

	return 0;
}

static void css_free_dma_mem(struct loader_private *p)
{
	if (p->dram.data) {
		dma_free_coherent(p->dev, p->dram.size, p->dram.data,
				  p->dram.phys);
		memset(&p->dram, 0, sizeof(p->dram));
	}
}

static void got_firmware(const struct firmware *fw, void *context)
{
	struct loader_private *p = (struct loader_private *)context;
	struct css_img_hdr *img_hdr;
	struct css_part_hdr *itcm_hdr;
	struct css_part_hdr *ahbram_hdr = NULL;
	struct css_part_hdr *dram_hdr;
	struct css_part_hdr *info_hdr;
	struct css_part_hdr *elf_hdr = NULL;
	void __iomem *virt_elf_md5;
	void __iomem *virt_boot_config;
	unsigned long csize;
	unsigned int res_end;
	struct boot_config bootcfg;
	dma_addr_t cfifo_l2c, cfifo_c2l;
	int ret;
	unsigned long clk_rate, half_rate;

	if (!fw) {
		p->state = CSS_FIRMWARE_NA;
		dev_err(p->dev, "Loading CSS firmware failed\n");
		return;
	}

	mutex_lock(&p->lock);
	if (css_power_up(p)) {
		dev_err(p->dev, "cannot power up CSS\n");
		mutex_unlock(&p->lock);
		return;
	}

	/* check image header */
	img_hdr = (struct css_img_hdr *)fw->data;

	if (img_hdr->parts < 5) {
		dev_err(p->dev, "invalid number of parts: %lu\n",
		       img_hdr->parts);
		goto out_err;
	}

	if (img_hdr->magic != CSS_MAGIC) {
		dev_err(p->dev, "invalid image magic\n");
		goto out_err;
	}

	if (img_hdr->img_version != CSS_IMG_VERSION) {
		dev_err(p->dev, "image version mismatch (got: v%x, "
		                "kernel expected: v%x)\n",
		                img_hdr->img_version, CSS_IMG_VERSION);
		goto out_err;
	}
	if (img_hdr->abi_version != CSS_ABI_VERSION) {
		dev_err(p->dev, "ABI mismatch (got: v%x, expected: v%x)\n",
		                img_hdr->abi_version, CSS_ABI_VERSION);
		if (!ignore_abi_mismatch)
			goto out_err;
	}
	dev_info(p->dev, "loading %s\n", img_hdr->info);

	csize = sizeof(*img_hdr);
	itcm_hdr   = (struct css_part_hdr *)(fw->data + csize);
	csize += sizeof(*itcm_hdr) + itcm_hdr->size;
	if ((csize > fw->size) ||
	    (itcm_hdr->size > resource_size(p->itcm.res))) {
		dev_err(p->dev, "ITCM size too big! sz=0x%08lX, max=0x%08X\n",
		        itcm_hdr->size, resource_size(p->itcm.res));
		goto out_err;
	}

	ahbram_hdr = (struct css_part_hdr *)(fw->data + csize);
	csize += sizeof(*ahbram_hdr) + ahbram_hdr->size;
	if ((csize > fw->size) ||
	    (p->ahbram.data && (ahbram_hdr->size > resource_size(p->ahbram.res)))) {
		dev_err(p->dev, "AHBRAM size too big! sz=0x%08lX, max=0x%08X\n",
		        ahbram_hdr->size, resource_size(p->ahbram.res));
		goto out_err;
	}

	dram_hdr   = (struct css_part_hdr *)(fw->data + csize);
	csize += sizeof(*dram_hdr) + dram_hdr->size;
	if ((csize > fw->size) || (dram_hdr->size > dram_hdr->rsize)) {
		dev_err(p->dev, "DRAM size too big! sz=0x%08lX, max=0x%08X\n",
		        dram_hdr->size, p->dram.size);
		goto out_err;
	}

	/* dfl_fla_css.info */
	info_hdr = (struct css_part_hdr *)(fw->data + csize);
	csize += sizeof(*info_hdr) + info_hdr->size;
	if (csize > fw->size) {
		dev_err(p->dev, "Info size too big! sz=0x%08lX\n",
			info_hdr->size);
		goto out_err;
	}

	/* ugt_output.h */
	info_hdr = (struct css_part_hdr *)(fw->data + csize);
	csize += sizeof(*info_hdr) + info_hdr->size;
	if (csize > fw->size) {
		dev_err(p->dev, "UGT size too big! sz=0x%08lX\n",
			info_hdr->size);
		goto out_err;
	}

	if (img_hdr->parts == 6) {
		elf_hdr = (struct css_part_hdr *)(fw->data + csize);
		csize += sizeof(*dram_hdr) + elf_hdr->size;
		if (csize > fw->size) {
			dev_err(p->dev, "ELF size too big! sz=0x%08lX\n",
			       elf_hdr->size);
			goto out_err;
		}
		dev_info(p->dev,
		         "MD5 sum of ELF file: 0x%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X"
			 "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X\n",
		         elf_hdr->md5[0],
		         elf_hdr->md5[1],
		         elf_hdr->md5[2],
		         elf_hdr->md5[3],
		         elf_hdr->md5[4],
		         elf_hdr->md5[5],
		         elf_hdr->md5[6],
		         elf_hdr->md5[7],
		         elf_hdr->md5[8],
		         elf_hdr->md5[9],
		         elf_hdr->md5[10],
		         elf_hdr->md5[11],
		         elf_hdr->md5[12],
		         elf_hdr->md5[13],
		         elf_hdr->md5[14],
		         elf_hdr->md5[15]);
	}

	res_end = p->itcm.res->start + resource_size(p->itcm.res);
	if ((itcm_hdr->dest_addr < p->itcm.res->start) ||
	    (itcm_hdr->dest_addr > res_end) ||
	    ((itcm_hdr->dest_addr + itcm_hdr->size) > res_end)) {
		dev_err(p->dev,
			"ITCM destination pointer out of range! ptr=0x%08lX\n",
			itcm_hdr->dest_addr);
		goto out_err;
	}

	if (!p->is_dvf97 && p->ahbram.data) {
		res_end = p->ahbram.res->start + resource_size(p->ahbram.res);
		if (((ahbram_hdr->dest_addr < p->ahbram.res->start) ||
		     (ahbram_hdr->dest_addr > res_end) ||
		     ((ahbram_hdr->dest_addr + ahbram_hdr->size) > res_end)) &&
		    ahbram_hdr->size != 0) {
			dev_err(p->dev,
				"AHBRAM destination pointer out of range! ptr=0x%08lX\n",
				ahbram_hdr->dest_addr);
			goto out_err;
		}
	}

	if ((img_hdr->dc_addr > dram_hdr->size) ||
	    ((img_hdr->dc_addr + 4) > dram_hdr->size)) {
		dev_err(p->dev, "DC pointer out of range! ptr=0x%08lX\n",
		       img_hdr->dc_addr);
		goto out_err;
	}

	if ((img_hdr->elf_md5_addr > dram_hdr->size) ||
	    ((img_hdr->elf_md5_addr + 4) > dram_hdr->size)) {
		dev_err(p->dev, "Elf MD5 pointer out of range! ptr=0x%08lX\n",
		       img_hdr->elf_md5_addr);
		goto out_err;
	}

	if (p->clk_css_parent) {
		clk_rate = clk_get_rate(p->clk_css_parent);
		if (clk_get_rate(p->clk_css) < clk_rate)
			clk_set_rate(p->clk_css, clk_rate);
	}

	/* set the CSS clock to half rate for writing TCMs */
	clk_rate = clk_get_rate(p->clk_css);
	half_rate = clk_round_rate(p->clk_css, clk_rate / 2);
	clk_set_rate(p->clk_css, half_rate);

	if (css_realloc_dma_mem(p, dram_hdr) != 0) {
		dev_err(p->dev, "not enough memory to load CSS firmware\n");
		goto out_err;
	}

	/* clear memory */
	memset_io(p->itcm.data, 0, resource_size(p->itcm.res));
	memset(p->dram.data, 0, p->dram.size);

	/* copy CSS code and data */
	memcpy_toio(p->itcm.data + (itcm_hdr->dest_addr - p->itcm.res->start),
	            (void *)itcm_hdr + sizeof(*itcm_hdr),
	            itcm_hdr->size);

	if (!p->is_dvf97 && (ahbram_hdr->size && p->ahbram.data))
		memcpy_toio(p->ahbram.data + (ahbram_hdr->dest_addr - p->ahbram.res->start),
		            (void *)ahbram_hdr + sizeof(*ahbram_hdr),
		            ahbram_hdr->size);

	memcpy(p->dram.data + dram_hdr->dest_addr,
	       (void *)dram_hdr + sizeof(*dram_hdr),
	       dram_hdr->size);

	if (img_hdr->parts == 6) {
		/* fill elf md5 sum of CSS */
		virt_elf_md5 = p->dram.data + img_hdr->elf_md5_addr;
		memcpy(virt_elf_md5, elf_hdr->md5, 16);
	}

	/* bring up COMA subsystem */
	ret = coma_setup(p->dev, &cfifo_l2c, &cfifo_c2l);
	if (ret) {
		dev_err(p->dev, "coma_setup failed: %d\n", ret);
		goto out_err;
	}

	/* fill the boot config structure for the CSS */
	bootcfg.dram_addr = (uint32_t)p->dram.phys;
	bootcfg.dram_size = p->dram.size;
	bootcfg.uart = p->uart;
	bootcfg.cfifo_l2c = cfifo_l2c;
	bootcfg.cfifo_c2l = cfifo_c2l;
	virt_boot_config = p->itcm.data + img_hdr->boot_config;
	memcpy_toio(virt_boot_config, (void *)&bootcfg, sizeof(bootcfg));

	/* restore the original CSS clock rate */
	clk_set_rate(p->clk_css, clk_rate);

	/* release ARM from reset */
	clk_disable(p->clk_css_arm);
	ret = reset_control_deassert(p->rst_css_arm);
	if (ret)
		goto out_err;

	clk_enable(p->clk_css_arm);
	mdelay(2);

	css_create_debugfs_files(p);

	/*
	 * Defer the initialization of in-kernel services a bit. Initializing
	 * them here wouldn't work because the cordless domain is not running
	 * yet.
	 */
	msleep(20);
	coma_init_services();

	p->state = CSS_FIRMWARE_LOADED;
	dev_info(p->dev, "successfully loaded CSS firmware\n");

	goto out;

out_err:
	dev_err(p->dev, "failed to load CSS firmware\n");
	css_power_down(p);
	p->state = CSS_FIRMWARE_NA;
	css_free_dma_mem(p);
out:
	release_firmware(fw);
	mutex_unlock(&p->lock);
}

static ssize_t load_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct loader_private *p = dev_get_drvdata(dev);
	int ret = 0;
	unsigned int value = simple_strtoul(buf, NULL, 0);

	mutex_lock(&p->lock);
	if ((value >= 1) && (p->state != CSS_FIRMWARE_PENDING)) {
		if (p->state == CSS_FIRMWARE_LOADED ||
		    p->state == CSS_FIRMWARE_PANIC) {

			css_remove_debugfs_files(p);
			css_power_down(p);
			coma_release(p->dev);
			p->state = CSS_FIRMWARE_NA;
		}

		ret = request_firmware_nowait(THIS_MODULE,
		                              1,
		                              "css-loader",
		                              dev,
		                              GFP_KERNEL,
		                              (void *)p,
		                              got_firmware);

		if (ret == 0)
			p->state = CSS_FIRMWARE_PENDING;
	} else if (value == 0 && (p->state != CSS_FIRMWARE_PENDING)) {
		if (p->state == CSS_FIRMWARE_LOADED ||
		    p->state == CSS_FIRMWARE_PANIC) {
			css_remove_debugfs_files(p);
			css_power_down(p);
			coma_release(p->dev);
			p->state = CSS_FIRMWARE_NA;
		}
	}
	mutex_unlock(&p->lock);

	if (ret != 0)
		return 0;

	return count;
}

static ssize_t state_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct loader_private *p = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", firmware_state_names[p->state]);
}

static ssize_t panic_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct loader_private *p = dev_get_drvdata(dev);

	sec_msg_trigger(p->panic, count);
	return count;
}

static ssize_t heartbeat_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct loader_private *p = dev_get_drvdata(dev);

	if (p->state != CSS_FIRMWARE_LOADED)
		return -ENODEV;

	return sprintf(buf, "%lu\n", sec_msg_read(p->counter));
}

static void css_panic_work(struct work_struct *work)
{
	struct loader_private *p = container_of(work, struct loader_private,
		panic_work);

	mutex_lock(&p->lock);
	if (p->state == CSS_FIRMWARE_LOADED) {
		dev_crit(p->dev, "CSS entered panic mode\n");
		p->state = CSS_FIRMWARE_PANIC;

		css_rf_reset_set(p);

		kobject_uevent(&p->dev->kobj, KOBJ_CHANGE);
	}
	mutex_unlock(&p->lock);
}

static struct device_attribute css_attrs[] = {
	__ATTR_WO(load),
	__ATTR_RO(state),
	__ATTR_WO(panic),
	__ATTR_RO(heartbeat),
};

#ifdef CONFIG_PM
static int css_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	struct loader_private *p = dev_get_drvdata(&pdev->dev);

	mutex_lock(&p->lock);
	if (p->state == CSS_FIRMWARE_PANIC) {
		ret = -EINVAL;
		goto out;
	}
	if (p->state == CSS_FIRMWARE_LOADED) {
		ret = coma_suspend();
		if (ret)
			goto out;
		css_remove_debugfs_files(p);
		css_power_down(p);
		coma_release(p->dev);
		p->state = CSS_FIRMWARE_SUSPENDED;
	}

out:
	mutex_unlock(&p->lock);

	return ret;
}

static int css_resume(struct platform_device *pdev)
{
	struct loader_private *p = dev_get_drvdata(&pdev->dev);
	int ret;

	mutex_lock(&p->lock);
	if (p->state == CSS_FIRMWARE_SUSPENDED) {
		ret = request_firmware_nowait(THIS_MODULE,
		                              1,
		                              "css-loader",
		                              &pdev->dev,
		                              GFP_KERNEL,
		                              (void *)p,
		                              got_firmware);

		if (ret == 0)
			p->state = CSS_FIRMWARE_PENDING;
	}
	mutex_unlock(&p->lock);

	return 0;
}
#else
#define css_suspend NULL
#define css_resume NULL
#endif

static int __init css_probe(struct platform_device *pdev)
{
	int ret, i;
	u32 value;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *mnp;
	struct resource *itcm_res;
	struct resource *dtcm_res;
	struct resource *ahbram_res;
	struct loader_private *p;
	struct reset_control *rst_rtc;

	if (!np) {
		dev_err(&pdev->dev, "must use device tree!\n");
		return -EINVAL;
	}

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret)
		return -ENOMEM;

	p = devm_kzalloc(&pdev->dev, sizeof(*p), GFP_KERNEL);
	if (!p) {
		ret = -ENOMEM;
		goto out_err;
	}

	/* get devicenode of reserved memory area */
	mnp = of_parse_phandle(np, "memory-region", 0);
	if (!mnp) {
		dev_err(&pdev->dev, "cannot read phandle of reserved memory\n");
		goto out_err;
	}

	/* read size property */
	if (of_property_read_u32(mnp, "size", &p->dram.dt_size)) {
		dev_err(&pdev->dev, "cannot read size of reserved memory\n");
		goto out_err;
	}

	p->state = CSS_FIRMWARE_NA;
	p->dev = &pdev->dev;
	INIT_WORK(&p->panic_work, css_panic_work);
	INIT_WORK(&p->pm_clk_work, css_pm_clk_work);
	platform_set_drvdata(pdev, p);
	mutex_init(&p->lock);

	ret = -EINVAL;
	itcm_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "itcm");
	if (!itcm_res)
		goto out_err;
	p->itcm.data = devm_ioremap_resource(&pdev->dev, itcm_res);
	if (!p->itcm.data)
		goto out_err;
	ahbram_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ahbram");
	if (ahbram_res) {
		p->ahbram.data = devm_ioremap_resource(&pdev->dev, ahbram_res);
		if (!p->ahbram.data)
			goto out_err;
	}
	dtcm_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dtcm");
	if (!dtcm_res)
		goto out_err;
	p->dtcm.data = devm_ioremap_resource(&pdev->dev, dtcm_res);
	if (!p->dtcm.data)
		goto out_err;

	p->itcm.res = itcm_res;
	p->dtcm.res = dtcm_res;
	p->ahbram.res = ahbram_res;

	p->rf_reset = of_get_named_gpio(np, "rf-reset-gpio", 0);
	p->debug_disabled = of_property_read_bool(np, "debug-disabled");
	ret = of_property_read_u32(np, "uart", &value);
	p->uart = ret ? -1 : value;
	ret = of_property_read_u32(np, "tdm-ownership", &value);
	p->tdm_ownership_mask = ret ? 0 : value;
	p->is_dvf97 = of_machine_is_compatible("dspg,dvf97");

	ret = -EINVAL;
	p->rst_css_arm = devm_reset_control_get(&pdev->dev, "cpu_reset");
	if (IS_ERR(p->rst_css_arm)) {
		dev_err(&pdev->dev, "no reset control for CSS-ARM\n");
		goto out_err;
	}

	p->rst_css = devm_reset_control_get(&pdev->dev, "sys_reset");
	if (IS_ERR(p->rst_css)) {
		dev_err(&pdev->dev, "no reset control for CSS\n");
		goto out_err;
	}

	p->rst_css_etm = devm_reset_control_get(&pdev->dev, "etm_reset");
	if (IS_ERR(p->rst_css_etm)) {
		dev_err(&pdev->dev, "no reset control for CSS-ETM\n");
		goto out_err;
	}

	if (p->is_dvf97) {
		dev_reset_names[CSS_RST_ADPCM] = NULL;
		dev_clock_names[CSS_CLK_ADPCM] = NULL;
		dev_reset_names[CSS_RST_GDMAC] = NULL;
		dev_clock_names[CSS_CLK_GDMAC] = NULL;
		dev_reset_names[CSS_RST_RFAPU] = NULL;
		dev_clock_names[CSS_CLK_RFAPU] = NULL;
		dev_reset_names[CSS_RST_TDM2] = NULL;
		dev_clock_names[CSS_CLK_TDM2] = NULL;
	}

	/* adapt UART clock and reset */
	if (p->uart >= 0 && p->uart <= 3) {
		dev_reset_names[CSS_RST_UART][4] = '0' + p->uart;
		dev_clock_names[CSS_CLK_UART][4] = '0' + p->uart;
	} else {
		dev_reset_names[CSS_RST_UART] = NULL;
		dev_clock_names[CSS_CLK_UART] = NULL;
	}

	/* adapt TDM resets/clocks depending on ownership */
	if ((p->tdm_ownership_mask & 0x1) == 0) {
		dev_reset_names[CSS_RST_TDM1] = NULL;
		dev_clock_names[CSS_CLK_TDM1] = NULL;
	}
	if ((p->tdm_ownership_mask & 0x2) == 0) {
		dev_reset_names[CSS_RST_TDM2] = NULL;
		dev_clock_names[CSS_CLK_TDM2] = NULL;
	}
	if ((p->tdm_ownership_mask & 0x4) == 0) {
		dev_reset_names[CSS_RST_TDM3] = NULL;
		dev_clock_names[CSS_CLK_TDM3] = NULL;
	}

	for (i = 0; i < _CSS_RST_LAST; i++) {
		if (dev_reset_names[i] == NULL) {
			p->rst_css_devices[i] = NULL;
			continue;
		}
		p->rst_css_devices[i] =
			devm_reset_control_get(&pdev->dev, dev_reset_names[i]);
		if (IS_ERR(p->rst_css_devices[i])) {
			dev_warn(&pdev->dev, "no reset control for %s\n",
				 dev_reset_names[i]);
			p->rst_css_devices[i] = NULL;
		}
	}

	rst_rtc = devm_reset_control_get(&pdev->dev, "rtc");
	if (!IS_ERR(rst_rtc)) {
		ret = reset_control_deassert(rst_rtc);
		if (ret) {
			dev_err(&pdev->dev, "failed to deassert RTC reset\n");
			goto out_err;
		}
	}

	if (gpio_is_valid(p->rf_reset)) {
		ret = devm_gpio_request(&pdev->dev, p->rf_reset,
					"dect-rf-reset");
		if (ret) {
			dev_err(&pdev->dev,
				"failed to request RF reset GPIO\n");
			goto out_err;
		}

		/* keep in reset */
		gpio_direction_output(p->rf_reset, 0);
	}

	ret = -ENOENT;
	p->clk_css = devm_clk_get(&pdev->dev, "sys_clk");
	if (IS_ERR(p->clk_css))
		goto out_err;
	p->clk_css_parent = devm_clk_get(&pdev->dev, "css_parent");
	if (IS_ERR(p->clk_css_parent))
		p->clk_css_parent = NULL;
	p->clk_css_arm = devm_clk_get(&pdev->dev, "cpu_clk");
	if (IS_ERR(p->clk_css_arm))
		goto out_err;
	p->clk_css_etm = devm_clk_get(&pdev->dev, "etm_clk");
	if (IS_ERR(p->clk_css_etm))
		goto out_err;
	if (p->clk_css_parent)
		clk_prepare(p->clk_css_parent);
	clk_prepare(p->clk_css);
	clk_prepare(p->clk_css_arm);
	clk_prepare(p->clk_css_etm);

	for (i = 0; i < _CSS_CLK_LAST; i++) {
		if (!dev_clock_names[i])
			continue;

		p->clk_css_devices[i] = devm_clk_get(&pdev->dev,
						     dev_clock_names[i]);
		if (IS_ERR(p->clk_css_devices[i])) {
			dev_warn(&pdev->dev, "could not get clock '%s'\n",
				 dev_clock_names[i]);
			p->clk_css_devices[i] = NULL;
		}
		clk_prepare(p->clk_css_devices[i]);
	}

	/* do this as the very last step */
	for (i = 0; i < ARRAY_SIZE(css_attrs); i++) {
		ret = device_create_file(&pdev->dev, &css_attrs[i]);
		if (ret) {
			while (--i >= 0)
				device_remove_file(&pdev->dev, &css_attrs[i]);
			dev_err(&pdev->dev,
				"failed to create css sysfs file\n");
			goto out_err;
		}
	}

	dev_info(&pdev->dev, "successfully probed");

	return 0;

out_err:
	of_reserved_mem_device_release(&pdev->dev);
	return ret;
}

static int __exit css_remove(struct platform_device *pdev)
{
	struct loader_private *p = platform_get_drvdata(pdev);
	int i;

	if (p->state == CSS_FIRMWARE_LOADED || p->state == CSS_FIRMWARE_PANIC) {
		css_remove_debugfs_files(p);
		css_power_down(p);
		coma_release(p->dev);
	}
	css_free_dma_mem(p);
	flush_work(&p->panic_work);
	flush_work(&p->pm_clk_work);

	for (i = 0; i < ARRAY_SIZE(css_attrs); i++)
		device_remove_file(&pdev->dev, &css_attrs[i]);

	of_reserved_mem_device_release(&pdev->dev);

	return 0;
}

static struct of_device_id css_of_ids[] = {
	{ .compatible = "dspg,css" },
	{ },
};

static struct platform_driver css_driver = {
	.driver = {
		.name = "css",
		.owner = THIS_MODULE,
		.of_match_table = css_of_ids,
	},
	.remove = __exit_p(css_remove),
	.suspend = css_suspend,
	.resume = css_resume,
};

static int __init css_init(void)
{
	return platform_driver_probe(&css_driver, css_probe);
}
module_init(css_init);

static void __exit css_exit(void)
{
	platform_driver_unregister(&css_driver);
}
module_exit(css_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CSS Loader");
MODULE_AUTHOR("DSP Group, Inc.");
