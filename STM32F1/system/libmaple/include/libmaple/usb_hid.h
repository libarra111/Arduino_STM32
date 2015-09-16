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
 * @file libmaple/include/libmaple/usb_hid.h
 * @brief USB HID (human interface device) support
 *
 * IMPORTANT: this API is unstable, and may change without notice.
 */

#ifdef USB_HID
 
#ifndef _LIBMAPLE_USB_HID_H_
#define _LIBMAPLE_USB_HID_H_

#include <libmaple/libmaple_types.h>
#include <libmaple/gpio.h>
#include <libmaple/usb.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Descriptors, etc.
 */
 

const uint8_t hid_report_descriptor[] = {
	//	Mouse
    0x05, 0x01,						// USAGE_PAGE (Generic Desktop)	// 54
    0x09, 0x02,						// USAGE (Mouse)
    0xa1, 0x01,						// COLLECTION (Application)
    0x09, 0x01,						//   USAGE (Pointer)
    0xa1, 0x00,						//   COLLECTION (Physical)
    0x85, 0x01,						//     REPORT_ID (1)
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

typedef enum _HID_REQUESTS
{
 
  GET_REPORT = 1,
  GET_IDLE,
  GET_PROTOCOL,
 
  SET_REPORT = 9,
  SET_IDLE,
  SET_PROTOCOL
 
} HID_REQUESTS;

#define USB_ENDPOINT_IN(addr)           ((addr) | 0x80)
#define HID_ENDPOINT_INT 				1
#define USB_ENDPOINT_TYPE_INTERRUPT     0x03
 
#define HID_DESCRIPTOR_TYPE             0x21
 
#define REPORT_DESCRIPTOR               0x22


typedef struct
{
	uint8_t len;			// 9
	uint8_t dtype;			// 0x21
	uint8_t	versionL;		// 0x101
	uint8_t	versionH;		// 0x101
	uint8_t	country;
	uint8_t	numDesc;
	uint8_t	desctype;		// 0x22 report
	uint8_t	descLenL;
	uint8_t	descLenH;
} HIDDescriptor;

#define USB_DEVICE_CLASS_HID              0x00
#define USB_DEVICE_SUBCLASS_HID           0x00
#define USB_INTERFACE_CLASS_HID           0x03
#define USB_INTERFACE_SUBCLASS_HID		  0x00
#define USB_INTERFACE_CLASS_DIC           0x0A

/*
 * Endpoint configuration
 */

#define USB_HID_CTRL_ENDP            0
#define USB_HID_CTRL_RX_ADDR         0x40
#define USB_HID_CTRL_TX_ADDR         0x80
#define USB_HID_CTRL_EPSIZE          0x40

#define USB_HID_TX_ENDP              1
#define USB_HID_TX_ADDR              0xC0
#define USB_HID_TX_EPSIZE            0x40

#define USB_HID_RX_ENDP              2
#define USB_HID_RX_ADDR              0x100
#define USB_HID_RX_EPSIZE            0x40

#ifndef __cplusplus
#define USB_HID_DECLARE_DEV_DESC(vid, pid)                           \
  {                                                                     \
      .bLength            = sizeof(usb_descriptor_device),              \
      .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE,                 \
      .bcdUSB             = 0x0200,                                     \
      .bDeviceClass       = USB_DEVICE_CLASS_HID,                       \
      .bDeviceSubClass    = USB_DEVICE_SUBCLASS_HID,                    \
      .bDeviceProtocol    = 0x00,                                       \
      .bMaxPacketSize0    = 0x40,                                       \
      .idVendor           = vid,                                        \
      .idProduct          = pid,                                        \
      .bcdDevice          = 0x0200,                                     \
      .iManufacturer      = 0x01,                                       \
      .iProduct           = 0x02,                                       \
      .iSerialNumber      = 0x00,                                       \
      .bNumConfigurations = 0x01,                                       \
 }
#endif

/*
 * HID interface
 */

void usb_hid_enable(gpio_dev*, uint8);
void usb_hid_disable(gpio_dev*, uint8);

void   usb_hid_putc(char ch);
uint32 usb_hid_tx(const uint8* buf, uint32 len);
uint32 usb_hid_tx_mod(const uint8* buf, uint32 len);
uint32 usb_hid_rx(uint8* buf, uint32 len);

uint32 usb_hid_data_available(void); /* in RX buffer */
uint16 usb_hid_get_pending(void);
uint8 usb_hid_is_transmitting(void);

void	HID_SendReport(uint8_t id, const void* data, uint32_t len);

//static RESULT HID_SetProtocol(void);
static uint8 *HID_GetProtocolValue(uint16 Length);
static uint8 *HID_GetReportDescriptor(uint16 Length);
static uint8 *HID_GetHIDDescriptor(uint16 Length);

#ifdef __cplusplus
}
#endif

#endif

#endif
