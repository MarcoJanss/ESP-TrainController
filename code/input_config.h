// input_config.h
#ifndef INPUT_CONFIG_H
#define INPUT_CONFIG_H

#include <FastLED.h>
#include <vector>
#include <string>

// Pin configurations
extern std::vector<int> pwmPins; // Allows dynamic resizing and management of PWM pins.
extern std::vector<int> digitalPins;
extern std::vector<int> fastLedPins;
extern std::vector<int> reservedPins;
extern int statusLedPin;
extern std::vector<int> availablePins; // List of pins available

// FastLED configuration
extern std::vector<int> numLeds;
extern std::vector<std::string> fastLedType;

// Network
extern int LISTEN_PORT;
extern int connectTimeout; // Connection timeout in seconds
extern const char* apPassword;
extern const char* deviceID;
extern const int AP_RETRY_INTERVAL;

#endif // INPUT_CONFIG_H