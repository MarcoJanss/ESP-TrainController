// pin_manager.h
#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include <FastLED.h>
#include <vector>
#include <string>
#include "input_config.h"

namespace PinManager {
	extern std::vector<CRGB*> fastLeds;
	
	void initializePins();
  void cleanupFastLED();
  void resetPins();
  String getPinDesignation();
  String postPinDesignation(const String &body);
  String getPinValues();
  String postPinValues(const String &body);
}

#endif
