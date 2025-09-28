/*
 * mipi_fb.h
 *
 *  Created on: Sep 3, 2014
 *      Author: arcxplorer
 */

#ifndef MIPI_FB_H_
#define MIPI_FB_H_

//#include <stdint.h>

/**
 * Structure that interfaces with the IOCTL of the framebuffer. To the IOCTL
 * that support read and write parameters use this structure to get and set
 * data from and to the driver
 */
typedef struct {
	uint32_t address;
	uint32_t value;
} fb_ioctl_data;

/**
 * IOCTL defines
 */
/**
 * @short IOCTL to read a byte from MIPI DSI CORE
 * - fb_data->address -> address to read
 * - fb_data->value -> return value
 */
#define FB_MIPI_CORE_READ		0x1000

/**
 * @short IOCTL to write a byte to MIPI DSI CORE
 * fb_data->address -> address to write to
 * fb_data->value -> value to write
 */
#define FB_MIPI_CORE_WRITE		0x1001

/**
 * @short IOCTL to get the device base address
 * fb_data->value -> base address
 */
#define FB_MIPI_BASE_ADDR		0x1002

/**
 * @short IOCTL to change the video pattern generator
 */
#define FB_MIPI_PATTERN_GENERATOR	0x1003


/************************************DPI****************************************/
/**
 * @short IOCTL to write DPI interface configuration
 */
#define FB_MIPI_DPI_WRITE	0x1010

/**
 * @short IOCTL to read DPI interface configuration
 */
#define FB_MIPI_DPI_READ	0x1011

/**
 * @short IOCTL to configure DPI interface with pre configured paramenters
 */
#define FB_MIPI_DPI_PRE_CONFIG	0x1012

/************************************EDPI****************************************/
/**
 * 
 * @short IOCTL to write EDPI interface configuration
 */
#define FB_MIPI_EDPI_WRITE	0x1015

/**
 * @short IOCTL to read EDPI interface configuration
 */
#define FB_MIPI_EDPI_READ	0x1016

/**
 * @short IOCTL to configure EDPI interface with pre configured paramenters
 */
#define FB_MIPI_EDPI_PRE_CONFIG	0x1017

/*******************************************************************************/
/**
 * 
 * @short IOCTL to start DSI platform
 */
#define FB_MIPI_PLATFORM_INIT	0x1020
/****************************WRITE********************************************/
/**
 * 
 * @short IOCTL to write DCS commands
 */
#define FB_MIPI_DCS_WRITE		0x1021

/**
 * 
 * @short IOCTL to write GEN commands
 */
#define FB_MIPI_GEN_WRITE		0x1022

/**
 * 
 * @short IOCTL to write Packets (long and short)
 */
#define FB_MIPI_PKT_WRITE		0x1023
/*****************************READ*********************************************/
/**
 * 
 * @short IOCTL to read DCS
 */
#define FB_MIPI_READ_DCS		0x1024
/**
 * 
 * @short IOCTL to read GEN
 */
#define FB_MIPI_READ_GEN		0x1025

/** 
 * @short IOCTL to read GEN
 */
#define FB_MIPI_READ_PACKET		0x1026
/*************************************************************************/
/** 
 * @short IOCTL to enable RX
 */
#define FB_MIPI_RX		0x1027

/** 
 * @short IOCTL to enable ACK
 */
#define FB_MIPI_ACK		0x1028

/** 
 * @short IOCTL to power PHY
 */
#define FB_MIPI_PHY_PWR		0x1029

/** 
 * @short IOCTL to power core
 */
#define FB_MIPI_CORE_PWR		0x1030

/** 
 * @short IOCTL to change clock mode (HS/LP)
 */
#define FB_MIPI_CMD_HS		0x1031

/** 
 * @short IOCTL to enable ECC
 */
#define FB_MIPI_ECC		0x1032

/** 
 * @short IOCTL to enable EOTP in TX
 */
#define FB_MIPI_EOTP_TX		0x1033

/** 
 * @short IOCTL to enable EOTP in RX
 */
#define FB_MIPI_EOTP_RX		0x1034

/** 
 * @short IOCTL to Shutdown display
 */
#define FB_MIPI_SDISPLAY		0x1035

/** 
 * @short IOCTL to control Colour mode (ON|OFF)
 */
#define FB_MIPI_CMODE		0x1036

/** 
 * @short IOCTL to send reset trigger - escape sequence by phy
 */
#define FB_MIPI_RESETTRIG		0x1037

/** 
 * @short IOCTL to Enable clock lane transmission
 */
#define FB_MIPI_HSCLK		0x1038

/** 
 * @short IOCTL to send Send ULPS escape mode
 */
#define FB_MIPI_ULPSSAFE		0x1039

/** 
 * @short IOCTL to Enable tearing effect
 */
#define FB_MIPI_TEAR		0x1040

/** 
 * @short IOCTL to configure PHY
 */
#define FB_MIPI_PHY		0x1041

/** 
 * @short IOCTL to configure peripheral response timeout
 */
#define FB_MIPI_PRESP		0x1042






/**
 * @short IOCTL to change the vif test mode 
 */
#define FB_MIPI_VIF_TEST	0x1100

/**
 * @short IOCTL to change the vif horizontal active pixels 
 */
#define FB_MIPI_VIF_HRES	0x1101

/**
 * @short IOCTL to change the vif horizontal blanking
 */
#define FB_MIPI_VIF_HBP		0x1102

/**
 * @short IOCTL to change the vif horizontal sync
 */
#define FB_MIPI_VIF_HSYNC	0x1103

/**
 * @short IOCTL to change the vif horizontal line time 
 */
#define FB_MIPI_VIF_LTIME	0x1104

/**
 * @short IOCTL to change the vif frames per second 
 */
#define FB_MIPI_VIF_FPS	0x1105

/**
 * @short IOCTL to change the vif vertical active pixels
 */
#define FB_MIPI_VIF_VRES	0x1106

/**
 * @short IOCTL to change the vif vertical blanking
 */
#define FB_MIPI_VIF_VBP	0x1107

/**
 * @short IOCTL to change the vif vertical front porch 
 */
#define FB_MIPI_VIF_VFP	0x1108

/**
 * @short IOCTL to change the vif horizontal sync
 */
#define FB_MIPI_VIF_VSYNC	0x1109

/**
 * @short IOCTL to change the vif phase select 
 */
#define FB_MIPI_VIF_PSEL	0x1110

/**
 * @short IOCTL to change the vif raw sequencing 
 */
#define FB_MIPI_VIF_RAW	0x1111

/**
 * @short IOCTL to change the vif colour (16|18|24) 
 */
#define FB_MIPI_VIF_BPP	0x1112

/**
 * @short IOCTL to change the vif pixel configuration (1|2|3)
 */
#define FB_MIPI_VIF_PCONF	0x1113

/**
 * @short IOCTL to change the vif eDPI interface enable
 */
#define FB_MIPI_VIF_EDPI	0x1114

/**
 * @short IOCTL to change the vif wait for tearing effect
 */
#define FB_MIPI_VIF_TE	0x1115

/**
 * @short IOCTL to change the vif predefined video resolution
 */
#define FB_MIPI_VIF_PRE	0x1116




#endif /* MIPI_FB_H_ */
