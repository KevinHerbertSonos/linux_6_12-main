#include <video/mipi_display.h>

#include "dw_dsi_cmd.h"
#include "dw_dsi_reg.h"

#define MIPI_DSIH_CMD_DBG

#define DSIH_FIFO_ACTIVE_WAIT		200

#define R_DSI_HOST_GEN_HDR		0x6C
#define R_DSI_HOST_GEN_PLD_DATA		0x70
#define R_DSI_HOST_CMD_PKT_STATUS	0x74

#if 0
static void
dsi_phy_tst_set_multiple(void __iomem *base, u32 reg, u8 *data, size_t length)
{
	u32 reg_write = 0x10000 + reg;

	if (!length || !data)
		return;

	/*
	 * latch reg first
	 */
	writel(reg_write, base + PHY_TST_CTRL1);
	writel(0x02, base + PHY_TST_CTRL0);
	writel(0x00, base + PHY_TST_CTRL0);

	/*
	 * then latch values
	 */
	while (length--) {
		writel(data[length], base + PHY_TST_CTRL1);
		writel(0x02, base + PHY_TST_CTRL0);
		writel(0x00, base + PHY_TST_CTRL0);
	}
}
#endif

static uint32_t
dw_dsi_read_word(struct dw_dsi *dev, uint32_t reg_address)
{
	uint32_t ret;

	if (dev == NULL) {
		pr_err("%s: device is null\n", __func__);
		return -ENODEV;
	}
	ret = ioread32(dev->ctx->base + reg_address);

	return ret;
}

static void
dw_dsi_write_word(struct dw_dsi *dev, uint32_t reg_address, uint32_t data)
{
	if (dev == NULL) {
		pr_err("%s: device is null\n", __func__);
		return;
	}
	iowrite32(data, dev->ctx->base + reg_address);
}

static uint32_t
dw_dsi_read_part(struct dw_dsi *dev, uint32_t reg_address,
		 uint8_t shift, uint8_t width)
{
	return (dw_dsi_read_word(dev, reg_address) >> shift) &
	       ((1 << width) - 1);
}

static void
dw_dsi_write_part(struct dw_dsi *dev, uint32_t reg_address,
		  uint32_t data, uint8_t shift, uint8_t width)
{
	uint32_t mask = (1 << width) - 1;
	uint32_t temp = dw_dsi_read_word(dev, reg_address);

	temp &= ~(mask << shift);
	temp |= (data & mask) << shift;
	dw_dsi_write_word(dev, reg_address, temp);
}

static int
dw_dsi_hal_gen_write_fifo_full(struct dw_dsi *dev)
{
	return dw_dsi_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 3, 1);
}

static int
dw_dsi_hal_gen_cmd_fifo_full(struct dw_dsi *dev)
{
	return dw_dsi_read_part(dev, R_DSI_HOST_CMD_PKT_STATUS, 1, 1);
}

static int
dw_dsi_hal_gen_packet_header(struct dw_dsi *dev, uint8_t vc,
			     uint8_t packet_type, uint8_t ms_byte,
			     uint8_t ls_byte)
{
	if (vc < 4) {
		dw_dsi_write_part(dev, R_DSI_HOST_GEN_HDR,
				     (ms_byte << 16) | (ls_byte << 8) |
				     ((vc << 6) | packet_type), 0, 24);
		return 1;
	}

	return 0;
}

static int
dw_dsi_hal_gen_packet_payload(struct dw_dsi *dev, uint32_t payload)
{
	if (dw_dsi_hal_gen_write_fifo_full(dev))
		return 0;
	dw_dsi_write_word(dev, R_DSI_HOST_GEN_PLD_DATA, payload);
	return 1;
}

static int
dw_dsi_gen_wr_packet(struct dw_dsi *dev, uint8_t vc, uint8_t data_type,
			uint8_t *params, uint16_t param_length)
{
	/* active delay iterator */
	int timeout, i, j;
	/* holds padding bytes needed */
	int compliment_counter = 0;
	uint8_t *payload = 0;
	/* temporary variable to arrange bytes into words */
	uint32_t temp = 0;
	uint16_t word_count = 0;

	if (dev == NULL) {
		pr_err("Null device\n");
		return -ENODEV;
	}

	if ((params == 0) && (param_length != 0)) {	/* pointer NULL */
		dev_err(dev->parent_dev, "Null params\n");
		return -EIO;
	}
	if (param_length > DW_DSI_MAX_PARAMS) {
		dev_err(dev->parent_dev, "param length too large");
		return -EIO;
	}

	if (param_length > 2) {	/* long packet - write word count to header, and the rest to payload */
		payload = params + 2 * sizeof(params[0]);
		word_count = (params[1] << 8) | params[0];
		if (word_count > 200) {
			dev_err(dev->parent_dev, "word count too large");
			return -EIO;
		}
		if ((param_length - 2) < word_count) {
			dev_err(dev->parent_dev,
				"sent > input payload. complemented with zeroes");
			compliment_counter = (param_length - 2) - word_count;
		} else if ((param_length - 2) > word_count) {
			dev_err(dev->parent_dev,
				"Overflow - input > sent. payload truncated");
		}

		for (i = 0; i < param_length - 2; i += j) {
			temp = 0;
			for (j = 0; j < 4 && j + i < param_length - 2; j++)
				temp |= payload[i + j] << (j * 8);

			/* check if payload Tx fifo is not full */
			for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT;
			     timeout++) {
				if (dw_dsi_hal_gen_packet_payload(dev, temp))
					break;
			}
			if (!(timeout < DSIH_FIFO_ACTIVE_WAIT)) {
				dev->timeout++;
				pr_err("timeout! %u\n", dev->timeout);
				return -ETIMEDOUT;
			}
		}

		/* if word count entered by the user more than actual parameters received
		 * fill with zeroes - a fail safe mechanism, otherwise controller will
		 * want to send data from an empty buffer */
		for (i = 0; i < compliment_counter; i++) {
			/* check if payload Tx fifo is not full */
			for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT;
			     timeout++) {
				if (!dw_dsi_hal_gen_packet_payload(dev, 0))
					break;
			}
			if (!(timeout < DSIH_FIFO_ACTIVE_WAIT)) {
				pr_err("timeout!!\n");
				return -ETIMEDOUT;
			}
		}
	}

	for (timeout = 0; timeout < DSIH_FIFO_ACTIVE_WAIT; timeout++) {
		/* check if payload Tx fifo is not full */
		if (!dw_dsi_hal_gen_cmd_fifo_full(dev)) {
			if (param_length == 0) {
				return dw_dsi_hal_gen_packet_header(dev, vc,
								data_type,
								0x0,
								0x0);

			} else if (param_length == 1) {
				return dw_dsi_hal_gen_packet_header(dev, vc,
								data_type,
								0x0,
								params[0]);
			} else {
				return dw_dsi_hal_gen_packet_header(dev, vc,
								data_type,
								params[1],
								params[0]);
			}
			break;
		}
	}

	return 0;
}

int
dw_dsi_send(struct dw_dsi *dev, struct dsi_cmd *cmd)
{
#ifdef MIPI_DSIH_CMD_DBG
	int i;

	if (!cmd->num_params)
		printk("DCS command 0x%2.2X; #params: %3.3u, params: [",
		       MIPI_DSI_DCS_SHORT_WRITE, cmd->num_params);
	else if (cmd->num_params == 1)
		printk("DCS command 0x%2.2X; #params: %3.3u, params: [",
		       MIPI_DSI_DCS_SHORT_WRITE_PARAM, cmd->num_params);
	else
		printk("DCS command 0x%2.2X; #params: %3.3u, params: [",
		       MIPI_DSI_DCS_LONG_WRITE, cmd->num_params);

	for (i = 0; i < cmd->num_params; i++)
		printk(" %2.2X", cmd->params[i]);
	printk(" ]\n");
#endif
	if (!cmd->num_params)
		return dw_dsi_gen_wr_packet(dev, 0,
					    MIPI_DSI_DCS_SHORT_WRITE,
					    cmd->params, cmd->num_params);
	else if (cmd->num_params == 1)
		return dw_dsi_gen_wr_packet(dev, 0,
					    MIPI_DSI_DCS_SHORT_WRITE_PARAM,
					    cmd->params, cmd->num_params);
	else
		return dw_dsi_gen_wr_packet(dev, 0,
					    MIPI_DSI_DCS_LONG_WRITE,
					    cmd->params, cmd->num_params);
}
