// network_manager.cpp
#include <WiFi.h>
#include <WiFiManager.h> // Library for managing WiFi connections
#include "network_manager.h"
#include "log_manager.h"
#include <ArduinoJson.h>
#include "input_config.h"
#include "status_led.h"
#include <FS.h>
#include <LittleFS.h>

namespace NetworkManager2 {
    std::vector<WiFiNetwork> savedNetworks;

    String postConnect(const String& body) {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            return R"({"error":"Invalid JSON"})";
        }

        if (!doc.containsKey("ssid")) {
            return R"({"error":"Missing SSID"})";
        }

        String ssid = doc["ssid"].as<String>();
        String password;

        // Check if the SSID exists in stored networks
        auto it = std::find_if(savedNetworks.begin(), savedNetworks.end(), [&](const WiFiNetwork& nw) {
            return nw.ssid == ssid;
        });

        if (it != savedNetworks.end()) {
            password = it->password; // Use stored password
        } else if (doc.containsKey("password")) {
            password = doc["password"].as<String>(); // Use provided password
        } else {
            return R"({"error":"Missing password for unknown SSID"})";
        }

        // Stop AP mode if active
        if (WiFi.getMode() == WIFI_AP) {
            WiFi.softAPdisconnect(true);
            Serial.println("Stopped AP mode.");
        }

        // Attempt to connect
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < connectTimeout) {
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to WiFi: " + WiFi.localIP().toString());
            interval = 0; // Solid LED
            return R"({"message":"Connected successfully","ip":")" + WiFi.localIP().toString() + R"("})";
        } else {
            Serial.println("Failed to connect to: " + ssid);
            WiFi.softAP(deviceID, apPassword); // Restart AP mode
            WiFi.softAPsetHostname(deviceID);
            interval = 250; // Fast blinking
            return R"({"error":"Failed to connect, AP mode restarted"})";
        }
    }


    bool loadNetworksFromStorage() {
        if (!LittleFS.begin(true)) {
            Serial.println("Failed to initialize LittleFS.");
            return false;
        }

        File file = LittleFS.open("/networks.json", "r");
        if (!file) {
            Serial.println("No saved networks found.");
            return false;
        }

        size_t fileSize = file.size();
        if (fileSize == 0) {
            Serial.println("Empty networks file.");
            file.close();
            return false;
        }

        std::vector<WiFiNetwork> loadedNetworks;
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, file);

        if (error) {
            Serial.println("Failed to parse networks file.");
            file.close();
            return false;
        }

        JsonArray array = doc.as<JsonArray>();
        for (JsonObject obj : array) {
            WiFiNetwork network;
            network.ssid = obj["ssid"].as<String>();
            network.password = obj["password"].as<String>();
            network.isDefault = obj["isDefault"].as<bool>();
            loadedNetworks.push_back(network);
        }

        file.close();
        savedNetworks = std::move(loadedNetworks);
        Serial.println("Loaded networks from storage.");
        return true;
    }

    bool saveNetworksToStorage() {
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();

        for (const WiFiNetwork& network : savedNetworks) {
            JsonObject obj = array.createNestedObject();
            obj["ssid"] = network.ssid;
            obj["password"] = network.password;
            obj["isDefault"] = network.isDefault;
        }

        File file = LittleFS.open("/networks.json", "w");
        if (!file) {
            Serial.println("Failed to open networks file for writing.");
            return false;
        }

        serializeJson(doc, file);
        file.close();
        Serial.println("Saved networks to storage.");
        return true;
    }


void initializeWiFi() {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) { // Automatic format on failure
        Serial.println("Failed to mount file system");
        return;
    }

    // Load stored networks
    if (!loadNetworksFromStorage()) {
        Serial.println("No networks loaded from storage");
    }

    // Check if there are any stored networks
    if (savedNetworks.empty()) {
        Serial.println("No saved networks, starting AP mode");
        WiFi.softAP(deviceID, apPassword);
        WiFi.softAPsetHostname(deviceID); // Set hostname in AP mode
        interval = 250; // Fast blinking for status LED
        return;
    }

    // Attempt to connect to the default or first available stored network
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(deviceID); // Set hostname in station mode

    // Find the default network or fallback to the first stored network
    auto defaultNetwork = std::find_if(savedNetworks.begin(), savedNetworks.end(), [](const WiFiNetwork& nw) {
        return nw.isDefault;
    });

    const WiFiNetwork& networkToConnect = (defaultNetwork != savedNetworks.end()) ? *defaultNetwork : savedNetworks[0];

    // Start connecting
    WiFi.begin(networkToConnect.ssid.c_str(), networkToConnect.password.c_str());
    Serial.println("Attempting to connect to WiFi: " + networkToConnect.ssid);

    isBlinking = true; // Enable blinking while attempting to connect

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < (connectTimeout * 1000)) {
        delay(100);
    }

    // Check connection status
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi: " + WiFi.localIP().toString());
        isBlinking = false; // Stop blinking, solid LED
    } else {
        Serial.println("Failed to connect, starting AP mode");
        WiFi.softAP(deviceID, apPassword);
        WiFi.softAPsetHostname(deviceID); // Set hostname in AP mode
        interval = 250; // Fast blinking for status LED

        // Start WiFiManager for portal configuration
        WiFiManager wifiManager;
        wifiManager.setConfigPortalTimeout(connectTimeout); // Set portal timeout
        if (wifiManager.startConfigPortal(deviceID, apPassword)) {
            Serial.println("New network added through portal");

            // Save the network from the portal into storage
            WiFiNetwork newNetwork;
            newNetwork.ssid = WiFi.SSID();
            newNetwork.password = WiFi.psk();
            newNetwork.isDefault = false; // Mark as not default initially

            auto it = std::find_if(savedNetworks.begin(), savedNetworks.end(), [&](const WiFiNetwork& nw) {
                return nw.ssid == newNetwork.ssid;
            });

            if (it != savedNetworks.end()) {
                // Update the existing network, preserving the default flag
                newNetwork.isDefault = it->isDefault;
                *it = newNetwork;
            } else {
                // Add the new network
                savedNetworks.push_back(newNetwork);
            }

            saveNetworksToStorage();
        } else {
            Serial.println("Portal timed out or failed, restarting AP mode");
        }

        // Retry Logic: Attempt to reconnect every AP_RETRY_INTERVAL seconds
        while (true) {
            delay(AP_RETRY_INTERVAL * 1000);

            WiFi.mode(WIFI_STA);
            WiFi.begin(networkToConnect.ssid.c_str(), networkToConnect.password.c_str());

            startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < (connectTimeout * 1000)) {
                delay(100);
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Reconnected to WiFi: " + WiFi.localIP().toString());
                isBlinking = false; // Stop blinking, solid LED
                break; // Exit retry loop
            }
        }
    }
}


    String getNetworks() {
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();

        for (const auto& network : savedNetworks) {
            JsonObject obj = array.createNestedObject();
            obj["ssid"] = network.ssid;
            obj["isDefault"] = network.isDefault;
        }

        String response;
        serializeJson(doc, response);
        return response;
    }

    String postNetwork(const String& body) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            return "{\"error\":\"Invalid JSON\"}";
        }

        WiFiNetwork newNetwork;
        newNetwork.ssid = doc["ssid"].as<String>();
        newNetwork.password = doc["password"].as<String>();
        newNetwork.isDefault = doc["isDefault"].as<bool>();

        auto it = std::find_if(savedNetworks.begin(), savedNetworks.end(), [&](const WiFiNetwork& nw) {
            return nw.ssid == newNetwork.ssid;
        });

        if (it != savedNetworks.end()) {
            *it = newNetwork;
        } else {
            savedNetworks.push_back(newNetwork);
        }

        if (newNetwork.isDefault) {
            for (auto& network : savedNetworks) {
                network.isDefault = (network.ssid == newNetwork.ssid);
            }
        }

        saveNetworksToStorage();
        return "{\"message\":\"Network added or updated successfully\"}";
    }

    String deleteNetwork(const String& body) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            return "{\"error\":\"Invalid JSON\"}";
        }

        String ssidToDelete = doc["ssid"].as<String>();
        auto it = std::remove_if(savedNetworks.begin(), savedNetworks.end(), [&](const WiFiNetwork& nw) {
            return nw.ssid == ssidToDelete;
        });

        if (it != savedNetworks.end()) {
            savedNetworks.erase(it, savedNetworks.end());
            saveNetworksToStorage();
            return "{\"message\":\"Network deleted successfully\"}";
        }

        return "{\"error\":\"Network not found\"}";
    }
}
