// status_led.h
#ifndef STATUS_LED_H
#define STATUS_LED_H

extern bool isBlinking;
extern unsigned long previousMillis;
extern bool ledState;
extern int interval;

namespace status_led {
    void handleBlinking();
}

#endif
