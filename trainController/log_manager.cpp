// log_manager.cpp
#include "log_manager.h"
#include <ArduinoJson.h>


std::vector<String> logBuffer;


String getCurrentTimestamp() {
    unsigned long millisSinceStart = millis();
    unsigned long seconds = millisSinceStart / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    char timestamp[20];
    snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(timestamp);
}


String getLog() {
    DynamicJsonDocument doc(1024);
    JsonArray logArray = doc.to<JsonArray>();

    for (String& logEntry : logBuffer) {
        logArray.add(logEntry); // Add log entry to the JSON array
    }

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}


void AddToLog(const String& message) {
    // Create a timestamp
    //String timestamp = getCurrentTimestamp();
    String timestamp = "getCurrentTimestamp()";
    
    // Construct the log entry
    String logEntry = timestamp + ": " + message;

    // Add the log entry to the vector
    logBuffer.push_back(logEntry);

    // Ensure the log does not exceed 1024 bytes
    size_t totalLength = 0;
    for (const String& entry : logBuffer) {
        totalLength += entry.length();
    }

    while (totalLength > 1024 && !logBuffer.empty()) {
        totalLength -= logBuffer.front().length();
        logBuffer.erase(logBuffer.begin()); // Remove the oldest entry
    }
}
