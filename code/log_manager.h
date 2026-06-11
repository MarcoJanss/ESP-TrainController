// log_manager.h
#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <Arduino.h>

String getCurrentTimestamp();
extern void AddToLog(const String &message);
extern String getLog();

#endif