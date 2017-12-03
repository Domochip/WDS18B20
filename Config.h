#ifndef Config_h
#define Config_h

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "data\config.html.gz.h"

#define DEFAULT_AP_SSID "WirelessDS18B20"
#define DEFAULT_AP_PSK "PasswordDS"

const char predefPassword[] PROGMEM = "ewcXoCt4HHjZUvY0";

#define MAX_NUMBER_OF_BUSES 4

class Config {
  public:
    char ssid[32 + 1] = {0};
    char password[64 + 1] = {0};
    char hostname[24 + 1] = {0};
    uint32_t ip = 0;
    uint32_t gw = 0;
    uint32_t mask = 0;
    uint32_t dns1 = 0;
    uint32_t dns2 = 0;

    byte numberOfBuses = 0;
    uint8_t owBusesPins[MAX_NUMBER_OF_BUSES][2];

    void SetDefaultValues() {
      ssid[0] = 0;
      password[0] = 0;
      hostname[0] = 0;
      ip = 0;
      gw = 0;
      mask = 0;
      dns1 = 0;
      dns2 = 0;

      numberOfBuses = 0;
      memset(owBusesPins, 0, MAX_NUMBER_OF_BUSES * 2);
    }
    static byte AsciiToHex(char c); //Utils
    static bool FingerPrintS2A(byte* fingerPrintArray, const char* fingerPrintToDecode);
    static char* FingerPrintA2S(char* fpBuffer, byte* fingerPrintArray, char separator = 0);

    bool Save();
    bool Load();
    void InitWebServer(AsyncWebServer &server, bool &shouldReboot);
  private :
    String GetJSON();
    bool SetFromParameters(AsyncWebServerRequest* request);
    uint16_t crc; ///!\ crc should always stay in last position
};


#endif

