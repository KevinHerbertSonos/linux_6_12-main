/*
 * DSPG DVF9A coma driver
 *
 * Copyright (c) 2016, DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/dvf9a.h>
#include <linux/of.h>
#include <linux/coma/coma.h>
#include <linux/coma/cfifo.h>
#include "cmsg-dvf9a.h"
#include "coma-dvf9a.h"

#define COMA_DVF9A "coma-dvf9a"

enum dvf9a_coma_irq_state {
	DVF9A_COMA_IRQ_FREE,
	DVF9A_COMA_IRQ_REGISTERED,
	DVF9A_COMA_IRQ_DISABLED,
};

struct dvf9a_coma {
	struct dvf9a *dvf9a;
	struct platform_device *pdev;
	struct {
		enum dvf9a_coma_irq_state state;
		int linux_irq;
	} irqs[DVF9A_NUM_IRQS];

	int service_id;
	int irq_request_id;
	struct work_struct request_irq_work;
	struct work_struct release_irq_work;
};
struct dvf9a_coma *dvf9a_coma;

static DECLARE_COMPLETION(dvf9a_coma_ack_acquire);

static int
dvf9a_coma_create_message(enum cmsg_dvf9a_types type,
			  union cmsg_dvf9a_params *params)
{
	return coma_cmsg_send(dvf9a_coma->service_id, (int)type,
			      (void *)params, sizeof(*params),
			      NULL, 0);
}

static irqreturn_t
dvf9a_coma_irq(int linux_irq, void *priv)
{
	int daif_irq = (int)priv;
	union cmsg_dvf9a_params params;

	/* keep interrupt disabled until other side acknowledges it */
	disable_irq(linux_irq);
	dvf9a_coma->irqs[daif_irq].state = DVF9A_COMA_IRQ_DISABLED;

	params.interrupt.irq = daif_irq;
	dvf9a_coma_create_message(CMSG_DVF9A_INTERRUPT, &params);

	return IRQ_HANDLED;
}

static void
handle_register_irq(struct work_struct *work)
{
	int daif_irq = dvf9a_coma->irq_request_id;
	int linux_irq, ret;
	union cmsg_dvf9a_params reply;

	if (daif_irq >= DVF9A_NUM_IRQS) {
		ret = -EINVAL;
		goto reply;
	}

	if (dvf9a_coma->irqs[daif_irq].state != DVF9A_COMA_IRQ_FREE) {
		ret = -EBUSY;
		goto reply;
	}

	linux_irq = daif_translate_irq(dvf9a_coma->dvf9a, daif_irq);
	if (linux_irq <= 0) {
		ret = -EINVAL;
		goto reply;
	}

	dvf9a_coma->irqs[daif_irq].linux_irq = linux_irq;
	dvf9a_coma->irqs[daif_irq].state = DVF9A_COMA_IRQ_REGISTERED;
	ret = devm_request_irq(&dvf9a_coma->pdev->dev, linux_irq,
			       dvf9a_coma_irq, 0, COMA_DVF9A, (void *)daif_irq);
	if (ret)
		dvf9a_coma->irqs[daif_irq].state = DVF9A_COMA_IRQ_FREE;

reply:
	reply.reply_register_irq.result = ret;
	dvf9a_coma_create_message(CMSG_DVF9A_REPLY_REGISTER_IRQ, &reply);
}

static void
handle_release_irq(struct work_struct *work)
{
	int irq = dvf9a_coma->irq_request_id;
	int ret = -EINVAL;
	union cmsg_dvf9a_params reply;

	if (irq >= DVF9A_NUM_IRQS)
		goto reply;

	if (dvf9a_coma->irqs[irq].state == DVF9A_COMA_IRQ_FREE)
		goto reply;

	devm_free_irq(&dvf9a_coma->pdev->dev, dvf9a_coma->irqs[irq].linux_irq,
		      (void *)irq);
	dvf9a_coma->irqs[irq].state = DVF9A_COMA_IRQ_FREE;
	ret = 0;

reply:
	reply.reply_release_irq.result = ret;
	dvf9a_coma_create_message(CMSG_DVF9A_REPLY_RELEASE_IRQ, &reply);
}

static void
dvf9a_coma_process_message(void *arg, struct cmsg *cmsg)
{
	int ret = 0;
	union cmsg_dvf9a_params *params =
				(union cmsg_dvf9a_params *)cmsg_params(cmsg);
	union cmsg_dvf9a_params reply;

	switch (cmsg->type) {
	case CMSG_DVF9A_REQUEST_REGISTER_IRQ:
		dvf9a_coma->irq_request_id = params->request_register_irq.irq;
		schedule_work(&dvf9a_coma->request_irq_work);
		break;
	case CMSG_DVF9A_REQUEST_RELEASE_IRQ:
		dvf9a_coma->irq_request_id = params->request_release_irq.irq;
		schedule_work(&dvf9a_coma->release_irq_work);
		break;
	case CMSG_DVF9A_ACK_INTERRUPT:
		if (dvf9a_coma->irqs[params->ack_interrupt.irq].state ==
		    DVF9A_COMA_IRQ_DISABLED) {
			dvf9a_coma->irqs[params->ack_interrupt.irq].state =
				DVF9A_COMA_IRQ_REGISTERED;
			enable_irq(dvf9a_coma->irqs[params->ack_interrupt.irq].linux_irq);
		}
		break;
	case CMSG_DVF9A_ACQUIRE:
		dvf9a_set_owner(dvf9a_coma->dvf9a, 0);
		ret = dvf9a_coma_create_message(CMSG_DVF9A_ACK_ACQUIRE, &reply);
		break;
	case CMSG_DVF9A_ACK_ACQUIRE:
		complete(&dvf9a_coma_ack_acquire);
		break;
	default:
		dev_err(&dvf9a_coma->pdev->dev, "invalid message type\n");
		return;
	}

	if (ret)
		dev_err(&dvf9a_coma->pdev->dev, "coma_cmsg_send failed\n");
}

static void
dvf9a_coma_remove_service(void *arg)
{
	int i;

	flush_work(&dvf9a_coma->request_irq_work);
	flush_work(&dvf9a_coma->release_irq_work);

	for (i = 0; i < DVF9A_NUM_IRQS; i++) {
		if (dvf9a_coma->irqs[i].state != DVF9A_COMA_IRQ_FREE)
			devm_free_irq(&dvf9a_coma->pdev->dev,
				      dvf9a_coma->irqs[i].linux_irq, (void *)i);
	}

	dvf9a_coma->service_id = -1;
}

int dvf9a_coma_service_init(void)
{
	int ret;

	if (!dvf9a_coma)
		return -ENODEV;

	ret = coma_register("dvf9a", 10240, NULL, dvf9a_coma_process_message,
			    dvf9a_coma_remove_service, NULL);
	if (ret < 0) {
		dev_err(&dvf9a_coma->pdev->dev, "coma_register failed\n");
		return ret;
	}
	dvf9a_coma->service_id = ret;

	return 0;
}
EXPORT_SYMBOL(dvf9a_coma_service_init);

void
dvf9a_coma_handle(struct dvf9a *dvf9a)
{
	if (!dvf9a_coma_create_message(CMSG_DVF9A_ACQUIRE, NULL))
		wait_for_completion(&dvf9a_coma_ack_acquire);
}

static int
dvf9a_coma_probe(struct platform_device *pdev)
{
	struct dvf9a *dvf9a = dev_get_drvdata(pdev->dev.parent);

	dvf9a_coma = devm_kzalloc(&pdev->dev, sizeof *dvf9a_coma, GFP_KERNEL);
	if (!dvf9a_coma) {
		dev_err(&pdev->dev, "Out of memory\n");
		return -ENOMEM;
	}
	dvf9a_coma->pdev = pdev;
	dvf9a_coma->dvf9a = dvf9a;
	dvf9a_coma->service_id = -1;
	dvf9a_set_owner(dvf9a, 1);
	dvf9a_handle_ownership(dvf9a, dvf9a_coma_handle);

	INIT_WORK(&dvf9a_coma->request_irq_work, handle_register_irq);
	INIT_WORK(&dvf9a_coma->release_irq_work, handle_release_irq);

	platform_set_drvdata(pdev, dvf9a_coma);

	dev_info(&pdev->dev, "successfully registered\n");

	return 0;
}

static int __exit
dvf9a_coma_remove(struct platform_device *dev)
{
	/* FIXME: racy wrt. dvf9a_coma_remove_service callback */
	if (dvf9a_coma->service_id >= 0) {
		coma_deregister(dvf9a_coma->service_id);
		dvf9a_coma_remove_service(NULL);
	}
	return 0;
}

static struct of_device_id dvf9a_coma_of_ids[] = {
	{ .compatible = "dspg,dvf9a-coma" },
	{ },
};

static struct platform_driver dvf9a_coma_platform_driver = {
	.driver = {
		.name	= COMA_DVF9A,
		.owner	= THIS_MODULE,
		.of_match_table = dvf9a_coma_of_ids,
	},
	.probe		= dvf9a_coma_probe,
	.remove		= __exit_p(dvf9a_coma_remove),
};

static int __init dvf9a_coma_init(void)
{
	return platform_driver_register(&dvf9a_coma_platform_driver);
}

static void __exit dvf9a_coma_exit(void)
{
	platform_driver_unregister(&dvf9a_coma_platform_driver);
}

module_init(dvf9a_coma_init);
module_exit(dvf9a_coma_exit);

MODULE_DESCRIPTION("DSPG DVF9A-COMA driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_ALIAS("platform:dvf9a-coma");
