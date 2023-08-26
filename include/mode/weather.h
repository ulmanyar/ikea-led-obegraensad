#pragma once

#include "constants.h"
#include <ArduinoJson.h>
#ifdef ESP32
#include <HTTPClient.h>
#endif
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#endif

#ifdef ENABLE_SERVER

#include "screen.h"
#include "mode.h"

extern unsigned long lastUpdate;

extern HTTPClient http;

void weatherSetup();
void weatherLoop();
void weatherUpdate();

#endif
