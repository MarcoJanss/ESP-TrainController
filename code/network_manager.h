// network_manager.h
#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <vector>  // To use std::vector
#include <string>

// Declare the WiFiNetwork struct
struct WiFiNetwork {
    String ssid;
    String password;
    bool isDefault;
};

// Declare the extern variable for savedNetworks (defined elsewhere)
extern std::vector<WiFiNetwork> savedNetworks;  // Global storage for networks

namespace NetworkManager2 {
	  String postConnect(const String& body);
    bool initializeWiFi();
    bool tryStoredNetworks();
    void startAPMode();
    String getNetworks();  // GET
    String postNetwork(const String& body);
    String deleteNetwork(const String& body);
    // Persistent storage methods
    bool loadNetworksFromStorage();
    bool saveNetworksToStorage();
}

#endif
