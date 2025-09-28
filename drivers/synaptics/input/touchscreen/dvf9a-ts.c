/*
 * dvf9a-ts.c - touchscreen driver for DSPG-DVF9A APU
 *
 * Copyright (c) 2014 DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mfd/dvf9a.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/of.h>

#define DRIVER_NAME "dvf9a-touchscreen"

struct dvf9a_touch_measure {
	unsigned int x;
	unsigned int y;
	unsigned int z1;
	unsigned int z2;
};

struct dvf9a_touch
{
	struct dvf9a *dvf9a;
	struct input_dev *input;
	char phys[32];
	int cond;
	int irq, irq_ready;

	long a0, a1, a2, a3, a4, a5, a6;

	u32 x_min, x_max;
	u32 y_min, y_max;
	u32 pressure_min, pressure_max;

	u32 timer_interval; /* In 10 msec units */
	u32 untouch_timeout; /* In 10 msec units */

	int first_press;

	wait_queue_head_t waitqueue;
};

#define MAX_12BIT ((1<<12)-1)
#define MAX_8BIT  ((1<<8)-1)

static void
dvf9a_touch_update(struct dvf9a_touch *dvf9a_touch, unsigned short reg,
		   unsigned short mask, unsigned short val)
{
	unsigned short val_tmp;

	val_tmp = daif_read(dvf9a_touch->dvf9a, reg);

	if (val_tmp != val) {
		val_tmp &= ~(mask);
		val_tmp |= val;
		daif_write(dvf9a_touch->dvf9a, reg, val_tmp);
	}
}

static irqreturn_t
dvf9a_touch_irq(int event, void *dev)
{
	struct dvf9a_touch *dvf9a_touch = (struct dvf9a_touch *)dev;

	disable_irq_nosync(dvf9a_touch->irq_ready);

	/* Signal the worker thread */
	dvf9a_touch->cond = 1;
	wake_up_interruptible(&dvf9a_touch->waitqueue);

	return IRQ_HANDLED;
}

static int
dvf9a_touch_read_values(struct dvf9a_touch_measure *pvalues,
			struct dvf9a_touch *dvf9a_touch)
{
	pvalues->x  = daif_read(dvf9a_touch->dvf9a, DAIF_TPDX)  & 0x7FFF;
	pvalues->y  = daif_read(dvf9a_touch->dvf9a, DAIF_TPDY)  & 0x7FFF;
	pvalues->z1 = daif_read(dvf9a_touch->dvf9a, DAIF_TPDZ1) & 0x7FFF;
	pvalues->z2 = daif_read(dvf9a_touch->dvf9a, DAIF_TPDZ2) & 0x7FFF;

	return 0;
}

static int
dvf9a_touch_report_events(struct input_dev *input,
			  struct dvf9a_touch *dvf9a_touch)
{
	struct dvf9a_touch_measure values;
	unsigned char adjusted_x;
	unsigned char adjusted_y;
	int xtemp, ytemp;

	dvf9a_touch_read_values(&values, dvf9a_touch);
	daif_write(dvf9a_touch->dvf9a, DAIF_ICU_ANA_STAT1, 1<<5);

	if (values.z1 || dvf9a_touch->first_press) {
		/* adjusted the x and y using linear algorithm */
		xtemp = values.x;
		ytemp = values.y;
		adjusted_x = (dvf9a_touch->a2 + dvf9a_touch->a0*xtemp +
		              dvf9a_touch->a1*ytemp) / dvf9a_touch->a6;
		adjusted_y = (dvf9a_touch->a5 + dvf9a_touch->a3*xtemp +
		              dvf9a_touch->a4*ytemp) / dvf9a_touch->a6;

		/* report position and pressure */
		input_report_abs(input, ABS_X, adjusted_x);
		input_report_abs(input, ABS_Y, adjusted_y);
		/* input_report_abs(input, ABS_PRESSURE, 100); */
		input_sync(input);
	}

	return values.z1;
}

static int
dvf9a_touch_thread_code(void *data)
 {
	struct dvf9a_touch *dvf9a_touch = (struct dvf9a_touch *)data;
	struct input_dev *input = dvf9a_touch->input;
	struct dvf9a_touch_measure values;
	long ret;

	allow_signal(SIGTERM);

	while (1) {
		dvf9a_touch->cond = 0;
		ret = wait_event_interruptible(dvf9a_touch->waitqueue,
					       dvf9a_touch->cond);
		if (ret < 0)
			return -EINTR;

		dvf9a_touch_read_values(&values, dvf9a_touch);
		daif_write(dvf9a_touch->dvf9a, DAIF_ICU_ANA_STAT1, 1<<5);

		input_report_key(input, BTN_TOUCH, 1);

		while (1) {
			ret = schedule_timeout_interruptible(
						dvf9a_touch->timer_interval);
			if (ret < 0)
				return -EINTR;

			dvf9a_touch_report_events(input, dvf9a_touch);
			dvf9a_touch->first_press = 0;
			enable_irq(dvf9a_touch->irq_ready);

			dvf9a_touch->cond = 0;
			ret = wait_event_interruptible_timeout(
						dvf9a_touch->waitqueue,
						dvf9a_touch->cond,
						dvf9a_touch->untouch_timeout);

			if (!ret) { /* timeout */
				/* Did we miss an interrupt because the
				 * processing of the dvf9a_touch-workqueue is
				 * delayed by other interrupt handlers or
				 * tasklets?
				 */
				if (daif_read(dvf9a_touch->dvf9a,
					      DAIF_ICU_ANA_SRC1) & (1<<5)) {
					dvf9a_touch_report_events(input,
								  dvf9a_touch);
					continue;
				}
				input_report_key(input, BTN_TOUCH, 0);
				input_report_abs(input, ABS_PRESSURE, 0);
				input_sync(input);
				dvf9a_touch->first_press = 1;
				break;
			} else if (ret > 0) { /* DVF9A interrupt */
				dvf9a_touch_report_events(input, dvf9a_touch);
				continue;
			} else {
				return -EINTR;
			}
		}
	}
	return 0;
}

void
dvf9a_touch_bypass_init(struct dvf9a_touch *dvf9a_touch)
{
	/* Change clock from 1.xMHz to 13.825MHz */
	daif_write(dvf9a_touch->dvf9a, DAIF_CMUAUXCLKDIV, 0x0);

	/* Enable auxiliary A/D analog part and bandgap circuit */
	dvf9a_touch_update(dvf9a_touch, DAIF_AUXEN, 0x3, 0x3);

	/* Power Management Touch Panel Control 2 Register */
	/* Amplifier is always enabled */
	daif_write(dvf9a_touch->dvf9a, DAIF_AUX_PMTPCTL2, 0x1);

	/* Power Management Touch Panel Control 1 Register */
	/* TPS_INT_EN & TPY2_IN_EN */
	dvf9a_touch_update(dvf9a_touch, DAIF_AUX_PMTPCTL1, 0x1001, 0x1001);

	/* Start touch panel automatic machine */
	daif_write(dvf9a_touch->dvf9a, DAIF_AUX_PMTPCTL2,
		   0x040C /* pre-scale by 8; SEQ_Z1 and Z2 */ |
		   (1<<4) /* TP_AUTO_MODE */ |
		   (0x4<<7) /* TP_ST_TIME: 4096 */);

	/* Disable RSSI checking */
	dvf9a_touch_update(dvf9a_touch, DAIF_RAADC, 1<<5, 1<<5);

	/* set ADMUX to TP output */
	daif_write(dvf9a_touch->dvf9a, DAIF_AUXADCFG, 1<<2);
}

static int
dvf9a_touch_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct dvf9a_touch *dvf9a_touch;
	struct input_dev *input_dev;
	struct dvf9a *dvf9a_touch_dvf9a = dev_get_drvdata(pdev->dev.parent);
	int ret = 0;
	struct resource *res;

	dvf9a_touch = kzalloc(sizeof(struct dvf9a_touch), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!dvf9a_touch || !input_dev) {
		ret = -ENOMEM;
		goto err_free_mem;
	}

	dvf9a_touch->dvf9a = dvf9a_touch_dvf9a;
	dvf9a_touch->input = input_dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "no irq resource\n");
		goto err_free_mem;
	}
	dvf9a_touch->irq = res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (!res) {
		dev_err(&pdev->dev, "no irq resource\n");
		goto err_free_mem;
	}
	dvf9a_touch->irq_ready = res->start;

	dvf9a_touch->first_press = 1;

	dvf9a_touch->a0 = 1;
	dvf9a_touch->a4 = 1;
	dvf9a_touch->a6 = 1;

	ret = of_property_read_u32(np, "a0", (u32 *)&dvf9a_touch->a0);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'a0'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "a1", (u32 *)&dvf9a_touch->a1);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'a1'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "a2", (u32 *)&dvf9a_touch->a2);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'a2'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "a3", (u32 *)&dvf9a_touch->a3);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'a3'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "a4", (u32 *)&dvf9a_touch->a4);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'a4'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "a5", (u32 *)&dvf9a_touch->a5);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'a5'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "a6", (u32 *)&dvf9a_touch->a6);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'a6'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "x_min", &dvf9a_touch->x_min);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'x_min'\n");
		goto err_free_mem;
	}

	dvf9a_touch->x_min = 0;
	dvf9a_touch->x_max = MAX_8BIT;
	dvf9a_touch->y_min = 0;
	dvf9a_touch->y_max = MAX_8BIT;
	dvf9a_touch->pressure_min = 0;
	dvf9a_touch->pressure_max = 2000;

	ret = of_property_read_u32(np, "x_max", &dvf9a_touch->x_max);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'x_max'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "y_min", &dvf9a_touch->y_min);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'y_min'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "y_max", &dvf9a_touch->y_max);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'y_max'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "pressure_min",
				   &dvf9a_touch->pressure_min);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'pressure_min'\n");
		goto err_free_mem;
	}

	ret = of_property_read_u32(np, "pressure_max",
				   &dvf9a_touch->pressure_max);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'pressure_max'\n");
		goto err_free_mem;
	}

	dvf9a_touch->timer_interval = 1;
	ret = of_property_read_u32(np, "timer_interval",
				   &dvf9a_touch->timer_interval);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'timer_interval'\n");
		goto err_free_mem;
	}

	dvf9a_touch->untouch_timeout = 15;
	ret = of_property_read_u32(np, "untouch_timeout",
				   &dvf9a_touch->untouch_timeout);
	if (ret && ret != -EINVAL) {
		dev_err(&pdev->dev, "invalid 'untouch_timeout\n");
		goto err_free_mem;
	}

	init_waitqueue_head(&dvf9a_touch->waitqueue);

	snprintf(dvf9a_touch->phys, sizeof(dvf9a_touch->phys),
		 "dvf9a_touch-touchscreen/input1");

	input_dev->name = DRIVER_NAME;
	input_dev->phys = dvf9a_touch->phys;

	platform_set_drvdata(pdev, dvf9a_touch);

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, dvf9a_touch->x_min,
			     dvf9a_touch->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, dvf9a_touch->y_min,
			     dvf9a_touch->y_max, 0, 0);
	/* input_set_abs_params(input_dev, ABS_PRESSURE,
				dvf9a_touch->pressure_min,
				dvf9a_touch->pressure_max, 0, 0); */

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&pdev->dev, "cannot register input device\n");
		goto err_free_mem;
	}

	kernel_thread(dvf9a_touch_thread_code, dvf9a_touch,
		      CLONE_FS | CLONE_FILES | SIGCHLD);

	dvf9a_touch_bypass_init(dvf9a_touch);

	/* Register irqhandler */
	ret = request_threaded_irq(dvf9a_touch->irq, NULL, dvf9a_touch_irq,
				   IRQF_SHARED, "dvf9a_touch", dvf9a_touch);
	if (ret) {
		dev_err(&pdev->dev, "failed to request interrupt %d\n",
			dvf9a_touch->irq);
		goto err_free_device;
	}

	/* Register irqhandler */
	ret = request_threaded_irq(dvf9a_touch->irq_ready, NULL,
				   dvf9a_touch_irq, IRQF_SHARED,
	                           "dvf9a_touch", dvf9a_touch);
	if (ret) {
		dev_err(&pdev->dev, "failed to request interrupt %d\n",
			dvf9a_touch->irq_ready);
		goto err_free_irq;
	}

	dev_info(&pdev->dev, "registered touchscreen\n");

	return 0;

err_free_irq:
	free_irq(dvf9a_touch->irq, dvf9a_touch);
err_free_device:
	input_unregister_device(input_dev);
	input_dev = NULL;
err_free_mem:
	if (input_dev)
		input_free_device(input_dev);
	kfree(dvf9a_touch);

	return ret;
}

static int
dvf9a_touch_remove(struct platform_device *pdev)
{
	struct dvf9a_touch *dvf9a_touch = platform_get_drvdata(pdev);
	struct dvf9a *dvf9a = dvf9a_touch->dvf9a;

	free_irq(dvf9a_touch->irq, dvf9a_touch);
	free_irq(dvf9a_touch->irq_ready, dvf9a_touch);
	input_unregister_device(dvf9a_touch->input);
	kfree(dvf9a_touch);

	/* disable touchpad; it might interfere with sampling (auto-mode) */
	daif_write(dvf9a, DAIF_AUX_PMTPCTL1, 0);
	daif_write(dvf9a, DAIF_AUX_PMTPCTL2, 0);

	return 0;
}

static void
dvf9a_touch_shutdown(struct platform_device *pdev)
{
	struct dvf9a_touch *dvf9a_touch = platform_get_drvdata(pdev);
	struct dvf9a *dvf9a = dvf9a_touch->dvf9a;

	/* disable touchpad; it might interfere with sampling (auto-mode) */
	daif_write(dvf9a, DAIF_AUX_PMTPCTL1, 0);
	daif_write(dvf9a, DAIF_AUX_PMTPCTL2, 0);
}

static struct of_device_id dvf9a_touch_of_ids[] = {
	{ .compatible = "dspg,dvf9a-touch" },
	{ },
};

MODULE_DEVICE_TABLE(of, dvf9a_touch_of_ids);

static struct platform_driver dvf9a_touch_driver = {
	.probe		= dvf9a_touch_probe,
	.remove		= dvf9a_touch_remove,
	.shutdown	= dvf9a_touch_shutdown,
	.driver		= {
		.of_match_table = dvf9a_touch_of_ids,
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init
dvf9a_touch_init(void)
{
	return platform_driver_register(&dvf9a_touch_driver);
}

module_init(dvf9a_touch_init);

static void __exit
dvf9a_touch_exit(void)
{
	platform_driver_unregister(&dvf9a_touch_driver);
}

module_exit(dvf9a_touch_exit);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("DVF9A Touchscreen Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dvf9a-touch");
