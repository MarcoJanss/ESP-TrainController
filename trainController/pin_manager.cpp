// pin_manager.cpp
#include "pin_manager.h"
#include "input_config.h"
#include "log_manager.h"
#include <ArduinoJson.h>
#include <FastLED.h>


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
        for (int pin : pwmPins) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW); // Initialize as OFF
            // ledcSetup(pin, 5000, 8); // 5kHz, 8-bit resolution
            // ledcAttachPin(pin, pin);
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


    String getPinDesignation() {
        DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();

        // Add pin configurations to the JSON response
        JsonArray pwmArray = root.createNestedArray("pwmPins");
        for (int pin : pwmPins) {
            pwmArray.add(pin);
        }
        JsonArray digitalArray = root.createNestedArray("digitalPins");
        for (int pin : digitalPins) {
            digitalArray.add(pin);
        }
        JsonArray fastLedArray = root.createNestedArray("fastLedPins");
        for (size_t i = 0; i < fastLedPins.size(); ++i) {
            JsonObject fastLedObj = fastLedArray.createNestedObject();
            fastLedObj["pin"] = fastLedPins[i];
            fastLedObj["type"] = fastLedType[i];
            fastLedObj["numLeds"] = numLeds[i];
        }

        String jsonResponse;
        serializeJson(doc, jsonResponse);
        return jsonResponse;
    }

    String postPinDesignation(const String &body) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            return "{\"error\":\"Invalid JSON\"}";
        }

        resetPins(); // Clear previous pin configurations

        for (JsonPair kv : doc.as<JsonObject>()) {
            String key = kv.key().c_str();
            JsonArray values = kv.value().as<JsonArray>();

            if (key == "pwmPins") {
                for (JsonVariant value : values) {
                    pwmPins.push_back(value.as<int>());
                }
            } else if (key == "digitalPins") {
                for (JsonVariant value : values) {
                    digitalPins.push_back(value.as<int>());
                }
            } else if (key == "fastLedPins") {
                for (JsonObject fastLedObj : values) {
                    fastLedPins.push_back(fastLedObj["pin"].as<int>());
                    fastLedType.push_back(std::string(fastLedObj["type"].as<String>().c_str()));
                    numLeds.push_back(fastLedObj["numLeds"].as<int>());
                }
            }
        }

        // Sort all pins
        std::sort(pwmPins.begin(), pwmPins.end());
        std::sort(digitalPins.begin(), digitalPins.end());
        std::sort(fastLedPins.begin(), fastLedPins.end());

        return "{\"message\":\"Pin designation updated successfully\"}";
    }


  String getPinValues() {
      DynamicJsonDocument doc(1024);
      JsonObject root = doc.to<JsonObject>();

      // Get digital pin values
      JsonArray digitalArray = root.createNestedArray("digitalPins");
      for (int pin : digitalPins) {
          digitalArray.add(digitalRead(pin));
      }

      // Get PWM pin values
      JsonArray pwmArray = root.createNestedArray("pwmPins");
      for (int pin : pwmPins) {
          pwmArray.add(ledcRead(pin));
      }

      // Get FastLED values
      JsonArray fastLedArray = root.createNestedArray("fastLedPins");
      for (size_t i = 0; i < fastLedPins.size(); ++i) {
          JsonObject fastLedObj = fastLedArray.createNestedObject();
          fastLedObj["pin"] = fastLedPins[i];
          fastLedObj["type"] = fastLedType[i];
          fastLedObj["numLeds"] = numLeds[i];

          // Add color as an array
          JsonArray colorArray = fastLedObj.createNestedArray("color");
          colorArray.add(fastLeds[i]->r);
          colorArray.add(fastLeds[i]->g);
          colorArray.add(fastLeds[i]->b);
      }

      String jsonResponse;
      serializeJson(doc, jsonResponse);
      return jsonResponse;
  }



  String postPinValues(const String &body) {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, body);

      if (error) {
          return "{\"error\":\"Invalid JSON\"}";
      }

      if (doc.containsKey("digitalPins")) {
          JsonArray digitalArray = doc["digitalPins"].as<JsonArray>();
          for (size_t i = 0; i < digitalPins.size() && i < digitalArray.size(); ++i) {
              int value = static_cast<int>(digitalArray[i].as<float>());
              value = constrain(value, 0, 1);
              digitalWrite(digitalPins[i], value);
          }
      }

      if (doc.containsKey("pwmPins")) {
          JsonArray pwmArray = doc["pwmPins"].as<JsonArray>();
          for (size_t i = 0; i < pwmPins.size() && i < pwmArray.size(); ++i) {
              int duty = static_cast<int>(pwmArray[i].as<float>());
              duty = constrain(duty, 0, 100);
              ledcWrite(pwmPins[i], map(duty, 0, 100, 0, 255));
          }
      }

      if (doc.containsKey("fastLedPins")) {
          JsonArray fastLedArray = doc["fastLedPins"].as<JsonArray>();
          for (size_t i = 0; i < fastLedArray.size() && i < fastLedPins.size(); ++i) {
              JsonObject ledObj = fastLedArray[i];
              int red = static_cast<int>(ledObj["r"].as<float>());
              int green = static_cast<int>(ledObj["g"].as<float>());
              int blue = static_cast<int>(ledObj["b"].as<float>());

              red = constrain(red, 0, 255);
              green = constrain(green, 0, 255);
              blue = constrain(blue, 0, 255);

              fill_solid(fastLeds[i], numLeds[i], CRGB(red, green, blue));
          }
          FastLED.show();
      }

      return "{\"message\":\"Pin values updated successfully\"}";
  }

}
