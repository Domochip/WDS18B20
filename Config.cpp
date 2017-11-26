
#include <Arduino.h>
#include <EEPROM.h>

#include "WirelessDS18B20.h" //for ifdef ESP01_PLATFORM
#include "Config.h"

uint16_t crc16(const uint8_t* data_p, uint16_t length) {
  uint8_t x;
  uint16_t crc = 0xFFFF;

  while (length--) {
    x = crc >> 8 ^ *data_p++;
    x ^= x >> 4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
  }
  return crc;
}


bool Config::Save() {
#ifdef ESP8266
  EEPROM.begin(sizeof(Config));
#endif

  // Init pointer
  uint8_t * p = (uint8_t *) this ;

  // Init CRC
  this->crc = crc16(p, (uint8_t*)&this->crc - (uint8_t*)this);

  //For each byte of Config object
  for (uint16_t i = 0; i < sizeof(Config); ++i) EEPROM.write(i, *(p + i));

#ifdef ESP8266
  EEPROM.end();
#endif

  return Load();
}


bool Config::Load() {
#ifdef ESP8266
  EEPROM.begin(sizeof(Config));
#endif

  //tmpConfig will be used to load EEPROM datas
  Config tmpConfig;

  //create pointer tmpConfig
  uint8_t * p = (uint8_t *) &tmpConfig ;

  // For size of Config, read bytes
  for (uint16_t i = 0; i < sizeof(Config); ++i) *(p + i) = EEPROM.read(i);

#ifdef ESP8266
  EEPROM.end();
#endif


  // Check CRC
  if (crc16(p, (uint8_t*)&tmpConfig.crc - (uint8_t*)&tmpConfig) == tmpConfig.crc) {
    *this = tmpConfig;
    return true;
  }

  return false;
}

//------------------------------------------
//simple function that convert an hexadecimal char to byte
byte Config::AsciiToHex(char c) {
  return (c < 0x3A) ? (c - 0x30) : (c > 0x60 ? c - 0x57 : c - 0x37);
}

//------------------------------------------
// Function to decode https FingerPrint String into array of 20 bytes
bool Config::FingerPrintS2A(byte* fingerPrintArray, const char* fingerPrintToDecode) {

  if (strlen(fingerPrintToDecode) < 40) return false;

  byte arrayPos = 0;
  for (byte i = 0; i < strlen(fingerPrintToDecode); i++) {

    if (fingerPrintToDecode[i] != ' ' && fingerPrintToDecode[i] != ':' && fingerPrintToDecode[i] != '-') {
      fingerPrintArray[arrayPos / 2] += AsciiToHex(fingerPrintToDecode[i]);
      if (arrayPos % 2 == 0) fingerPrintArray[arrayPos / 2] *= 0x10;
      arrayPos++;
    }
    if (arrayPos == 40) return false;
  }

  return true;
}
//------------------------------------------
// Function that convert fingerprint Array to char array (with separator) (char array need to be provided)
char* Config::FingerPrintA2S(char* fpBuffer, byte* fingerPrintArray, char separator) {

  fpBuffer[0] = 0;

  for (byte i = 0; i < 20; i++) {
    sprintf_P(fpBuffer, PSTR("%s%02x"), fpBuffer, fingerPrintArray[i]);
    if (i != 19 && separator != 0) {
      fpBuffer[strlen(fpBuffer) + 1] = 0;
      fpBuffer[strlen(fpBuffer)] = separator;
    }
  }
  return fpBuffer;
}

void Config::InitWebServer(AsyncWebServer &server, bool &shouldReboot) {

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, F("text/html"), (const uint8_t*)confightmlgz, sizeof(confightmlgz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/gc", HTTP_GET, [this](AsyncWebServerRequest * request) {
    request->send(200, F("text/json"), GetJSON());
  });

  server.on("/sc", HTTP_POST, [this, &shouldReboot](AsyncWebServerRequest * request) {
    shouldReboot = SetFromParameters(request);
  });

  server.on("/wnl", HTTP_GET, [this](AsyncWebServerRequest * request) {
    WiFi.scanNetworksAsync([request](int n) {
      String networksJSON(F("{\"wnl\":["));
      for (int i = 0; i < n; i++) {
        networksJSON = networksJSON + '"' + WiFi.SSID(i) + '"';
        if (i != (n - 1)) networksJSON += ',';
      }
      networksJSON += F("]}");
      request->send(200, F("text/json"), networksJSON);
    });
  });
}

String Config::GetJSON() {

  //{"s":"Wifi","h":"TotoPC","n":1,"b0i":3,"b0o":0,"b":"1.4 (ESP01)","u":"5d23h50m","f":45875}

  String gc = F("{\"s\":\"");
  //there is a predefined special password (mean to keep already saved one)
  gc = gc + ssid + F("\",\"p\":\"") + (__FlashStringHelper*)predefPassword + F("\",\"h\":\"") + hostname + '"';

#if !ESP01_PLATFORM
  gc = gc + F(",\"n\":") + numberOfBuses + F(",\"nm\":") + MAX_NUMBER_OF_BUSES;
  for (int i = 0; i < numberOfBuses; i++) {
    gc = gc + F(",\"b") + i + F("i\":") + owBusesPins[i][0] + F(",\"b") + i + F("o\":") + owBusesPins[i][1];
  }
#else
  gc += F(",\"e\":1,\"n\":1,\"nm\":1,\"b0i\":3,\"b0o\":0");
#endif

  gc += '}';

  return gc;
}

bool Config::SetFromParameters(AsyncWebServerRequest* request) {

  //temp config
  Config tempConfig;

  //Parse Parameters
  if (!request->hasParam(F("s"), true)) {
    request->send(400, F("text/html"), F("SSID missing"));
    return false;
  }
  else strcpy(tempConfig.ssid, request->getParam(F("s"), true)->value().c_str());

  if (request->hasParam(F("p"), true) && request->getParam(F("p"), true)->value().length() < sizeof(tempConfig.password)) strcpy(tempConfig.password, request->getParam(F("p"), true)->value().c_str());
  if (request->hasParam(F("h"), true) && request->getParam(F("h"), true)->value().length() < sizeof(tempConfig.hostname)) strcpy(tempConfig.hostname, request->getParam(F("h"), true)->value().c_str());


#if !ESP01_PLATFORM
  char tempNumberOfBusesA[2]; //only one char
  if (!request->hasParam(F("n"), true)) {
    request->send(400, F("text/html"), F("Missing number of OW Buses"));
    return false;
  }

  tempConfig.numberOfBuses = request->getParam(F("n"), true)->value().toInt();
  if (tempConfig.numberOfBuses < 1 || tempConfig.numberOfBuses > MAX_NUMBER_OF_BUSES) {
    request->send(400, F("text/html"), F("Incorrect number of OW Buses"));
    return false;
  }
  char busPinName[4] = {'b', '0', 'i', 0};
  for (int i = 0; i < tempConfig.numberOfBuses; i++) {
    char busPinA[4] = {0};
    busPinName[1] = '0' + i;
    busPinName[2] = 'i';
    if (!!request->hasParam(busPinName, true)) {
      request->send(400, F("text/html"), F("A PinIn value is missing"));
      return false;
    }
    if (request->getParam(busPinName, true)->value().toInt() == 0 && request->getParam(busPinName, true)->value() != "0") {
      request->send(400, F("text/html"), F("A PinIn value is incorrect"));
      return false;
    }
    tempConfig.owBusesPins[i][0] = request->getParam(busPinName, true)->value().toInt();

    busPinA[0] = 0;
    busPinName[2] = 'o';
    if (!!request->hasParam(busPinName, true)) {
      request->send(400, F("text/html"), F("A PinOut value is missing"));
      return false;
    }
    if (request->getParam(busPinName, true)->value().toInt() == 0 && request->getParam(busPinName, true)->value() != "0") {
      request->send(400, F("text/html"), F("A PinOut value is incorrect"));
      return false;
    }
    tempConfig.owBusesPins[i][1] = request->getParam(busPinName, true)->value().toInt();
  }
#endif

  //check for previous password ssid and apiKey (there is a predefined special password that mean to keep already saved one)
  if (!strcmp_P(tempConfig.password, predefPassword)) strcpy(tempConfig.password, password);

#if ESP01_PLATFORM
  tempConfig.numberOfBuses = 1;
  tempConfig.owBusesPins[0][0] = 3;
  tempConfig.owBusesPins[0][1] = 0;
#endif

  //then save
  bool result = tempConfig.Save();

  //Send client answer
  if (result) request->send(200);
  else request->send(500, F("text/html"), F("Configuration hasn't been saved"));

  return result;
}


