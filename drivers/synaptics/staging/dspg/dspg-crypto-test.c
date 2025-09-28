/*
 * Tests for the DVF99 HW Crypto driver
 *
 *  Copyright (C) 2013 DSP Group
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/mfd/dspg-crypto.h>
#include "dspg-crypto-test.h"

#define DSPG_CRYPTO_TEST "dspg-crypto-test"

#define TEST_SUCCESS	1
#define TEST_FAILURE	2

static void
hexdump(unsigned char *buf, unsigned int len)
{
	print_hex_dump(KERN_CONT, "        ", DUMP_PREFIX_NONE, 16, 1, buf, len, false);
}

static void
dump_hash_test(struct hash_testvec *test)
{
	printk("      plaintext = %s\n", test->plaintext);
	printk("      key size = %d\n", test->ksize);
	printk("      digest size = %d\n", test->dsize);
	printk("      expected:\n");
	hexdump(test->digest, test->dsize);
}

static void
dump_cipher_test(struct cipher_testvec *test)
{
	printk("      key size = %d\n", test->klen);
	printk("      input size = %d\n", test->ilen);
	printk("      expected:\n");
	hexdump(test->result, test->rlen);
}

static void
digest_done(int status, void *context)
{
	struct hash_testvec *test = (struct hash_testvec *)context;
	struct crypto_op *op = (struct crypto_op *)(test->op);
	u8* result_buf = (u8*)(op->data_out_virt);
	int ret = 0;

	if (memcmp(result_buf, test->digest, test->dsize)) {
		printk("FAILED\n");
		dump_hash_test(test);
		printk("      result:\n");
		hexdump(result_buf, test->dsize);
		ret = TEST_FAILURE;
	} else {
		printk("SUCCEEDED\n");
		ret = TEST_SUCCESS;
	}

	// dspg_crypto_free_operation(op);
	kfree(op->data_in_virt);
	kfree(op->data_out_virt);

	if (test->wq) {
		test->done = ret;
		wake_up(test->wq);
	}
}

static int
test_digest(const char *name, u32 algorithm, struct hash_testvec *test)
{
	int ret = 0;
	// @todo: channel should be allocated by the driver
	u32 channel = 0;
	// Crypto operator in/out memory must be contiguous!
	u8* op_in_buf = kzalloc(test->psize, GFP_KERNEL);
	u8* op_out_buf = kzalloc(test->dsize, GFP_KERNEL);
	struct crypto_op *op = 0;
	DECLARE_WAIT_QUEUE_HEAD_ONSTACK(wq);

	printk("   Test:%s - ", name);
	//printk("   Test:%s - ", test->key);
	printk("   Key size:%d - ", test->ksize);
	memcpy(op_in_buf, test->plaintext, test->psize);

	op = dspg_crypto_alloc_operation(op_in_buf,
			test->psize,
			op_out_buf,
			test->dsize,
			algorithm,
			channel,
			test->key,
			test->ksize,
			NULL, NULL, 0, 0, 0);

	if (!op) {
		printk("   Test:%s - FAILED to allocate Operation", name);
		return -1;
	}

	op->done = digest_done;
	op->context = test;

	test->op = op;
	test->wq = &wq;
	test->done = 0;
	ret = dspg_crypto_submit_operation(op);

	wait_event_timeout(wq, test->done, HZ);
	if (!test->done)
		printk("DMA memcpy timed out!\n");

	return test->done;
}
// @todo: make tests sync/async with wait-queue
// @todo: crypto operation should be opaque to the client

static void
cipher_done(int status, void *context)
{
	struct cipher_testvec *test = (struct cipher_testvec *)context;
	struct crypto_op *op = (struct crypto_op *)(test->op);
	u8* result_buf = (u8*)(op->data_out_virt);
	int ret = 0;

	if (memcmp(result_buf, test->result, test->rlen)) {
		printk("FAILED\n");
		dump_cipher_test(test);
		printk("      result:\n");
		hexdump(result_buf, test->rlen);
		ret = TEST_FAILURE;
	} else {
		printk("SUCCEEDED\n");
		ret = TEST_SUCCESS;
	}

	// dspg_crypto_free_operation(op);
	kfree(op->data_in_virt);
	kfree(op->data_out_virt);

	if (test->wq) {
		test->done = ret;
		wake_up(test->wq);
	}
}

static int
test_cipher(const char *name, u32 algorithm, struct cipher_testvec *test)
{
	int ret = 0;
	// @todo: channel should be allocated by the driver
	u32 channel = 0;
	// Crypto operator in/out memory must be contiguous!
	u8* op_in_buf = kzalloc(test->ilen + test->alen, GFP_KERNEL);
	u8* op_out_buf = kzalloc(test->rlen, GFP_KERNEL);
	struct crypto_op *op = 0;
	DECLARE_WAIT_QUEUE_HEAD_ONSTACK(wq);

	printk("   Test:%s - ", name);
	//printk("   Test:%s - ", test->key);
	printk("   Key size:%d - ", test->klen);
	memcpy(op_in_buf, test->input, test->ilen + test->alen);

	op = dspg_crypto_alloc_operation(op_in_buf,
					 test->ilen,
					 op_out_buf,
					 test->rlen,
					 algorithm,
					 channel,
					 test->key,
					 test->klen,
					 test->iv,
					 test->a,
					 test->alen,
					 test->clen,
					 0);
	if (!op) {
		printk("\n   Test:%s - FAILED to allocate operation", name);
		return -1;
	}

	op->done = cipher_done;
	op->context = test;

	test->op = op;
	test->wq = &wq;
	test->done = 0;
	ret = dspg_crypto_submit_operation(op);

	wait_event_timeout(wq, test->done, HZ);
	if (!test->done)
		printk("DMA memcpy timed out!\n");

	return test->done;
}

static int
__init dspg_crypto_test_probe(struct platform_device *pdev)
{
	int i, alg;
	char test_name[512];

	printk("HW Crypto digest tests: hashes\n");
	for (alg = 0; alg < ARRAY_SIZE(hash_tests); alg++) {
		for (i = 0; i < hash_tests[alg].count; i++) {
			hash_tests[alg].vecs[i].dsize = hash_tests[alg].dsize;
			sprintf(test_name, "%s[%d]", hash_tests[alg].name, i);
			test_digest(test_name, hash_tests[alg].algorithm,
				    &hash_tests[alg].vecs[i]);
		}
	}

	printk("HW Crypto digest tests: ciphers\n");
	for (alg = 0; alg < ARRAY_SIZE(cipher_tests); alg++) {
		for (i = 0; i < cipher_tests[alg].count; i++) {
			sprintf(test_name, "%s[%d]", cipher_tests[alg].name, i);
			test_cipher(test_name, cipher_tests[alg].algorithm,
				    &cipher_tests[alg].vecs[i]);
		}
	}

	return 0;
}

static const struct of_device_id dspg_crypto_test_of_ids[] = {
	{ .compatible = "dspg,test-crypto" },
	{ },
};

MODULE_DEVICE_TABLE(of, dspg_crypto_test_of_ids);

static struct platform_driver dspg_crypto_test_driver = {
	.driver = {
		.name = DSPG_CRYPTO_TEST,
		.owner = THIS_MODULE,
		.of_match_table = dspg_crypto_test_of_ids,
	},
	//.remove = __exit_p(dspg_crypto_test_remove),
};

static int __init dspg_crypto_test_init(void)
{
	return platform_driver_probe(&dspg_crypto_test_driver,
				     dspg_crypto_test_probe);
}
module_init(dspg_crypto_test_init);

static void __exit dspg_crypto_test_exit(void)
{
	platform_driver_unregister(&dspg_crypto_test_driver);
}
module_exit(dspg_crypto_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DSP Group, Inc.");
MODULE_DESCRIPTION("dspg-crypto-test - verification tests for DVF99 cryptographic security module");
