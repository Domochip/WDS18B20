#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "WirelessDS18B20.h"


//Return JSON of AppData1 content
String AppData1::GetJSON() {

  String gc;

#if !ESP01_PLATFORM
  gc = gc + F("\"n\":") + numberOfBuses + F(",\"nm\":") + MAX_NUMBER_OF_BUSES;
  for (int i = 0; i < numberOfBuses; i++) {
    gc = gc + F(",\"b") + i + F("i\":") + owBusesPins[i][0] + F(",\"b") + i + F("o\":") + owBusesPins[i][1];
  }
#else
  gc += F("\"e\":1,\"n\":1,\"nm\":1,\"b0i\":3,\"b0o\":0");
#endif

  return gc;
}

//Parse HTTP Request into an AppData1 structure
bool AppData1::SetFromParameters(AsyncWebServerRequest* request, AppData1 &tempAppData1) {

#if !ESP01_PLATFORM
  char tempNumberOfBusesA[2]; //only one char
  if (!request->hasParam(F("n"), true)) {
    request->send(400, F("text/html"), F("Missing number of OW Buses"));
    return false;
  }

  tempAppData1.numberOfBuses = request->getParam(F("n"), true)->value().toInt();
  if (tempAppData1.numberOfBuses < 1 || tempAppData1.numberOfBuses > MAX_NUMBER_OF_BUSES) {
    request->send(400, F("text/html"), F("Incorrect number of OW Buses"));
    return false;
  }
  char busPinName[4] = {'b', '0', 'i', 0};
  for (byte i = 0; i < tempAppData1.numberOfBuses; i++) {
    char busPinA[4] = {0};
    busPinName[1] = '0' + i;
    busPinName[2] = 'i';
    if (!request->hasParam(busPinName, true)) {
      request->send(400, F("text/html"), F("A PinIn value is missing"));
      return false;
    }
    if (request->getParam(busPinName, true)->value().toInt() == 0 && request->getParam(busPinName, true)->value() != "0") {
      request->send(400, F("text/html"), F("A PinIn value is incorrect"));
      return false;
    }
    tempAppData1.owBusesPins[i][0] = request->getParam(busPinName, true)->value().toInt();

    busPinA[0] = 0;
    busPinName[2] = 'o';
    if (!request->hasParam(busPinName, true)) {
      request->send(400, F("text/html"), F("A PinOut value is missing"));
      return false;
    }
    if (request->getParam(busPinName, true)->value().toInt() == 0 && request->getParam(busPinName, true)->value() != "0") {
      request->send(400, F("text/html"), F("A PinOut value is incorrect"));
      return false;
    }
    tempAppData1.owBusesPins[i][1] = request->getParam(busPinName, true)->value().toInt();
  }
#endif

#if ESP01_PLATFORM
  tempAppData1.numberOfBuses = 1;
  tempAppData1.owBusesPins[0][0] = 3;
  tempAppData1.owBusesPins[0][1] = 0;
#endif

  return true;
}


//----------------------------------------------------------------------
// --- DS18B20Bus Class---
//----------------------------------------------------------------------

//-----------------------------------------------------------------------
// DS18X20 Read ScratchPad command
boolean DS18B20Bus::ReadScratchPad(byte addr[], byte data[]) {

  boolean crcScratchPadOK = false;

  //read scratchpad (if 3 failures occurs, then return the error
  for (byte i = 0; i < 3; i++) {
    // read scratchpad of the current device
    reset();
    select(addr);
    write(0xBE); // Read ScratchPad
    for (byte j = 0; j < 9; j++) { // read 9 bytes
      data[j] = read();
    }
    if (crc8(data, 8) == data[8]) {
      crcScratchPadOK = true;
      i = 3; //end for loop
    }
  }

  return crcScratchPadOK;
}
//------------------------------------------
// DS18X20 Write ScratchPad command
void DS18B20Bus::WriteScratchPad(byte addr[], byte th, byte tl, byte cfg) {

  reset();
  select(addr);
  write(0x4E); // Write ScratchPad
  write(th); //Th 80°C
  write(tl); //Tl 0°C
  write(cfg); //Config
}
//------------------------------------------
// DS18X20 Copy ScratchPad command
void DS18B20Bus::CopyScratchPad(byte addr[]) {

  reset();
  select(addr);
  write(0x48); //Copy ScratchPad
}
//------------------------------------------
// DS18X20 Start Temperature conversion
void DS18B20Bus::StartConvertT(byte addr[]) {
  reset();
  select(addr);
  write(0x44); // start conversion
}

//------------------------------------------
// Constructor for WebDS18B20Bus that call constructor of parent class OneWireDualPin
DS18B20Bus::DS18B20Bus(uint8_t pinIn, uint8_t pinOut): OneWireDualPin(pinIn, pinOut) {};
//------------------------------------------
// Function to initialize DS18X20 sensors
void DS18B20Bus::SetupTempSensors() {

  byte i, j;
  byte addr[8];
  byte data[9];
  boolean scratchPadReaded;

  //while we find some devices
  while (search(addr)) {

    //if ROM received is incorrect or not a DS1822 or DS18B20 THEN continue to next device
    if ((crc8(addr, 7) != addr[7]) || (addr[0] != 0x22 && addr[0] != 0x28)) continue;

    scratchPadReaded = ReadScratchPad(addr, data);
    //if scratchPad read failed then continue to next 1-Wire device
    if (!scratchPadReaded) continue;

    //if config is not correct
    if (data[2] != 0x50 || data[3] != 0x00 || data[4] != 0x5F) {

      //write ScratchPad with Th=80°C, Tl=0°C, Config 11bit resolution
      WriteScratchPad(addr, 0x50, 0x00, 0x5F);

      scratchPadReaded = ReadScratchPad(addr, data);
      //if scratchPad read failed then continue to next 1-Wire device
      if (!scratchPadReaded) continue;

      //so we finally can copy scratchpad to memory
      CopyScratchPad(addr);
    }
  }
}
//------------------------------------------
// function that get temperature from a DS18X20 and return it in JSON (run convertion, get scratchpad then calculate temperature)
String DS18B20Bus::GetTempJSON(byte addr[]) {

  byte i, j;
  byte data[12];

  StartConvertT(addr);

  //wait for conversion end (DS18B20 are powered)
  while (read_bit() == 0) delay(10);

  //if read of scratchpad failed (3 times inside function) then return empty String
  if (!ReadScratchPad(addr, data)) return String();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (addr[0] == 0x10) { //type S temp Sensor
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  //result is (float)raw / 16.0;

  //make JSON
  String gtJSON(F("{\"Temperature\": "));
  gtJSON.reserve(28);
  gtJSON += String((float)raw / 16.0, 2) + '}';

  //return JSON temperature
  return gtJSON;
}
//------------------------------------------
// List DS18X20 sensor ROMCode and return it in JSON list
String DS18B20Bus::GetRomCodeListJSON() {

  bool first = true;
  uint8_t romCode[8];

  //prepare JSON structure
  String grclJSON(F("{\"TemperatureSensorList\": [\r\n"));

  reset_search();

  while (search(romCode)) {

    //if ROM received is incorrect or not a Temperature sensor THEN continue to next device
    if ((crc8(romCode, 7) != romCode[7]) || (romCode[0] != 0x10 && romCode[0] != 0x22 && romCode[0] != 0x28)) continue;

    //increase grclJSON size to limit heap fragment
    grclJSON.reserve(grclJSON.length() + 22);

    //populate JSON answer with romCode found
    if (!first) grclJSON += F(",\r\n");
    else first = false;
    grclJSON += '"';
    for (byte i = 0; i < 8; i++) {
      if (romCode[i] < 16)grclJSON += '0';
      grclJSON += String(romCode[i], HEX);
    }
    grclJSON += '"';
  }
  //Finalize JSON structure
  grclJSON += F("\r\n]}");

  return grclJSON;
}

//----------------------------------------------------------------------
// --- WebDS18B20Buses Class---
//----------------------------------------------------------------------
//------------------------------------------
// return True if s contain only hexadecimal figure
boolean WebDS18B20Buses::isROMCodeString(const char* s) {

  if (strlen(s) != 16) return false;
  for (int i = 0; i < 16; i++) {
    if (!isHexadecimalDigit(s[i])) return false;
  }
  return true;
}


//------------------------------------------
//return WebDS18B20Buses Status in JSON
String WebDS18B20Buses::GetStatus() {

  String statusJSON('{');
  //nothing to send yet
  statusJSON += '}';

  return statusJSON;
}


//------------------------------------------
//Function to initiate WebDS18B20Buses with Config
void WebDS18B20Buses::Init(AppData1 &appData1) {

  Serial.print(F("Start WebDS18B20Buses"));

  _appData1 = &appData1;

#if ESP01_PLATFORM
  _appData1->numberOfBuses = 1;
  _appData1->owBusesPins[0][0] = 3;
  _appData1->owBusesPins[0][1] = 0;
#endif

  _initialized = (_appData1->numberOfBuses > 0);


#if ESP01_PLATFORM
  Serial.flush();
  delay(5);
  Serial.end();
#endif

  for (byte i = 0; i < _appData1->numberOfBuses; i++) {
    DS18B20Bus(_appData1->owBusesPins[i][0], _appData1->owBusesPins[i][1]).SetupTempSensors();
  }

#if ESP01_PLATFORM
  Serial.begin(SERIAL_SPEED);
#endif

  Serial.println(F(" : OK"));
}

//------------------------------------------
void WebDS18B20Buses::InitWebServer(AsyncWebServer &server) {

  server.on("/gs1", HTTP_GET, [this](AsyncWebServerRequest * request) {
    request->send(200, F("text/json"), GetStatus());
  });


  server.on("/getL", HTTP_GET, [this](AsyncWebServerRequest * request) {

    bool requestPassed = false;
    byte busNumberPassed = 0;

    //check DS18B20Buses is initialized
    if (!_initialized) {
      request->send(400, F("text/html"), F("Buses not Initialized"));
      return;
    }

    char paramName[5] = {'b', 'u', 's', '0', 0};
    byte i = 0;

    //for each bus numbers
    while (i < _appData1->numberOfBuses && !requestPassed) {
      //build paramName
      paramName[3] = '0' + i;

      //check bus param is there
      if (request->hasParam(paramName)) {
        busNumberPassed = i;
        requestPassed = true;
      }
      i++;
    }

    //if no correct request passed
    if (!requestPassed) {
      //answer with error and return
      request->send(400, F("text/html"), F("No valid request received"));
      return;
    }

#if ESP01_PLATFORM
    Serial.flush();
    delay(5);
    Serial.end();
#endif

    //list OneWire Temperature sensors
    request->send(200, F("text/json"), DS18B20Bus(_appData1->owBusesPins[busNumberPassed][0], _appData1->owBusesPins[busNumberPassed][1]).GetRomCodeListJSON());

#if ESP01_PLATFORM
    Serial.begin(SERIAL_SPEED);
#endif
  });

  server.on("/getT", HTTP_GET, [this](AsyncWebServerRequest * request) {

    bool requestPassed = false;
    byte busNumberPassed = 0;
    byte romCodePassed[8];

    //check DS18B20Buses is initialized
    if (!_initialized) {
      request->send(400, F("text/html"), F("Buses not Initialized"));
      return;
    }

    char paramName[5] = {'b', 'u', 's', '0', 0};
    byte i = 0;

    //for each bus numbers
    while (i < _appData1->numberOfBuses && !requestPassed) {
      //build paramName
      paramName[3] = '0' + i;

      //check bus param is there
      if (request->hasParam(paramName)) {

        //get ROMCode
        const char* ROMCodeA = request->getParam(paramName)->value().c_str();
        //if it's a correct ROMCode
        if (isROMCodeString(ROMCodeA)) {
          //Parse it
          for (byte j = 0; j < 8; j++) romCodePassed[j] = (Utils::AsciiToHex(ROMCodeA[j * 2]) * 0x10) + Utils::AsciiToHex(ROMCodeA[(j * 2) + 1]);
          busNumberPassed = i;
          requestPassed = true;
        }
      }
      i++;
    }

    //if no correct request passed
    if (!requestPassed) {
      //answer with error and return
      request->send(400, F("text/html"), F("No valid request received"));
      return;
    }


#if ESP01_PLATFORM
    Serial.flush();
    delay(5);
    Serial.end();
#endif

    //Read Temperature
    String temperatureJSON = DS18B20Bus(_appData1->owBusesPins[busNumberPassed][0], _appData1->owBusesPins[busNumberPassed][1]).GetTempJSON(romCodePassed);

#if ESP01_PLATFORM
    Serial.begin(SERIAL_SPEED);
#endif

    if (temperatureJSON.length() > 0) request->send(200, F("text/json"), temperatureJSON);
    else request->send(500, F("text/html"), F("Read sensor failed"));
  });

}

//------------------------------------------
//Run for timer
void WebDS18B20Buses::Run() {
  //nothing to do
}
