#include "WifiMan.h"

bool WifiMan::Init(char* ssid, char* password, char* hostname, char* apSSID, char* apPassword, uint16_t retryPeriod) {

  bool result = false;

  //build "unique" AP name (DEFAULT_AP_SSID + 4 last digit of ChipId)
  _apSsid[0] = 0;
  strcpy(_apSsid, apSSID);
  uint16 id = ESP.getChipId() & 0xFFFF;
  byte endOfSsid = strlen(_apSsid);
  byte num = (id & 0xF000) / 0x1000;
  _apSsid[endOfSsid] = num + ((num <= 9) ? 0x30 : 0x37);
  num = (id & 0xF00) / 0x100;
  _apSsid[endOfSsid + 1] = num + ((num <= 9) ? 0x30 : 0x37);
  num = (id & 0xF0) / 0x10;
  _apSsid[endOfSsid + 2] = num + ((num <= 9) ? 0x30 : 0x37);
  num = id & 0xF;
  _apSsid[endOfSsid + 3] = num + ((num <= 9) ? 0x30 : 0x37);
  _apSsid[endOfSsid + 4] = 0;

  _retryPeriod = retryPeriod;

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.print(n); Serial.print(F("N-CH"));
  if (n) {
    while (_apChannel < 12) {
      int i = 0;
      while (i < n && WiFi.channel(i) != _apChannel) i++;
      if (i == n) break;
      _apChannel++;
    }
  }
  Serial.print(_apChannel);

  //if STA is requested
  if (ssid[0]) {

    //if not connected or config changed then reconnect
    if (!WiFi.isConnected() || WiFi.SSID() != ssid || WiFi.psk() != password) {
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
    //Set hostname
    WiFi.hostname(hostname);

    //Wait 20sec for connection
    for (int i = 0; i < 200 && !WiFi.isConnected(); i++) {
      if ((i % 10) == 0) Serial.print(".");
      delay(100);
    }
    if (WiFi.isConnected()) {
      Serial.print('('); Serial.print(WiFi.localIP()); Serial.print(F(") "));
      WiFi.enableAP(false);
      result = true;
      WiFi.persistent(false);
    }
    else {
      Serial.print(F("Not Yet Connected "));
    }

    //Configure handlers
    _wifiHandler1 = WiFi.onStationModeDisconnected([this, apPassword](const WiFiEventStationModeDisconnected & evt) {
      if (!(WiFi.getMode()&WIFI_AP)) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(_apSsid, apPassword, _apChannel);
        Serial.print(F("WiFiDisco : Enabling AP (")); Serial.print(WiFi.softAPIP()); Serial.println(')');
        _retryStation = true;
      }
    });
    _wifiHandler2 = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP & evt) {
      if (WiFi.getMode()&WIFI_AP) WiFi.enableAP(false);
      Serial.print(F("WiFiReco : ")); Serial.println(WiFi.localIP());
    });

  }
  else {
    WiFi.disconnect();
    //Enable AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apSsid, apPassword, _apChannel);
    Serial.print(F(" AP mode(")); Serial.print(WiFi.softAPIP()); Serial.print(F(") "));
    result = true;
    WiFi.persistent(false);
  }

  return result;
};

void WifiMan::Run() {

  if (_retryStation && (millis() / (_retryPeriod * 100) % 10 == 0)) {
    Serial.print(F("Try WiFiReco"));
    //WiFi.begin(config.ssid, config.password); //ssid and password still stored because no WiFi.disconnect called
    WiFi.begin();

    //Wait 10sec for connection
    for (int i = 0; i < 100 && !WiFi.isConnected(); i++) {
      if ((i % 10) == 0) Serial.print(".");
      delay(100);
    }

    //if not connected
    if (!WiFi.isConnected()) {
      Serial.println(F("Failed"));
      //disable station mode
      WiFi.mode(WIFI_AP);
    }
    // disable retry and AP mode is disabled by wifiHandler(2)
    else _retryStation = false;
  }

};
