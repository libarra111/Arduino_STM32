#include <usb_hid_device.h>

#define lPin 33
#define bPin 32
#define BUTTON_DEBOUNCE_DELAY 1

void setup() {
  
  pinMode(lPin, OUTPUT);
  HID.begin();
  
}

void loop() {

  delay(1500);Joystick.X(1000);
  delay(1500);Joystick.Y(1000);
  delay(1500);Joystick.X(0);
  delay(1500);Joystick.Y(0);
  
}
