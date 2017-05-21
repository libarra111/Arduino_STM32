/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2011 LeafLabs LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file libmaple/usb/stm32f1/usb_device.c
 * @brief USB Composite with CDC ACM and HID support.
 *
 * FIXME: this works on the STM32F1 USB peripherals, and probably no
 * place else. Nonportable bits really need to be factored out, and
 * the result made cleaner.
 */

#ifndef NO_USB

#include <libmaple/usb_device.h>

#include <libmaple/usb.h>
#include <libmaple/nvic.h>
#include <libmaple/delay.h>

/* Private headers */
#include "usb_lib_globals.h"
#include "usb_reg_map.h"

/* usb_lib headers */
#include "usb_type.h"
#include "usb_core.h"
#include "usb_def.h"

/******************************************************************************
 ******************************************************************************
 ***
 ***   HACK ALERT! FIXME FIXME FIXME FIXME!
 ***
 ***   A bunch of LeafLabs-specific configuration lives in here for
 ***   now.  This mess REALLY needs to get teased apart, with
 ***   appropriate pieces moved into Wirish.
 ***
 ******************************************************************************
 *****************************************************************************/

#if !(defined(BOARD_maple) || defined(BOARD_maple_RET6) ||      \
      defined(BOARD_maple_mini) || defined(BOARD_maple_native))
#warning USB CDC ACM relies on LeafLabs board-specific configuration.\
    You may have problems on non-LeafLabs boards.
#endif

uint32 ProtocolValue;

// Are we currently sending an IN packet?
static volatile int8 transmitting;

#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J)
static void hidDataTxCb(void);
static void hidDataRxCb(void);

/*
 * Report Descriptor
 */

const uint8_t hid_report_descriptor[] = {	// Libarra. HID report descriptor that includes keyboard, mouse and joystick
#if defined(USB_HID_KMJ) || defined(USB_HID_KM)
	//	Mouse
    0x05, 0x01,						// USAGE_PAGE (Generic Desktop)	// 54
    0x09, 0x02,						// USAGE (Mouse)
    0xa1, 0x01,						// COLLECTION (Application)
    0x85, 0x01,						//   REPORT_ID (1)
    0x09, 0x01,						//   USAGE (Pointer)
    0xa1, 0x00,						//   COLLECTION (Physical)
    0x05, 0x09,						//     USAGE_PAGE (Button)
    0x19, 0x01,						//     USAGE_MINIMUM (Button 1)
    0x29, 0x03,						//     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,						//     LOGICAL_MINIMUM (0)
    0x25, 0x01,						//     LOGICAL_MAXIMUM (1)
    0x95, 0x03,						//     REPORT_COUNT (3)
    0x75, 0x01,						//     REPORT_SIZE (1)
    0x81, 0x02,						//     INPUT (Data,Var,Abs)
    0x95, 0x01,						//     REPORT_COUNT (1)
    0x75, 0x05,						//     REPORT_SIZE (5)
    0x81, 0x03,						//     INPUT (Cnst,Var,Abs)
    0x05, 0x01,						//     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,						//     USAGE (X)
    0x09, 0x31,						//     USAGE (Y)
    0x09, 0x38,						//     USAGE (Wheel)
    0x15, 0x81,						//     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,						//     LOGICAL_MAXIMUM (127)
    0x75, 0x08,						//     REPORT_SIZE (8)
    0x95, 0x03,						//     REPORT_COUNT (3)
    0x81, 0x06,						//     INPUT (Data,Var,Rel)
    0xc0,      						//   END_COLLECTION
    0xc0,      						// END_COLLECTION

	//	Keyboard
    0x05, 0x01,						// USAGE_PAGE (Generic Desktop)	// 47
    0x09, 0x06,						// USAGE (Keyboard)
    0xa1, 0x01,						// COLLECTION (Application)
    0x85, 0x02,						//   REPORT_ID (2)
    0x05, 0x07,						//   USAGE_PAGE (Keyboard)

	0x19, 0xe0,						//   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,						//   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,						//   LOGICAL_MINIMUM (0)
    0x25, 0x01,						//   LOGICAL_MAXIMUM (1)
    0x75, 0x01,						//   REPORT_SIZE (1)

	0x95, 0x08,						//   REPORT_COUNT (8)
    0x81, 0x02,						//   INPUT (Data,Var,Abs)
    0x95, 0x01,						//   REPORT_COUNT (1)
    0x75, 0x08,						//   REPORT_SIZE (8)
    0x81, 0x03,						//   INPUT (Cnst,Var,Abs)

	0x95, 0x06,						//   REPORT_COUNT (6)
    0x75, 0x08,						//   REPORT_SIZE (8)
    0x15, 0x00,						//   LOGICAL_MINIMUM (0)
    0x25, 0x65,						//   LOGICAL_MAXIMUM (101)
    0x05, 0x07,						//   USAGE_PAGE (Keyboard)

	0x19, 0x00,						//   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,						//   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,						//   INPUT (Data,Ary,Abs)
    0xc0,      						// END_COLLECTION
#endif
	
#if defined(USB_HID_KMJ) || defined(USB_HID_J)	
	//  Joystick
	0x05, 0x01,						// Usage Page (Generic Desktop)
	0x09, 0x04,						// Usage (Joystick)
	0xA1, 0x01,						// Collection (Application)
    0x85, 0x03,						//   REPORT_ID (3)
	0x15, 0x00,						//	 Logical Minimum (0)
	0x25, 0x01,						//   Logical Maximum (1)
	0x75, 0x01,						//   Report Size (1)
	0x95, 0x20,						//   Report Count (32)
	0x05, 0x09,						//   Usage Page (Button)
	0x19, 0x01,						//   Usage Minimum (Button #1)
	0x29, 0x20,						//   Usage Maximum (Button #32)
	0x81, 0x02,						//   Input (variable,absolute)
	0x15, 0x00,						//   Logical Minimum (0)
	0x25, 0x07,						//   Logical Maximum (7)
	0x35, 0x00,						//   Physical Minimum (0)
	0x46, 0x3B, 0x01,				//   Physical Maximum (315)
	0x75, 0x04,						//   Report Size (4)
	0x95, 0x01,						//   Report Count (1)
	0x65, 0x14,						//   Unit (20)
    0x05, 0x01,                     //   Usage Page (Generic Desktop)
	0x09, 0x39,						//   Usage (Hat switch)
	0x81, 0x42,						//   Input (variable,absolute,null_state)
    0x05, 0x01,                     //Usage Page (Generic Desktop)
	0x09, 0x01,						//Usage (Pointer)
    0xA1, 0x00,                     //Collection ()
	0x15, 0x00,						//   Logical Minimum (0)
	0x26, 0xFF, 0x03,				//   Logical Maximum (1023)
	0x75, 0x0A,						//   Report Size (10)
	0x95, 0x04,						//   Report Count (4)
	0x09, 0x30,						//   Usage (X)
	0x09, 0x31,						//   Usage (Y)
	0x09, 0x32,						//   Usage (Z)
	0x09, 0x35,						//   Usage (Rz)
	0x81, 0x02,						//   Input (variable,absolute)
    0xC0,                           // End Collection
	0x15, 0x00,						// Logical Minimum (0)
	0x26, 0xFF, 0x03,				// Logical Maximum (1023)
	0x75, 0x0A,						// Report Size (10)
	0x95, 0x02,						// Report Count (2)
	0x09, 0x36,						// Usage (Slider)
	0x09, 0x36,						// Usage (Slider)
	0x81, 0x02,						// Input (variable,absolute)
    0xC0,                           // End Collection
#endif
	
	
#ifdef USB_RAWHID
	//	RAW HID
	0x06, LSB(RAWHID_USAGE_PAGE), MSB(RAWHID_USAGE_PAGE),	// 30
	0x0A, LSB(RAWHID_USAGE), MSB(RAWHID_USAGE),

	0xA1, 0x01,				// Collection 0x01
    0x85, 0x03,             // REPORT_ID (3)
	0x75, 0x08,				// report size = 8 bits
	0x15, 0x00,				// logical minimum = 0
	0x26, 0xFF, 0x00,		// logical maximum = 255

	0x95, 64,				// report count TX
	0x09, 0x01,				// usage
	0x81, 0x02,				// Input (array)

	0x95, 64,				// report count RX
	0x09, 0x02,				// usage
	0x91, 0x02,				// Output (array)
	0xC0					// end collection
#endif
};
 
static ONE_DESCRIPTOR HID_Report_Descriptor = {
    (uint8*)&hid_report_descriptor,
    sizeof(hid_report_descriptor)
};

#define HID_INTERFACE_NUMBER 	0x02

/* I/O state */

#define HID_RX_BUFFER_SIZE	256 // must be power of 2
#define HID_RX_BUFFER_SIZE_MASK (HID_RX_BUFFER_SIZE-1)
/* Received data */
static volatile uint8 hidBufferRx[HID_RX_BUFFER_SIZE];
/* Write index to hidBufferRx */
static volatile uint32 hid_rx_head;
/* Read index from hidBufferRx */
static volatile uint32 hid_rx_tail;

#define HID_TX_BUFFER_SIZE	256 // must be power of 2
#define HID_TX_BUFFER_SIZE_MASK (HID_TX_BUFFER_SIZE-1)
// Tx data
static volatile uint8 hidBufferTx[HID_TX_BUFFER_SIZE];
// Write index to hidBufferTx
static volatile uint32 hid_tx_head;
// Read index from hidBufferTx
static volatile uint32 hid_tx_tail;

/*
 * HID interface
 */

/* This function is non-blocking.
 *
 * It copies data from a user buffer into the USB peripheral TX
 * buffer, and returns the number of bytes copied. */
uint32 usb_hid_tx(const uint8* buf, uint32 len)
{
	if (len==0) return 0; // no data to send

	uint32 head = hid_tx_head; // load volatile variable
	uint32 tx_unsent = (head - hid_tx_tail) & HID_TX_BUFFER_SIZE_MASK;

    // We can only put bytes in the buffer if there is place
    if (len > (HID_TX_BUFFER_SIZE-tx_unsent-1) ) {
        len = (HID_TX_BUFFER_SIZE-tx_unsent-1);
    }
	if (len==0) return 0; // buffer full

	uint16 i;
	// copy data from user buffer to USB Tx buffer
	for (i=0; i<len; i++) {
		hidBufferTx[head] = buf[i];
		head = (head+1) & HID_TX_BUFFER_SIZE_MASK;
	}
	hid_tx_head = head; // store volatile variable

	while(transmitting >= 0);
	
	if (transmitting<0) {
		hidDataTxCb(); // initiate data transmission
	}

    return len;
}



uint32 usb_hid_data_available(void) {
    return (hid_rx_head - hid_rx_tail) & HID_RX_BUFFER_SIZE_MASK;
}

uint16 usb_hid_get_pending(void) {
    return (hid_tx_head - hid_tx_tail) & HID_TX_BUFFER_SIZE_MASK;
}

/* Non-blocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
uint32 usb_hid_rx(uint8* buf, uint32 len)
{
    /* Copy bytes to buffer. */
    uint32 n_copied = usb_hid_peek(buf, len);

    /* Mark bytes as read. */
	uint16 tail = hid_rx_tail; // load volatile variable
	tail = (tail + n_copied) & HID_RX_BUFFER_SIZE_MASK;
	hid_rx_tail = tail; // store volatile variable

	uint32 rx_unread = (hid_rx_head - tail) & HID_RX_BUFFER_SIZE_MASK;
    // If buffer was emptied to a pre-set value, re-enable the RX endpoint
    if ( rx_unread <= 64 ) { // experimental value, gives the best performance
        usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);
	}
    return n_copied;
}

/* Non-blocking byte lookahead.
 *
 * Looks at unread bytes without marking them as read. */
uint32 usb_hid_peek(uint8* buf, uint32 len)
{
    int i;
    uint32 tail = hid_rx_tail;
	uint32 rx_unread = (hid_rx_head-tail) & HID_RX_BUFFER_SIZE_MASK;

    if (len > rx_unread) {
        len = rx_unread;
    }

    for (i = 0; i < len; i++) {
        buf[i] = hidBufferRx[tail];
        tail = (tail + 1) & HID_RX_BUFFER_SIZE_MASK;
    }

    return len;
}

/*
 * Callbacks
 */
 
static void hidDataTxCb(void)
{
	uint32 tail = hid_tx_tail; // load volatile variable
	uint32 tx_unsent = (hid_tx_head - tail) & HID_TX_BUFFER_SIZE_MASK;
	if (tx_unsent==0) {
		if ( (--transmitting)==0) goto flush_hid; // no more data to send
		return; // it was already flushed, keep Tx endpoint disabled
	}
	transmitting = 1;
    // We can only send up to USB_CDCACM_TX_EPSIZE bytes in the endpoint.
    if (tx_unsent > USB_HID_TX_EPSIZE) {
        tx_unsent = USB_HID_TX_EPSIZE;
    }
	// copy the bytes from USB Tx buffer to PMA buffer
	uint32 *dst = usb_pma_ptr(USB_HID_TX_ADDR);
    uint16 tmp = 0;
	uint16 val;
	int i;
	for (i = 0; i < tx_unsent; i++) {
		val = hidBufferTx[tail];
		tail = (tail + 1) & HID_TX_BUFFER_SIZE_MASK;
		if (i&1) {
			*dst++ = tmp | (val<<8);
		} else {
			tmp = val;
		}
	}
    if ( tx_unsent&1 ) {
        *dst = tmp;
    }
	hid_tx_tail = tail; // store volatile variable
flush_hid:
	// enable Tx endpoint
    usb_set_ep_tx_count(USB_HID_TX_ENDP, tx_unsent);
    usb_set_ep_tx_stat(USB_HID_TX_ENDP, USB_EP_STAT_TX_VALID);
}


static void hidDataRxCb(void)
{
	uint32 head = hid_rx_head; // load volatile variable

	uint32 ep_rx_size = usb_get_ep_rx_count(USB_HID_RX_ENDP);
	// This copy won't overwrite unread bytes as long as there is 
	// enough room in the USB Rx buffer for next packet
	uint32 *src = usb_pma_ptr(USB_HID_RX_ADDR);
    uint16 tmp = 0;
	uint8 val;
	uint32 i;
	for (i = 0; i < ep_rx_size; i++) {
		if (i&1) {
			val = tmp>>8;
		} else {
			tmp = *src++;
			val = tmp&0xFF;
		}
		hidBufferRx[head] = val;
		head = (head + 1) & HID_RX_BUFFER_SIZE_MASK;
	}
	hid_rx_head = head; // store volatile variable

	uint32 rx_unread = (head - hid_rx_tail) & HID_RX_BUFFER_SIZE_MASK;
	// only enable further Rx if there is enough room to receive one more packet
	if ( rx_unread < (HID_RX_BUFFER_SIZE-USB_HID_RX_EPSIZE) ) {
		usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);
	}
}

static uint8* usbGetHIDReportDescriptor(uint16 Length){
  return Standard_GetDescriptorData(Length, &HID_Report_Descriptor);
}

/*
static RESULT HID_SetProtocol(void){
	uint8 wValue0 = pInformation->USBwValue0;
	ProtocolValue = wValue0;
	return USB_SUCCESS;
}
*/

static uint8* usbGetProtocolValue(uint16 Length){ // Libarra. function needed for the HID, gets the protocol value in usbNoDataSetup
	if (Length == 0){
		pInformation->Ctrl_Info.Usb_wLength = 1;
		return NULL;
	} else {
		return (uint8 *)(&ProtocolValue);
	}
}

#endif



static void vcomDataTxCb(void);
static void vcomDataRxCb(void);
static uint8* vcomGetSetLineCoding(uint16);

static void usbInit(void);
static void usbReset(void);
static RESULT usbDataSetup(uint8 request);
static RESULT usbNoDataSetup(uint8 request);
static RESULT usbGetInterfaceSetting(uint8 interface, uint8 alt_setting);
static uint8* usbGetDeviceDescriptor(uint16 length);
static uint8* usbGetConfigDescriptor(uint16 length);
static uint8* usbGetStringDescriptor(uint16 length);
static void usbSetConfiguration(void);
static void usbSetDeviceAddress(void);

/*
 * Descriptors
 */

/* FIXME move to Wirish */
#define LEAFLABS_ID_VENDOR                0x1EAF
#define MAPLE_ID_PRODUCT                  0x0004
static const usb_descriptor_device usbCompositeDescriptor_Device =
    USB_DECLARE_DEV_DESC(LEAFLABS_ID_VENDOR, MAPLE_ID_PRODUCT);

typedef struct {
    usb_descriptor_config_header Config_Header;
    //CDCACM
	IADescriptor 					IAD;
    usb_descriptor_interface     	CCI_Interface;
    CDC_FUNCTIONAL_DESCRIPTOR(2) 	CDC_Functional_IntHeader;
    CDC_FUNCTIONAL_DESCRIPTOR(2) 	CDC_Functional_CallManagement;
    CDC_FUNCTIONAL_DESCRIPTOR(1) 	CDC_Functional_ACM;
    CDC_FUNCTIONAL_DESCRIPTOR(2) 	CDC_Functional_Union;
    usb_descriptor_endpoint      	ManagementEndpoint;
    usb_descriptor_interface     	DCI_Interface;
    usb_descriptor_endpoint      	DataOutEndpoint;
    usb_descriptor_endpoint      	DataInEndpoint;
    //HID
    #if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J)
    usb_descriptor_interface     	HID_Interface;
	HIDDescriptor			 	 	HID_Descriptor;
    usb_descriptor_endpoint      	HIDDataInEndpoint;
    usb_descriptor_endpoint      	HIDDataOutEndpoint;
    #endif
} __packed usb_descriptor_config;

#define MAX_POWER (100 >> 1)


#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. sets the number of usb interfaces depending on the usb type
#define NUMINTERFACES 			0x03
#else
#define NUMINTERFACES 			0x02
#endif

#define CCI_INTERFACE_NUMBER 	0x00
#define DCI_INTERFACE_NUMBER 	0x01

#define MAX_POWER (100 >> 1)
static const usb_descriptor_config usbCompositeDescriptor_Config = {
    .Config_Header = {
        .bLength              = sizeof(usb_descriptor_config_header),
        .bDescriptorType      = USB_DESCRIPTOR_TYPE_CONFIGURATION,
        .wTotalLength         = sizeof(usb_descriptor_config),
        .bNumInterfaces       = NUMINTERFACES,
        .bConfigurationValue  = 0x01,
        .iConfiguration       = 0x00,
        .bmAttributes         = (USB_CONFIG_ATTR_BUSPOWERED |
                                 USB_CONFIG_ATTR_SELF_POWERED),
        .bMaxPower            = MAX_POWER,
    },
    
    //CDCACM
	.IAD = {
		.bLength			= 0x08,
		.bDescriptorType	= 0x0B,
		.bFirstInterface	= CCI_INTERFACE_NUMBER,
		.bInterfaceCount	= 0x02,
		.bFunctionClass		= 0x02,
		.bFunctionSubClass	= 0x02,
		.bFunctionProtocol	= 0x01,
		.iFunction			= 0x02,
	},
	
    .CCI_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = CCI_INTERFACE_NUMBER,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x01,
        .bInterfaceClass    = USB_INTERFACE_CLASS_CDC,
        .bInterfaceSubClass = USB_INTERFACE_SUBCLASS_CDC_ACM,
        .bInterfaceProtocol = 0x01, /* Common AT Commands */
        .iInterface         = 0x03,
    },

    .CDC_Functional_IntHeader = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(2),
        .bDescriptorType = 0x24,
        .SubType         = 0x00,
        .Data            = {0x01, 0x10},
    },

    .CDC_Functional_CallManagement = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(2),
        .bDescriptorType = 0x24,
        .SubType         = 0x01,
        .Data            = {0x03, DCI_INTERFACE_NUMBER},
    },

    .CDC_Functional_ACM = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(1),
        .bDescriptorType = 0x24,
        .SubType         = 0x02,
        .Data            = {0x06},
    },

    .CDC_Functional_Union = {
        .bLength         = CDC_FUNCTIONAL_DESCRIPTOR_SIZE(2),
        .bDescriptorType = 0x24,
        .SubType         = 0x06,
        .Data            = {CCI_INTERFACE_NUMBER, DCI_INTERFACE_NUMBER},
    },

    .ManagementEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN |
                             USB_CDCACM_MANAGEMENT_ENDP),
        .bmAttributes     = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize   = USB_CDCACM_MANAGEMENT_EPSIZE,
        .bInterval        = 0xFF,
    },

    .DCI_Interface = {
        .bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = DCI_INTERFACE_NUMBER,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x02,
        .bInterfaceClass    = USB_INTERFACE_CLASS_DIC,
        .bInterfaceSubClass = 0x00, /* None */
        .bInterfaceProtocol = 0x00, /* None */
        .iInterface         = 0x00,
    },

    .DataOutEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_OUT |
                             USB_CDCACM_RX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_CDCACM_RX_EPSIZE,
        .bInterval        = 0x00,
    },

    .DataInEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USB_CDCACM_TX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_CDCACM_TX_EPSIZE,
        .bInterval        = 0x00,
    },
    
    //HID
    
	#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. HID part of the descriptor
    
	.HID_Interface = {
		.bLength            = sizeof(usb_descriptor_interface),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_INTERFACE,
        .bInterfaceNumber   = HID_INTERFACE_NUMBER,
        .bAlternateSetting  = 0x00,
        .bNumEndpoints      = 0x02,
        .bInterfaceClass    = USB_INTERFACE_CLASS_HID,
        .bInterfaceSubClass = USB_INTERFACE_SUBCLASS_HID,
        .bInterfaceProtocol = 0x00, /* Common AT Commands */
        .iInterface         = 0x04,
	},
	.HID_Descriptor = {
		.len				= 9,//sizeof(HIDDescDescriptor),
		.dtype				= HID_DESCRIPTOR_TYPE,
		.versionL			= 0x10,
		.versionH			= 0x01,
		.country			= 0x00,
		.numDesc			= 0x01,
		.desctype			= REPORT_DESCRIPTOR,//0x22,
		.descLenL			= sizeof(hid_report_descriptor),
		.descLenH			= 0x00,
	},
	.HIDDataInEndpoint = {
		.bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_IN | USB_HID_TX_ENDP),//0x81,//USB_HID_TX_ADDR,
        .bmAttributes     = USB_ENDPOINT_TYPE_INTERRUPT,
        .wMaxPacketSize   = USB_HID_TX_EPSIZE,//0x40,//big enough for a keyboard 9 byte packet and for a mouse 5 byte packet
        .bInterval        = 0x0A,
	},
    .HIDDataOutEndpoint = {
        .bLength          = sizeof(usb_descriptor_endpoint),
        .bDescriptorType  = USB_DESCRIPTOR_TYPE_ENDPOINT,
        .bEndpointAddress = (USB_DESCRIPTOR_ENDPOINT_OUT | USB_HID_RX_ENDP),
        .bmAttributes     = USB_EP_TYPE_BULK,
        .wMaxPacketSize   = USB_HID_RX_EPSIZE,
        .bInterval        = 0x00,
    },
    #endif
};

/*
  String Descriptors:

  we may choose to specify any or none of the following string
  identifiers:

  iManufacturer:    LeafLabs
  iProduct:         Maple
  iSerialNumber:    NONE
  iConfiguration:   NONE
  iInterface(CCI):  NONE
  iInterface(DCI):  NONE

*/

/* Unicode language identifier: 0x0409 is US English */
/* FIXME move to Wirish */
static const usb_descriptor_string usbCompositeDescriptor_LangID = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(1),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {0x09, 0x04},
};

/* FIXME move to Wirish */
static const usb_descriptor_string usbCompositeDescriptor_iManufacturer = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(8),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'L', 0, 'e', 0, 'a', 0, 'f', 0,
                'L', 0, 'a', 0, 'b', 0, 's', 0},
};

/* FIXME move to Wirish */
static const usb_descriptor_string usbCompositeDescriptor_iProduct = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(5),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'M', 0, 'a', 0, 'p', 0, 'l', 0, 'e', 0},
};

/* FIXME move to Wirish */
static const usb_descriptor_string usbVcomDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(3),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'V', 0, 'C', 0, 'O', 0, 'M', 0},
};

/* FIXME move to Wirish */
static const usb_descriptor_string usbHIDDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(3),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'H', 0, 'I', 0, 'D', 0},
};

/* FIXME move to Wirish */
static const usb_descriptor_string usbCompositeDescriptor_iInterface = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(3),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'C', 0, 'O', 0, 'M', 0, 'P', 0, 'O', 0, 'S', 0, 'I', 0, 'T', 0, 'E', 0},
};

static ONE_DESCRIPTOR Device_Descriptor = {
    (uint8*)&usbCompositeDescriptor_Device,
    sizeof(usb_descriptor_device)
};

static ONE_DESCRIPTOR Config_Descriptor = {
    (uint8*)&usbCompositeDescriptor_Config,
    sizeof(usb_descriptor_config)
};

#define N_STRING_DESCRIPTORS 3
static ONE_DESCRIPTOR String_Descriptor[N_STRING_DESCRIPTORS] = {
    {(uint8*)&usbCompositeDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1)},
    {(uint8*)&usbCompositeDescriptor_iManufacturer,USB_DESCRIPTOR_STRING_LEN(8)},
    {(uint8*)&usbCompositeDescriptor_iProduct,     USB_DESCRIPTOR_STRING_LEN(5)},
};

/*
 * Etc.
 */

/* I/O state */

#define CDC_SERIAL_RX_BUFFER_SIZE	256 // must be power of 2
#define CDC_SERIAL_RX_BUFFER_SIZE_MASK (CDC_SERIAL_RX_BUFFER_SIZE-1)

/* Received data */
static volatile uint8 vcomBufferRx[CDC_SERIAL_RX_BUFFER_SIZE];
/* Write index to vcomBufferRx */
static volatile uint32 vcom_rx_head;
/* Read index from vcomBufferRx */
static volatile uint32 vcom_rx_tail;

#define CDC_SERIAL_TX_BUFFER_SIZE	256 // must be power of 2
#define CDC_SERIAL_TX_BUFFER_SIZE_MASK (CDC_SERIAL_TX_BUFFER_SIZE-1)
// Tx data
static volatile uint8 vcomBufferTx[CDC_SERIAL_TX_BUFFER_SIZE];
// Write index to vcomBufferTx
static volatile uint32 vcom_tx_head;
// Read index from vcomBufferTx
static volatile uint32 vcom_tx_tail;



/* Other state (line coding, DTR/RTS) */

static volatile usb_cdcacm_line_coding line_coding = {
    /* This default is 115200 baud, 8N1. */
    .dwDTERate   = 115200,
    .bCharFormat = USB_CDCACM_STOP_BITS_1,
    .bParityType = USB_CDCACM_PARITY_NONE,
    .bDataBits   = 8,
};

/* DTR in bit 0, RTS in bit 1. */
static volatile uint8 line_dtr_rts = 0;

/*
 * Endpoint callbacks
 */

#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. if usb type includes HID, then the HID rx and tx callbacks are added
static void (*ep_int_in[7])(void) =
    {vcomDataTxCb,
     NOP_Process,
     NOP_Process,
     hidDataTxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process};

static void (*ep_int_out[7])(void) =
    {NOP_Process,
     NOP_Process,
     vcomDataRxCb,
     NOP_Process,
     hidDataRxCb,
     NOP_Process,
     NOP_Process};
#else
static void (*ep_int_in[7])(void) =
    {vcomDataTxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

static void (*ep_int_out[7])(void) =
    {NOP_Process,
     NOP_Process,
     vcomDataRxCb,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};
#endif

/*
 * Globals required by usb_lib/
 *
 * Mark these weak so they can be overriden to implement other USB
 * functionality.
 */

#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. number of endpoints are set according to the usb type
#define NUM_ENDPTS				  0x06
#else
#define NUM_ENDPTS                0x04
#endif

__weak DEVICE Device_Table = {
    .Total_Endpoint      = NUM_ENDPTS,
    .Total_Configuration = 1
};

#define MAX_PACKET_SIZE            0x40  /* 64B, maximum for USB FS Devices */
__weak DEVICE_PROP Device_Property = {
    .Init                        = usbInit,
    .Reset                       = usbReset,
    .Process_Status_IN           = NOP_Process,
    .Process_Status_OUT          = NOP_Process,
    .Class_Data_Setup            = usbDataSetup,
    .Class_NoData_Setup          = usbNoDataSetup,
    .Class_Get_Interface_Setting = usbGetInterfaceSetting,
    .GetDeviceDescriptor         = usbGetDeviceDescriptor,
    .GetConfigDescriptor         = usbGetConfigDescriptor,
    .GetStringDescriptor         = usbGetStringDescriptor,
    .RxEP_buffer                 = NULL,
    .MaxPacketSize               = MAX_PACKET_SIZE
};

__weak USER_STANDARD_REQUESTS User_Standard_Requests = {
    .User_GetConfiguration   = NOP_Process,
    .User_SetConfiguration   = usbSetConfiguration,
    .User_GetInterface       = NOP_Process,
    .User_SetInterface       = NOP_Process,
    .User_GetStatus          = NOP_Process,
    .User_ClearFeature       = NOP_Process,
    .User_SetEndPointFeature = NOP_Process,
    .User_SetDeviceFeature   = NOP_Process,
    .User_SetDeviceAddress   = usbSetDeviceAddress
};

/*
 * User hooks
 */

static void (*rx_hook)(unsigned, void*) = 0;
static void (*iface_setup_hook)(unsigned, void*) = 0;

void usb_cdcacm_set_hooks(unsigned hook_flags, void (*hook)(unsigned, void*)) {
    if (hook_flags & USB_CDCACM_HOOK_RX) {
        rx_hook = hook;
    }
    if (hook_flags & USB_CDCACM_HOOK_IFACE_SETUP) {
        iface_setup_hook = hook;
    }
}

/*
 * CDC ACM interface
 */

void usb_enable(gpio_dev *disc_dev, uint8 disc_bit) {
    /* Present ourselves to the host. Writing 0 to "disc" pin must
     * pull USB_DP pin up while leaving USB_DM pulled down by the
     * transceiver. See USB 2.0 spec, section 7.1.7.3. */
    gpio_set_mode(disc_dev, disc_bit, GPIO_OUTPUT_PP);
    gpio_write_bit(disc_dev, disc_bit, 0);

    /* Initialize the USB peripheral. */
    usb_init_usblib(USBLIB, ep_int_in, ep_int_out);
}

void usb_disable(gpio_dev *disc_dev, uint8 disc_bit) {
    /* Turn off the interrupt and signal disconnect (see e.g. USB 2.0
     * spec, section 7.1.7.3). */
    nvic_irq_disable(NVIC_USB_LP_CAN_RX0);
    gpio_write_bit(disc_dev, disc_bit, 1);
}

void usb_cdcacm_putc(char ch) {
    while (!usb_cdcacm_tx((uint8*)&ch, 1))
        ;
}

/* This function is non-blocking.
 *
 * It copies data from a user buffer into the USB peripheral TX
 * buffer, and returns the number of bytes copied. */
uint32 usb_cdcacm_tx(const uint8* buf, uint32 len)
{
	if (len==0) return 0; // no data to send

	uint32 head = vcom_tx_head; // load volatile variable
	uint32 tx_unsent = (head - vcom_tx_tail) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;

    // We can only put bytes in the buffer if there is place
    if (len > (CDC_SERIAL_TX_BUFFER_SIZE-tx_unsent-1) ) {
        len = (CDC_SERIAL_TX_BUFFER_SIZE-tx_unsent-1);
    }
	if (len==0) return 0; // buffer full

	uint16 i;
	// copy data from user buffer to USB Tx buffer
	for (i=0; i<len; i++) {
		vcomBufferTx[head] = buf[i];
		head = (head+1) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
	}
	vcom_tx_head = head; // store volatile variable
	
	while(transmitting >= 0);
	
	if (transmitting<0) {
		vcomDataTxCb(); // initiate data transmission
	}

    return len;
}



uint32 usb_cdcacm_data_available(void) {
    return (vcom_rx_head - vcom_rx_tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
}

uint8 usb_is_transmitting(void) {
    return transmitting;
}

uint16 usb_cdcacm_get_pending(void) {
    return (vcom_tx_head - vcom_tx_tail) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
}

/* Non-blocking byte receive.
 *
 * Copies up to len bytes from our private data buffer (*NOT* the PMA)
 * into buf and deq's the FIFO. */
uint32 usb_cdcacm_rx(uint8* buf, uint32 len)
{
    /* Copy bytes to buffer. */
    uint32 n_copied = usb_cdcacm_peek(buf, len);

    /* Mark bytes as read. */
	uint16 tail = vcom_rx_tail; // load volatile variable
	tail = (tail + n_copied) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
	vcom_rx_tail = tail; // store volatile variable

	uint32 rx_unread = (vcom_rx_head - tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
    // If buffer was emptied to a pre-set value, re-enable the RX endpoint
    if ( rx_unread <= 64 ) { // experimental value, gives the best performance
        usb_set_ep_rx_stat(USB_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);
	}
    return n_copied;
}

/* Non-blocking byte lookahead.
 *
 * Looks at unread bytes without marking them as read. */
uint32 usb_cdcacm_peek(uint8* buf, uint32 len)
{
    int i;
    uint32 tail = vcom_rx_tail;
	uint32 rx_unread = (vcom_rx_head-tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;

    if (len > rx_unread) {
        len = rx_unread;
    }

    for (i = 0; i < len; i++) {
        buf[i] = vcomBufferRx[tail];
        tail = (tail + 1) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
    }

    return len;
}

uint32 usb_cdcacm_peek_ex(uint8* buf, uint32 offset, uint32 len)
{
    int i;
    uint32 tail = (vcom_rx_tail + offset) & CDC_SERIAL_RX_BUFFER_SIZE_MASK ;
	uint32 rx_unread = (vcom_rx_head-tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;

    if (len + offset > rx_unread) {
        len = rx_unread - offset;
    }

    for (i = 0; i < len; i++) {
        buf[i] = vcomBufferRx[tail];
        tail = (tail + 1) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
    }

    return len;
}

/* Roger Clark. Added. for Arduino 1.0 API support of Serial.peek() */
int usb_cdcacm_peek_char() 
{
    if (usb_cdcacm_data_available() == 0) 
	{
		return -1;
    }

    return vcomBufferRx[vcom_rx_tail];
}

uint8 usb_cdcacm_get_dtr() {
    return ((line_dtr_rts & USB_CDCACM_CONTROL_LINE_DTR) != 0);
}

uint8 usb_cdcacm_get_rts() {
    return ((line_dtr_rts & USB_CDCACM_CONTROL_LINE_RTS) != 0);
}

void usb_cdcacm_get_line_coding(usb_cdcacm_line_coding *ret) {
    ret->dwDTERate = line_coding.dwDTERate;
    ret->bCharFormat = line_coding.bCharFormat;
    ret->bParityType = line_coding.bParityType;
    ret->bDataBits = line_coding.bDataBits;
}

int usb_cdcacm_get_baud(void) {
    return line_coding.dwDTERate;
}

int usb_cdcacm_get_stop_bits(void) {
    return line_coding.bCharFormat;
}

int usb_cdcacm_get_parity(void) {
    return line_coding.bParityType;
}

int usb_cdcacm_get_n_data_bits(void) {
    return line_coding.bDataBits;
}

/*
 * Callbacks
 */
static void vcomDataTxCb(void)
{
	uint32 tail = vcom_tx_tail; // load volatile variable
	uint32 tx_unsent = (vcom_tx_head - tail) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
	if (tx_unsent==0) {
		if ( (--transmitting)==0) goto flush_vcom; // no more data to send
		return; // it was already flushed, keep Tx endpoint disabled
	}
	transmitting = 1;
    // We can only send up to USB_CDCACM_TX_EPSIZE bytes in the endpoint.
    if (tx_unsent > USB_CDCACM_TX_EPSIZE) {
        tx_unsent = USB_CDCACM_TX_EPSIZE;
    }
	// copy the bytes from USB Tx buffer to PMA buffer
	uint32 *dst = usb_pma_ptr(USB_CDCACM_TX_ADDR);
    uint16 tmp = 0;
	uint16 val;
	int i;
	for (i = 0; i < tx_unsent; i++) {
		val = vcomBufferTx[tail];
		tail = (tail + 1) & CDC_SERIAL_TX_BUFFER_SIZE_MASK;
		if (i&1) {
			*dst++ = tmp | (val<<8);
		} else {
			tmp = val;
		}
	}
    if ( tx_unsent&1 ) {
        *dst = tmp;
    }
	vcom_tx_tail = tail; // store volatile variable
flush_vcom:
	// enable Tx endpoint
    usb_set_ep_tx_count(USB_CDCACM_TX_ENDP, tx_unsent);
    usb_set_ep_tx_stat(USB_CDCACM_TX_ENDP, USB_EP_STAT_TX_VALID);
}


static void vcomDataRxCb(void)
{
	uint32 head = vcom_rx_head; // load volatile variable

	uint32 ep_rx_size = usb_get_ep_rx_count(USB_CDCACM_RX_ENDP);
	// This copy won't overwrite unread bytes as long as there is 
	// enough room in the USB Rx buffer for next packet
	uint32 *src = usb_pma_ptr(USB_CDCACM_RX_ADDR);
    uint16 tmp = 0;
	uint8 val;
	uint32 i;
	for (i = 0; i < ep_rx_size; i++) {
		if (i&1) {
			val = tmp>>8;
		} else {
			tmp = *src++;
			val = tmp&0xFF;
		}
		vcomBufferRx[head] = val;
		head = (head + 1) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
	}
	vcom_rx_head = head; // store volatile variable

	uint32 rx_unread = (head - vcom_rx_tail) & CDC_SERIAL_RX_BUFFER_SIZE_MASK;
	// only enable further Rx if there is enough room to receive one more packet
	if ( rx_unread < (CDC_SERIAL_RX_BUFFER_SIZE-USB_CDCACM_RX_EPSIZE) ) {
		usb_set_ep_rx_stat(USB_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);
	}

    if (rx_hook) {
        rx_hook(USB_CDCACM_HOOK_RX, 0);
    }
}

static uint8* vcomGetSetLineCoding(uint16 length) {
    if (length == 0) {
        pInformation->Ctrl_Info.Usb_wLength = sizeof(struct usb_cdcacm_line_coding);
    }
    return (uint8*)&line_coding;
}

static void usbInit(void) {
    pInformation->Current_Configuration = 0;

    USB_BASE->CNTR = USB_CNTR_FRES;

    USBLIB->irq_mask = 0;
    USB_BASE->CNTR = USBLIB->irq_mask;
    USB_BASE->ISTR = 0;
    USBLIB->irq_mask = USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
    USB_BASE->CNTR = USBLIB->irq_mask;

    USB_BASE->ISTR = 0;
    USBLIB->irq_mask = USB_ISR_MSK;
    USB_BASE->CNTR = USBLIB->irq_mask;

    nvic_irq_enable(NVIC_USB_LP_CAN_RX0);
    USBLIB->state = USB_UNCONNECTED;
}

#define BTABLE_ADDRESS        0x00
static void usbReset(void) {
    pInformation->Current_Configuration = 0;

    /* current feature is current bmAttributes */
    pInformation->Current_Feature = (USB_CONFIG_ATTR_BUSPOWERED |
                                     USB_CONFIG_ATTR_SELF_POWERED);

    USB_BASE->BTABLE = BTABLE_ADDRESS;

    /* setup control endpoint 0 */
    usb_set_ep_type(USB_EP0, USB_EP_EP_TYPE_CONTROL);
    usb_set_ep_tx_stat(USB_EP0, USB_EP_STAT_TX_STALL);
    usb_set_ep_rx_addr(USB_EP0, USB_CDCACM_CTRL_RX_ADDR);
    usb_set_ep_tx_addr(USB_EP0, USB_CDCACM_CTRL_TX_ADDR);
    usb_clear_status_out(USB_EP0);

    usb_set_ep_rx_count(USB_EP0, pProperty->MaxPacketSize);
    usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);

    /* setup management endpoint 1  */
    usb_set_ep_type(USB_CDCACM_MANAGEMENT_ENDP, USB_EP_EP_TYPE_INTERRUPT);
    usb_set_ep_tx_addr(USB_CDCACM_MANAGEMENT_ENDP,
                       USB_CDCACM_MANAGEMENT_ADDR);
    usb_set_ep_tx_stat(USB_CDCACM_MANAGEMENT_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_CDCACM_MANAGEMENT_ENDP, USB_EP_STAT_RX_DISABLED);

    /* TODO figure out differences in style between RX/TX EP setup */

    /* set up data endpoint OUT (RX) */
    usb_set_ep_type(USB_CDCACM_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_CDCACM_RX_ENDP, USB_CDCACM_RX_ADDR);
    usb_set_ep_rx_count(USB_CDCACM_RX_ENDP, USB_CDCACM_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_CDCACM_RX_ENDP, USB_EP_STAT_RX_VALID);

    /* set up data endpoint IN (TX)  */
    usb_set_ep_type(USB_CDCACM_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_CDCACM_TX_ENDP, USB_CDCACM_TX_ADDR);
    usb_set_ep_tx_stat(USB_CDCACM_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_CDCACM_TX_ENDP, USB_EP_STAT_RX_DISABLED);
	
	//HID
#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. sets hid endpoint types and addresses
	
    /* set up hid endpoint OUT (RX) */
    usb_set_ep_type(USB_HID_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_HID_RX_ENDP, USB_HID_RX_ADDR);
    usb_set_ep_rx_count(USB_HID_RX_ENDP, USB_HID_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_HID_RX_ENDP, USB_EP_STAT_RX_VALID);

    /* set up hid endpoint IN (TX)  */
    usb_set_ep_type(USB_HID_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_HID_TX_ENDP, USB_HID_TX_ADDR);
    usb_set_ep_tx_stat(USB_HID_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_HID_TX_ENDP, USB_EP_STAT_RX_DISABLED);
    
#endif

    USBLIB->state = USB_ATTACHED;
    SetDeviceAddress(0);

    /* Reset the RX/TX state */
    
    //VCOM
	vcom_rx_head = 0;
	vcom_rx_tail = 0;
	vcom_tx_head = 0;
	vcom_tx_tail = 0;
    
	transmitting = -1;
    
	//HID
#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. resets hid variables
	hid_rx_head = 0;
	hid_rx_tail = 0;
	hid_tx_head = 0;
	hid_tx_tail = 0;
#endif
}

static RESULT usbDataSetup(uint8 request) {
    uint8* (*CopyRoutine)(uint16) = 0;

    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
        switch (request) {
        case USB_CDCACM_GET_LINE_CODING:
            CopyRoutine = vcomGetSetLineCoding;
            break;
        case USB_CDCACM_SET_LINE_CODING:
            CopyRoutine = vcomGetSetLineCoding;
            break;
        default:
            break;
        }

        /* Call the user hook. */
        if (iface_setup_hook) {
            uint8 req_copy = request;
            iface_setup_hook(USB_CDCACM_HOOK_IFACE_SETUP, &req_copy);
        }
    }
	
#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. sets CopyRoutine depending on the HID type of request
	if(Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT)){
    	switch (request){
    		case GET_DESCRIPTOR:
    			if(pInformation->USBwIndex0 == HID_INTERFACE_NUMBER){
						
					if (pInformation->USBwValue1 == REPORT_DESCRIPTOR){
						CopyRoutine = usbGetHIDReportDescriptor;
					} else if (pInformation->USBwValue1 == HID_DESCRIPTOR_TYPE){
						CopyRoutine = usbGetConfigDescriptor;
					}
					
				}
    			break;
    		case GET_PROTOCOL:
    			CopyRoutine = usbGetProtocolValue;
    			break;
		}
	}
#endif

    if (CopyRoutine == NULL) {
        return USB_UNSUPPORT;
    }

    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*CopyRoutine)(0);
    return USB_SUCCESS;
}

static RESULT usbNoDataSetup(uint8 request) {
    RESULT ret = USB_UNSUPPORT;

    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
        switch (request) {
	        case USB_CDCACM_SET_COMM_FEATURE:
	            /* We support set comm. feature, but don't handle it. */
	            ret = USB_SUCCESS;
	            break;
	        case USB_CDCACM_SET_CONTROL_LINE_STATE:
	            /* Track changes to DTR and RTS. */
	            line_dtr_rts = (pInformation->USBwValues.bw.bb0 &
	                            (USB_CDCACM_CONTROL_LINE_DTR |
	                             USB_CDCACM_CONTROL_LINE_RTS));
	            ret = USB_SUCCESS;
	            break;
#if defined(USB_HID_KMJ) || defined(USB_HID_KM) || defined(USB_HID_J) // Libarra. sets the protocol value for the HID
	        case SET_PROTOCOL:
	            //uint8 wValue0 = pInformation->USBwValue0;
				//ProtocolValue = wValue0;
				ProtocolValue = pInformation->USBwValue0;
	            ret = USB_SUCCESS;
	            break;
#endif
        }

        /* Call the user hook. */
        if (iface_setup_hook) {
            uint8 req_copy = request;
            iface_setup_hook(USB_CDCACM_HOOK_IFACE_SETUP, &req_copy);
        }
    }
    return ret;
}

static RESULT usbGetInterfaceSetting(uint8 interface, uint8 alt_setting) {
    if (alt_setting > 0) {
        return USB_UNSUPPORT;
    } else if (interface > 1) {
        return USB_UNSUPPORT;
    }

    return USB_SUCCESS;
}

static uint8* usbGetDeviceDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &Device_Descriptor);
}

static uint8* usbGetConfigDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &Config_Descriptor);
}

static uint8* usbGetStringDescriptor(uint16 length) {
    uint8 wValue0 = pInformation->USBwValue0;

    if (wValue0 > N_STRING_DESCRIPTORS) {
        return NULL;
    }
    return Standard_GetDescriptorData(length, &String_Descriptor[wValue0]);
}

static void usbSetConfiguration(void) {
    if (pInformation->Current_Configuration != 0) {
        USBLIB->state = USB_CONFIGURED;
    }
}

static void usbSetDeviceAddress(void) {
    USBLIB->state = USB_ADDRESSED;
}

#endif // NO_USB
