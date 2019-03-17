#ifndef WirelessDS18B20_h
#define WirelessDS18B20_h

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Main.h"
#include "base\Utils.h"
#include "base\Base.h"

const char appDataPredefPassword[] PROGMEM = "ewcXoCt4HHjZUvY1";

#include "data\status1.html.gz.h"
#include "data\config1.html.gz.h"

#include <math.h> //for std::isnan
#include "OneWireDualPin.h"
#include <PubSubClient.h>
#include "SimpleTimer.h"

#define DEFAULT_CONVERT_PERIOD 30 //Period in seconds used to refresh sensor tmperature if no MQTT used

#define ONEWIRE_PIN_IN D5
#define ONEWIRE_PIN_OUT D6

//intermediate class that corresponds to a OneWire Bus with DS18B20 sensors
class DS18B20Bus : public OneWireDualPin
{
private:
  boolean ReadScratchPad(byte addr[], byte data[]);
  void WriteScratchPad(byte addr[], byte th, byte tl, byte cfg);
  void CopyScratchPad(byte addr[]);

public:
  typedef struct
  {
    uint8_t nbSensors = 0;
    byte (*romCodes)[8] = NULL;
    float *temperatures = NULL;
  } TemperatureList;

  TemperatureList *temperatureList = NULL;

  DS18B20Bus(uint8_t pinIn, uint8_t pinOut);
  void SetupTempSensors(); //Set sensor to 12bits resolution
  void StartConvertT();
  void ReadTemperatures();

  String GetRomCodeListJSON();
  float GetTemp(byte addr[]);
  String GetTempJSON(byte addr[]);
  byte GetAllTemp(uint8_t *&romCodes, float *&temperatures);
  String GetAllTempJSON();
};

//Real Application class
class WebDS18B20Bus : public Application
{
private:
#define HA_MQTT_GENERIC_1 0 //separated level topic (/$romcode$/temperature)
#define HA_MQTT_GENERIC_2 1 //same level topic (/$romcode$)

  typedef struct
  {
    byte type = HA_MQTT_GENERIC_1;
    uint32_t port = 1883;
    char username[128 + 1] = {0};
    char password[150 + 1] = {0};
    struct
    {
      char baseTopic[64 + 1] = {0};
    } generic;
  } MQTT;

#define HA_PROTO_DISABLED 0
#define HA_PROTO_MQTT 1

  typedef struct
  {
    byte protocol = HA_PROTO_DISABLED;
    bool tls = false;
    char hostname[64 + 1] = {0};
    uint16_t uploadPeriod = 60;
    MQTT mqtt;
  } HomeAutomation;

  DS18B20Bus *_ds18b20Bus;

  HomeAutomation ha;
  int _haSendResult = 0;

  bool _initialized = false;
  SimpleTimer _timers[2]; //(0: for Temperature conversion; 1: for HA if enabled)
  WiFiClient *_wifiClient = NULL;
  WiFiClientSecure *_wifiClientSecure = NULL;
  PubSubClient *_pubSubClient = NULL;

  boolean isROMCodeString(const char *s);

  void ConvertTick();
  void UploadTick();

  void SetConfigDefaultValues();
  void ParseConfigJSON(DynamicJsonDocument &doc);
  bool ParseConfigWebRequest(AsyncWebServerRequest *request);
  String GenerateConfigJSON(bool forSaveFile);
  String GenerateStatusJSON();
  bool AppInit(bool reInit);
  const uint8_t *GetHTMLContent(WebPageForPlaceHolder wp);
  size_t GetHTMLContentSize(WebPageForPlaceHolder wp);
  void AppInitWebServer(AsyncWebServer &server, bool &shouldReboot, bool &pauseApplication);
  void AppRun();

public:
  WebDS18B20Bus(char appId, String fileName);
};

#endif
