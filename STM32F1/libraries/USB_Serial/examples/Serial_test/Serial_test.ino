#include <usb_serial.h>

#define lPin 33

void setup() {
  
  pinMode(lPin, OUTPUT);

  Serial.begin();
}

void loop() {
  delay(1500);
  Serial.println("Hello World!!! ");
}
