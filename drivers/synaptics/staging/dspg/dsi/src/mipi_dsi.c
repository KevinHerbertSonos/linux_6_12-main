/**
 * @file mipi_dsi.c
 * @brief MIPI DSI controller driver
 *
 * Copyright (C) 2014 Synopsys, Inc. All rights reserved.
 *
 * @version 1.00a first release
 */

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include "includes.h"
#include "frame_buffer.h"
#include "video_if.h"
#include "mipi_displays.h"
#include "../api/mipi_dsih_api.h"
#include "../api/mipi_dsih_dphy.h"
#include "../api/mipi_dsih_hal.h"

/** @short License information */
MODULE_LICENSE("GPL");
/** @short Author information */
MODULE_AUTHOR("Synopsys");
/** @short Device description */
MODULE_DESCRIPTION("MIPI DSI module driver");
/** @short Device version */
MODULE_VERSION("1.0");


/**
 * @short List of driver arguments
 */
int verbose = 0;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Enable (1) or disable (0) the debug traces.");

/**
 * @short List of the devices
 * Linked list that contains the installed devices
 */
static LIST_HEAD(devlist_global);

/**
 * @short List of allocated memory
 * Linked list that contains the allocated memory
 */
static struct mem_alloc *alloc_list;

/**
* @short Init DSI parameters
* @param[in] dev MIPI DSI device
* @return none
*/
void mipi_dsi_init_param(struct mipi_dsi_dev *dev)
{
	dphy_t * phy = &dev->phy;
	dsih_dpi_video_t * video = &dev->dpi_video;
	dsih_cmd_mode_video_t * edpi_video = &dev->cmd_mode_video;

	pr_info("%s:DSI initialization\n", FUNC_NAME);
	phy->reference_freq = 25000; /* [KHz] */

	if (dev == NULL){
		pr_err("Dev Null\n");
		return;
	}

	dev->max_lanes = 4; /* DWC MIPI D-PHY Bidir TSMC40LP has only 2 lanes */
	dev->max_bta_cycles = 4095;
	dev->color_mode_polarity = 1;
	dev->shut_down_polarity = 1;

	/* Open instance first - to make sure addresses and other attributes are correct */
	mipi_dsih_open(dev);

	/* initialise DPI video params */
	video->no_of_lanes = 1;
	video->non_continuous_clock = 0;
	video->virtual_channel = 0;
	video->video_mode = 4; /* invalid value*/
	video->byte_clock = 0; /* KHz  */
	video->pixel_clock = 0;   /* dpi_clock KHz*/
	video->color_coding = COLOR_CODE_MAX; /* invalid value*/
	video->is_18_loosely = 0;
	video->h_polarity = 0;
	video->h_active_pixels = 0; /* hadr*/
	video->h_sync_pixels = 0;
	video->h_back_porch_pixels = 0;   /* hbp */
	video->h_total_pixels = 0;  /* hfp */
	video->v_polarity = 0;
	video->v_active_lines = 0; /* vadr*/
	video->v_sync_lines = 0;
	video->v_back_porch_lines = 0;   /* vbp */
	video->v_total_lines = 0;  /* vfp */
	/* as of 1.20a */
	video->max_hs_to_lp_cycles = 50; /* value for max freq */
	video->max_lp_to_hs_cycles = 153; /* value for max freq */
	video->max_clk_hs_to_lp_cycles = 66; /* value for max freq */
	video->max_clk_lp_to_hs_cycles = 181; /* value for max freq */

	/* Initialise eDPI params */
	edpi_video->color_coding = 0;
	edpi_video->virtual_channel = 0;
	edpi_video->lp = 0;
	edpi_video->te = 0;
	edpi_video->h_start = 0;
	edpi_video->h_active_pixels = 0;
	edpi_video->packet_size = 0;
	edpi_video->v_start = 0;
	edpi_video->v_active_lines = 0;
}

/**
* @short Start DSI platform
* @param[in] dev MIPI DSI device
* @param[in] display Type of display (M or S)
* @param[in] video_mode Video mode or command mode
* @param[in] lanes number of lanes
* @return none
*/
void
dsi_platform_init(struct mipi_dsi_dev *dev, int display, int video_mode, int lanes)
{
	uint8_t dsi_command_param[2] = {0};

	pr_info("%s:DSI Platform Init\n", FUNC_NAME);
	if (dsi_screen_init(dev, display)) {
		dev_err(dev->parent_dev, "could not initialize DSI panel\n");
		return;
	}

	switch (video_mode) {
	case COMMAND_MODE:
		/* command mode */
		pr_info("%s:Command Mode\n", FUNC_NAME);
		dsi_command_param[0] = 0x40;
		dsi_command_param[1] = 0x01;
		mipi_dsih_gen_wr_packet(dev, 0, 0x15, dsi_command_param, 2);

		if (!pre_command_mode(dev, display, lanes)) {
			pr_info("%s: eDPI Video\n", FUNC_NAME);
			if (!mipi_dsih_edpi_video(dev))
				dev_err(dev->parent_dev,"error configuring video\n");

			copy_edpi_param_changes(&dev->cmd_mode_video,&dev->cmd_mode_video_old);
		}

		mipi_dsih_dphy_configure(dev, lanes, DEFAULT_BYTE_CLOCK);
#if 0
		video_if_edpi(dev,1);
#endif
		break;
	case VIDEO_MODE:
		/* video mode */
		pr_info("%s:Video Mode\n", FUNC_NAME);

		if (!pre_video_mode(dev, display, lanes)) {
			pr_info("%s: DPI Video\n", FUNC_NAME);
			if (mipi_dsih_dpi_video(dev))
				dev_err(dev->parent_dev,"error configuring video\n");
/*
			dsi_command_param[0] = 0xFF;
			mipi_dsih_dcs_rd_cmd(dev, 0, 0x0A, 1,
					     dsi_command_param);
			dev_info(dev->parent_dev,
				 "read: %2.2X\n", dsi_command_param[0]);
*/
			copy_dpi_param_changes(&dev->dpi_video, &dev->dpi_video_old);
		}

		break;
	default:
		dev_err(dev->parent_dev,"Invalid mode\n");
		break;
	}
	mipi_dsih_reset_controller(dev);
}

static irqreturn_t
dwc_mipi_dsi_handler(int irq, void *dev_id)
{
	struct mipi_dsi_dev *dev = NULL;
	uint32_t status_0;
	uint32_t status_1;

	if (dev_id == NULL)
		return IRQ_NONE;

	dev = dev_id;

	/* interrupts are solely for indicating errors, so they are just
	 * printed for debugging purposes */
	status_0 = mipi_dsih_hal_int_status_0(dev, 0xffffffff);
	status_1 = mipi_dsih_hal_int_status_1(dev, 0xffffffff);

	dev_info_ratelimited(dev->parent_dev, "IRQ 0 %X IRQ 1 %X\n", status_0,
			     status_1);

	return IRQ_HANDLED;
}

#if 0
irqreturn_t
videobridge_handler(int irq, void *dev_id)
{
	struct mipi_dsi_dev *dev = NULL;

	if (dev_id == NULL)
		return IRQ_NONE;

	dev = dev_id;

	/* DO SOMETHING? */

	return IRQ_HANDLED;
}
#endif

/**
 * @short Map memory blocks
 * @param[in,out] main MIPI DSI structure
 * @return Return -ENOMEM if one of the blocks is not mapped and 0 if all
 * blocks are mapped successful.
 */
int
map_memory_blocks(struct platform_device *pdev, struct mipi_dsi_dev *dev)
{
	struct resource *mem = NULL;

	/* Device tree information: Base addresses & mapping */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev,"Base address of the device is not set.\n"
			"Refer to device tree.\n");
		return -ENXIO;
	}

	dev->core_mem_size = resource_size(mem);
	if (!request_mem_region(mem->start, resource_size(mem),
				"dwc_mipi_dsi")) {
		dev_err(dev->parent_dev,
			"%s:Unable to request mem region for DWC MIPI DSI\n",
			FUNC_NAME);
		return -ENOMEM;
	}
	dev->core_addr = ioremap(mem->start, resource_size(mem));
	if (!dev->core_addr) {
		dev_err(dev->parent_dev, "%s:Unable to map resource\n",
			FUNC_NAME);
		release_region(mem->start, resource_size(mem));
		return -ENOMEM;
	}
#if 0
	/* Map Clock Manager */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (mem == NULL) {
		dev_err(&pdev->dev,"Base address of the device is not set.\n"
			"Refer to device tree.\n");
		return -ENXIO;
	}
	dev->clk_mem_size = resource_size(mem);
	if (!request_mem_region(mem->start, resource_size(mem), "clk")) {
		dev_err(dev->parent_dev,
			"%s:Unable to request mem region for Clock Manager\n",
			FUNC_NAME);
		return -ENOMEM;
	}
	dev->clk_addr = ioremap(mem->start, resource_size(mem));
	if (!dev->clk_addr) {
		dev_err(dev->parent_dev, "%s:Unable to map clk_addr resource\n",
				FUNC_NAME);
		return -ENOMEM;
	}

	/* Map Video Bridge */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Base address of the device is not set.\n"
			"Refer to device tree.\n");
		return -ENXIO;
	}
	dev->vid_bridge_mem_size = resource_size(mem);
	if (!request_mem_region(mem->start, resource_size(mem), "video_bridge")) {
		dev_err(dev->parent_dev,
			"%s:Unable to request mem region for Video Bridge\n",
			FUNC_NAME);
		return -ENOMEM;
	}
	dev->vid_bridge_addr = ioremap(mem->start, resource_size(mem));
	if (!dev->vid_bridge_addr) {
		dev_err(dev->parent_dev,"%s:Unable to map video_bridge_base_addr resource\n",
			FUNC_NAME);
		return -ENOMEM;
	}
#endif
	return 0;
}

/**
 * @short Release memory blocks
 * @param[in,out] main MIPI DSI structure
 * @return Void
 */
void
release_memory_blocks(struct mipi_dsi_dev *dev)
{
	release_mem_region((uint32_t)dev->core_addr, dev->core_mem_size);
	iounmap(dev->core_addr);
#if 0
	release_mem_region((uint32_t)dev->clk_addr, dev->clk_mem_size);
	iounmap(dev->clk_addr);

	release_mem_region((uint32_t)dev->vid_bridge_addr, dev->vid_bridge_mem_size);
	iounmap(dev->vid_bridge_addr);
#endif
}

static int
prepare_block(struct mipi_dsi_dev *dev, int skip_reset)
{
	struct clk *apb_clk;
	struct clk *ref_clk;
	struct reset_control *rst_ctrl;

	rst_ctrl = devm_reset_control_get(dev->parent_dev, NULL);
	if (IS_ERR(rst_ctrl)) {
		dev_err(dev->parent_dev, "could not get reset control\n");
		return PTR_ERR(rst_ctrl);
	}

	apb_clk = devm_clk_get(dev->parent_dev, "apb");
	if (IS_ERR(apb_clk)) {
		dev_err(dev->parent_dev, "could not get apb clock\n");
		return PTR_ERR(apb_clk);
	}

	ref_clk = devm_clk_get(dev->parent_dev, "refclk");
	if (IS_ERR(ref_clk)) {
		dev_err(dev->parent_dev, "could not get reference clock\n");
		return PTR_ERR(ref_clk);
	}

	if (!skip_reset) {
		/* reset on, clocks on */
		clk_prepare_enable(apb_clk);
		clk_prepare_enable(ref_clk);
		reset_control_assert(rst_ctrl);

		/* reset on, clocks off */
		clk_disable(apb_clk);
		clk_disable(ref_clk);
	}

	/* reset off */
	reset_control_deassert(rst_ctrl);

	if (!skip_reset) {
		/* reset off, clocks on */
		clk_enable(apb_clk);
		clk_enable(ref_clk);
	} else {
		clk_prepare_enable(apb_clk);
		clk_prepare_enable(ref_clk);
	}

	return 0;
}

/**
 * @short Register interrupts
 * @param[in,out] main MIPI DSI structure
 * @return Return error if one of the requests is not successful. Does not
 * stop the execution if on of the interrupts is not registered, although the
 * error is returned at the end of the function.
 */
static int
register_interrupts(struct mipi_dsi_dev *dev)
{
	int ret = 0;

	/* Core Interrupts */
	ret = devm_request_irq(dev->parent_dev, dev->irq[0],
			       dwc_mipi_dsi_handler, IRQF_SHARED,
			       "dwc_mipi_dsi_handler", dev);
	if (ret) {
		dev_err(dev->parent_dev, "%s:Could not register dwc_mipi_dsi_1 interrupt\n",
			FUNC_NAME);
	}
#if 0
	/* Bridge Interrupts */
	ret = devm_request_irq(dev->parent_dev, dev->irq[1],
			       videobridge_handler, IRQF_SHARED,
			       "videobridge_handler", dev);
	if (ret) {
		dev_err(dev->parent_dev, "%s:Could not register videobridge1 interrupt\n",
			FUNC_NAME);
	}
#endif
	return ret;
}

/**
 * @short Unregister interrupts
 * @param[in,out] main MIPI DSI structure
 * @return Void
 */
void
release_interrupts(struct mipi_dsi_dev *dev)
{
	int i = 0;

	for(i = 0; i < (sizeof(dev->irq) / sizeof(dev->irq[0])); i++) {
		if (dev->irq[i])
			devm_free_irq(dev->parent_dev, dev->irq[i], dev);
	}
}

/**
 * @short Allocate memory
 * @param[in] info String to associate with memory allocated
 * @param[in] size Size of the memory to allocate
 * @param[in,out] allocated Pointer to the structure that contains the info
 * about the allocation
 * @return Void
 */
void *
alloc_mem(char *info, size_t size, struct mem_alloc *allocated)
{
	struct mem_alloc *new = NULL;
	int *return_pnt = NULL;

	// first time
	if (alloc_list == NULL) {
		alloc_list = kzalloc(sizeof(struct mem_alloc), GFP_KERNEL);
		if (!alloc_list) {
			printk( KERN_ERR "%s:Couldn't create alloc_list\n",
			FUNC_NAME);
			return NULL;
		}
		alloc_list->instance = 0;
		alloc_list->info = "allocation list - instance 0";
		alloc_list->size = 0;
		alloc_list->pointer = NULL;
		alloc_list->last = NULL;
		alloc_list->prev = NULL;
	}

	// alloc pretended memory
	return_pnt = kzalloc(size, GFP_KERNEL);
	if (!return_pnt) {
		printk(KERN_ERR "%s:Couldn't allocate memory: %s\n",
		       FUNC_NAME, info);
		return NULL;
	}

	// alloc memory for the infostructure
	new = kzalloc(sizeof(struct mem_alloc), GFP_KERNEL);
	if (!new) {
		printk(KERN_ERR
		       "%s:Couldn't allocate memory\n",
		       FUNC_NAME);
		kfree(return_pnt);
		return NULL;
	}

	new->instance = ++alloc_list->instance;
	new->info = info;
	new->size = size;
	alloc_list->size += size;
	new->pointer = return_pnt;
	if (alloc_list->last == NULL)
		new->prev = alloc_list; /* First instance */
	else
		new->prev = alloc_list->last;
	alloc_list->last = new;
	new->last = new;

	return return_pnt;
}

/**
 * @short Free all memory
 * This was implemented this way so that all memory allocated was de-allocated
 * and to avoid memory leaks.
 * @return Void
 */
void
free_all_mem(void){
	if(alloc_list != NULL){
		if(verbose)
			printk( KERN_INFO "%s:Total size allocated: %d\n",
			FUNC_NAME, alloc_list->size);

		while (alloc_list->instance != 0) {
			struct mem_alloc *this;
			this = alloc_list->last;
			// cut this from list
			alloc_list->last = this->prev;
			alloc_list->instance--;
			alloc_list->size -= this->size;
			// free allocated memory
			kfree(this->pointer);
			// free this memory
			printk(KERN_INFO "%s:Freeing: %s\n",
			       FUNC_NAME, this->info);
			kfree(this);
		}
		if (verbose)
			printk(KERN_INFO "%s:Total end size: %d\n", FUNC_NAME,
					alloc_list->size);
		kfree(alloc_list);
		alloc_list = NULL;
	}
}

/**
 * @short Initialization routine - Entry point of the driver
 * @param[in] pdev pointer to the platform device structure
 * @return 0 on success and a negative number on failure
 * Refer to Linux errors.
 */
static int
mipi_dsi_init(struct platform_device *pdev)
{
	int error = 0;
	int ret = 0;
	int i = 0;
	struct mipi_dsi_dev *dev = NULL;
	struct device_node *np = pdev->dev.of_node;
	int lanes = 4, display = -1, display_mode = VIDEO_MODE;
	u32 tmp;

	pr_info("****************************************\n");
	pr_info("%s:Installing SNPS MIPI DSI module\n", FUNC_NAME);
	pr_info("****************************************\n");

	np = of_parse_phandle(np, "video-in", 0);
	if (np) {
		if (!of_find_device_by_node(np))
			return -EPROBE_DEFER;
		else
			of_node_put(np);
	}
	np = pdev->dev.of_node;

	if (!of_property_read_u32(np, "dsi-lanes", &tmp))
		lanes = (int)tmp;
	else
		dev_warn(&pdev->dev, "setting default 4 lanes\n");

	if (!of_property_read_u32(np, "display", &tmp)) {
		display = (int)tmp;
	} else {
		dev_err(&pdev->dev, "no display specified\n");
		return -EIO;
	}

	if (!of_property_read_u32(np, "display-mode", &tmp))
		display_mode = (int)tmp;
	else
		dev_dbg(&pdev->dev, "setting default mode video\n");

	pr_info("%s:Device registration\n", FUNC_NAME);
	dev = alloc_mem("MIPI DSI Device", sizeof(struct mipi_dsi_dev), NULL);
	if (!dev) {
		pr_err("%s:Could not allocated mipi_dsi_dev\n", FUNC_NAME);
		return -ENOMEM;
	}

	memset(dev, 0, sizeof(struct mipi_dsi_dev));

	dev->parent_dev = &pdev->dev;

	dev->device_name = "MIPI_DSI";
	pr_info("%s:Driver's name '%s'\n", FUNC_NAME, dev->device_name);

	pr_info("%s:Map memory blocks\n", FUNC_NAME);
	if (map_memory_blocks(pdev, dev) < 0) {
		pr_err("%s:Map memory blocks failed\n", FUNC_NAME);
		goto free_mem;
	}

	pr_info("%s:Get IRQ numbers\n", FUNC_NAME);
	for (i = 0; i < /*(sizeof(dev->irq) / sizeof(dev->irq[0]))*/1; i++) {
		dev->irq[i] = platform_get_irq(pdev, i);
		if (dev->irq[i] <= 0) {
			pr_err("%s:IRQ number %d invalid.\n", FUNC_NAME, i);
		}
	}

	/* now that everything is fine, add it to device list */
	list_add_tail(&dev->devlist, &devlist_global);
#if 0
	pr_info("%s:Init Frame Buffer\n", FUNC_NAME);
	init_frame_buffer(dev);

	ret = register_framebuffer(&dev->fb.info);
	if (ret < 0) {
		pr_err("%s:register framebuffer device failed\n", FUNC_NAME);
		goto free_mem;
	}

	pr_info("fb %d registered!\n", dev->fb.info.node);
#endif
	dev->ldo_dsi = devm_regulator_get_optional(&pdev->dev, "ldo-dsi");
	if (IS_ERR(dev->ldo_dsi)) {
		dev_info(dev->parent_dev, "no LDO DSI regulator available\n");
		dev->ldo_dsi = NULL;
	} else if (regulator_enable(dev->ldo_dsi)) {
		dev_err(dev->parent_dev,
			"failed to enable LDO DSI regulator\n");
		error = -EIO;
		goto free_mem;
	}

	prepare_block(dev, of_property_read_bool(np, "skip-reset"));
	register_interrupts(dev);

	mipi_dsi_init_param(dev);
#if 0
	/* configure video bridge */
	video_if_config(dev, TREMOLO_M);
	video_if_data_source(dev, 0x01);
	video_if_data_target(dev, 0x01);
#endif
	/* configure platform */
	dsi_platform_init(dev, display, display_mode, lanes);

	/* un-mask interrupts */
	mipi_dsih_hal_int_mask_0(dev, 0xffffffff);
	mipi_dsih_hal_int_mask_1(dev, 0xffffffff);
#if 0
	/* configure test mode */
	video_if_test_mode(dev, 4);
#endif
	return ret;

free_mem:
	release_memory_blocks(dev);
	free_all_mem();
	return error;
}

/**
 * @short Exit routine - Exit point of the driver
 * @param[in] pdev pointer to the platform device structure
 * @return 0 on success and a negative number on failure
 * Refer to Linux errors.
 */
static int
mipi_dsi_exit(struct platform_device *pdev)
{
	struct mipi_dsi_dev *dev;
	struct list_head *list;

	pr_info("**************************************\n");
	pr_info("%s:Removing SNPS MIPI DSI module\n", FUNC_NAME);
	pr_info("**************************************\n");

	while (!list_empty(&devlist_global)) {
		list = devlist_global.next;
		list_del(list);
		dev = list_entry(list, struct mipi_dsi_dev, devlist);

		// Unregister interrupts
		pr_info("%s:Release interrupts\n", FUNC_NAME);
		release_interrupts(dev);

		// Release memory blocks
		pr_info("%s:Release memory blocks\n", FUNC_NAME);
		release_memory_blocks(dev);

		if (dev->ldo_dsi)
			regulator_disable(dev->ldo_dsi);

		free_all_mem();
	}
	return 0;
}

/**
 * @short of_device_id structure
 */
static const struct of_device_id dw_mipi_dsi[] = {
	{ .compatible =	"snps,dw-mipi-dsi" },
	{ }
};
MODULE_DEVICE_TABLE(of, dw_mipi_dsi);

/**
 * @short Platform driver structure
 */
static struct platform_driver __refdata dwc_mipi_dsi_pdrv = {
	.remove = mipi_dsi_exit,
	.probe = mipi_dsi_init,
	.driver = {
		.name = "synopsys-dsi",
		.owner = THIS_MODULE,
		.of_match_table = dw_mipi_dsi,
	},
};
module_platform_driver(dwc_mipi_dsi_pdrv);
