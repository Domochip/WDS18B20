#ifndef WifiMan_h
#define WifiMan_h

#include <ESP8266WiFi.h>

class WifiMan {

  private:

    WiFiEventHandler _wifiHandler1, _wifiHandler2;
    int _apChannel = 2;
    char _apSsid[64];
    uint16_t _retryPeriod = 300;
    bool _retryStation = false;

  public:
    bool Init(char* ssid, char* password, char* hostname, char* apSSID, char* apPassword, uint16_t retryPeriod = 300);
    void Run();
};

#endif
