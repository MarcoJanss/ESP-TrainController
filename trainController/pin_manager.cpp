// pin_manager.cpp
#include "pin_manager.h"
#include "input_config.h"
#include "log_manager.h"
#include <ArduinoJson.h>
#include <FastLED.h>
#include <algorithm>
#include "esp32-hal-ledc.h"


namespace PinManager {

    // FastLED configuration
    std::vector<CRGB*> fastLeds;

	// Function to initialize the pins
    void initializePins() {
		// Sort all vectors
		std::sort(pwmPins.begin(), pwmPins.end());
		std::sort(digitalPins.begin(), digitalPins.end());
		std::sort(fastLedPins.begin(), fastLedPins.end());
		std::sort(reservedPins.begin(), reservedPins.end());
		
        // Configure PWM pins
        Serial.println("Size of pwmPins: " + String(pwmPins.size()));
        for (size_t i = 0; i < pwmPins.size(); i++) {
            int channel = i; // Assign a unique PWM channel
            ledcAttach(pwmPins[i], 5000, 8); // 5 kHz frequency, 8-bit resolution
            ledcWrite(pwmPins[i], 0); // Initialize as OFF
        }

        // Configure digital pins
        for (int pin : digitalPins) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW); // Initialize as OFF
        }

        // Configure FastLED pins
        fastLeds.clear();
        for (size_t i = 0; i < fastLedPins.size(); ++i) {
            fastLeds.push_back(new CRGB[numLeds[i]]);
            // FastLED.addLeds<WS2812, fastLedPins[i]>(fastLeds[i], numLeds[i]);
            FastLED.clear(true);
        }

        // Configure status LEDs
        pinMode(statusLedPin, OUTPUT);
        digitalWrite(statusLedPin, LOW); // Start with LED off
        

        Serial.println("Pins initialized.");
        AddToLog("Pins initialized.");
    }


	// Function to clean up FastLED
    void cleanupFastLED() {
        // Free memory allocated for FastLED
        for (CRGB* leds : fastLeds) {
            delete[] leds;
        }
        fastLeds.clear();
        Serial.println("FastLED memory cleaned up.");
        AddToLog("FastLED memory cleaned up.");
    }


	// Function to reset PinDesignation and pinValues
    void resetPins() {
        // Clear values for pins to ensure clean state
		// pwmPins
        for (int pin : pwmPins) {
            ledcWrite(pin, 0); // Reset PWM pins to 0
        }
		pwmPins.clear();
		
		// digitalPins
        for (int pin : digitalPins) {
            digitalWrite(pin, LOW); // Reset digital pins to LOW
        }
		digitalPins.clear();
		
		//fastLedPins
        for (size_t i = 0; i < fastLeds.size(); ++i) {
            fill_solid(fastLeds[i], numLeds[i], CRGB::Black); // Turn off FastLEDs
        }
        FastLED.show();
		fastLedPins.clear();
		fastLedType.clear();
		numLeds.clear();
		cleanupFastLED(); // Clean up previous FastLED setup
		
        Serial.println("Pins reset.");
        AddToLog("Pins reset.");
    }


  // Get pin designations
  String getPinDesignation() {
      DynamicJsonDocument doc(1024);
      
      // Create arrays for each pin type
      JsonArray digitalArray = doc.createNestedArray("digitalPins");
      for (int pin : digitalPins) {
          digitalArray.add(pin);
      }

      JsonArray pwmArray = doc.createNestedArray("pwmPins");
      for (int pin : pwmPins) {
          pwmArray.add(pin);
      }

      JsonArray fastLedArray = doc.createNestedArray("fastLedPins");
      for (int pin : fastLedPins) {
          fastLedArray.add(pin);
      }

      JsonArray reservedArray = doc.createNestedArray("reservedPins");
      for (int pin : reservedPins) {
          reservedArray.add(pin);
      }

      JsonArray availableArray = doc.createNestedArray("availablePins");
      for (int pin : availablePins) {
          availableArray.add(pin);
      }

      String response;
      serializeJson(doc, response);
      return response;
  }



  // Post pin designation
  String postPinDesignation(const String& body) {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, body);

      if (error) {
          return R"({"error":"Invalid JSON", "details":")" + String(error.c_str()) + R"("})";
      }

      JsonObject root = doc.as<JsonObject>();

      std::vector<int> inputDigitalPins;
      std::vector<int> inputPwmPins;
      std::vector<int> inputFastLedPins;

      // Parse pins
      if (root.containsKey("digitalPins")) {
          JsonArray digitalArray = root["digitalPins"].as<JsonArray>();
          for (JsonVariant value : digitalArray) {
              inputDigitalPins.push_back(value.as<int>());
          }
      }

      if (root.containsKey("pwmPins")) {
          JsonArray pwmArray = root["pwmPins"].as<JsonArray>();
          for (JsonVariant value : pwmArray) {
              inputPwmPins.push_back(value.as<int>());
          }
      }

      if (root.containsKey("fastLedPins")) {
          JsonArray fastLedArray = root["fastLedPins"].as<JsonArray>();
          for (JsonVariant value : fastLedArray) {
              inputFastLedPins.push_back(value.as<int>());
          }
      }

      // Check for duplicate pins across lists
      std::vector<int> duplicates;
      for (int pin : inputDigitalPins) {
          if (std::find(inputPwmPins.begin(), inputPwmPins.end(), pin) != inputPwmPins.end() ||
              std::find(inputFastLedPins.begin(), inputFastLedPins.end(), pin) != inputFastLedPins.end()) {
              duplicates.push_back(pin);
          }
      }
      for (int pin : inputPwmPins) {
          if (std::find(inputFastLedPins.begin(), inputFastLedPins.end(), pin) != inputFastLedPins.end()) {
              duplicates.push_back(pin);
          }
      }

      if (!duplicates.empty()) {
          DynamicJsonDocument errorDoc(1024);
          errorDoc["error"] = "Pins in multiple lists";
          JsonArray duplicatePins = errorDoc.createNestedArray("duplicates");
          for (int pin : duplicates) {
              duplicatePins.add(pin);
          }
          errorDoc["input"] = root;
          String errorResponse;
          serializeJson(errorDoc, errorResponse);
          return errorResponse;
      }

      // Validate pins against availablePins and reservedPins
      std::vector<int> invalidPins;
      std::vector<int> reservedConflictPins;

      for (int pin : inputDigitalPins) {
          if (std::find(reservedPins.begin(), reservedPins.end(), pin) != reservedPins.end()) {
              reservedConflictPins.push_back(pin);
          } else if (std::find(availablePins.begin(), availablePins.end(), pin) == availablePins.end()) {
              invalidPins.push_back(pin);
          }
      }

      for (int pin : inputPwmPins) {
          if (std::find(reservedPins.begin(), reservedPins.end(), pin) != reservedPins.end()) {
              reservedConflictPins.push_back(pin);
          } else if (std::find(availablePins.begin(), availablePins.end(), pin) == availablePins.end()) {
              invalidPins.push_back(pin);
          }
      }

      for (int pin : inputFastLedPins) {
          if (std::find(reservedPins.begin(), reservedPins.end(), pin) != reservedPins.end()) {
              reservedConflictPins.push_back(pin);
          } else if (std::find(availablePins.begin(), availablePins.end(), pin) == availablePins.end()) {
              invalidPins.push_back(pin);
          }
      }

      if (!invalidPins.empty() || !reservedConflictPins.empty()) {
          DynamicJsonDocument errorDoc(1024);
          errorDoc["error"] = "Pin validation failed";

          JsonArray invalidArray = errorDoc.createNestedArray("invalidPins");
          for (int pin : invalidPins) {
              invalidArray.add(pin);
          }

          JsonArray reservedArray = errorDoc.createNestedArray("reservedConflictPins");
          for (int pin : reservedConflictPins) {
              reservedArray.add(pin);
          }

          JsonArray availableArray = errorDoc.createNestedArray("availablePins");
          for (int pin : availablePins) {
              availableArray.add(pin);
          }

          JsonArray reservedPinsArray = errorDoc.createNestedArray("reservedPins");
          for (int pin : reservedPins) {
              reservedPinsArray.add(pin);
          }

          String errorResponse;
          serializeJson(errorDoc, errorResponse);
          return errorResponse;
      }

      // Assign valid pins to corresponding lists
      digitalPins = inputDigitalPins;
      pwmPins = inputPwmPins;
      fastLedPins = inputFastLedPins;

      // Sort the lists for consistency
      std::sort(digitalPins.begin(), digitalPins.end());
      std::sort(pwmPins.begin(), pwmPins.end());
      std::sort(fastLedPins.begin(), fastLedPins.end());

      // Reinitialize the pins
      initializePins();  // This ensures the new pins are properly configured
      
      return R"({"message":"Pin designation updated successfully"})";
  }


  String getPinValues() {
      DynamicJsonDocument doc(1024);
      JsonObject root = doc.to<JsonObject>();

      // Get digital pin values
      JsonObject digitalObj = root.createNestedObject("digitalPins");
      for (int pin : digitalPins) {
          digitalObj[String(pin)] = digitalRead(pin);
      }

      // Get PWM pin values
      JsonObject pwmObj = root.createNestedObject("pwmPins");
      for (int pin : pwmPins) {
          pwmObj[String(pin)] = ledcRead(pin);
      }

      // Get FastLED values
      JsonArray fastLedArray = root.createNestedArray("fastLedPins");
      for (size_t i = 0; i < fastLedPins.size(); ++i) {
          JsonObject fastLedObj = fastLedArray.createNestedObject();
          fastLedObj["pin"] = fastLedPins[i];
          fastLedObj["type"] = fastLedType[i];
          fastLedObj["numLeds"] = numLeds[i];

          // Add color values
          JsonArray colorArray = fastLedObj.createNestedArray("color");
          colorArray.add(fastLeds[i]->r);
          colorArray.add(fastLeds[i]->g);
          colorArray.add(fastLeds[i]->b);
      }

      String jsonResponse;
      serializeJson(doc, jsonResponse);
      return jsonResponse;
  }


  String postPinValues(const String& body) {
      Serial.println("postPinValues started");
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, body);
      
      if (error) {
          return R"({"error":"Invalid JSON", "details":")" + String(error.c_str()) + R"("})";
      }

      JsonObject root = doc.as<JsonObject>();
      JsonObject digitalValues = root["digital"].as<JsonObject>();
      JsonObject pwmValues = root["pwm"].as<JsonObject>();
      JsonObject fastLedValues = root["fastLed"].as<JsonObject>();

      std::vector<String> errors;

      // Validate digital pins
      for (JsonPair kv : digitalValues) {
          int pin = String(kv.key().c_str()).toInt();
          int value = kv.value().as<int>();

          if (std::find(digitalPins.begin(), digitalPins.end(), pin) == digitalPins.end()) {
              errors.push_back("Pin " + String(pin) + " is not designated as digital");
          } else if (value < 0 || value > 1) {
              errors.push_back("Digital pin " + String(pin) + " must be 0 or 1");
          } else {
              digitalWrite(pin, value);
          }
      }

      // Validate PWM pins
      for (JsonPair kv : pwmValues) {
          int pin = String(kv.key().c_str()).toInt();
          int value = kv.value().as<int>();

          if (std::find(pwmPins.begin(), pwmPins.end(), pin) == pwmPins.end()) {
              errors.push_back("Pin " + String(pin) + " is not designated as PWM");
          } else if (value < 0 || value > 100) {
              errors.push_back("PWM pin " + String(pin) + " must be between 0 and 100");
          } else {
              ledcWrite(pin, map(value, 0, 100, 0, 255));
          }
      }

      // Validate FastLED pins
      for (JsonPair kv : fastLedValues) {
          int pin = String(kv.key().c_str()).toInt();
          JsonObject colorObj = kv.value().as<JsonObject>();

          int r = colorObj["r"].as<int>();
          int g = colorObj["g"].as<int>();
          int b = colorObj["b"].as<int>();

          if (std::find(fastLedPins.begin(), fastLedPins.end(), pin) == fastLedPins.end()) {
              errors.push_back("Pin " + String(pin) + " is not designated as FastLED");
          } else if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
              errors.push_back("FastLED pin " + String(pin) + " values (r, g, b) must be between 0 and 255");
          } else {
              fill_solid(fastLeds[pin], numLeds[pin], CRGB(r, g, b));
          }
      }

      FastLED.show();

      // Return errors if any
      if (!errors.empty()) {
          DynamicJsonDocument errorDoc(1024);
          JsonArray errorArray = errorDoc.createNestedArray("errors");
          for (const String& err : errors) {
              errorArray.add(err);
          }
          String errorResponse;
          serializeJson(errorDoc, errorResponse);
          return errorResponse;
      }

      return R"({"message":"Pin values updated successfully"})";
  }

}
