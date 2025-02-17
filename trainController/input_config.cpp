// input_config.cpp
#include "input_config.h"

// Pin configurations
std::vector<int> pwmPins = {1, 2};
std::vector<int> digitalPins = {3, 4};
std::vector<int> fastLedPins = {5};
std::vector<int> reservedPins = {7, 8, 9, 10, 20, 21};
int statusLedPin = 7;
std::vector<int> availablePins = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21};

// FastLED configuration
std::vector<int> numLeds = {60};
std::vector<std::string> fastLedType = {"WS2812"};

// Network
int LISTEN_PORT = 80;
int connectTimeout = 60;
const char* apPassword = "Ditiseentest";
const char* deviceID = "1234-5678-9012";
const int AP_RETRY_INTERVAL = 30000; // 30 seconds