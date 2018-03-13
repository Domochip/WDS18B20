#ifndef WirelessDS18B20_h
#define WirelessDS18B20_h

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Main.h"
#include "src\Utils.h"

#include "OneWireDualPin.h"


const char appDataPredefPassword[] PROGMEM = "ewcXoCt4HHjZUvY1";

#define MAX_NUMBER_OF_BUSES 4

//Structure of Application Data 1
class AppData1 {

  public:
    byte numberOfBuses = 0;
    uint8_t owBusesPins[MAX_NUMBER_OF_BUSES][2];


    void SetDefaultValues() {
      numberOfBuses = 0;
      memset(owBusesPins, 0, MAX_NUMBER_OF_BUSES * 2);
    }

    String GetJSON();
    bool SetFromParameters(AsyncWebServerRequest* request, AppData1 &tempAppData);
};


//intermediate class that corresponds to a OneWire Bus with DS12B20 sensors
class DS18B20Bus: public OneWireDualPin {
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

class WebDS18B20Buses {

  private:
    AppData1* _appData1;

    bool _initialized = false;

    boolean isROMCodeString(const char* s);

    String GetStatus();

  public:
    void Init(AppData1 &appData1);
    void InitWebServer(AsyncWebServer &server);
    void Run();
};

#endif
