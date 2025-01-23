// status_led.cpp
#include <Arduino.h>
#include "input_config.h" // Access to statusLedPin and other shared variables

bool isBlinking = false;
unsigned long previousMillis = 0;
bool ledState = false;
int interval = 500; // Blink interval (milliseconds)

namespace status_led {
    void handleBlinking() {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            ledState = !ledState;
            
            // Iterate over all status LEDs
            digitalWrite(statusLedPin, ledState ? HIGH : LOW);
    }
  }
}
