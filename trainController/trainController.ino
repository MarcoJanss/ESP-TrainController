// main.ino
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "network_manager.h"
#include "pin_manager.h"
#include "status_led.h"
#include "input_config.h"
#include "log_manager.h"


// Server instance
AsyncWebServer server(80);

// State variables
enum WiFiState { CONNECTING, AP_MODE, CONNECTED };
WiFiState currentState = CONNECTING;
unsigned long lastRetryTime = 0;
unsigned long lastBlinkTime = 0;

void setup() {
    Serial.begin(115200);

    // Initialize WiFi
    Serial.println("Initializing WiFi...");
    if (!NetworkManager2::initializeWiFi()) {
        Serial.println("Starting AP mode...");
        currentState = AP_MODE;
    } else {
        currentState = CONNECTED;
        Serial.println("Done Initializing WiFi...");
        AddToLog("Done Initializing WiFi...");
    }


  
  // Initialize the pin manager for pin setups
  Serial.println("Initializing pins");
  AddToLog("Initializing pins");
  PinManager::initializePins();
  Serial.println("Done Initializing pins...");
  AddToLog("Done Initializing pins...");


  // Register REST endpoints
  Serial.println("Setting up server...");
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(200, "text/plain", "Server is running");
  });

  //// pinDesignation
  // Get
  server.on("/pinDesignation", HTTP_GET, [](AsyncWebServerRequest *request) {
      String response = PinManager::getPinDesignation();
      request->send(200, "application/json", response);
  });
  // Post
  server.on("/pinDesignation", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasParam("body", true)) {
          String body = request->getParam("body", true)->value();
          String response = PinManager::postPinDesignation(body);
          request->send(200, "application/json", response);
      } else {
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      }
  });
  //// pinValues
  // Get
  server.on("/pinValues", HTTP_GET, [](AsyncWebServerRequest *request) {
      String response = PinManager::getPinValues();
      request->send(200, "application/json", response);
  });
  // Post
  server.on("/pinValues", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (request->hasParam("body", true)) {
          String body = request->getParam("body", true)->value();
          String response = PinManager::postPinValues(body);
          request->send(200, "application/json", response);
      } else {
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      }
  });
  //// network
  // Get
  server.on("/network", HTTP_GET, [](AsyncWebServerRequest* request) {
      String response = NetworkManager2::getNetworks();
      request->send(200, "application/json", response);
  });
  // Post
  server.on("/network", HTTP_POST, [](AsyncWebServerRequest* request) {
      if (request->hasParam("body", true)) {
          String body = request->getParam("body", true)->value();
          String response = NetworkManager2::postNetwork(body);
          request->send(200, "application/json", response);
      } else {
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      }
  });
  // Delete
  server.on("/network", HTTP_DELETE, [](AsyncWebServerRequest* request) {
      if (request->hasParam("body", true)) {
          String body = request->getParam("body", true)->value();
          String response = NetworkManager2::deleteNetwork(body);
          request->send(200, "application/json", response);
      } else {
          request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      }
  });
    //// Log
    // Get
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
        String response = getLog();
        request->send(200, "application/json", response);
    });
    // POST /connect: Temporarily connect to a specified WiFi network
    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (request->hasParam("body", true)) {
            String body = request->getParam("body", true)->value();
            String response = NetworkManager2::postConnect(body);
            request->send(200, "application/json", response);
        } else {
            request->send(400, "application/json", R"({"error":"Invalid JSON"})");
        }
    });
    //// Other, undefined routes
    // server.onNotFound([](AsyncWebServerRequest *request) {
    //   request->send(404, "application/json", "{\"error\":\"This is not the route you're looking for..\"}");
    // });
  
  Serial.println("Starting server...");
  server.begin();
  Serial.println("Server started successfully.");
}

void loop() {
    unsigned long currentMillis = millis();
    
    // Handle LED blinking
    if (currentMillis - lastBlinkTime >= interval) {
        lastBlinkTime = currentMillis;
        ledState = !ledState;
        digitalWrite(statusLedPin, ledState);
    }

    // Handle state transitions
    switch (currentState) {
        case CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Connected to WiFi: " + WiFi.localIP().toString());
                currentState = CONNECTED;
                isBlinking = false; // Stop blinking (solid LED)
            } else if (currentMillis - lastRetryTime >= connectTimeout * 1000) {
                lastRetryTime = currentMillis;
                Serial.println("Retrying WiFi connection...");
                WiFi.reconnect();
            }
            break;

        case AP_MODE:
            // Periodically retry stored networks
            if (currentMillis - lastRetryTime >= AP_RETRY_INTERVAL * 1000) {
                lastRetryTime = currentMillis;
                Serial.println("Retrying stored networks...");
                if (NetworkManager2::tryStoredNetworks()) {
                    currentState = CONNECTED;
                }
            }
            break;

        case CONNECTED:
            // No ongoing tasks in this state; add monitoring logic if needed
            break;
    }
}
