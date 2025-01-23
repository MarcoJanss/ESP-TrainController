// main.ino
//#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "network_manager.h"
#include "pin_manager.h"
#include "status_led.h"
#include "input_config.h"
#include "log_manager.h"


// Server instance
AsyncWebServer server(80);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the pin manager for pin setups
  PinManager::initializePins();

  // Start the server
  server.begin();
  Serial.println("ESPAsyncWebServer started");

  // Register REST endpoints
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
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "application/json", "{\"error\":\"This is not the route you're looking for..\"}");
  });
  
  //Initialize WiFi and networking services
  NetworkManager2::initializeWiFi();
}


void loop() {
  // handle status led
  if (isBlinking) {
    status_led::handleBlinking();
  }
}
