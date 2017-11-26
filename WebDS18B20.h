#ifndef WebDS18B20_h
#define WebDS18B20_h

#include "OneWireDualPin.h"

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

class WebDS18B20Buses {
  private:
    bool _initialized = false;
    byte _nbOfBuses;
    uint8_t (*_owBusesPins)[2];

    static byte AsciiToHex(char c); //Utils
    boolean isROMCodeString(const char* s);
    String GetStatus();

  public:
    void Init(byte nbOfBuses, uint8_t owBusesPins[][2]);
    void InitWebServer(AsyncWebServer &server);

};

#endif
