/*
 * linux/drivers/video/backlight/dvf9a_pwm_bl.c - DVF9A PWM backlight control
 *
 * (C) Copyright 2014, DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/clk.h>
#include <linux/backlight.h>
#include <linux/mfd/core.h>
#include <linux/mfd/dvf9a.h>

#define PWM_EN   (1 << 5)
#define PWM_HIZ  (1 << 6)
#define PWM_MODE (1 << 7)

#define DVF9A_BL_HIZ 1

struct dvf9a_pwm_bl_device {
	struct dvf9a *dvf9a;
	struct backlight_device *bl;
	int brightness;
	int mode;
	int modulo;
	int pwm;
	int prescalar;
	int pwm_duty_reg, pwm_cfg1_reg, pwm_cfg2_reg, pwm_cfg3_reg;
};

static void
dvf9a_pwm_bl_set_backlight_openloop(struct dvf9a_pwm_bl_device *dvf9a_pwm_bl,
				    int brightness)
{
	unsigned short val;

	/* Set brightness by changing the duty-cycle of the pwm */
	daif_write(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_duty_reg,
		   brightness % dvf9a_pwm_bl->modulo);

	val = daif_read(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg2_reg);

	if (brightness) /* Enable PWM */
		val |= PWM_EN;
	else /* Disable PWM */
		val &= ~PWM_EN;

	daif_write(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg2_reg, val);
}

static void
dvf9a_pwm_bl_set_backlight_hiz(struct dvf9a_pwm_bl_device *dvf9a_pwm_bl,
			       int brightness)
{
	unsigned short val;

	val = daif_read(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg2_reg);

	/* Set brightness by toggling 'hiz' (output to high Z when disabled) */
	if (brightness)
		val |= PWM_HIZ;
	else
		val &= ~PWM_HIZ;

	daif_write(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg2_reg, val);
}

static int
dvf9a_pwm_bl_update_status(struct backlight_device *dev)
{
	struct backlight_properties *props = &dev->props;
	struct dvf9a_pwm_bl_device *dvf9a_pwm_bl = dev_get_drvdata(&dev->dev);
	int brightness = props->brightness;

	if ((props->power != FB_BLANK_UNBLANK) ||
	    (props->state & BL_CORE_SUSPENDED))
		brightness = 0;

	if (dvf9a_pwm_bl->mode == DVF9A_BL_HIZ)
		dvf9a_pwm_bl_set_backlight_hiz(dvf9a_pwm_bl, brightness);
	else
		dvf9a_pwm_bl_set_backlight_openloop(dvf9a_pwm_bl, brightness);

	return 0;
}

static int
dvf9a_pwm_bl_get_brightness(struct backlight_device *dev)
{
	struct backlight_properties *props = &dev->props;

	return props->brightness;
}

static const struct backlight_ops dvf9a_pwm_bl_ops = {
	.options	= BL_CORE_SUSPENDRESUME,
	.get_brightness	= dvf9a_pwm_bl_get_brightness,
	.update_status	= dvf9a_pwm_bl_update_status,
};

static int
dvf9a_pwm_bl_probe(struct platform_device *pdev)
{
	struct dvf9a *dvf9a = dev_get_drvdata(pdev->dev.parent);
	struct device_node *np = pdev->dev.of_node;
	struct dvf9a_pwm_bl_device *dvf9a_pwm_bl;
	struct backlight_properties props;
	char name[11];
	int ret = 0;
	u32 val;

	dvf9a_pwm_bl = devm_kzalloc(&pdev->dev,
				    sizeof(struct dvf9a_pwm_bl_device),
				    GFP_KERNEL);
	if (!dvf9a_pwm_bl)
		return -ENOMEM;

	dvf9a_pwm_bl->dvf9a = dvf9a;
	platform_set_drvdata(pdev, dvf9a_pwm_bl);

	dvf9a_pwm_bl->mode = 0;

	val = 8;
	ret = of_property_read_u32(np, "modulo", &val);

	switch (val) {
	case 8:
	case 16:
	case 32:
	case 64:
	case 128:
	case 256:
		break;
	default:
		val = ~0;
		break;
	}

	if ((ret && ret != -EINVAL) || (val > 256)) {
		dev_err(&pdev->dev, "invalid 'modulo'\n");
		ret = -EINVAL;
		goto err;
	}
	dvf9a_pwm_bl->modulo = val;

	val = 0;
	ret = of_property_read_u32(np, "pwm", &val);
	if ((ret && ret != -EINVAL) || (val > 1)) {
		dev_err(&pdev->dev, "invalid 'pwm'\n");
		ret = -EINVAL;
		goto err;
	}
	dvf9a_pwm_bl->pwm = val;

	val = 0;
	ret = of_property_read_u32(np, "duty", &val);
	if ((ret && ret != -EINVAL) ||
	    (val > dvf9a_pwm_bl->modulo)) {
		dev_err(&pdev->dev, "invalid 'duty'\n");
		ret = -EINVAL;
		goto err;
	}
	dvf9a_pwm_bl->brightness = val;

	val = 0;
	ret = of_property_read_u32(np, "prescalar", &val);
	if ((ret && ret != -EINVAL) || (val >= 1024)) {
		dev_err(&pdev->dev, "invalid 'prescalar'\n");
		ret = -EINVAL;
		goto err;
	}
	dvf9a_pwm_bl->prescalar = val;

	if (dvf9a_pwm_bl->pwm == 0) {
		dvf9a_pwm_bl->pwm_duty_reg = DAIF_PWM0_DUTY;
		dvf9a_pwm_bl->pwm_cfg1_reg = DAIF_PWM0_CFG1;
		dvf9a_pwm_bl->pwm_cfg2_reg = DAIF_PWM0_CFG2;
		dvf9a_pwm_bl->pwm_cfg3_reg = DAIF_PWM0_CFG3;
	} else {
		dvf9a_pwm_bl->pwm_duty_reg = DAIF_PWM1_DUTY;
		dvf9a_pwm_bl->pwm_cfg1_reg = DAIF_PWM1_CFG1;
		dvf9a_pwm_bl->pwm_cfg2_reg = DAIF_PWM1_CFG2;
		dvf9a_pwm_bl->pwm_cfg3_reg = DAIF_PWM1_CFG3;
	}

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_PLATFORM;
	props.max_brightness = dvf9a_pwm_bl->modulo;
	props.brightness = dvf9a_pwm_bl->brightness % dvf9a_pwm_bl->modulo;

	snprintf(name, sizeof(name), "dvf9a-bl.%d", dvf9a_pwm_bl->pwm);
	dvf9a_pwm_bl->bl = backlight_device_register(name, &pdev->dev,
						     dvf9a_pwm_bl,
						     &dvf9a_pwm_bl_ops,
						     &props);
	if (IS_ERR(dvf9a_pwm_bl->bl)) {
		dev_err(&pdev->dev, "failed to register backlight device\n");
		ret = PTR_ERR(dvf9a_pwm_bl->bl);
		goto err;
	}

	val = (dvf9a_pwm_bl->prescalar >> 8) & 0x3;
	switch (dvf9a_pwm_bl->modulo) {
	case 8:
		val |= (0 << 2);
		break;
	case 16:
		val |= (1 << 2);
		break;
	case 32:
		val |= (4 << 2);
		break;
	case 64:
		val |= (5 << 2);
		break;
	case 128:
		val |= (6 << 2);
		break;
	case 256:
		val |= (7 << 2);
		break;
	}
	daif_write(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg1_reg, 0);
	daif_write(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg2_reg, val);
	daif_write(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg3_reg,
		   dvf9a_pwm_bl->prescalar & 0xff);

	dvf9a_apu_clk(dvf9a_pwm_bl->dvf9a, 1);

	backlight_update_status(dvf9a_pwm_bl->bl);

	dev_info(&pdev->dev, "successfully registered\n");
	ret = 0;

err:
	return ret;
}

static int
dvf9a_pwm_bl_remove(struct platform_device *pdev)
{
	struct dvf9a_pwm_bl_device *dvf9a_pwm_bl = platform_get_drvdata(pdev);

	dvf9a_apu_clk(dvf9a_pwm_bl->dvf9a, 0);
	backlight_device_unregister(dvf9a_pwm_bl->bl);

	return 0;
}

static void
dvf9a_pwm_bl_shutdown(struct platform_device *pdev)
{
	struct dvf9a_pwm_bl_device *dvf9a_pwm_bl = platform_get_drvdata(pdev);
	unsigned short val;

	/* disable PWM before rebooting */
	val = daif_read(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg2_reg);
	val &= ~(PWM_HIZ | PWM_EN);
	daif_write(dvf9a_pwm_bl->dvf9a, dvf9a_pwm_bl->pwm_cfg2_reg, val);
}

static const struct of_device_id dvf9a_pwm_bl_of_ids[] = {
	{ .compatible = "dspg,dvf9a-backlight" },
	{ },
};

static struct platform_driver dvf9a_pwm_bl_driver = {
	.driver = {
		.name = "dvf9a-backlight",
		.owner = THIS_MODULE,
		.of_match_table = dvf9a_pwm_bl_of_ids,
	},
	.probe = dvf9a_pwm_bl_probe,
	.remove = dvf9a_pwm_bl_remove,
	.shutdown = dvf9a_pwm_bl_shutdown,
};
module_platform_driver(dvf9a_pwm_bl_driver);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_ALIAS("platform:dvf9a-bl");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DVF9A PWM backlight");
