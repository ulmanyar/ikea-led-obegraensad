#include <Arduino.h>
#include <SPI.h>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include "constants.h"
#include "mode/mode.h"
#include "websocket.h"
#include "secrets.h"
#include "ota.h"
#include "webserver.h"
#include "screen.h"
#include "mode/mode.h"
#include "mode/rain.h"

unsigned long previousMillis = 0;
unsigned long interval = 30000;

Rain main_rain;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  //pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT);

// server
#ifdef ENABLE_SERVER
  // wifi
  int attempts = 0;
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED && attempts < 7)
  {
    delay(2000);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Couldn't connect to WiFi, resetting");
    ESP.restart();
  }
  else
  {
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // set time server
  configTzTime(TZ_INFO, NTP_SERVER);

  initOTA(server);
  initWebsocketServer(server);
  initWebServer();
#endif

  Screen.setup();
  Screen.clear();
}

void loop() {
    main_rain.setup();
    //uint8_t ext_buffer[ROWS * COLS] = {0};
    // Try explicit call to _render()
    //unsigned long currentMillis = millis();
    //if (currentMillis - previousMillis >= 2000) {
    //    previousMillis = currentMillis;
    //    for (uint16_t pixel = 0; pixel < ROWS * COLS; pixel++) {
    //        //ext_buffer[pixel] = random(0,255);
    //        ext_buffer[pixel] = pixel * 16;
    //    }
    //    Screen.setBuffer(ext_buffer);
    //    Screen.render();
    //}
#ifdef ENABLE_SERVER
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }
  cleanUpClients();
#endif
}
