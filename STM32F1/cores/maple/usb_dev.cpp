/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
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

/* Copyright (c) 2011, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2013 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @brief USB Composite Device
 */

#ifndef NO_USB

#include "usb_dev.h"

#include "string.h"
#include "stdint.h"

#include <libmaple/nvic.h>
#include <libmaple/usb_device.h>
#include <libmaple/usb.h>
#include <libmaple/iwdg.h>

#include "wirish.h"

/*
 * Hooks used for bootloader reset signalling
 */

#if BOARD_HAVE_SERIALUSB
static void rxHook(unsigned, void*);
static void ifaceSetupHook(unsigned, void*);
#endif

/*
 * USB Device
 */
 
// Libarra. USBDevice is the class used to initialize the device.
USBDevice::USBDevice(void){
	
}

void USBDevice::begin(void){
	if(!enabled){
		usb_enable(BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT);
#if BOARD_HAVE_SERIALUSB
		usb_cdcacm_set_hooks(USB_CDCACM_HOOK_RX, rxHook);
		usb_cdcacm_set_hooks(USB_CDCACM_HOOK_IFACE_SETUP, ifaceSetupHook);
#endif
		enabled = true;
	}
}

void USBDevice::end(void){
	if(enabled){
	    usb_disable(BOARD_USB_DISC_DEV, BOARD_USB_DISC_BIT);
#if BOARD_HAVE_SERIALUSB
		usb_cdcacm_remove_hooks(USB_CDCACM_HOOK_RX | USB_CDCACM_HOOK_IFACE_SETUP);
#endif
		enabled = false;
	}
}

/*
 * USB HID interface
 */
 
#if defined(USB_HID_KMJ) || defined(USB_HID_KM)

//================================================================================
//================================================================================
//	Mouse

HIDMouse::HIDMouse(void) : _buttons(0){
}

void HIDMouse::begin(void){
	USBDev.begin();
}

void HIDMouse::end(void){
	USBDev.end();
}

void HIDMouse::click(uint8_t b)
{
	_buttons = b;
	move(0,0,0);
	_buttons = 0;
	move(0,0,0);
}

void HIDMouse::move(signed char x, signed char y, signed char wheel)
{
	uint8_t m[4];
	m[0] = _buttons;
	m[1] = x;
	m[2] = y;
	m[3] = wheel;
	
	uint8_t buf[1+sizeof(m)];
	buf[0] = 1;//report id
	uint8_t i;
	for(i=0;i<sizeof(m);i++){
		buf[i+1] = m[i];
	}
	
	usb_hid_tx(buf, sizeof(buf));
	
	/* flush out to avoid having the pc wait for more data */
	usb_hid_tx(NULL, 0);
}

void HIDMouse::buttons(uint8_t b)
{
	if (b != _buttons)
	{
		_buttons = b;
		move(0,0,0);
	}
}

void HIDMouse::press(uint8_t b)
{
	buttons(_buttons | b);
}

void HIDMouse::release(uint8_t b)
{
	buttons(_buttons & ~b);
}

bool HIDMouse::isPressed(uint8_t b)
{
	if ((b & _buttons) > 0)
		return true;
	return false;
}



//================================================================================
//================================================================================
//	Keyboard

HIDKeyboard::HIDKeyboard(void){
	
}

void HIDKeyboard::sendReport(KeyReport* keys)
{
	//HID_SendReport(2,keys,sizeof(KeyReport));
	uint8_t buf[9];//sizeof(_keyReport)+1;
	buf[0] = 2; //report id;
	buf[1] = _keyReport.modifiers;
	buf[2] = _keyReport.reserved;
	
	uint8_t i;
	for(i=0;i<sizeof(_keyReport.keys);i++){
		buf[i+3] = _keyReport.keys[i];
	}
	usb_hid_tx(buf, sizeof(buf));
	
	/* flush out to avoid having the pc wait for more data */
	usb_hid_tx(NULL, 0);
}

void HIDKeyboard::begin(void){
	USBDev.begin();
}

void HIDKeyboard::end(void) {
	USBDev.end();
}

size_t HIDKeyboard::press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			//setWriteError();
			return 0;
		}
		if (k & SHIFT) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			//setWriteError();
			return 0;
		}
	}
	
	sendReport(&_keyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t HIDKeyboard::release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = _asciimap[k];
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}
	
	sendReport(&_keyReport);
	return 1;
}

void HIDKeyboard::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
	
	sendReport(&_keyReport);
}

size_t HIDKeyboard::write(uint8_t c)
{
	uint8_t p = 0;

	p = press(c);	// Keydown
	release(c);		// Keyup
	
	return (p);		// Just return the result of press() since release() almost always returns 1
}

#endif //keyboard & mouse

#if defined(USB_HID_KMJ) || defined(USB_HID_J)

//================================================================================
//================================================================================
//	Joystick

void HIDJoystick::sendReport(void){
	usb_hid_tx(joystick_Report, sizeof(joystick_Report));
	
	/* flush out to avoid having the pc wait for more data */
	usb_hid_tx(NULL, 0);
}

HIDJoystick::HIDJoystick(void){
	
}

void HIDJoystick::begin(void){
	USBDev.begin();
}

void HIDJoystick::end(void){
	USBDev.end();
}

void HIDJoystick::button(uint8_t button, bool val){
	button--;
	uint8_t mask = (1 << (button & 7));
	if (val) {
		if (button < 8) joystick_Report[1] |= mask;
		else if (button < 16) joystick_Report[2] |= mask;
		else if (button < 24) joystick_Report[3] |= mask;
		else if (button < 32) joystick_Report[4] |= mask;
	} else {
		mask = ~mask;
		if (button < 8) joystick_Report[1] &= mask;
		else if (button < 16) joystick_Report[2] &= mask;
		else if (button < 24) joystick_Report[3] &= mask;
		else if (button < 32) joystick_Report[4] &= mask;
	}
	
	
	sendReport();
}

void HIDJoystick::X(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[5] = (joystick_Report[5] & 0x0F) | (val << 4);
	joystick_Report[6] = (joystick_Report[6] & 0xC0) | (val >> 4);
	
	
	sendReport();
}

void HIDJoystick::Y(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[6] = (joystick_Report[6] & 0x3F) | (val << 6);
	joystick_Report[7] = (val >> 2);
	
	
	sendReport();
}

void HIDJoystick::position(uint16_t x, uint16_t y){
	if (x > 1023) x = 1023;
	if (y > 1023) y = 1023;
	joystick_Report[5] = (joystick_Report[5] & 0x0F) | (x << 4);
	joystick_Report[6] = (x >> 4) | (y << 6);
	joystick_Report[7] = (y >> 2);
	
	
	sendReport();
}

void HIDJoystick::Z(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[8] = val;
	joystick_Report[9] = (joystick_Report[9] & 0xFC) | (val >> 8);
	
	
	sendReport();
}

void HIDJoystick::Zrotate(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[9] = (joystick_Report[9] & 0x03) | (val << 2);
	joystick_Report[10] = (joystick_Report[10] & 0xF0) | (val >> 6);
	
	
	sendReport();
}

void HIDJoystick::sliderLeft(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[10] = (joystick_Report[10] & 0x0F) | (val << 4);
	joystick_Report[11] = (joystick_Report[11] & 0xC0) | (val >> 4);
	
	
	sendReport();
}

void HIDJoystick::sliderRight(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[11] = (joystick_Report[11] & 0x3F) | (val << 6);
	joystick_Report[12] = (val >> 2);
	
	
	sendReport();
}

void HIDJoystick::slider(uint16_t val){
	if (val > 1023) val = 1023;
	joystick_Report[10] = (joystick_Report[10] & 0x0F) | (val << 4);
	joystick_Report[11] = (val >> 4) | (val << 6);
	joystick_Report[12] = (val >> 2);
	
	
	sendReport();
}

void HIDJoystick::hat(int16_t dir){
	uint8_t val;
	if (dir < 0) val = 15;
	else if (dir < 23) val = 0;
	else if (dir < 68) val = 1;
	else if (dir < 113) val = 2;
	else if (dir < 158) val = 3;
	else if (dir < 203) val = 4;
	else if (dir < 245) val = 5;
	else if (dir < 293) val = 6;
	else if (dir < 338) val = 7;
	joystick_Report[5] = (joystick_Report[5] & 0xF0) | val;
	
	
	sendReport();
}

#endif // joystic


/*
 * USBSerial interface
 */

#define USB_TIMEOUT 50

USBSerial::USBSerial(void) {
#if !BOARD_HAVE_SERIALUSB
    ASSERT(0);
#endif
}

void USBSerial::begin(void) {
	USBDev.begin();
}

//Roger Clark. Two new begin functions has been added so that normal Arduino Sketches that use Serial.begin(xxx) will compile.
void USBSerial::begin(unsigned long ignoreBaud) 
{
	volatile unsigned long removeCompilerWarningsIgnoreBaud=ignoreBaud;

	ignoreBaud=removeCompilerWarningsIgnoreBaud;
}
void USBSerial::begin(unsigned long ignoreBaud, uint8_t ignore)
{
	volatile unsigned long removeCompilerWarningsIgnoreBaud=ignoreBaud;
	volatile uint8_t removeCompilerWarningsIgnore=ignore;

	ignoreBaud=removeCompilerWarningsIgnoreBaud;
	ignore=removeCompilerWarningsIgnore;
}

void USBSerial::end(void) {
	USBDev.end();
}

size_t USBSerial::write(uint8 ch) {
size_t n = 0;
    this->write(&ch, 1);
		return n;
}

size_t USBSerial::write(const char *str) {
size_t n = 0;
    this->write((const uint8*)str, strlen(str));
	return n;
}

size_t USBSerial::write(const uint8 *buf, uint32 len)
{
size_t n = 0;
    if (!this->isConnected() || !buf) {
        return 0;
    }

    uint32 txed = 0;
    while (txed < len) {
        txed += usb_cdcacm_tx((const uint8*)buf + txed, len - txed);
    }

	return n;
}

int USBSerial::available(void) {
    return usb_cdcacm_data_available();
}

int USBSerial::peek(void)
{
    uint8 b;
	if (usb_cdcacm_peek(&b, 1)==1)
	{
		return b;
	}
	else
	{
		return -1;
	}
}

void USBSerial::flush(void)
{
/*Roger Clark. Rather slow method. Need to improve this */
    uint8 b;
	while(usb_cdcacm_data_available())
	{
		this->read(&b, 1);
	}
    return;
}

uint32 USBSerial::read(uint8 * buf, uint32 len) {
    uint32 rxed = 0;
    while (rxed < len) {
        rxed += usb_cdcacm_rx(buf + rxed, len - rxed);
    }

    return rxed;
}

/* Blocks forever until 1 byte is received */
int USBSerial::read(void) {
    uint8 b;
	/*
	    this->read(&b, 1);
    return b;
	*/
	
	if (usb_cdcacm_rx(&b, 1)==0)
	{
		return -1;
	}
	else
	{
		return b;
	}
}

uint8 USBSerial::pending(void) {
    return usb_cdcacm_get_pending();
}

uint8 USBSerial::isConnected(void) {
    return usb_is_connected(USBLIB) && usb_is_configured(USBLIB) && usb_cdcacm_get_dtr();
}

uint8 USBSerial::getDTR(void) {
    return usb_cdcacm_get_dtr();
}

uint8 USBSerial::getRTS(void) {
    return usb_cdcacm_get_rts();
}

/*
 * Bootloader hook implementations
 */

#if BOARD_HAVE_SERIALUSB

enum reset_state_t {
    DTR_UNSET,
    DTR_HIGH,
    DTR_NEGEDGE,
    DTR_LOW
};

static reset_state_t reset_state = DTR_UNSET;

static void ifaceSetupHook(unsigned hook, void *requestvp) {
    uint8 request = *(uint8*)requestvp;

    // Ignore requests we're not interested in.
    if (request != USB_CDCACM_SET_CONTROL_LINE_STATE) {
        return;
    }

#ifdef USB_HARDWARE 
    // We need to see a negative edge on DTR before we start looking
    // for the in-band magic reset byte sequence.
    uint8 dtr = usb_cdcacm_get_dtr();
    switch (reset_state) {
    case DTR_UNSET:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_HIGH:
        reset_state = dtr ? DTR_HIGH : DTR_NEGEDGE;
        break;
    case DTR_NEGEDGE:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_LOW:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    }
#endif

#if defined(BOOTLOADER_robotis)
    uint8 dtr = usb_cdcacm_get_dtr();
    uint8 rts = usb_cdcacm_get_rts();

    if (rts && !dtr) {
        reset_state = DTR_NEGEDGE;
    }
#endif

	if ((usb_cdcacm_get_baud() == 1200) && (reset_state == DTR_NEGEDGE)) {
		iwdg_init(IWDG_PRE_4, 10);
		while (1);
	}
}

#define RESET_DELAY 100000
#ifdef USB_HARDWARE 
static void wait_reset(void) {
  delay_us(RESET_DELAY);
  nvic_sys_reset();
}
#endif

#define STACK_TOP 0x20000800
#define EXC_RETURN 0xFFFFFFF9
#define DEFAULT_CPSR 0x61000000
static void rxHook(unsigned hook, void *ignored) {
    /* FIXME this is mad buggy; we need a new reset sequence. E.g. NAK
     * after each RX means you can't reset if any bytes are waiting. */
    if (reset_state == DTR_NEGEDGE) {
        reset_state = DTR_LOW;

        if (usb_cdcacm_data_available() >= 4) {
            // The magic reset sequence is "1EAF".
#ifdef USB_HARDWARE 
            static const uint8 magic[4] = {'1', 'E', 'A', 'F'};	
#else
	#if defined(BOOTLOADER_robotis)
				static const uint8 magic[4] = {'C', 'M', '9', 'X'};
	#else
				static const uint8 magic[4] = {'1', 'E', 'A', 'F'};	
	#endif			
#endif

            uint8 chkBuf[4];

            // Peek at the waiting bytes, looking for reset sequence,
            // bailing on mismatch.
            usb_cdcacm_peek_ex(chkBuf, usb_cdcacm_data_available() - 4, 4);
            for (unsigned i = 0; i < sizeof(magic); i++) {
                if (chkBuf[i] != magic[i]) {
                    return;
                }
            }

#ifdef USB_HARDWARE 
            // Got the magic sequence -> reset, presumably into the bootloader.
            // Return address is wait_reset, but we must set the thumb bit.
            uintptr_t target = (uintptr_t)wait_reset | 0x1;
            asm volatile("mov r0, %[stack_top]      \n\t" // Reset stack
                         "mov sp, r0                \n\t"
                         "mov r0, #1                \n\t"
                         "mov r1, %[target_addr]    \n\t"
                         "mov r2, %[cpsr]           \n\t"
                         "push {r2}                 \n\t" // Fake xPSR
                         "push {r1}                 \n\t" // PC target addr
                         "push {r0}                 \n\t" // Fake LR
                         "push {r0}                 \n\t" // Fake R12
                         "push {r0}                 \n\t" // Fake R3
                         "push {r0}                 \n\t" // Fake R2
                         "push {r0}                 \n\t" // Fake R1
                         "push {r0}                 \n\t" // Fake R0
                         "mov lr, %[exc_return]     \n\t"
                         "bx lr"
                         :
                         : [stack_top] "r" (STACK_TOP),
                           [target_addr] "r" (target),
                           [exc_return] "r" (EXC_RETURN),
                           [cpsr] "r" (DEFAULT_CPSR)
                         : "r0", "r1", "r2");
#endif

#if defined(BOOTLOADER_robotis)
            iwdg_init(IWDG_PRE_4, 10);
#endif

            /* Can't happen. */
            ASSERT_FAULT(0);
        }
    }
}

#endif  // BOARD_HAVE_SERIALUSB

#if BOARD_HAVE_SERIALUSB
#ifdef USB_HARDWARE 

USBDevice USBDev;

USBSerial Serial;

#if defined(USB_HID_KMJ) || defined(USB_HID_KM)
HIDMouse Mouse;
HIDKeyboard Keyboard;
#endif
#if defined(USB_HID_KMJ) || defined(USB_HID_J)
HIDJoystick Joystick;
#endif

#endif
#endif

#endif //NO_USB
