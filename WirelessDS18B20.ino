#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>

//Please, have a look at WirelessDS18B20.h for information and configuration of Arduino project

#include "WifiMan.h"
#include "Config.h"
#include "WebDS18B20.h"

#include "WirelessDS18B20.h"


#include "data\fw.html.gz.h"
#include "data\status.html.gz.h"
#include "data\jquery-3.2.1.min.js.gz.h"
#if DEVELOPPER_MODE
#include "data\test.html.gz.h"
#endif

//Config object
Config config;

//WifiMan
WifiMan wifiMan;

//AsyncWebServer
AsyncWebServer server(80);
//flag to use from web update to reboot the ESP
bool shouldReboot = false;

//DS18B20 bus
WebDS18B20Buses webDSBuses;


//-----------------------------------------------------------------------
void InitSystemWebServer(AsyncWebServer &server) {

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), (const uint8_t*)statushtmlgz, sizeof(statushtmlgz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gs0", HTTP_GET, [](AsyncWebServerRequest * request) {

    unsigned long minutes = millis() / 60000;
    char ssJSON[120];
    snprintf_P(ssJSON, sizeof(ssJSON), PSTR("{\"b\":\"%s\",\"u\":\"%dd%dh%dm\""), VERSION, (byte)(minutes / 1440), (byte)(minutes / 60 % 24), (byte)(minutes % 60));
    snprintf_P(ssJSON + strlen(ssJSON), sizeof(ssJSON) - strlen(ssJSON), PSTR(",\"ap\":\"%s\",\"ai\":\"%s\""), ((WiFi.getMode()&WIFI_AP) ? "on" : "off"), ((WiFi.getMode()&WIFI_AP) ? WiFi.softAPIP().toString().c_str() : "-"));
    snprintf_P(ssJSON + strlen(ssJSON), sizeof(ssJSON) - strlen(ssJSON), PSTR(",\"sta\":\"%s\",\"stai\":\"%s\""), (config.ssid[0] ? "on" : "off"), (config.ssid[0] ? (WiFi.isConnected() ? WiFi.localIP().toString().c_str() : "Not Connected") : "-"));
    snprintf_P(ssJSON + strlen(ssJSON), sizeof(ssJSON) - strlen(ssJSON), PSTR(",\"f\":%d}"), ESP.getFreeHeap());
    request->send(200, F("text/json"), ssJSON);
  });
  server.on("/fw", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), (const uint8_t*)fwhtmlgz, sizeof(fwhtmlgz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/fw", HTTP_POST, [](AsyncWebServerRequest * request) {
    shouldReboot = !Update.hasError();
    if (shouldReboot) {
      AsyncWebServerResponse *response = request->beginResponse(200, F("text/html"), F("Firmware Successfully Uploaded<script>setTimeout(function(){if('referrer' in document)window.location=document.referrer;},10000);</script>"));
      response->addHeader("Connection", "close");
      request->send(response);
    }
    else {
      AsyncWebServerResponse *response = request->beginResponse(500, F("text/html"), F("Firmware Update Error : End failed"));
      response->addHeader("Connection", "close");
      request->send(response);
    }
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.on("/jquery-3.2.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), (const uint8_t*)jquery321minjsgz, sizeof(jquery321minjsgz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.onNotFound([](AsyncWebServerRequest * request) {
    request->send(404);
  });

#if DEVELOPPER_MODE
  server.addHandler(new SPIFFSEditor("TODO", "TODO"));

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), (const uint8_t*)testhtmlgz, sizeof(testhtmlgz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
#endif
}

//-----------------------------------------------------------------------
// Setup function
//-----------------------------------------------------------------------
void setup(void) {

  Serial.begin(SERIAL_SPEED);
  Serial.println("");
  delay(200);

  Serial.print(F("J6B Wireless DS18B20 ")); Serial.println(VERSION);
  Serial.println(F("---Booting---"));
  Serial.println(F("Wait Rescue button for 5 seconds"));

#if ESP01_PLATFORM
  bool skipExistingConfig = false;
  pinMode(2, INPUT_PULLUP);
  for (int i = 0; i < 100 && skipExistingConfig == false; i++) {
    if (digitalRead(2) == LOW) skipExistingConfig = true;
    delay(50);
  }
#else
  bool skipExistingConfig = false;
  pinMode(RESCUE_BTN_PIN, (RESCUE_BTN_PIN != 16) ? INPUT_PULLUP : INPUT);
  for (int i = 0; i < 100 && skipExistingConfig == false; i++) {
    if (digitalRead(RESCUE_BTN_PIN) == LOW) skipExistingConfig = true;
    delay(50);
  }
#endif

  Serial.print(F("Start Config"));

  //initialize config with default values
  config.SetDefaultValues();

  //if skipExistingConfig is false then load the existing config
  if (!skipExistingConfig) {
    if (!config.Load()) Serial.println(F(" : Failed to load config!!!---------"));
    else Serial.println(F(" : OK"));
  }
  else Serial.println(F(" : OK (Config Skipped)"));


  Serial.print(F("Start WiFi : "));
  if (wifiMan.Init(config.ssid, config.password, config.hostname, DEFAULT_AP_SSID, DEFAULT_AP_PSK)) Serial.println(F("OK"));
  else Serial.println(F("FAILED"));

  Serial.print(F("Start OneWire"));

#if ESP01_PLATFORM
  Serial.flush();
  delay(5);
  Serial.end();

  config.numberOfBuses = 1;
  config.owBusesPins[0][0] = 3;
  config.owBusesPins[0][1] = 0;
#endif

  webDSBuses.Init(config.numberOfBuses, config.owBusesPins);

#if ESP01_PLATFORM
  Serial.begin(SERIAL_SPEED);
#endif

  Serial.print(F(" : OK\r\nStart WebServer"));

  InitSystemWebServer(server);
  config.InitWebServer(server, shouldReboot);
  webDSBuses.InitWebServer(server);
  DefaultHeaders::Instance().addHeader("Expires", "0"); //Add expires:0 to headers of all answer
  server.begin();
  Serial.println(F(" : OK"));

  Serial.println(F("---End of setup()---"));
}

//-----------------------------------------------------------------------
// Main Loop function
//-----------------------------------------------------------------------
void loop() {

  wifiMan.Run();

  if (shouldReboot) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

  yield();
}
