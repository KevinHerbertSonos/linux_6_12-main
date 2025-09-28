/*
 * I2C slave mode DVF99 driver
 *
 * Copyright (C) 2015 DSPG Technologies India pvt Ltd
 * 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * Because most IP blocks can only detect one I2C slave address anyhow, this
 * driver does not support simulating EEPROM types which take more than one
 * address. It is prepared to simulate bigger EEPROMs with an internal 16 bit
 * pointer, yet implementation is deferred until the need actually arises.
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/completion.h>
#include <linux/i2c-pnx.h>
#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#define I2C_PNX_FIFO_DEPTH      64
#define I2C_SLAVE_DEVICE "i2cslave"
#define I2C_SLAVE_CLASS "i2c-slave"

struct slave_data{
	struct class *class;
	struct device *slave_dev;
	dev_t devt;
	wait_queue_head_t waitq;
	struct kfifo fifo;
	spinlock_t lock;
};

static struct slave_data *g_data;

/* callback registered to adapter's i2c_slave_interrupt routine */
 
static int i2c_slave_dvf99_slave_cb(struct i2c_client *client,
				     enum i2c_slave_event event, u8 *val)
{

	switch (event) {
	case I2C_SLAVE_WRITE_RECEIVED:
			/* Avoides confrontation with the kfifo_reset */
			spin_lock(&g_data->lock);
			kfifo_put(&g_data->fifo,val);
			spin_unlock(&g_data->lock);
		break;
	case I2C_SLAVE_READ_PROCESSED:
	case I2C_SLAVE_READ_REQUESTED:
		break;
	case I2C_SLAVE_STOP:
			/* wakes up read fops if waiting for FIFO data*/
			wake_up_interruptible(&g_data->waitq);
		break;
	case I2C_SLAVE_WRITE_REQUESTED:
	default:
		break;
	}

	return 0;
}

static int i2c_slave_dvf99_open(struct inode *node,struct file *fl)
{
	unsigned long flags;
	
	spin_lock_irqsave(&g_data->lock,flags);

	/* Flushes the FIFO stale data*/
	kfifo_reset(&g_data->fifo);

	/* Initialzes the wait queue */
	init_waitqueue_head(&g_data->waitq);

	spin_unlock_irqrestore(&g_data->lock,flags);

        return 0;
}
static int i2c_slave_dvf99_close(struct inode *node,struct file *fl)
{

        return 0;
}
static ssize_t i2c_slave_dvf99_read(struct file *fl,char __user *buf,size_t len, loff_t *off)
{
	
	u32 copied;

	if(kfifo_len(&g_data->fifo))  /* If FIFO data is already present, copies to user buffer */
	{
		
		if(kfifo_to_user(&g_data->fifo,buf,len,&copied)== -EFAULT)
        	        dev_err(g_data->slave_dev,"copy failed, fifo length is %d\n",
						kfifo_len(&g_data->fifo));
	}
	else{    /* If the FIFO is empty waites for the data  and then copies to user buffer */

        	wait_event_interruptible(g_data->waitq,(kfifo_len(&g_data->fifo)));
        	if(kfifo_to_user(&g_data->fifo,buf,len,&copied)== -EFAULT)
			dev_err(g_data->slave_dev,"Failed to copy,fifo length is %d\n",
						kfifo_len(&g_data->fifo));
	}

        return copied;
}


static struct file_operations i2c_slave_file_ops ={
        .owner  = THIS_MODULE,
        .open   = i2c_slave_dvf99_open,
        .read   = i2c_slave_dvf99_read,
        .release = i2c_slave_dvf99_close,

};

static int i2c_slave_dvf99_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret=0;

	g_data = kzalloc(sizeof(struct slave_data),GFP_KERNEL);
	if(!g_data){
		dev_err(&client->dev,"Memory allocation failed \n");
		return -ENOMEM;
	}

	spin_lock_init(&g_data->lock);

        init_waitqueue_head(&g_data->waitq);

	i2c_set_clientdata(client, g_data);

	ret = kfifo_alloc(&g_data->fifo,I2C_PNX_FIFO_DEPTH,GFP_KERNEL);
	if(ret){
		dev_err(&client->dev,"kfifo allocation failed \n");
		goto free_mem;
	}
	
	/*Initializes adapter with slave settings  and registers slave callback*/
	ret = i2c_slave_register(client, i2c_slave_dvf99_slave_cb);
	if (ret) {
		dev_err(&client->dev,"i2c_slave_register failed \n");
		goto free_fifo;
	}
	ret = register_chrdev(0, I2C_SLAVE_DEVICE, &i2c_slave_file_ops);
        if (ret < 0) {
                dev_err(&client->dev,"register_chrdev %s failed\n", I2C_SLAVE_DEVICE);
		goto slave_unreg;
        }

        g_data->class = class_create(THIS_MODULE, I2C_SLAVE_CLASS);
        if (IS_ERR(g_data->class)) {
                dev_err(&client->dev,"failed to create i2c-slave class\n");
		goto unreg_chrdev;
        }
        g_data->devt = MKDEV(ret, 0);
         g_data->slave_dev = device_create(g_data->class,NULL,g_data->devt, NULL,I2C_SLAVE_DEVICE);
        if (IS_ERR(g_data->slave_dev)) {
                dev_err(&client->dev, "failed to create i2cslave device\n");
                goto out_class_destroy;
        }

	
	return 0;

out_class_destroy:
	class_destroy(g_data->class);
unreg_chrdev:
	unregister_chrdev(MAJOR(g_data->devt),I2C_SLAVE_DEVICE);
slave_unreg:
	i2c_slave_unregister(client);
free_fifo:
	kfifo_free(&g_data->fifo);
free_mem:
	kfree(g_data);
	return ret;
};

static int i2c_slave_dvf99_remove(struct i2c_client *client)
{
	device_unregister(g_data->slave_dev);
	class_destroy(g_data->class);
	unregister_chrdev(MAJOR(g_data->devt),I2C_SLAVE_DEVICE);
	i2c_slave_unregister(client);
	kfifo_free(&g_data->fifo);
	kfree(g_data);

	return 0;
}

static const struct i2c_device_id i2c_slave_dvf99_id[] = {
	{ "i2c-slave-dvf99" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, i2c_slave_dvf99_id);

static struct i2c_driver i2c_slave_dvf99_driver = {
	.driver = {
		.name = "i2c-slave-dvf99",
		.owner = THIS_MODULE,
	},
	.probe = i2c_slave_dvf99_probe,
	.remove = i2c_slave_dvf99_remove,
	.id_table = i2c_slave_dvf99_id,
};
module_i2c_driver(i2c_slave_dvf99_driver);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("I2C slave mode DVF99 driver");
MODULE_LICENSE("GPL");
