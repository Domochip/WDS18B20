#ifndef WirelessDS18B20_h
#define WirelessDS18B20_h

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Main.h"
#include "src\Utils.h"
#include "src\Base.h"

#include "OneWireDualPin.h"

const char appDataPredefPassword[] PROGMEM = "ewcXoCt4HHjZUvY1";

#define MAX_NUMBER_OF_BUSES 4

//intermediate class that corresponds to a OneWire Bus with DS12B20 sensors
class DS18B20Bus : public OneWireDualPin
{
private:
  boolean ReadScratchPad(byte addr[], byte data[]);
  void WriteScratchPad(byte addr[], byte th, byte tl, byte cfg);
  void CopyScratchPad(byte addr[]);
  void StartConvertT(byte addr[]);

public:
  DS18B20Bus(uint8_t pinIn, uint8_t pinOut);
  void SetupTempSensors();
  String GetTempJSON(byte addr[]);
  String GetRomCodeListJSON();
};

//Real Application class
class WebDS18B20Buses : public Application
{
private:
  byte numberOfBuses = 0;
  uint8_t owBusesPins[MAX_NUMBER_OF_BUSES][2];

  bool _initialized = false;

  boolean isROMCodeString(const char *s);

  void SetConfigDefaultValues();
  void ParseConfigJSON(JsonObject &root);
  bool ParseConfigWebRequest(AsyncWebServerRequest *request);
  String GenerateConfigJSON(bool forSaveFile);
  String GenerateStatusJSON();
  bool AppInit(bool reInit);
  void AppInitWebServer(AsyncWebServer &server, bool &shouldReboot, bool &pauseApplication);
  void AppRun();

public:
  WebDS18B20Buses(char appId, String fileName);
};

#endif
