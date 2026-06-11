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
#include <ESPmDNS.h>


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


    bool initializeWiFi() {
        // Initialize LittleFS
        if (!LittleFS.begin(true)) {
            Serial.println("Failed to mount LittleFS");
            return false;
        }

        // Load stored networks
        if (!loadNetworksFromStorage()) {
            Serial.println("No networks loaded from storage");
        }

        // Check if there are saved networks
        if (savedNetworks.empty()) {
            Serial.println("No saved networks, starting AP mode");
            startAPMode();
            return false;
        }

        // Attempt to connect to the default or first available stored network
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(deviceID); // Set hostname in station mode

        // Configure mDNS
        if (MDNS.begin(deviceID)) { // Start mDNS with the deviceID as the hostname
            Serial.println("mDNS responder started. Hostname: " + String(deviceID) + ".local");
            AddToLog("mDNS responder started. Hostname: " + String(deviceID) + ".local");
        } else {
            Serial.println("Error starting mDNS responder!");
        }

        // Find the default network or fallback to the first stored network
        auto defaultNetwork = std::find_if(savedNetworks.begin(), savedNetworks.end(), [](const WiFiNetwork& nw) {
            return nw.isDefault;
        });

        const WiFiNetwork& networkToConnect = (defaultNetwork != savedNetworks.end()) ? *defaultNetwork : savedNetworks[0];

        // Start connecting
        WiFi.begin(networkToConnect.ssid.c_str(), networkToConnect.password.c_str());
        Serial.println("Attempting to connect to WiFi: " + networkToConnect.ssid);
        AddToLog("Attempting to connect to WiFi: " + networkToConnect.ssid);

        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < (connectTimeout * 1000)) {
            delay(100); // Small delay to avoid excessive polling
        }

        // Check connection
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to WiFi: " + WiFi.localIP().toString());
            AddToLog("Connected to WiFi: " + WiFi.localIP().toString());
            isBlinking = false; // Stop blinking, solid LED

            return true;
        } else {
            Serial.println("Failed to connect, starting AP mode");
            AddToLog("Failed to connect, starting AP mode");
            startAPMode();
            return false;
        }
    }

    void startAPMode() {
        WiFi.softAP(deviceID, apPassword);
        WiFi.softAPsetHostname(deviceID);

        if (MDNS.begin(deviceID)) { // Start mDNS in AP mode
            Serial.println("mDNS responder started in AP mode. Hostname: " + String(deviceID) + ".local");
            AddToLog("mDNS responder started in AP mode. Hostname: " + String(deviceID) + ".local");
        } else {
            Serial.println("Error starting mDNS responder in AP mode!");
            AddToLog("Error starting mDNS responder in AP mode!");
        }

        Serial.println("AP Mode started. SSID: " + String(deviceID) + ", Password: " + String(apPassword));
        interval = 250; // Fast blinking for status LED
    }
    
    bool tryStoredNetworks() {
        if (WiFi.getMode() != WIFI_AP) {
            return false; // Not in AP mode, no need to retry
        }

        Serial.println("Attempting to reconnect to stored networks...");
        
        if (savedNetworks.empty()) {
            Serial.println("No stored networks to retry.");
            return false;
        }

        WiFi.mode(WIFI_STA); // Switch to Station mode for reconnection

        // Attempt to connect to the default network first
        auto defaultNetwork = std::find_if(savedNetworks.begin(), savedNetworks.end(), [](const WiFiNetwork& nw) {
            return nw.isDefault;
        });

        if (defaultNetwork != savedNetworks.end()) {
            Serial.println("Trying default network: " + defaultNetwork->ssid);
            WiFi.begin(defaultNetwork->ssid.c_str(), defaultNetwork->password.c_str());
        } else {
            Serial.println("No default network, trying all stored networks...");
            for (const auto& network : savedNetworks) {
                Serial.println("Trying network: " + network.ssid);
                WiFi.begin(network.ssid.c_str(), network.password.c_str());

                unsigned long startAttemptTime = millis();
                while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < (connectTimeout * 1000)) {
                    delay(100);
                }

                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("Successfully reconnected to: " + WiFi.localIP().toString());
                    WiFi.softAPdisconnect(true);  // Disable AP mode
                    isBlinking = false; // Stop LED blinking
                    return true; // Connection successful
                }
            }
        }

        // If still not connected, revert to AP mode
        Serial.println("Failed to reconnect, remaining in AP mode.");
        WiFi.softAP(deviceID, apPassword);
        WiFi.softAPsetHostname(deviceID);
        interval = 250; // Fast blinking for status LED
        return false; // Connection failed
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
