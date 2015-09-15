#define lPin 33
#define bPin 32
#define BUTTON_DEBOUNCE_DELAY 1

bool isButtonPressed();

void setup() {
  
  pinMode(lPin, OUTPUT);
  
}

void loop() {
  
  if(isButtonPressed()){
    Mouse.click(MOUSE_RIGHT);
    //Mouse.click(MOUSE_LEFT);
  }
  
}

bool isButtonPressed() {
    if (digitalRead(bPin) == HIGH) {
        delay(BUTTON_DEBOUNCE_DELAY);
        while (digitalRead(bPin) == HIGH)
            ;
        return true;
    }
    return false;
}
