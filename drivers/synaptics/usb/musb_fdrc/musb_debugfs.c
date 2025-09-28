/*
 * MUSB OTG driver debugfs support
 *
 * Copyright 2010 Nokia Corporation
 * Contact: Felipe Balbi <felipe.balbi@nokia.com>
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
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>

#include "musb_core.h"
#include "musb_debug.h"

struct musb_register_map {
	char			*name;
	unsigned		offset;
	unsigned		size;
};

static const struct musb_register_map musb_regmap[] = {
	{ "FAddr",		0x00,	8 },
	{ "Power",		0x01,	8 },
	{ "Frame1",		0x0c,	8 },
	{ "Frame2",		0x0d,	8 },
	{ "Index",		0x0e,	8 },
	{ "DevCtl",		0x0f,	8 },
	{ "TxCSR1",		0x11,	8 },
	{ "TxCSR2",		0x12,	8 },
	{ "Count0",		0x16,	8 },
	{ "NakLimit0",		0x19,	8 },
	{ "TxMaxP",		0x10,	8 },
	{ "RxMaxP",		0x13,	8 },
	{ "RxCSR1",		0x14,	8 },
	{ "RxCSR2",		0x15,	8 },
	{ "RxCount1",		0x16,	8 },
	{ "RxCount2",		0x17,	8 },
	{ "TxType",		0x18,	8 },
	{ "TxInterval",		0x19,	8 },
	{ "RxType",		0x1A,	8 },
	{ "RxInterval",		0x1B,	8 },
	{ "FIFOSize",		0x1F,	8 },
	{  }	/* Terminating Entry */
};

static struct dentry *musb_debugfs_root;

static int musb_regdump_show(struct seq_file *s, void *unused)
{
	struct musb		*musb = s->private;
	unsigned		i;

	seq_printf(s, "MUSB (M)FDRC Register Dump\n");

	for (i = 0; i < ARRAY_SIZE(musb_regmap); i++) {
		switch (musb_regmap[i].size) {
		case 8:
			seq_printf(s, "%-12s: %02x\n", musb_regmap[i].name,
				   musb_readb(musb->mregs,
					      musb_regmap[i].offset));
			break;
		case 16:
			seq_printf(s, "%-12s: %04x\n", musb_regmap[i].name,
				   musb_readw(musb->mregs,
					      musb_regmap[i].offset));
			break;
		case 32:
			seq_printf(s, "%-12s: %08x\n", musb_regmap[i].name,
				   musb_readl(musb->mregs,
					      musb_regmap[i].offset));
			break;
		}
	}

	return 0;
}

static int musb_regdump_open(struct inode *inode, struct file *file)
{
	return single_open(file, musb_regdump_show, inode->i_private);
}

/*there is no TESTMODE register in FDRC, removed implementation of function */
/*TODO: decide if to remove the function or there is another
 * register that can be used instead and implement with it */
static int musb_test_mode_show(struct seq_file *s, void *unused)
{
	return 0;
}

static const struct file_operations musb_regdump_fops = {
	.open			= musb_regdump_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int musb_test_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, musb_test_mode_show, inode->i_private);
}

/*There is no TESTMODE register in FDRC, removed implementation of function */
/*TODO: decide if to remove the function or there is another
 * register that can be used instead and implement with it */
static ssize_t musb_test_mode_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	return 0;
}

static const struct file_operations musb_test_mode_fops = {
	.open			= musb_test_mode_open,
	.write			= musb_test_mode_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

int musb_init_debugfs(struct musb *musb)
{
	struct dentry		*root;
	struct dentry		*file;
	int			ret;

	root = debugfs_create_dir("musb", NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	file = debugfs_create_file("regdump", S_IRUGO, root, musb,
			&musb_regdump_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("testmode", S_IRUGO | S_IWUSR,
			root, musb, &musb_test_mode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	musb_debugfs_root = root;

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void /* __init_or_exit */ musb_exit_debugfs(struct musb *musb)
{
	debugfs_remove_recursive(musb_debugfs_root);
}
