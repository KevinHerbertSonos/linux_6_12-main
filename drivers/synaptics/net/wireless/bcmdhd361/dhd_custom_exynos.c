/*
 * Platform Dependent file for Samsung Exynos
 *
 * Copyright (C) 2025 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2025, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id$
 *
 */

#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include <linux/unistd.h>
#include <linux/bug.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#if defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810) || \
	defined(CONFIG_SOC_EXYNOS9820) || defined(CONFIG_SOC_EXYNOS9830) || \
	defined(CONFIG_SOC_EXYNOS2100) || defined(CONFIG_SOC_EXYNOS1000)
#include <linux/exynos-pci-ctrl.h>
#endif /* CONFIG_SOC_EXYNOS8895 || CONFIG_SOC_EXYNOS9810 ||
	* CONFIG_SOC_EXYNOS9820 || CONFIG_SOC_EXYNOS9830 ||
	* CONFIG_SOC_EXYNOS2100 || CONFIG_SOC_EXYNOS1000
	*/

#if defined(CONFIG_64BIT)
#include <asm-generic/gpio.h>
#endif /* CONFIG_64BIT */

#ifdef BCMDHD_MODULAR
#if IS_ENABLED(CONFIG_SEC_SYSFS)
#include <linux/sec_sysfs.h>
#endif /* CONFIG_SEC_SYSFS */
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif /* CONFIG_SEC_SYSFS */
#else
#if defined(CONFIG_SEC_SYSFS)
#include <linux/sec_sysfs.h>
#elif defined(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif /* CONFIG_SEC_SYSFS */
#endif /* BCMDHD_MODULAR */
#include <linux/wlan_plat.h>

#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_NOBLESSE)
#define PINCTL_DELAY 150
#elif defined(CONFIG_WLAN_HOMECONTROL) || defined(CONFIG_SOC_S5E5515)
#define PINCTL_DELAY 200
#endif /* CONFIG_MACH_A7LTE || CONFIG_NOBLESSE || CONFIG_SOC_S5E5515 */

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
extern void dhd_exit_wlan_mem(void);
extern int dhd_init_wlan_mem(void);
extern void *dhd_wlan_mem_prealloc(int section, unsigned long size);
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

#define WIFI_TURNON_DELAY	200
static int wlan_pwr_on = -1;

#ifdef DHD_USE_HOST_WAKE
static int wlan_host_wake_irq = 0;
static unsigned int wlan_host_wake_up = -1;
#endif /* DHD_USE_HOST_WAKE */

#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_NOBLESSE) || \
	defined(CONFIG_WLAN_HOMECONTROL) || defined(CONFIG_SOC_S5E5515)
extern struct device *mmc_dev_for_wlan;
#endif /* CONFIG_MACH_A7LTE || CONFIG_NOBLESSE || CONFIG_SOC_S5E5515 */

#ifdef BCMPCIE
extern int pcie_ch_num;
extern void exynos_pcie_pm_resume(int);
extern void exynos_pcie_pm_suspend(int);
#endif /* BCMPCIE */

#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS9110) || \
	defined(CONFIG_SOC_EXYNOS3830) || defined(CONFIG_SOC_S5E5515)
extern struct mmc_host *wlan_mmc;
extern void mmc_ctrl_power(struct mmc_host *host, bool onoff);
#endif /* SOC_EXYNOS7870 || CONFIG_SOC_EXYNOS9110 */

#ifndef CUSTOMER_HW4
static
#endif /* CUSTOMER_HW4 */
int
dhd_wlan_power(int onoff)
{
#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_NOBLESSE) || \
	defined(CONFIG_WLAN_HOMECONTROL) || defined(CONFIG_SOC_S5E5515)
	struct pinctrl *pinctrl = NULL;
#endif /* CONFIG_MACH_A7LTE || ONFIG_NOBLESSE || CONFIG_SOC_S5E5515 */

	printk(KERN_INFO"%s Enter: power %s\n", __FUNCTION__, onoff ? "on" : "off");

	if (gpio_direction_output(wlan_pwr_on, onoff)) {
		printk(KERN_ERR "%s failed to control WLAN_REG_ON to %s\n",
			__FUNCTION__, onoff ? "HIGH" : "LOW");
		return -EIO;
	}

#if defined(CONFIG_MACH_A7LTE) || defined(CONFIG_NOBLESSE) || \
	defined(CONFIG_WLAN_HOMECONTROL) || defined(CONFIG_SOC_S5E5515)
	if (onoff) {
		msleep(PINCTL_DELAY);
		pinctrl = devm_pinctrl_get_select(mmc_dev_for_wlan, "sdio_wifi_on");
		if (IS_ERR(pinctrl))
			printk(KERN_INFO "%s WLAN SDIO GPIO control error\n", __FUNCTION__);
	} else {
		pinctrl = devm_pinctrl_get_select(mmc_dev_for_wlan, "sdio_wifi_off");
		if (IS_ERR(pinctrl))
			printk(KERN_INFO "%s WLAN SDIO GPIO control error\n", __FUNCTION__);
	}
#endif /* CONFIG_MACH_A7LTE || CONFIG_NOBLESSE */

#if defined(CONFIG_SOC_EXYNOS7870) || defined(CONFIG_SOC_EXYNOS9110) || \
	defined(CONFIG_SOC_EXYNOS3830) || defined(CONFIG_SOC_S5E5515)
	if (wlan_mmc)
		mmc_ctrl_power(wlan_mmc, onoff);
#endif /* SOC_EXYNOS7870 || CONFIG_SOC_EXYNOS9110 || CONFIG_SOC_S5E5515 */
	return 0;
}

static int
dhd_wlan_reset(int onoff)
{
	return 0;
}

#ifndef BCMPCIE
extern void (*notify_func_callback)(void *dev_id, int state);
extern void *mmc_host_dev;
#endif /* !BCMPCIE */

static int
dhd_wlan_set_carddetect(int val)
{
#ifndef BCMPCIE
	pr_err("%s: notify_func=%p, mmc_host_dev=%p, val=%d\n",
		__FUNCTION__, notify_func_callback, mmc_host_dev, val);

	if (notify_func_callback) {
		notify_func_callback(mmc_host_dev, val);
	} else {
		pr_warning("%s: Nobody to notify\n", __FUNCTION__);
	}
#else
	if (val) {
		exynos_pcie_pm_resume(pcie_ch_num);
	} else {
		exynos_pcie_pm_suspend(pcie_ch_num);
	}
#endif /* BCMPCIE */

	return 0;
}

int __init
dhd_wlan_init_gpio(void)
{
	const char *wlan_node = "samsung,brcm-wlan";
	struct device_node *root_node = NULL;
	struct device *wlan_dev;

	/* BE Careful when merge, This is modification for dualization */
	wlan_dev = sec_device_find("wlan");
	if (wlan_dev == NULL) {
		wlan_dev = sec_device_create(NULL, "wlan");
	}

	root_node = of_find_compatible_node(NULL, NULL, wlan_node);
	if (!root_node) {
		WARN(1, "failed to get device node of bcm4354\n");
		return -ENODEV;
	}

	/* ========== WLAN_PWR_EN ============ */
	wlan_pwr_on = of_get_gpio(root_node, 0);
	if (!gpio_is_valid(wlan_pwr_on)) {
		WARN(1, "Invalied gpio pin : %d\n", wlan_pwr_on);
		return -ENODEV;
	}

	if (gpio_request(wlan_pwr_on, "WLAN_REG_ON")) {
		WARN(1, "fail to request gpio(WLAN_REG_ON)\n");
		return -ENODEV;
	}
#ifdef BCMPCIE
	gpio_direction_output(wlan_pwr_on, 1);
	msleep(WIFI_TURNON_DELAY);
#else
	gpio_direction_output(wlan_pwr_on, 0);
#endif /* BCMPCIE */
	gpio_export(wlan_pwr_on, 1);
	if (wlan_dev)
		gpio_export_link(wlan_dev, "WLAN_REG_ON", wlan_pwr_on);

#ifdef BCMPCIE
	exynos_pcie_pm_resume(pcie_ch_num);
#endif /* BCMPCIE */

#ifdef DHD_USE_HOST_WAKE
	/* ========== WLAN_HOST_WAKE ============ */
	wlan_host_wake_up = of_get_gpio(root_node, 1);
	if (!gpio_is_valid(wlan_host_wake_up)) {
		WARN(1, "Invalied gpio pin : %d\n", wlan_host_wake_up);
		return -ENODEV;
	}

	if (gpio_request(wlan_host_wake_up, "WLAN_HOST_WAKE")) {
		WARN(1, "fail to request gpio(WLAN_HOST_WAKE)\n");
		return -ENODEV;
	}
	gpio_direction_input(wlan_host_wake_up);
	gpio_export(wlan_host_wake_up, 1);
	if (wlan_dev)
		gpio_export_link(wlan_dev, "WLAN_HOST_WAKE", wlan_host_wake_up);

	wlan_host_wake_irq = gpio_to_irq(wlan_host_wake_up);
#endif /* DHD_USE_HOST_WAKE */

	return 0;
}

#ifdef DHD_USE_HOST_WAKE
static int dhd_wlan_get_wake_irq(void)
{
	return gpio_to_irq(wlan_host_wake_up);
}

static int dhd_get_wlan_oob_gpio_level(void)
{
	return gpio_is_valid(wlan_host_wake_up) ?
		gpio_get_value_cansleep(wlan_host_wake_up) : -1;
}

int dhd_get_wlan_oob_gpio(void)
{
	return dhd_get_wlan_oob_gpio_level();
}
#endif /* DHD_USE_HOST_WAKE */

struct resource dhd_wlan_resources = {
	.name  = "bcmdhd_wlan_irq",
	.start = 0, /* Dummy */
	.end   = 0, /* Dummy */
	.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE |
#ifdef BCMPCIE
	IORESOURCE_IRQ_HIGHEDGE,
#else /* non-BCMPCIE */
	IORESOURCE_IRQ_HIGHLEVEL,
#endif /* BCMPCIE */
};

struct wifi_platform_data dhd_wlan_control = {
	.set_power      = dhd_wlan_power,
	.set_reset      = dhd_wlan_reset,
	.set_carddetect = dhd_wlan_set_carddetect,
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	.mem_prealloc   = dhd_wlan_mem_prealloc,
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */
#ifdef DHD_USE_HOST_WAKE
	.get_wake_irq   = dhd_wlan_get_wake_irq,
#endif /* DHD_USE_HOST_WAKE */
};
EXPORT_SYMBOL(dhd_wlan_control);

int __init
dhd_wlan_init(void)
{
	int ret;

	printk(KERN_INFO "%s: START.......\n", __FUNCTION__);
	ret = dhd_wlan_init_gpio();
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to initiate GPIO, ret=%d\n",
			__FUNCTION__, ret);
		goto fail;
	}

#ifdef DHD_USE_HOST_WAKE
	dhd_wlan_resources.start = wlan_host_wake_irq;
	dhd_wlan_resources.end = wlan_host_wake_irq;
#endif /* DHD_USE_HOST_WAKE */

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	ret = dhd_init_wlan_mem();
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to alloc reserved memory,"
			" ret=%d\n", __FUNCTION__, ret);
	}
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

fail:
	return ret;
}

void
dhd_wlan_deinit(void)
{
#ifdef DHD_USE_HOST_WAKE
	gpio_free(wlan_host_wake_up);
#endif /* DHD_USE_HOST_WAKE */
	gpio_free(wlan_pwr_on);

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	dhd_exit_wlan_mem();
#endif /*  CONFIG_BROADCOM_WIFI_RESERVED_MEM */
}

#ifndef BCMDHD_MODULAR
#if defined(CONFIG_MACH_UNIVERSAL7420) || defined(CONFIG_SOC_EXYNOS8890) || \
	defined(CONFIG_SOC_EXYNOS8895) || defined(CONFIG_SOC_EXYNOS9810) || \
	defined(CONFIG_SOC_EXYNOS9820) || defined(CONFIG_SOC_EXYNOS9830)
#if defined(CONFIG_DEFERRED_INITCALLS)
deferred_module_init(dhd_wlan_init);
#elif defined(late_initcall)
late_initcall(dhd_wlan_init);
#endif /* CONFIG_DEFERRED_INITCALLS */
#else /* default */
module_init(dhd_wlan_init);
#endif /* CONFIG Exynos PCIE Platforms */
module_exit(dhd_wlan_deinit);
#endif /* !BCMDHD_MODULAR */
