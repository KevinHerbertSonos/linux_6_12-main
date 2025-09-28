/*
 * linux/drivers/staging/dspg/dspg-crypto.c - DSPG cryptographic security module
 *
 *  Copyright (C) 2013, 2017 DSP Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/ccu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/dspg-crypto.h>
#include <linux/hw_random.h>
#include <linux/reset.h>
#include <linux/kthread.h>
#include <crypto/aes.h>
#include <crypto/des.h>
#include <crypto/internal/skcipher.h>
#include <crypto/algapi.h>
#include <crypto/internal/hash.h>
#include <mach/platform.h>

#define DSPG_CRYPTO "dspg-crypto"

#define DSPG_AES_PRIORITY	300
#define DSPG_DES_PRIORITY	300
#define DSPG_RC4_PRIORITY	300

#define DSPG_CRYPTO_BUFFER_ORDER	4
#define DSPG_CRYPTO_BUFFER_SIZE	(PAGE_SIZE << DSPG_CRYPTO_BUFFER_ORDER)

/* Register map */
enum REG_CFG {
	REG_CFG_CFG				= 0x0000,
	REG_CFG_QOS				= 0x0004,
};

/* True Random Number Generator (TRNG) registers */
enum REG_TRNG {
	REG_TRNG_CFG				= 0x0008,
	REG_TRNG_KICK				= 0x000C,
	REG_TRNG_STATUS				= 0x0010,
	REG_TRNG_CLR_STATUS			= 0x0014,
};

/* Interrupt status and control registers */
enum REG_IRQ {
	REG_IRQ_DESC_OWNER_CHANGED		= 0x0100,
	REG_IRQ_DESC_OWNER_CHANGED_CAUSE	= 0x0104,
	REG_IRQ_DESC_OWNER_CHANGED_CLR		= 0x0108,
	REG_IRQ_HEAD_PTR_WENT_NULL		= 0x010C,
	REG_IRQ_HEAD_PTR_WENT_NULL_ENABLE	= 0x0110,
	REG_IRQ_HEAD_PTR_WENT_NULL_CAUSE	= 0x0114,
	REG_IRQ_HEAD_PTR_WENT_NULL_CLR		= 0x0118,

	REG_IRQ_CRYPTO_AHB_EXCEPTION		= 0x011C,
	REG_IRQ_CRYPTO_HW_EXCEPTION		= 0x0120,
	REG_IRQ_CRYPTO_DESC_EXCEPTION		= 0x0124,
	REG_IRQ_CRYPTO_EXCEPTION_ENABLE		= 0x0128,
	REG_IRQ_CRYPTO_EXCEPTION_CAUSE		= 0x0140,
	REG_IRQ_CRYPTO_EXCEPTION_CLR		= 0x0144,
};

enum irq_exception {
	IRQ_EXCEPTION_AHB,
	IRQ_EXCEPTION_HW,
	IRQ_EXCEPTION_DESC,
};

/*
 * Channel Headpointer registers
 * All headpointers point to the first descriptor in the corresponding
 * linked list and are initialized by software.  There are:
 * - 24 high priority (VoIP) channels
 * - 4 medium priority (streaming data) channels.
 * - 4 medium priority (data) channels.
 */
enum  REG_HEADPTR {
	REG_CHANNEL_HEADPTR			= 0x0200,
	REG_HEADPTR_ENABLED			= 0x0280,
	REG_HEADPTR_SET				= 0x0284,
	REG_HEADPTR_RESET			= 0x0288,
};

#define NUM_CHANNELS			32
#define MIN_CHANNEL			 0
#define MAX_CHANNEL			(NUM_CHANNELS - 1)

/* Descriptor control fields */
#define DESC_CTL_DESC_OWNER_HW		0x80000000 /* 0 = SW; 1 = HW */
#define DESC_CTL_INT_REQ		0x40000000
#define DESC_CTL_LOCK_NEXT		0x20000000
#define DESC_CTL_SAVE_STATE		0x18000000
#define DESC_CTL_GCM_FAIL		0x04000000

/* Channel HW descriptor */
struct hw_crypto_desc {
	u32 desc_ctl;		/* Descriptor control */
	u32 link_ptr;		/* 32-bit pointer to next descriptor */
	u32 data_in_phys;	/* 32-bit pointer to input data */
	u32 data_out_phys;	/* 32-bit pointer to output data */
	u32 data_in_len;	/* 31-bit length of input data */
	u32 data_out_len;	/* 31-bit length of output data */
	u32 iv[8];		/* Initialization vector */
	u32 state_index;
	u32 data_in_total_len;	/* Full packet data length if last descriptor
				 * of multiple-descriptor packet of HASH/HMAC
				 */
};

/* TRNG operation */
#define TRNG_STATUS_BUSY		0x00000001
#define TRNG_STATUS_READY		0x00000002
#define TRNG_MAGIC_SEQ			0xaa55bb11
#define TRNG_STATUS_READY_CLR		0x00000001

#define TRNG_CONFIG_HW_TUNE		0x00000000

/* Keystore mapping */
#define KS_KEY_128_START		0x1000 /* indices 0 - 31  */
#define KS_KEY_256_START		0x1200 /* indices 32 - 79 */
#define TRNG_CONFIG_KEY_INDEX		0x0000004F /* last 256 bit keystore */
#define SECRET_KEY_INDEX		0x00000050

struct dspg_crypto_dev {
	struct platform_device *pdev;
	void __iomem *regs;
	struct clk *clk;
	struct reset_control *reset;

	struct dma_pool *desc_dma_pool;
	spinlock_t lock; /* Protect write access to descriptor hardware */
	u32 irq;

	struct completion completion;

	int chip_version;

	struct crypto_op *pending_ops[NUM_CHANNELS];
	int key_store[NUM_CHANNELS];
	unsigned long channels[BITS_TO_LONGS(NUM_CHANNELS)];

	struct hwrng rng;
	struct task_struct *kthread;
	struct crypto_queue queue;

	void *in_buf;
	void *out_buf;
	size_t buf_len;
};

static struct dspg_crypto_dev *_dspg_crypto_dev;

struct dspg_crypto_ctx {
	struct dspg_crypto_dev	*dev;
	int			keylen;
	u8			key[AES_KEYSIZE_256];
	u32			block_size;
	int			cmd;
};

static unsigned long
crypto_readl(struct dspg_crypto_dev *dev, unsigned long reg)
{
	return readl(dev->regs + reg);
}

static void
crypto_writel(struct dspg_crypto_dev *dev, unsigned long reg, unsigned long val)
{
	writel(val, dev->regs + reg);
}

static void
dspg_crypto_int_enable(struct dspg_crypto_dev *dev, unsigned long ch,
		       int enable)
{
	u32 temp = crypto_readl(dev, REG_IRQ_HEAD_PTR_WENT_NULL_ENABLE);

	if (enable)
		crypto_writel(dev, REG_IRQ_HEAD_PTR_WENT_NULL_ENABLE,
			      temp |   1 << ch);
	else
		crypto_writel(dev, REG_IRQ_HEAD_PTR_WENT_NULL_ENABLE,
			      temp & ~(1 << ch));

	temp = crypto_readl(dev, REG_IRQ_CRYPTO_EXCEPTION_ENABLE);
	if (enable)
		crypto_writel(dev, REG_IRQ_CRYPTO_EXCEPTION_ENABLE,
			      temp |   1 << ch);
	else
		crypto_writel(dev, REG_IRQ_CRYPTO_EXCEPTION_ENABLE,
			      temp & ~(1 << ch));
}

static void
dspg_crypto_free_key(struct dspg_crypto_dev *dev, int channel, u8 key_idx)
{
	dev->key_store[channel] = 0;
}

static void
dspg_crypto_op_done(u32 ch, struct dspg_crypto_dev *dev)
{
	struct crypto_op *op = dev->pending_ops[ch];

	dma_unmap_single(&dev->pdev->dev, op->hw_desc->data_in_phys,
			 op->hw_desc->data_in_len, DMA_TO_DEVICE);
	dma_unmap_single(&dev->pdev->dev, op->hw_desc->data_out_phys,
			 op->hw_desc->data_out_len, DMA_FROM_DEVICE);
	dma_pool_free(dev->desc_dma_pool, op->hw_desc, op->hw_desc_phys);

	dspg_crypto_int_enable(dev, ch, 0);

	dspg_crypto_free_key(dev, ch, 0);

	op->done(dev, 0, op->context);

	kfree(op);
}

static irqreturn_t
dspg_crypto_isr(int irq, void *dev_id)
{
	struct dspg_crypto_dev *dev = (struct dspg_crypto_dev *)dev_id;
	u32 channel, completed_desc;
	u32 irq_head_ptr_went_null, irq_desc_owner_changed;
	u32 irq_exception_cause;
	u32 irq_ahb_exception;
	u32 irq_hw_exception;
	u32 irq_desc_exception;

	irq_ahb_exception = crypto_readl(dev, REG_IRQ_CRYPTO_AHB_EXCEPTION);
	irq_hw_exception = crypto_readl(dev, REG_IRQ_CRYPTO_HW_EXCEPTION);
	irq_desc_exception = crypto_readl(dev, REG_IRQ_CRYPTO_DESC_EXCEPTION);

	/* clear interrupts */
	irq_exception_cause = crypto_readl(dev,
					   REG_IRQ_CRYPTO_EXCEPTION_CAUSE);
	crypto_writel(dev, REG_IRQ_CRYPTO_EXCEPTION_CLR, irq_exception_cause);

	irq_desc_owner_changed = crypto_readl(dev,
					      REG_IRQ_DESC_OWNER_CHANGED);
	crypto_writel(dev, REG_IRQ_DESC_OWNER_CHANGED_CLR,
		      irq_desc_owner_changed);

	irq_head_ptr_went_null = crypto_readl(dev, REG_IRQ_HEAD_PTR_WENT_NULL);
	crypto_writel(dev, REG_IRQ_HEAD_PTR_WENT_NULL_CLR,
		      irq_head_ptr_went_null);

	if (irq_exception_cause)
		dev_err(&dev->pdev->dev,
			"irq exception 0x%x (ahb 0x%x, hw 0x%x, desc 0x%x)\n",
			irq_exception_cause, irq_ahb_exception,
			irq_hw_exception, irq_desc_exception);
		/* TODO: invoke dspg_crypto_op_done & report failure */

	/*
	 * ATTENTION: The following call will guarantee that all memory
	 * transactions indicated by the device irq flags have actually hit the
	 * memory. Just process the flags which have been sampled. When reading
	 * the status again any new IRQ flags MUST NOT be processed until
	 * ccu_barrier() is called again!
	 */
	ccu_barrier(0);

	completed_desc = irq_exception_cause | irq_head_ptr_went_null;
	for (channel = 0; completed_desc != 0; channel++, completed_desc >>= 1)
		if (completed_desc & 1)
			dspg_crypto_op_done(channel, dev);

	return IRQ_HANDLED;
}

void
dspg_crypto_generate_trng(struct dspg_crypto_dev *dev)
{
	clk_enable(dev->clk);

	while ((crypto_readl(dev, REG_TRNG_STATUS) & TRNG_STATUS_BUSY) ==
	       TRNG_STATUS_BUSY)
		schedule();

	crypto_writel(dev, REG_TRNG_CFG,
		      TRNG_CONFIG_HW_TUNE | TRNG_CONFIG_KEY_INDEX);

	crypto_writel(dev, REG_TRNG_KICK, TRNG_MAGIC_SEQ);
	while ((crypto_readl(dev, REG_TRNG_STATUS) & TRNG_STATUS_READY) !=
	       TRNG_STATUS_READY)
		schedule();

	crypto_writel(dev, REG_TRNG_CLR_STATUS, TRNG_STATUS_READY_CLR);

	clk_disable(dev->clk);
}

int
dspg_crypto_get_trng(struct dspg_crypto_dev *dev, unsigned int *buf, size_t max)
{
	int i, num = NUM_OF_TRNG;

	if (max < num)
		num = max;

	clk_enable(dev->clk);

	for (i = 0; i < num; i++)
		buf[i] = crypto_readl(dev, KS_KEY_256_START +
				      0x20 * (TRNG_CONFIG_KEY_INDEX - 32) +
				      (i * sizeof(u32)));

	clk_disable(dev->clk);

	return num;
}

/* TODO: handle allocation of SECRET_KEY */
static int
dspg_crypto_alloc_key(struct dspg_crypto_dev *dev, int channel, u8 key_size,
		      u8 *key_idx, u8 *key)
{
	int i;
	u32 word, keybase;

	if (*key_idx == 80)
		return 0;

	if (key_size <= 16) {
		*key_idx = channel;
		keybase = KS_KEY_128_START;
	} else if (key_size <= 32) {
		*key_idx = channel + 32;
		keybase = KS_KEY_256_START;
	} else {
		return -1;
	}

	if (!dev->key_store[channel]) {
		/* load the key to the key-store */
		for (i = 0; i < key_size; i += 4) {
			word = *((u32 *)(key + i));
			crypto_writel(dev, keybase + i, word);
		}
		dev->key_store[channel] = 1;
	}

	return 0;
}

static struct hw_crypto_desc *
dspg_crypto_alloc_hw_desc(struct dspg_crypto_dev *dev, struct crypto_op *op,
			  u8 mode, int channel, u8 key_size, u8 *key_idx,
			  u8 *key)
{
	struct hw_crypto_desc *hw_desc;

	if (dspg_crypto_alloc_key(dev, channel, key_size, key_idx, key))
		return NULL;

	hw_desc = dma_pool_alloc(dev->desc_dma_pool, GFP_KERNEL,
				 &op->hw_desc_phys);
	if (!hw_desc) {
		dspg_crypto_free_key(dev, channel, *key_idx);
		return NULL;
	}

	memset(hw_desc, 0, sizeof(*hw_desc));
	op->hw_desc = hw_desc;
	key_size = (key_size == 0) ? 0 : key_size - 1;
	hw_desc->desc_ctl = DESC_CTL_DESC_OWNER_HW	|
			    DESC_CTL_INT_REQ		|
			    (*key_idx << 16)		|
			    (key_size << 8)		|
			mode;

	return hw_desc;
}

static int
dspg_crypto_verify_buffers(struct dspg_crypto_dev *dev, void *in,
			   size_t in_len, void *out, size_t out_len)
{
	/*
	 * Output DMA is performed using 32-bit access, therefore 'out' must
	 * be 32-bit aligned start address. If 'out_len' is not integer
	 * multiple of 32 bits, then last write will overwrite up to 3 bytes
	 * beyond end of data to next 32-bit aligned address.
	 */
	/* if (((u32)out%4) || (out_len%4)) goto err_align; */

	/*
	 * Byte-aligned input are allowed.  However, if encrypt/decrypt
	 * in-place is desired, 'in' has the same size & alignment
	 * restrictions as 'out'
	 */
	if ((in == out) && (((u32)in % 4) || (in_len % 4)))
		goto err_align;

	/*
	 * If selected mode is ECB or CBC block cipher (mode is set to
	 * DES/3DES ECB or CBC, AES ECB or CBC modes), then software is
	 * responsible for performing the Cipher padding to ensure input data
	 * length is multiple of selected Cipher block size. A Crypto
	 * Exception interrupt will be generated if software fails to perform
	 * this task.  Software must determine what cipher padding scheme is
	 * required and use this to pad input.
	 */
	 /* TODO: implement this logic */
	return 0;

err_align:
	dev_err(&dev->pdev->dev, "Buffer alignment error\n");
	return -EINVAL;
}

/* @todo: implement allocation logic */
static int
dspg_crypto_allocate_channel(int channel)
{
	/* TODO: handle case of channel==-1 (driver chooses channel) */
	if ((channel < MIN_CHANNEL) || (channel > MAX_CHANNEL))
		return -EINVAL;

	/*
	 * TODO: manage real allocation of channel resource
	 * return EADDRINUSE;
	 */
	return 0;
}

/* @todo: implement free logic */
static int
dspg_crypto_free_channel(int channel)
{
	return 0;
}

/* @todo - verify key len is legal */
static int
dspg_crypto_verify_mode(struct dspg_crypto_dev *dev, u8 mode, size_t out_len)
{
	if (dev->chip_version <= 1) {
		if ((mode == DESC_CTL_MODE_AES_GCM_ENCRYPT) ||
		    (mode == DESC_CTL_MODE_AES_GCM_DECRYPT))
			return -1;
	}

	switch (mode) {
	case DESC_CTL_MODE_MD5:
	case DESC_CTL_MODE_MD5_HMAC:
		if (out_len != 16)
			return -1;
		break;

	case DESC_CTL_MODE_SHA1_160:
	case DESC_CTL_MODE_SHA1_160_HMAC:
		if (out_len != 20)
			return -1;
		break;

	case DESC_CTL_MODE_SHA2_224:
	case DESC_CTL_MODE_SHA2_224_HMAC:
		if (out_len != 28)
			return -1;
		break;

	case DESC_CTL_MODE_SHA2_256:
	case DESC_CTL_MODE_SHA2_256_HMAC:
		if (out_len != 32)
			return -1;
		break;

	case DESC_CTL_MODE_AES_CTR_ENCRYPT:
	case DESC_CTL_MODE_AES_CBC_ENCRYPT:
		/*
		 *if ((out_len!=16) && (out_len!=32) && (out_len!=64))
		 *	return -1;
		 */
		break;

	default:
		return 0;
	}

	return 0;
}

struct crypto_op *
dspg_crypto_alloc_operation(struct dspg_crypto_dev *dev,
			    struct scatterlist *in, size_t in_len,
			    struct scatterlist *out, size_t out_len,
			    u8 mode, int channel, u8 *key, size_t key_len,
			    void *iv, int secret)
{
	struct hw_crypto_desc *hw_desc;
	struct crypto_op *op;
	u8 key_idx = 0;

	if (secret)
		key_idx = 80;

	if (dspg_crypto_verify_mode(dev, mode, out_len))
		return NULL;

	if (dspg_crypto_verify_buffers(dev, in, in_len, out, out_len))
		return NULL;

	if (dspg_crypto_allocate_channel(channel))
		return NULL;

	op = kzalloc(sizeof(*op), GFP_KERNEL);
	if (!op)
		goto err_op_alloc;

	op->channel = channel;

	sg_copy_to_buffer(in, sg_nents(in), dev->in_buf, in_len);

	op->data_in_virt = dev->in_buf;
	op->data_out_virt = dev->out_buf;

	hw_desc = dspg_crypto_alloc_hw_desc(dev, op, mode, channel, key_len,
					    &key_idx, key);
	if (!hw_desc)
		goto err_desc_alloc;

	dev->pending_ops[channel] = op;

	hw_desc->data_in_len = in_len;
	hw_desc->data_in_phys = dma_map_single(&dev->pdev->dev,
					       op->data_in_virt,
					       hw_desc->data_in_len,
					       DMA_TO_DEVICE);

	hw_desc->data_out_len = out_len;
	hw_desc->data_out_phys = dma_map_single(&dev->pdev->dev,
						op->data_out_virt,
						hw_desc->data_out_len,
						DMA_FROM_DEVICE);

	if (iv)
		memcpy(hw_desc->iv, iv, 16);

	return op;

err_desc_alloc:
	dev_err(&dev->pdev->dev, "Failed to allocate HW descriptor memory\n");
	kfree(op);

err_op_alloc:
	dev_err(&dev->pdev->dev, "Failed to allocate operator memory\n");
	dspg_crypto_free_channel(channel);

	return NULL;
}

void
dspg_crypto_free_operation(struct crypto_op *op)
{
	kfree(op);
}

int
dspg_crypto_submit_operation(struct dspg_crypto_dev *dev, struct crypto_op *op)
{
	u32 temp = 0;
	u32 ch = op->channel;

	/* assign the desc to the channel's headpointer */
	crypto_writel(dev, REG_CHANNEL_HEADPTR + (ch * 4), op->hw_desc_phys);

	dspg_crypto_int_enable(dev, ch, 1);

	ccu_barrier(0);

	/* enable the channel */
	temp = crypto_readl(dev, REG_HEADPTR_SET);
	crypto_writel(dev, REG_HEADPTR_SET, temp | 1 << ch);

	return 0;
}

static int dspg_aes_setkey(struct crypto_skcipher *tfm, const u8 *key,
			   unsigned int keylen)
{
	struct dspg_crypto_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct dspg_crypto_dev *dev = ctx->dev;

	dev_dbg(&dev->pdev->dev, "dspg_aes_setkey\n");

	if (keylen != AES_KEYSIZE_128 &&
	    keylen != AES_KEYSIZE_192 &&
	    keylen != AES_KEYSIZE_256) {
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int dspg_des_setkey(struct crypto_skcipher *tfm, const u8 *key,
			   unsigned int keylen)
{
	struct dspg_crypto_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct dspg_crypto_dev *dev = ctx->dev;

	dev_dbg(&dev->pdev->dev, "dspg_des_setkey\n");

	if (keylen != DES_KEY_SIZE) {
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int dspg_des3_setkey(struct crypto_skcipher *tfm, const u8 *key,
			    unsigned int keylen)
{
	struct dspg_crypto_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct dspg_crypto_dev *dev = ctx->dev;

	dev_dbg(&dev->pdev->dev, "dspg_des3_setkey\n");

	if (keylen != DES_KEY_SIZE &&
	    keylen != (DES_KEY_SIZE * 2) &&
	    keylen != DES3_EDE_KEY_SIZE) {
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static int dspg_arc4_setkey(struct crypto_skcipher *tfm, const u8 *key,
			    unsigned int keylen)
{
	struct dspg_crypto_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct dspg_crypto_dev *dev = ctx->dev;

	dev_dbg(&dev->pdev->dev, "dspg_arc4_setkey\n");

	if ((keylen < 5) || (keylen > 32)) {
		return -EINVAL;
	}

	memcpy(ctx->key, key, keylen);
	ctx->keylen = keylen;

	return 0;
}

static void
dspg_crypto_done(struct dspg_crypto_dev *dev, int status, void *context)
{
	/*
	 * unsigned long flags;

	 * spin_lock_irqsave(&dev->lock, flags);
	 * clear_bit_unlock(op->channel, dev->channels);
	 * spin_unlock_irqrestore(&dev->lock, flags);
	 */

	complete(&dev->completion);
}

static int dspg_crypto_process(struct skcipher_request *req)
{
	static struct crypto_op *dspg_crypto_op;
	struct dspg_crypto_ctx *ctx;
	struct dspg_crypto_dev *dev;
	int ret;
	unsigned long flags;
	int secret = 0;

	ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	dev = ctx->dev;

	if (req->base.flags & CRYPTO_TFM_RES_SECRET_KEY)
		secret = 1;

	dev_dbg(&dev->pdev->dev, "dspg_%s, cmd 0x%x (bytes %d)%s\n",
		ctx->cmd & 1 ? "decrypt" : "encrypt", ctx->cmd, req->cryptlen,
		secret ? ", using secret key" : "");

	clk_enable(dev->clk);

	dspg_crypto_op = dspg_crypto_alloc_operation(dev,
						     req->src, req->cryptlen,
						     req->dst, req->cryptlen,
						     ctx->cmd, 0, /* channel */
						     ctx->key, ctx->keylen,
						     req->iv, secret);

	reinit_completion(&dev->completion);

	if (dspg_crypto_op) {
		dspg_crypto_op->done = dspg_crypto_done;
		dspg_crypto_op->context = dspg_crypto_op;
		spin_lock_irqsave(&dev->lock, flags);
		dspg_crypto_submit_operation(dev, dspg_crypto_op);
		spin_unlock_irqrestore(&dev->lock, flags);
	}

	ret = wait_for_completion_timeout(&dev->completion, HZ);
	if (ret <= 0)
		dev_dbg(&dev->pdev->dev, "completed %d\n", ret);

	clk_disable(dev->clk);

	if (ret == 0)
		return -ETIMEDOUT;
	if (ret > 0)
		ret = 0;

	if (!sg_copy_from_buffer(req->dst, sg_nents(req->dst), dev->out_buf,
				 req->cryptlen))
		return -EINVAL;

	return ret;
}

static int dspg_crypto_queue_manage(void *data)
{
	struct dspg_crypto_dev *dev = (struct dspg_crypto_dev *)data;
	struct crypto_async_request *async_req;
	struct crypto_async_request *backlog;
	unsigned long flags;
	int ret = 0;

	do {
		__set_current_state(TASK_INTERRUPTIBLE);

		spin_lock_irqsave(&dev->lock, flags);
		backlog = crypto_get_backlog(&dev->queue);
		async_req = crypto_dequeue_request(&dev->queue);
		spin_unlock_irqrestore(&dev->lock, flags);

		if (backlog)
			backlog->complete(backlog, -EINPROGRESS);

		if (async_req) {
			struct skcipher_request *req =
				skcipher_request_cast(async_req);

			ret = dspg_crypto_process(req);

			async_req->complete(async_req, ret);

			continue;
		}

		schedule();
	} while (!kthread_should_stop());

	return 0;
}

static int dspg_crypt(struct skcipher_request *req, int cmd)
{
	struct dspg_crypto_ctx *ctx;
	struct dspg_crypto_dev *dev;
	unsigned long flags;
	int ret;

	ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	dev = ctx->dev;

	spin_lock_irqsave(&dev->lock, flags);
	ctx->cmd = cmd;
	ret = crypto_enqueue_request(&dev->queue, &req->base);
	spin_unlock_irqrestore(&dev->lock, flags);

	wake_up_process(dev->kthread);

	return ret;
}

static int dspg_aes_ctr_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_CTR_ENCRYPT);
}

static int dspg_aes_ctr_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_CTR_DECRYPT);
}

static int dspg_aes_cbc_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_CBC_ENCRYPT);
}

static int dspg_aes_cbc_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_CBC_DECRYPT);
}

static int dspg_aes_ecb_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_ECB_ENCRYPT);
}

static int dspg_aes_ecb_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_ECB_DECRYPT);
}

static int dspg_aes_ofb_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_F8_ENCRYPT);
}

static int dspg_aes_ofb_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_AES_F8_DECRYPT);
}

static int dspg_des_cbc_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_DES_CBC_ENCRYPT);
}

static int dspg_des_cbc_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_DES_CBC_DECRYPT);
}

static int dspg_des_ecb_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_DES_ECB_ENCRYPT);
}

static int dspg_des_ecb_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_DES_ECB_DECRYPT);
}

static int dspg_des3_cbc_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_3DES_CBC_ENCRYPT);
}

static int dspg_des3_cbc_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_3DES_CBC_DECRYPT);
}

static int dspg_des3_ecb_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_3DES_ECB_ENCRYPT);
}

static int dspg_des3_ecb_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_3DES_ECB_DECRYPT);
}

static int dspg_arc4_encrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_RC4_ENCRYPT);
}

static int dspg_arc4_decrypt(struct skcipher_request *req)
{
	return dspg_crypt(req, DESC_CTL_MODE_RC4_DECRYPT);
}

static int dspg_cra_init(struct crypto_skcipher *tfm)
{
	struct dspg_crypto_dev *dev = _dspg_crypto_dev;
	struct dspg_crypto_ctx *ctx = crypto_skcipher_ctx(tfm);

	ctx->dev = dev;

	dev_dbg(&ctx->dev->pdev->dev, "dspg_cra_init\n");

	crypto_skcipher_set_reqsize(tfm, sizeof(struct dspg_crypto_ctx));

	return 0;
}

static void dspg_cra_exit(struct crypto_skcipher *tfm)
{
	struct dspg_crypto_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct dspg_crypto_dev *dev = ctx->dev;

	dev_dbg(&dev->pdev->dev, "dspg_cra_exit\n");
}

static struct skcipher_alg dspg_algs[] = {
{
	.base.cra_name		= "ctr(aes)",
	.base.cra_driver_name	= "dspg-ctr-aes",
	.base.cra_priority		= DSPG_AES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= AES_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= AES_MIN_KEY_SIZE,
	.max_keysize	= AES_MAX_KEY_SIZE,
	.setkey		= dspg_aes_setkey,
	.encrypt	= dspg_aes_ctr_encrypt,
	.decrypt	= dspg_aes_ctr_decrypt,
},
{
	.base.cra_name		= "ecb(aes)",
	.base.cra_driver_name	= "dspg-ecb-aes",
	.base.cra_priority		= DSPG_AES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= AES_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= AES_MIN_KEY_SIZE,
	.max_keysize	= AES_MAX_KEY_SIZE,
	.setkey		= dspg_aes_setkey,
	.encrypt	= dspg_aes_ecb_encrypt,
	.decrypt	= dspg_aes_ecb_decrypt,
},
{
	.base.cra_name		= "cbc(aes)",
	.base.cra_driver_name	= "dspg-cbc-aes",
	.base.cra_priority		= DSPG_AES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= AES_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= AES_MIN_KEY_SIZE,
	.max_keysize	= AES_MAX_KEY_SIZE,
	.setkey		= dspg_aes_setkey,
	.encrypt	= dspg_aes_cbc_encrypt,
	.decrypt	= dspg_aes_cbc_decrypt,
},
{
	.base.cra_name		= "ofb(aes)",
	.base.cra_driver_name	= "dspg-ofb-aes",
	.base.cra_priority		= DSPG_AES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= AES_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= AES_MIN_KEY_SIZE,
	.max_keysize	= AES_MIN_KEY_SIZE,
	.setkey		= dspg_aes_setkey,
	.encrypt	= dspg_aes_ofb_encrypt,
	.decrypt	= dspg_aes_ofb_decrypt,
},
{
	.base.cra_name		= "cbc(des)",
	.base.cra_driver_name	= "dspg-cbc-des",
	.base.cra_priority		= DSPG_DES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= DES_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= DES_KEY_SIZE,
	.max_keysize	= DES_KEY_SIZE,
	.setkey		= dspg_des_setkey,
	.encrypt	= dspg_des_cbc_encrypt,
	.decrypt	= dspg_des_cbc_decrypt,
},
{
	.base.cra_name		= "ecb(des)",
	.base.cra_driver_name	= "dspg-ecb-des",
	.base.cra_priority		= DSPG_DES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= DES_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= DES_KEY_SIZE,
	.max_keysize	= DES_KEY_SIZE,
	.setkey		= dspg_des_setkey,
	.encrypt	= dspg_des_ecb_encrypt,
	.decrypt	= dspg_des_ecb_decrypt,
},
{
	.base.cra_name		= "cbc(des3_ede)",
	.base.cra_driver_name	= "dspg-cbc-des3",
	.base.cra_priority		= DSPG_DES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= DES_KEY_SIZE,
	.max_keysize	= DES3_EDE_KEY_SIZE,
	.setkey		= dspg_des3_setkey,
	.encrypt	= dspg_des3_cbc_encrypt,
	.decrypt	= dspg_des3_cbc_decrypt,
},
{
	.base.cra_name		= "ecb(des3_ede)",
	.base.cra_driver_name	= "dspg-ecb-des3",
	.base.cra_priority		= DSPG_DES_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= DES3_EDE_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= DES_KEY_SIZE,
	.max_keysize	= DES3_EDE_KEY_SIZE,
	.setkey		= dspg_des3_setkey,
	.encrypt	= dspg_des3_ecb_encrypt,
	.decrypt	= dspg_des3_ecb_decrypt,
},
{
	.base.cra_name		= "ecb(arc4)",
	.base.cra_driver_name	= "dspg-arc4",
	.base.cra_priority		= DSPG_RC4_PRIORITY,
	.base.cra_flags		= CRYPTO_ALG_TYPE_SKCIPHER |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_ASYNC,
	.base.cra_blocksize		= 1,
	.base.cra_ctxsize		= sizeof(struct dspg_crypto_ctx),
	.base.cra_alignmask		= 0xf,
	.base.cra_module		= THIS_MODULE,

	.init		= dspg_cra_init,
	.exit		= dspg_cra_exit,
	.min_keysize	= 5,
	.max_keysize	= 32,
	.setkey		= dspg_arc4_setkey,
	.encrypt	= dspg_arc4_encrypt,
	.decrypt	= dspg_arc4_decrypt,
},
};

static int
dspg_crypto_rng_read(struct hwrng *rng, void *data, size_t max, bool wait)
{
	struct dspg_crypto_dev *dev =
				container_of(rng, struct dspg_crypto_dev, rng);

	dspg_crypto_generate_trng(dev);

	return dspg_crypto_get_trng(dev, data,
				    max / sizeof(u32)) * sizeof(u32);
}

static int __init
dspg_crypto_probe(struct platform_device *pdev)
{
	struct dspg_crypto_dev *dev;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
	int i, j, irq, ret;
	u32 cfg;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	_dspg_crypto_dev = dev;
	dev->pdev = pdev;
	platform_set_drvdata(pdev, dev);

	init_completion(&dev->completion);

	dev->in_buf = (void *)__get_free_pages(GFP_KERNEL,
					       DSPG_CRYPTO_BUFFER_ORDER);
	dev->out_buf = (void *)__get_free_pages(GFP_KERNEL,
						DSPG_CRYPTO_BUFFER_ORDER);
	dev->buf_len = DSPG_CRYPTO_BUFFER_SIZE;

	if (!dev->in_buf || !dev->out_buf) {
		kfree(dev);
		return -ENOMEM;
	}

	dev->chip_version = dvf_get_chip_rev();

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "out of memory\n");
		kfree(dev);
		return irq;
	}
	dev->irq = irq;

	if (!request_mem_region(res->start, resource_size(res),	DSPG_CRYPTO)) {
		dev_err(&pdev->dev, "failed to request memory region\n");
		ret = -EBUSY;
		goto err_kfree;
	}

	dev->regs = ioremap(res->start, resource_size(res));
	if (!dev->regs) {
		dev_err(&pdev->dev, "unable to map registers\n");
		ret = -ENOMEM;
		goto err_release_reg;
	}

	dev->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(dev->clk)) {
		dev_err(&pdev->dev, "could not get clock\n");
		ret = PTR_ERR(dev->clk);
		goto err_clk;
	}
	clk_prepare(dev->clk);

	dev->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(dev->reset)) {
		dev_err(&pdev->dev, "cannot get reset control\n");
		ret = PTR_ERR(dev->reset);
		goto err_clk;
	}

	ret = reset_control_deassert(dev->reset);
	if (ret) {
		dev_err(&pdev->dev, "failed to deassert reset\n");
		goto err_clk;
	}

	spin_lock_init(&dev->lock);

	ret = request_irq(dev->irq, dspg_crypto_isr, 0, DSPG_CRYPTO, dev);
	if (ret)
		goto err_irq;

	dev->desc_dma_pool = dma_pool_create(DSPG_CRYPTO, &pdev->dev,
		sizeof(struct hw_crypto_desc), NUM_CHANNELS, 0);
	if (!dev->desc_dma_pool) {
		ret = -ENOMEM;
		goto err_dma;
	}

	ret = of_platform_populate(np, NULL, NULL, &pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "error populating subdevices\n");
		goto err_of_populate;
	}

	clk_enable(dev->clk);
	cfg = crypto_readl(dev, REG_CFG_CFG);
	clk_disable(dev->clk);

	dev->rng.name = DSPG_CRYPTO;
	dev->rng.read = dspg_crypto_rng_read;

	ret = hwrng_register(&dev->rng);
	if (ret < 0)
		dev_warn(&pdev->dev, "could not register hwrng\n");

	crypto_init_queue(&dev->queue, 1); /* TBD: extend to 32 */

	dev->kthread = kthread_run(dspg_crypto_queue_manage, dev,
				   "dspg_crypto");
	if (IS_ERR(dev->kthread))
		return PTR_ERR(dev->kthread);

	for (i = 0; i < ARRAY_SIZE(dspg_algs); i++) {
		ret = crypto_register_skcipher(&dspg_algs[i]);
		if (ret)
			goto err_algs;
	}

	dev_info(&pdev->dev, "DSPG cryptographic security module (revision %d/%d)\n",
		 (cfg >> 20) & 0xf, (cfg >> 16) & 0xf);

	return 0;

err_algs:
	for (j = 0; j < i; j++)
		crypto_unregister_skcipher(&dspg_algs[j]);
err_of_populate:
	dma_pool_destroy(dev->desc_dma_pool);
err_dma:
	free_irq(irq, dev);
err_irq:
	clk_disable(dev->clk);
	clk_put(dev->clk);
err_clk:
	iounmap(dev->regs);
	dev->regs = NULL;
err_release_reg:
	release_mem_region(res->start, resource_size(res));
err_kfree:
	kfree(dev);
	dev = NULL;

	return ret;
}

static int
__exit dspg_crypto_remove(struct platform_device *pdev)
{
	struct dspg_crypto_dev *dev = platform_get_drvdata(pdev);
	int i;

	kthread_stop(dev->kthread);

	for (i = 0; i < ARRAY_SIZE(dspg_algs); i++)
		crypto_unregister_skcipher(&dspg_algs[i]);
	dma_pool_destroy(dev->desc_dma_pool);
	free_irq(dev->irq, dev);
	clk_disable(dev->clk);
	clk_put(dev->clk);
	iounmap(dev->regs);
	free_page((unsigned long)dev->in_buf);
	free_page((unsigned long)dev->out_buf);
	kfree(dev);

	return 0;
}

struct of_match_data {
	unsigned int userspace_only;
};

static struct of_match_data of_match_data[] = {
	[0] = { 0, },
	[1] = { 1, },
};

static const struct of_device_id dspg_crypto_of_ids[] = {
	{ .compatible = "dspg,crypto", .data = &of_match_data[0], },
	{ .compatible = "dspg,userspace-crypto", .data = &of_match_data[1], },
	{ },
};
MODULE_DEVICE_TABLE(of, dspg_crypto_of_ids);

static int __init
dspg_crypto_drv_probe(struct platform_device *pdev)
{
	const struct of_match_data *data;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;

	if (!np)
		return -ENODEV;

	match = of_match_node(dspg_crypto_of_ids, np);
	if (!match)
		return -ENODEV;

	data = (struct of_match_data *)match->data;
	if (!data)
		return -EINVAL;

	if (data->userspace_only) {
		struct clk *clk;

		clk = clk_get(&pdev->dev, DSPG_CRYPTO);
		if (IS_ERR(clk)) {
			dev_err(&pdev->dev, "could not get clock\n");
			return PTR_ERR(clk);
		}
		platform_set_drvdata(pdev, (void *)clk);
		clk_enable(clk);

		return 0;
	}

	return dspg_crypto_probe(pdev);
}

static int __exit
dspg_crypto_drv_remove(struct platform_device *pdev)
{
	const struct of_match_data *data;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;

	match = of_match_node(dspg_crypto_of_ids, np);
	data = (struct of_match_data *)match->data;
	if (data->userspace_only) {
		struct clk *clk = platform_get_drvdata(pdev);

		clk_disable(clk);
		clk_put(clk);

		return 0;
	}

	return dspg_crypto_remove(pdev);
}

static struct platform_driver dspg_crypto_driver = {
	.driver = {
		.name = DSPG_CRYPTO,
		.owner = THIS_MODULE,
		.of_match_table = dspg_crypto_of_ids,
	},
	.remove = dspg_crypto_drv_remove,
};

static int __init dspg_crypto_init(void)
{
	return platform_driver_probe(&dspg_crypto_driver,
				     dspg_crypto_drv_probe);
}
module_init(dspg_crypto_init);

static void __exit dspg_crypto_exit(void)
{
	platform_driver_unregister(&dspg_crypto_driver);
}
module_exit(dspg_crypto_exit);

MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("DSPG Cryptographic security module driver");
MODULE_LICENSE("GPL");
