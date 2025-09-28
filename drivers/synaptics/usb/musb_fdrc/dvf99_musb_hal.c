/*

 * This is the MUSBFDRC USB controller used by DSPG's DVF99 SoC

 * This file is part of the Inventra Controller Driver for Linux.
 *
 * The Inventra Controller Driver for Linux is free software; you
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 2 as published by the Free Software
 * Foundation.
 *
 * The Inventra Controller Driver for Linux is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with The Inventra Controller Driver for Linux ; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/err.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/power_supply.h>
#include <linux/timer.h>
#include <linux/usb/usb_phy_generic.h>
#include <linux/regulator/consumer.h>
#include "musb_core.h"
#include "dvf99_musb_hal.h"
#include <mach/hardware.h>
#include <mach/platform.h>

#define MAX_MUSB_POWER_REGISTER_READ_RETRIES 10000
/****************functions declaration***************************/
static void dvf99_musb_source_power(struct musb *musb, int is_on);
void _dvf99_disconnect_prep(struct work_struct *data);
/****************functions declaration***************************/

struct dvf99_musb_board_data {
	u8	interface_type;
	u8	mode;
	u16	power;
	unsigned extvbus:1;
	void	(*set_phy_power)(u8 on);
	void	(*clear_irq)(void);
	void	(*set_mode)(u8 mode);
	void	(*reset)(void);
};

enum musb_interface    {MUSB_INTERFACE_ULPI, MUSB_INTERFACE_UTMI};

struct dvf99_glue {
	struct device		*dev;
	struct platform_device	*musb;
};

#define glue_to_musb(g)		platform_get_drvdata(g->musb)

/****************** variables ***********************/
struct				regulator *vbus_reg;
struct clk			*mac_clk;
struct reset_control		*reset;
static int mode = MUSB_PERIPHERAL; /* default mode will be device */
/****************** variables ***********************/

static void musb_do_idle(struct timer_list *t)
{
	struct musb *musb = from_timer(musb, t, idle_timer);
	unsigned long	flags;
	u8	power;
	u8	devctl;

	spin_lock_irqsave(&musb->lock, flags);

	switch (musb->xceiv->otg->state) {
	case OTG_STATE_A_WAIT_BCON:

		devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
		if (devctl & MUSB_DEVCTL_CID) {
			musb->xceiv->otg->state = OTG_STATE_B_IDLE;
			MUSB_DEV_MODE(musb);
		} else {
			musb->xceiv->otg->state = OTG_STATE_A_IDLE;
			MUSB_HST_MODE(musb);
		}
		break;
	case OTG_STATE_A_SUSPEND:
		/* finish RESUME signaling? */
		if (musb->port1_status & MUSB_PORT_STAT_RESUME) {
			power = musb_readb(musb->mregs, MUSB_POWER);
			power &= ~MUSB_POWER_RESUME;
			dev_dbg(musb->controller,
				"root port resume stopped, power %02x\n"
				, power);
			musb_writeb(musb->mregs, MUSB_POWER, power);
			musb->is_active = 1;
			musb->port1_status &= ~(USB_PORT_STAT_SUSPEND
						| MUSB_PORT_STAT_RESUME);
			musb->port1_status |= USB_PORT_STAT_C_SUSPEND << 16;
			usb_hcd_poll_rh_status(musb_to_hcd(musb));
			/* NOTE: it might really be A_WAIT_BCON ... */
			musb->xceiv->otg->state = OTG_STATE_A_HOST;
		}
		break;
	case OTG_STATE_A_HOST:
		devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
		/* BDEVICE field replace by CID field */
		if (devctl &  MUSB_DEVCTL_CID)
			musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		else
			musb->xceiv->otg->state = OTG_STATE_A_WAIT_BCON;
	default:
		break;
	}
	spin_unlock_irqrestore(&musb->lock, flags);
}


static void dvf99_musb_try_idle(struct musb *musb, unsigned long timeout)
{
	unsigned long		default_timeout = jiffies + msecs_to_jiffies(3);
	static unsigned long	last_timer;

	if (timeout == 0)
		timeout = default_timeout;

	/* Never idle if active, or when VBUS timeout is not set as host */
	if (musb->is_active ||
	    ((musb->a_wait_bcon == 0) &&
	     (musb->xceiv->otg->state == OTG_STATE_A_WAIT_BCON))) {
		dev_dbg(musb->controller, "%s active, deleting timer\n",
			usb_otg_state_string(musb->xceiv->otg->state));
		del_timer(&musb->idle_timer);
		last_timer = jiffies;
		return;
	}

	if (time_after(last_timer, timeout)) {
		if (!timer_pending(&musb->idle_timer)) {
			last_timer = timeout;
		} else {
			dev_dbg(musb->controller, "Longer idle timer already pending, ignoring\n");
			return;
		}
	}
	last_timer = timeout;

	dev_dbg(musb->controller, "%s inactive, for idle timer for %lu ms\n",
		usb_otg_state_string(musb->xceiv->otg->state),
		(unsigned long)jiffies_to_msecs(timeout - jiffies));
	mod_timer(&musb->idle_timer, timeout);
}

static void dvf99_musb_set_vbus(struct musb *musb, int is_on)
{
	struct usb_otg	*otg = musb->xceiv->otg;
	u8		devctl;
	unsigned long timeout = jiffies + msecs_to_jiffies(1000);
	int ret = 1;
	/* FDRC controls CPEN, but beware current surges during device
	 * connect.  They can trigger transient overcurrent conditions
	 * that must be ignored.
	 */

	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	if (is_on) {
		if (musb->xceiv->otg->state == OTG_STATE_A_IDLE) {
			/* start the session */
			devctl |= MUSB_DEVCTL_SESSION;
			musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);
			/*
			 * Wait for the musb to set as A device to enable the
			 * VBUS
			 */

			while (musb_readb(musb->mregs, MUSB_DEVCTL) & 0x80) {

				cpu_relax();

				if (time_after(jiffies, timeout)) {
					dev_err(musb->controller,
						"configured as A device timeout");
					ret = -EINVAL;
					break;
				}
			}


			if (ret && otg->set_vbus)
				otg_set_vbus(otg, 1);
		} else {

			musb->is_active = 1;
			otg->default_a = 1;
			musb->xceiv->otg->state = OTG_STATE_A_WAIT_VRISE;
			devctl |= MUSB_DEVCTL_SESSION;
			MUSB_HST_MODE(musb);
		}

	} else {

		musb->is_active = 0;

		/* NOTE:  we're skipping A_WAIT_VFALL -> A_IDLE and
		 * jumping right to B_IDLE...
		 */

		otg->default_a = 0;
		musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		/* If VBUS is OFF session is probably ended - de-assert
		 * SESSION bit */
		devctl &= ~MUSB_DEVCTL_SESSION;

		MUSB_DEV_MODE(musb);
	}

	musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);

	dev_dbg(musb->controller, "VBUS %s, devctl %02x "
		/* otg %3x conf %08x prcm %08x */ "\n",
		usb_otg_state_string(musb->xceiv->otg->state),
		musb_readb(musb->mregs, MUSB_DEVCTL));


}

static int dvf99_musb_set_mode(struct musb *musb, u8 musb_mode)
{
	u8	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	devctl |= MUSB_DEVCTL_SESSION;
	musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);

	return 0;
}

static inline void dvf99_low_level_exit(struct musb *musb)
{

	/* if supported - standbuy VBUS/USB from
	 * DVF99 registers - in omap there was forced standby */

}

static inline void dvf99_low_level_init(struct musb *musb)
{

	/* if supported - standbuy VBUS/USB from
	 * DVF99 registers - in omap there was forced standby */

}

static void dvf99_musb_source_power(struct musb *musb, int is_on)
{
	int ret = 0;
	u32 usb11_gcr1;

	usb11_gcr1 = dvf_get_usb11_gcr();

	if (vbus_reg) {
		struct regulator *reg = vbus_reg;

		if (is_on && regulator_is_enabled(reg) <= 0) {
			ret = regulator_enable(reg);
			usb11_gcr1 |= DVF99_USB11_GCR1_USB_GLB_ON_MSK;
		} else if (!is_on && regulator_is_enabled(reg) > 0) {
			ret = regulator_disable(reg);
			usb11_gcr1 &= ~DVF99_USB11_GCR1_USB_GLB_ON_MSK;
		}
	}

}


static int dvf99_musb_init(struct musb *musb)
{
	u32 usb11_gcr1;
	u32 fdrc_pwr;
	u8 devctl;
	u8 faddr;
	u8 intrusbe;
	u16 retry = DVF99_USB11_NUM_RESET_RETRIES;

	usb_phy_generic_register();
	musb->xceiv = usb_get_phy(USB_PHY_TYPE_USB2);
	if (IS_ERR_OR_NULL(musb->xceiv))
		goto unregister;

	timer_setup(&musb->idle_timer, musb_do_idle, 0);

	/*Init workqueue dvf99_disconnect_prep which contains msleep*/
	INIT_DELAYED_WORK(&musb->dvf99_disconnect_prep_s, _dvf99_disconnect_prep);

	/* read FDRC power register and print */
	fdrc_pwr = musb_readl(musb->mregs, MUSB_POWER);

	usb11_gcr1 = dvf_get_usb11_gcr();
	/* USB11 PHY power control */
	usb11_gcr1 |= DVF99_USB11_GCR1_USB_GLB_ON_MSK;
	/* VBUS val valid indication to the USB11 MAC */
	usb11_gcr1 |= DVF99_USB11_GCR1_VBUSVAL_MSK;
	if (mode == MUSB_PERIPHERAL) {
		/*VBUS session valid indication to the USB11 MAC*/
		usb11_gcr1 |= DVF99_USB11_GCR1_VBUSSES_MSK |
		/* VBUS session end threshold comparison
		 * indication to the USB11 MAC */
		DVF99_USB11_GCR1_VBUSLO_MSK |
		DVF99_USB11_GCR1_USB_PUDP_CTRL_PU_ACTIVATED_MSK |
		DVF99_USB11_GCR1_CID;

		usb11_gcr1 &= ~(DVF99_USB11_GCR1_USB_PDM |
				DVF99_USB11_GCR1_USB_PUM);
	} else if (mode == MUSB_HOST) {
		usb11_gcr1 |= DVF99_USB11_GCR1_USB_PUDP_CTRL_PD_ACTIVATED_MSK |
		DVF99_USB11_GCR1_VBUSEN | DVF99_USB11_GCR1_USB_PDM;
		usb11_gcr1 &= ~(DVF99_USB11_GCR1_CID |
			DVF99_USB11_GCR1_USB_PUDP_CTRL_PU_ACTIVATED_MSK);
	}
	usb11_gcr1 |= DVF99_USB11_GCR1_USB_BLOCK_RES_MSK;
	dvf_set_usb11_gcr(usb11_gcr1);
	/*disable mac clock */
	clk_disable(mac_clk);
	/* reset DVF99 USB11 controller by asserting
	* DVF99_USB11_GCR1_USB_BLOCK_RES_MSK bit for 0x200 retries */
	while (--retry) {
		if (!(dvf_get_usb11_gcr() &
		      DVF99_USB11_GCR1_USB_BLOCK_RES_MSK))
			break;
	}
	/* in the end of reset de-assert the bit to finish */
	if (dvf_get_usb11_gcr() & DVF99_USB11_GCR1_USB_BLOCK_RES_MSK) {
		usb11_gcr1 &= ~DVF99_USB11_GCR1_USB_BLOCK_RES_MSK;
		dvf_set_usb11_gcr(usb11_gcr1);
	}

	/* enable mac clock */
	clk_enable(mac_clk);
	if (mode == MUSB_HOST) {
		fdrc_pwr = musb_readl(musb->mregs, MUSB_POWER);
		devctl = musb_readl(musb->mregs, MUSB_DEVCTL);
		devctl |= MUSB_DEVCTL_SESSION;
		musb_writel(musb->mregs, MUSB_DEVCTL, devctl);
		dvf99_musb_source_power(musb, HOST_VBUS_POWER_ON);
	}

	if (mode == MUSB_PERIPHERAL) {
		/* reset faddr */
		faddr = musb_readb(musb->mregs,MUSB_FADDR);
		faddr = 0x0;
		musb_writeb(musb->mregs, MUSB_FADDR, faddr);

		/* enable ISO_UPD */
		fdrc_pwr = musb_readb(musb->mregs, MUSB_POWER);
		fdrc_pwr |= MUSB_POWER_ISOUPDATE;
		musb_writeb(musb->mregs, MUSB_POWER, fdrc_pwr);

		/* enable interrupts */
		intrusbe =  (MUSB_INTR_VBUSERROR | MUSB_INTR_SESSREQ |
			     MUSB_INTR_DISCONNECT | MUSB_INTR_CONNECT |
			     MUSB_INTR_RESET | MUSB_INTR_RESUME);
		musb_writeb(musb->mregs, MUSB_INTRUSBE, intrusbe);

	}
	return 0;

unregister:
		usb_phy_generic_register();
		return -ENODEV;
}

static void dvf99_musb_enable(struct musb *musb)
{
	u8		devctl;
	unsigned long timeout = jiffies + msecs_to_jiffies(1000);
	struct device *dev = musb->controller;
	struct musb_hdrc_platform_data *pdata = dev->platform_data;
	struct dvf99_musb_board_data *data = pdata->board_data;

	switch (musb->xceiv->last_event) {

	case USB_EVENT_ID:
		usb_phy_init(musb->xceiv);
		if (data->interface_type != MUSB_INTERFACE_UTMI)
			break;
		devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
		/* start the session */
		devctl |= MUSB_DEVCTL_SESSION;
		musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);
		while (musb_readb(musb->mregs, MUSB_DEVCTL) &
				/* BDEVICE field replace by CID field */
				MUSB_DEVCTL_CID) {
			cpu_relax();

			if (time_after(jiffies, timeout)) {
				dev_err(dev, "configured as A device timeout");
				break;
			}
		}
		break;

	case USB_EVENT_VBUS:
		usb_phy_init(musb->xceiv);
		break;

	default:
		break;
	}
	if (mode == MUSB_HOST)
		dvf99_musb_source_power(musb, HOST_VBUS_POWER_ON);

}

static void dvf99_musb_disable(struct musb *musb)
{

	if (musb->xceiv->last_event)
		usb_phy_shutdown(musb->xceiv);

	if (mode == MUSB_HOST)
		dvf99_musb_source_power(musb, HOST_VBUS_POWER_OFF);


}

static int dvf99_musb_exit(struct musb *musb)
{
	del_timer_sync(&musb->idle_timer);

	dvf99_low_level_exit(musb);
	usb_put_phy(musb->xceiv);

	return 0;
}

/* dvf99_connect_prep() sets VBUSSES and VBUSLO in USB11_GCR1 register.
 * This function is required because VBUSSES,VBUSLO and VBUSVAL are
 * disconnected from HW circuitry/comparators and thus OTG functionality
 * is disabled. Pull-up control is transferred to be fully controlled
 * by USB core. Internal pull-downs are always enabled in host mode.
 */
static void dvf99_connect_prep(struct musb *musb)
{
	u32 usb11_gcr1;
	u32 cnt = MAX_MUSB_POWER_REGISTER_READ_RETRIES;
	u8 devctl, power;
	u8 completed;

	usb11_gcr1 = dvf_get_usb11_gcr();
	usb11_gcr1 |= (DVF99_USB11_GCR1_USB_PDM |
		       DVF99_USB11_GCR1_VBUSSES_MSK |
		       DVF99_USB11_GCR1_VBUSLO_MSK);
	usb11_gcr1 &= ~(DVF99_USB11_GCR1_USB_PUM |
			DVF99_USB11_GCR1_USB_PUDP_CTRL_PU_ACTIVATED_MSK);

	if (mode == MUSB_HOST)
		usb11_gcr1 |= DVF99_USB11_GCR1_USB_PUDP_CTRL_PD_ACTIVATED_MSK;
	else
		usb11_gcr1 &= ~DVF99_USB11_GCR1_USB_PUDP_CTRL_PD_ACTIVATED_MSK;

	dvf_set_usb11_gcr(usb11_gcr1);

	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
	power = musb_readb(musb->mregs, MUSB_POWER);

	do {
		devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
		power = musb_readb(musb->mregs, MUSB_POWER);
		completed = ((devctl & MUSB_DEVCTL_HM) &&
			     (power & MUSB_POWER_VBUS_VAL) &&
			     (power & MUSB_POWER_VBUS_SESS) &&
			     (power & MUSB_POWER_VBUS_LO));
		cnt--;
	} while ((!completed) && cnt);

}

static void dvf99_disconnect_prep(struct musb *musb)
{
	schedule_delayed_work(&musb->dvf99_disconnect_prep_s, msecs_to_jiffies(200));
}

/* _dvf99_disconnect_prep() de-assert VBUSSES and VBUSLO in USB11_GCR1 register.
 * This function is required because VBUSSES,VBUSLO and VBUSVAL are
 * disconnected from HW circuitry/comparators and thus OTG functionality
 * is disabled */
void _dvf99_disconnect_prep(struct work_struct *data)
{
	u32 usb11_gcr1;
	struct musb *musb;
	u8 power;
	u32 cnt = MAX_MUSB_POWER_REGISTER_READ_RETRIES;

	musb = container_of(data, struct musb, dvf99_disconnect_prep_s.work);

	/* busy wait on usb bus reset done.
	 * If bus stuck on reset, will be handled at end of function
	 */
	do {
		power = musb_readb(musb->mregs, MUSB_POWER);
		cnt--;
	} while ((power & MUSB_POWER_RESET) && (cnt > 0));
	

	musb_root_disconnect(musb);

	/* prepare USB controller for next connect */
	usb11_gcr1 = dvf_get_usb11_gcr();
	usb11_gcr1 &= ~(DVF99_USB11_GCR1_VBUSVAL_MSK |
			DVF99_USB11_GCR1_VBUSSES_MSK |
			DVF99_USB11_GCR1_VBUSLO_MSK);
	if (mode != MUSB_HOST)
		usb11_gcr1 &= ~DVF99_USB11_GCR1_USB_PDM;
	dvf_set_usb11_gcr(usb11_gcr1);

	/* wait for DVF99_USB11_GCR1 register change to affect usb controller */
	mdelay(1);

	usb11_gcr1 = dvf_get_usb11_gcr();
	usb11_gcr1 |= DVF99_USB11_GCR1_VBUSVAL_MSK;
	dvf_set_usb11_gcr(usb11_gcr1);

	/* ensure that usb bus is not reset */
	power = musb_readb(musb->mregs, MUSB_POWER);
	musb_writeb(musb->mregs, MUSB_POWER, power & ~(MUSB_POWER_RESET));

}

static const struct musb_platform_ops dvf99_ops = {
	.init			= dvf99_musb_init,
	.exit			= dvf99_musb_exit,
	.set_mode		= dvf99_musb_set_mode,
	.try_idle		= dvf99_musb_try_idle,
	.set_vbus		= dvf99_musb_set_vbus,
	.dvf99_connect_prep	= dvf99_connect_prep,
	.dvf99_disconnect_prep	= dvf99_disconnect_prep,
	.enable			= dvf99_musb_enable,
	.disable		= dvf99_musb_disable,
};

static u64 dvf99_dmamask = DMA_BIT_MASK(32);

static int dvf99_probe(struct platform_device *pdev)
{
	struct musb_hdrc_platform_data	*pdata = pdev->dev.platform_data;
	struct platform_device		*musb;
	struct dvf99_glue		*glue;
	int				ret = -ENOMEM;

	struct device_node              *np = pdev->dev.of_node;
	struct dvf99_musb_board_data     *data;
	struct musb_hdrc_config         *config;
	int				value;
	unsigned long			rate;

	if (np && !of_property_read_u32(np, "mode", (u32 *)&value))
		mode = value;

	if (mode != MUSB_HOST && mode != MUSB_PERIPHERAL) {
		dev_err(&pdev->dev,
			"mode valid values are 1 (for HOST), 2 (for PERIPHERAL)\n");
		ret = -EINVAL;
		goto err0;
	}

	glue = kzalloc(sizeof(*glue), GFP_KERNEL);
	if (!glue) {
		dev_err(&pdev->dev, "failed to allocate glue context\n");
		goto err0;
	}

	musb = platform_device_alloc("musb-fdrc", -1);
	if (!musb) {
		dev_err(&pdev->dev, "failed to allocate musb device\n");
		goto err1;
	}

	musb->dev.parent		= &pdev->dev;
	musb->dev.dma_mask		= &dvf99_dmamask;
	musb->dev.coherent_dma_mask	= dvf99_dmamask;
	glue->dev			= &pdev->dev;
	glue->musb			= musb;

	if (np) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev,
				"failed to allocate musb platfrom data\n");
			ret = -ENOMEM;
			goto err2;
		}

		data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
		if (!data) {
			dev_err(&pdev->dev,
				"failed to allocate musb board data\n");
			ret = -ENOMEM;
			goto err2;
		}

		config = devm_kzalloc(&pdev->dev, sizeof(*config), GFP_KERNEL);
		if (!data) {
			dev_err(&pdev->dev,
				"failed to allocate musb fdrc config\n");
			goto err2;
		}

		pdata->mode = mode;
		config->multipoint = of_property_read_bool(np, "multipoint");
		of_property_read_u32(np, "num_eps", (u32 *)&config->num_eps);
		of_property_read_u32(np, "ram_bits", (u32 *)&config->ram_bits);
		pdata->config = config;

	}

	pdata->platform_ops		= &dvf99_ops;

	platform_set_drvdata(pdev, glue);

	ret = platform_device_add_resources(musb, pdev->resource,
			pdev->num_resources);
	if (ret) {
		dev_err(&pdev->dev, "failed to add resources\n");
		goto err2;
	}

	ret = platform_device_add_data(musb, pdata, sizeof(*pdata));
	if (ret) {
		dev_err(&pdev->dev, "failed to add platform_data\n");
		goto err2;
	}

	reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(reset)) {
		dev_err(&pdev->dev, "failed to get reset control\n");
		ret = PTR_ERR(reset);
		goto err2;
	}

	ret = reset_control_deassert(reset);
	if (ret) {
		dev_err(&pdev->dev, "failed to deassert reset\n");
		goto err2;
	}

	/* adding mac clock */
	mac_clk = devm_clk_get(&pdev->dev, "mac");
	if (IS_ERR(mac_clk)) {
		dev_err(&pdev->dev, "failed to get clock\n");
		goto err2;
	}
	rate = clk_round_rate(mac_clk, 48000000);
	if (rate < 47880000 || rate > 48120000) {
		dev_err(&pdev->dev, "clock tree configuration invalid\n");
		goto err2;
	}
	if (clk_set_rate(mac_clk, rate)) {
		dev_err(&pdev->dev, "cannot set clock to 48MHz\n");
		goto err2;
	}
	/* enable mac clock */
	clk_prepare_enable(mac_clk);

	vbus_reg = regulator_get(&pdev->dev, "vbus");


	if (IS_ERR(vbus_reg)) {
		int err = PTR_ERR(vbus_reg);
		dev_warn(&pdev->dev, "cannot get vbus regulator: %d\n", err);
		vbus_reg = NULL;
	}

	pm_runtime_enable(&pdev->dev);

	ret = platform_device_add(musb);
	if (ret) {
		dev_err(&pdev->dev, "failed to register musb device\n");
		goto err2;
	}

	return 0;

err2:
	platform_device_put(musb);

err1:
	kfree(glue);

err0:
	return ret;
}

static int dvf99_remove(struct platform_device *pdev)
{
	struct dvf99_glue		*glue = platform_get_drvdata(pdev);

	clk_disable(mac_clk);
	pm_runtime_disable(&pdev->dev);
	platform_device_del(glue->musb);
	platform_device_put(glue->musb);
	kfree(glue);

	if (vbus_reg) {
		if (regulator_is_enabled(vbus_reg))
			regulator_disable(vbus_reg);
		regulator_put(vbus_reg);
	}

	return 0;
}



#ifdef CONFIG_PM

static int dvf99_runtime_suspend(struct device *dev)
{
	struct dvf99_glue		*glue = dev_get_drvdata(dev);
	struct musb			*musb = glue_to_musb(glue);

	if (musb) {
		/* if CONFIG_PM supported, suspend by
		 * writing to DVF99 register */

		dvf99_low_level_exit(musb);
		usb_phy_set_suspend(musb->xceiv, 1);
	}

	return 0;
}

static int dvf99_runtime_resume(struct device *dev)
{
	struct dvf99_glue		*glue = dev_get_drvdata(dev);
	struct musb			*musb = glue_to_musb(glue);

	if (musb) {
		dvf99_low_level_init(musb);

		/* if CONFIG_PM supported, resume by
		 * writing to DVF99 register */

		usb_phy_set_suspend(musb->xceiv, 0);
	}

	return 0;
}

static struct dev_pm_ops dvf99_pm_ops = {
	.runtime_suspend = dvf99_runtime_suspend,
	.runtime_resume = dvf99_runtime_resume,
};

#define DEV_PM_OPS	(&dvf99_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif

static const struct of_device_id dvf99_id_table[] = {
	{
		.compatible = "mentor,musb"
	},

	{},
};

MODULE_DEVICE_TABLE(of, dvf99_id_table);

static struct platform_driver dvf99_driver = {
	.probe		= dvf99_probe,
	.remove		= dvf99_remove,
	.driver		= {
		.name	= "musb-dvf99",
		.pm	= DEV_PM_OPS,
		.of_match_table = of_match_ptr(dvf99_id_table),

	},
};

/* set mode when loading as external modal to switch role */
module_param(mode, int, 0400);

static int __init dvf99_init(void)
{
	return platform_driver_register(&dvf99_driver);
}
module_init(dvf99_init);

static void __exit dvf99_exit(void)
{
	platform_driver_unregister(&dvf99_driver);
}
module_exit(dvf99_exit);

MODULE_DESCRIPTION("dvf99 MUSB Glue Layer");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_LICENSE("GPL");
