#include <ESP8266WiFi.h>

#include "WirelessDS18B20.h"

//----------------------------------------------------------------------
// --- DS18B20Bus Class---
//----------------------------------------------------------------------

//-----------------------------------------------------------------------
// DS18X20 Read ScratchPad command
boolean DS18B20Bus::ReadScratchPad(byte addr[], byte data[])
{

  boolean crcScratchPadOK = false;

  //read scratchpad (if 3 failures occurs, then return the error
  for (byte i = 0; i < 3; i++)
  {
    // read scratchpad of the current device
    reset();
    select(addr);
    write(0xBE); // Read ScratchPad
    for (byte j = 0; j < 9; j++)
    { // read 9 bytes
      data[j] = read();
    }
    if (crc8(data, 8) == data[8])
    {
      crcScratchPadOK = true;
      i = 3; //end for loop
    }
  }

  return crcScratchPadOK;
}
//------------------------------------------
// DS18X20 Write ScratchPad command
void DS18B20Bus::WriteScratchPad(byte addr[], byte th, byte tl, byte cfg)
{

  reset();
  select(addr);
  write(0x4E); // Write ScratchPad
  write(th);   //Th 80°C
  write(tl);   //Tl 0°C
  write(cfg);  //Config
}
//------------------------------------------
// DS18X20 Copy ScratchPad command
void DS18B20Bus::CopyScratchPad(byte addr[])
{

  reset();
  select(addr);
  write(0x48); //Copy ScratchPad
}

//------------------------------------------
// Constructor for WebDS18B20Bus that call constructor of parent class OneWireDualPin
DS18B20Bus::DS18B20Bus(uint8_t pinIn, uint8_t pinOut) : OneWireDualPin(pinIn, pinOut){};
//------------------------------------------
// Function to initialize DS18X20 sensors
void DS18B20Bus::SetupTempSensors()
{

  byte addr[8];
  byte data[9];
  boolean scratchPadReaded;

  //while we find some devices
  while (search(addr))
  {

    //if ROM received is incorrect or not a DS1822 or DS18B20 THEN continue to next device
    if ((crc8(addr, 7) != addr[7]) || (addr[0] != 0x22 && addr[0] != 0x28))
      continue;

    scratchPadReaded = ReadScratchPad(addr, data);
    //if scratchPad read failed then continue to next 1-Wire device
    if (!scratchPadReaded)
      continue;

    //if config is not correct
    if (data[2] != 0x50 || data[3] != 0x00 || data[4] != 0x7F)
    {
      //write ScratchPad with Th=80°C, Tl=0°C, Config 12bits resolution
      WriteScratchPad(addr, 0x50, 0x00, 0x7F);

      scratchPadReaded = ReadScratchPad(addr, data);
      //if scratchPad read failed then continue to next 1-Wire device
      if (!scratchPadReaded)
        continue;

      //so we finally can copy scratchpad to memory
      CopyScratchPad(addr);
    }
  }
}
//------------------------------------------
// DS18X20 Start Temperature conversion
void DS18B20Bus::StartConvertT()
{
  reset();
  skip();
  write(0x44); // start conversion
}
//------------------------------------------
// DS18X20 Read Temperatures from all sensors
void DS18B20Bus::ReadTemperatures()
{
  //tempList will receive new list and values
  TemperatureList *tempList = new TemperatureList();

  uint8_t romCode[8];

  //list all romCodes
  reset_search();
  while (search(romCode))
  {

    //if ROM received is incorrect or not a Temperature sensor THEN continue to next device
    if ((crc8(romCode, 7) != romCode[7]) || (romCode[0] != 0x10 && romCode[0] != 0x22 && romCode[0] != 0x28))
      continue;

    //allocate memory
    if (!tempList->romCodes)
      tempList->romCodes = (byte(*)[8])malloc(++tempList->nbSensors * 8 * sizeof(byte));
    else
      tempList->romCodes = (byte(*)[8])realloc(tempList->romCodes, ++tempList->nbSensors * 8 * sizeof(byte));

    //copy the romCode
    for (byte i = 0; i < 8; i++)
    {
      tempList->romCodes[tempList->nbSensors - 1][i] = romCode[i];
    }
  }

  //prepare temperatures list
  tempList->temperatures = (float *)malloc(tempList->nbSensors * sizeof(float));
  byte data[12]; //buffer that receive scratchpad
  //now read all temperatures
  for (byte i = 0; i < tempList->nbSensors; i++)
  {
    //if read of scratchpad failed (3 times inside function) then put NaN in the list
    if (!ReadScratchPad(tempList->romCodes[i], data))
      tempList->temperatures[i] = (0.0 / 0.0); //NaN
    else                                       //else we were able to read data from sensor
    {

      // Convert the data to actual temperature
      // because the result is a 16 bit signed integer, it should
      // be stored to an "int16_t" type, which is always 16 bits
      // even when compiled on a 32 bit processor.
      int16_t raw = (data[1] << 8) | data[0];
      if (tempList->romCodes[i][0] == 0x10)
      {                 //type S temp Sensor
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10)
        {
          // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
      }
      else
      {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00)
          raw = raw & ~7; // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20)
          raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40)
          raw = raw & ~1; // 11 bit res, 375 ms
        // default is 12 bit resolution, 750 ms conversion time
      }

      //final temperature is raw/16

      //return temperature
      tempList->temperatures[i] = (float)raw / 16.0;
    }
  }

  //Our new value list is ready
  //switch to make it available
  TemperatureList *oldList = temperatureList; //backup current one
  temperatureList = tempList;                 //make new one available

  //now we need to free memory of the old one
  if (oldList)
  {
    if (oldList->romCodes)
      free(oldList->romCodes);
    if (oldList->temperatures)
      free(oldList->temperatures);
    free(oldList);
  }
}
//------------------------------------------
// List DS18X20 sensor ROMCode and return it in JSON list
String DS18B20Bus::GetRomCodeListJSON()
{
  //prepare JSON structure
  String grclJSON(F("{\"TemperatureSensorList\": [\r\n"));

  if (temperatureList)
  {
    for (byte i = 0; i < temperatureList->nbSensors; i++)
    {

      //populate JSON answer with romCode found
      if (i)
        grclJSON += F(",\r\n");

      grclJSON += '"';
      for (byte j = 0; j < 8; j++)
      {
        if (temperatureList->romCodes[i][j] < 16)
          grclJSON += '0';
        grclJSON += String(temperatureList->romCodes[i][j], HEX);
      }
      grclJSON += '"';
    }
  }

  //Finalize JSON structure
  grclJSON += F("\r\n]}");

  return grclJSON;
}

//------------------------------------------
// function that get temperature from a DS18X20 and return it in float (run convertion, get scratchpad then calculate temperature)
float DS18B20Bus::GetTemp(byte addr[] = NULL)
{

  if (!temperatureList)
    return (0.0 / 0.0);

  byte pos = 0;

  while (pos < temperatureList->nbSensors)
  {
    //if romCode match
    if (addr[0] == temperatureList->romCodes[pos][0] && addr[1] == temperatureList->romCodes[pos][1] && addr[2] == temperatureList->romCodes[pos][2] && addr[3] == temperatureList->romCodes[pos][3] && addr[4] == temperatureList->romCodes[pos][4] && addr[5] == temperatureList->romCodes[pos][5] && addr[6] == temperatureList->romCodes[pos][6] && addr[7] == temperatureList->romCodes[pos][7])
      break; //stop while
    pos++;
  }

  if (pos == temperatureList->nbSensors) //if we reached the end of the list
    return (0.0 / 0.0);                  //return NaN
  else
    return temperatureList->temperatures[pos]; //return temperature
}

//------------------------------------------
// function that get temperature from a DS18X20 and return it in JSON
String DS18B20Bus::GetTempJSON(byte addr[])
{
  //get temperature
  float res = GetTemp(addr);

  //if read failed return empty string
  if (std::isnan(res))
    return String();

  //otherwise make and return JSON
  String gtJSON(F("{\"Temperature\": "));
  gtJSON += String(res, 2);
  gtJSON = gtJSON + '}';

  return gtJSON;
}

//------------------------------------------
// function that get temperature from all DS18X20 and return it in JSON
String DS18B20Bus::GetAllTempJSON()
{
  //JSON string to return
  String gatJSON('{');

  if (temperatureList)
  {
    //for each sensors
    for (byte i = 0; i < temperatureList->nbSensors; i++)
    {
      //build JSON
      if (i)
        gatJSON += ',';

      gatJSON += '"';
      for (byte j = 0; j < 8; j++)
      {
        if (temperatureList->romCodes[i][j] < 16)
          gatJSON += '0';
        gatJSON += String(temperatureList->romCodes[i][j], HEX);
      }
      gatJSON += F("\":");
      if (!std::isnan(temperatureList->temperatures[i]))
        gatJSON += String(temperatureList->temperatures[i], 2);
      else
        gatJSON += F("\"NaN\"");
    }
  }

  gatJSON += '}';

  //return JSON temperatures
  return gatJSON;
}

//----------------------------------------------------------------------
// --- WebDS18B20Buses Class---
//----------------------------------------------------------------------
//------------------------------------------
// return True if s contain only hexadecimal figure
boolean WebDS18B20Bus::isROMCodeString(const char *s)
{

  if (strlen(s) != 16)
    return false;
  for (int i = 0; i < 16; i++)
  {
    if (!isHexadecimalDigit(s[i]))
      return false;
  }
  return true;
}

//------------------------------------------
// Execute code to start temperature conversion of all sensors
void WebDS18B20Bus::ConvertTick()
{
  //StartConvert
  _ds18b20Bus->StartConvertT();
  delay(800);
  //ReadTemp
  _ds18b20Bus->ReadTemperatures();
}

//------------------------------------------
// Connect then Subscribe to MQTT
bool WebDS18B20Bus::MqttConnect()
{
  if (!WiFi.isConnected())
    return false;

  char sn[9];
  sprintf_P(sn, PSTR("%08x"), ESP.getChipId());

  //generate clientID
  String clientID(F(APPLICATION1_NAME));
  clientID += sn;

  //Connect
  if (!_ha.mqtt.username[0])
    _mqttClient.connect(clientID.c_str());
  else
    _mqttClient.connect(clientID.c_str(), _ha.mqtt.username, _ha.mqtt.password);

  if (_mqttClient.connected())
  {
    //Subscribe to needed topic
  }

  return _mqttClient.connected();
}

//------------------------------------------
//Callback used when an MQTT message arrived
void WebDS18B20Bus::MqttCallback(char *topic, uint8_t *payload, unsigned int length) {}

//------------------------------------------
// Execute code to upload temperature to MQTT if enable
void WebDS18B20Bus::PublishTick()
{
  //if Home Automation upload not enabled then return
  if (_ha.protocol == HA_PROTO_DISABLED)
    return;

  //----- MQTT Protocol configured -----
  if (_ha.protocol == HA_PROTO_MQTT)
  {
    //if we are connected
    if (_mqttClient.connected())
    {
      //prepare topic
      String completeTopic, thisSensorTopic;
      switch (_ha.mqtt.type)
      {
      case HA_MQTT_GENERIC_1:
        completeTopic = _ha.mqtt.generic.baseTopic;

        //check for final slash
        if (completeTopic.length() && completeTopic.charAt(completeTopic.length() - 1) != '/')
          completeTopic += '/';

        //complete the topic
        completeTopic += F("$romcode$/temperature");
        break;
      case HA_MQTT_GENERIC_2:
        completeTopic = _ha.mqtt.generic.baseTopic;

        //check for final slash
        if (completeTopic.length() && completeTopic.charAt(completeTopic.length() - 1) != '/')
          completeTopic += '/';

        //complete the topic
        completeTopic += F("$romcode$");
        break;
      }

      //Replace placeholders
      if (completeTopic.indexOf(F("$sn$")) != -1)
      {
        char sn[9];
        sprintf_P(sn, PSTR("%08x"), ESP.getChipId());
        completeTopic.replace(F("$sn$"), sn);
      }

      if (completeTopic.indexOf(F("$mac$")) != -1)
        completeTopic.replace(F("$mac$"), WiFi.macAddress());

      if (completeTopic.indexOf(F("$model$")) != -1)
        completeTopic.replace(F("$model$"), APPLICATION1_NAME);

      char romCodeA[17] = {0};

      if (_ds18b20Bus->temperatureList) //if there is a list
      {
        _haSendResult = true;

        //for each sensors found
        for (byte i = 0; i < _ds18b20Bus->temperatureList->nbSensors && _haSendResult; i++)
        {
          //if temperature is OK
          if (!std::isnan(_ds18b20Bus->temperatureList->temperatures[i]))
          {
            //convert romCode to text in oneshot
            sprintf_P(romCodeA, PSTR("%02x%02x%02x%02x%02x%02x%02x%02x"), _ds18b20Bus->temperatureList->romCodes[i][0], _ds18b20Bus->temperatureList->romCodes[i][1], _ds18b20Bus->temperatureList->romCodes[i][2], _ds18b20Bus->temperatureList->romCodes[i][3], _ds18b20Bus->temperatureList->romCodes[i][4], _ds18b20Bus->temperatureList->romCodes[i][5], _ds18b20Bus->temperatureList->romCodes[i][6], _ds18b20Bus->temperatureList->romCodes[i][7]);

            //copy completeTopic in order to "complete" it ...
            thisSensorTopic = completeTopic;

            if (thisSensorTopic.indexOf(F("$romcode$")) != -1)
              thisSensorTopic.replace(F("$romcode$"), romCodeA);

            //send
            _haSendResult = _mqttClient.publish(thisSensorTopic.c_str(), String(_ds18b20Bus->temperatureList->temperatures[i], 2).c_str());
          }
        }
      }
    }
  }
}

//------------------------------------------
//Used to initialize configuration properties to default values
void WebDS18B20Bus::SetConfigDefaultValues()
{
  _ds18b20Bus = NULL;

  _ha.protocol = HA_PROTO_DISABLED;
  _ha.tls = false;
  _ha.hostname[0] = 0;
  _ha.uploadPeriod = 60;

  _ha.mqtt.type = HA_MQTT_GENERIC_1;
  _ha.mqtt.port = 1883;
  _ha.mqtt.username[0] = 0;
  _ha.mqtt.password[0] = 0;
  _ha.mqtt.generic.baseTopic[0] = 0;
};
//------------------------------------------
//Parse JSON object into configuration properties
void WebDS18B20Bus::ParseConfigJSON(DynamicJsonDocument &doc)
{
  if (!doc[F("haproto")].isNull())
    _ha.protocol = doc[F("haproto")];
  if (!doc[F("hatls")].isNull())
    _ha.tls = doc[F("hatls")];
  if (!doc[F("hahost")].isNull())
    strlcpy(_ha.hostname, doc[F("hahost")], sizeof(_ha.hostname));
  if (!doc[F("haupperiod")].isNull())
    _ha.uploadPeriod = doc[F("haupperiod")];

  if (!doc[F("hamtype")].isNull())
    _ha.mqtt.type = doc[F("hamtype")];
  if (!doc[F("hamport")].isNull())
    _ha.mqtt.port = doc[F("hamport")];
  if (!doc[F("hamu")].isNull())
    strlcpy(_ha.mqtt.username, doc[F("hamu")], sizeof(_ha.mqtt.username));
  if (!doc[F("hamp")].isNull())
    strlcpy(_ha.mqtt.password, doc[F("hamp")], sizeof(_ha.mqtt.password));

  if (!doc[F("hamgbt")].isNull())
    strlcpy(_ha.mqtt.generic.baseTopic, doc[F("hamgbt")], sizeof(_ha.mqtt.generic.baseTopic));
};
//------------------------------------------
//Parse HTTP POST parameters in request into configuration properties
bool WebDS18B20Bus::ParseConfigWebRequest(AsyncWebServerRequest *request)
{

  //Parse HA protocol
  if (request->hasParam(F("haproto"), true))
    _ha.protocol = request->getParam(F("haproto"), true)->value().toInt();

  //if an home Automation protocol has been selected then get common param
  if (_ha.protocol != HA_PROTO_DISABLED)
  {
    if (request->hasParam(F("hatls"), true))
      _ha.tls = (request->getParam(F("hatls"), true)->value() == F("on"));
    else
      _ha.tls = false;
    if (request->hasParam(F("hahost"), true) && request->getParam(F("hahost"), true)->value().length() < sizeof(_ha.hostname))
      strcpy(_ha.hostname, request->getParam(F("hahost"), true)->value().c_str());
    if (request->hasParam(F("haupperiod"), true))
      _ha.uploadPeriod = request->getParam(F("haupperiod"), true)->value().toInt();
  }

  //Now get specific param
  switch (_ha.protocol)
  {

  case HA_PROTO_MQTT:

    if (request->hasParam(F("hamtype"), true))
      _ha.mqtt.type = request->getParam(F("hamtype"), true)->value().toInt();
    if (request->hasParam(F("hamport"), true))
      _ha.mqtt.port = request->getParam(F("hamport"), true)->value().toInt();
    if (request->hasParam(F("hamu"), true) && request->getParam(F("hamu"), true)->value().length() < sizeof(_ha.mqtt.username))
      strcpy(_ha.mqtt.username, request->getParam(F("hamu"), true)->value().c_str());
    char tempPassword[64 + 1] = {0};
    //put MQTT password into temporary one for predefpassword
    if (request->hasParam(F("hamp"), true) && request->getParam(F("hamp"), true)->value().length() < sizeof(tempPassword))
      strcpy(tempPassword, request->getParam(F("hamp"), true)->value().c_str());
    //check for previous password (there is a predefined special password that mean to keep already saved one)
    if (strcmp_P(tempPassword, appDataPredefPassword))
      strcpy(_ha.mqtt.password, tempPassword);

    switch (_ha.mqtt.type)
    {
    case HA_MQTT_GENERIC_1:
    case HA_MQTT_GENERIC_2:
      if (request->hasParam(F("hamgbt"), true) && request->getParam(F("hamgbt"), true)->value().length() < sizeof(_ha.mqtt.generic.baseTopic))
        strcpy(_ha.mqtt.generic.baseTopic, request->getParam(F("hamgbt"), true)->value().c_str());

      if (!_ha.hostname[0] || !_ha.mqtt.generic.baseTopic[0])
        _ha.protocol = HA_PROTO_DISABLED;
      break;
    }
    break;
  }

  return true;
};
//------------------------------------------
//Generate JSON from configuration properties
String WebDS18B20Bus::GenerateConfigJSON(bool forSaveFile = false)
{
  String gc('{');

  gc = gc + F("\"haproto\":") + _ha.protocol;
  gc = gc + F(",\"hatls\":") + _ha.tls;
  gc = gc + F(",\"hahost\":\"") + _ha.hostname + '"';
  gc = gc + F(",\"haupperiod\":") + _ha.uploadPeriod;

  //if for WebPage or protocol selected is MQTT
  if (!forSaveFile || _ha.protocol == HA_PROTO_MQTT)
  {
    gc = gc + F(",\"hamtype\":") + _ha.mqtt.type;
    gc = gc + F(",\"hamport\":") + _ha.mqtt.port;
    gc = gc + F(",\"hamu\":\"") + _ha.mqtt.username + '"';
    if (forSaveFile)
      gc = gc + F(",\"hamp\":\"") + _ha.mqtt.password + '"';
    else
      gc = gc + F(",\"hamp\":\"") + (__FlashStringHelper *)appDataPredefPassword + '"'; //predefined special password (mean to keep already saved one)

    gc = gc + F(",\"hamgbt\":\"") + _ha.mqtt.generic.baseTopic + '"';
  }

  gc += '}';

  return gc;
};
//------------------------------------------
//Generate JSON of application status
String WebDS18B20Bus::GenerateStatusJSON()
{
  String gs('{');

  gs = gs + F("\"at\":");
  gs = gs + _ds18b20Bus->GetAllTempJSON();

  gs = gs + F(",\"has1\":\"");
  switch (_ha.protocol)
  {
  case HA_PROTO_DISABLED:
    gs = gs + F("Disabled");
    break;
  case HA_PROTO_MQTT:
    gs = gs + F("MQTT Connection State : ");
    switch (_mqttClient.state())
    {
    case MQTT_CONNECTION_TIMEOUT:
      gs = gs + F("Timed Out");
      break;
    case MQTT_CONNECTION_LOST:
      gs = gs + F("Lost");
      break;
    case MQTT_CONNECT_FAILED:
      gs = gs + F("Failed");
      break;
    case MQTT_CONNECTED:
      gs = gs + F("Connected");
      break;
    case MQTT_CONNECT_BAD_PROTOCOL:
      gs = gs + F("Bad Protocol Version");
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
      gs = gs + F("Incorrect ClientID ");
      break;
    case MQTT_CONNECT_UNAVAILABLE:
      gs = gs + F("Server Unavailable");
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
      gs = gs + F("Bad Credentials");
      break;
    case MQTT_CONNECT_UNAUTHORIZED:
      gs = gs + F("Connection Unauthorized");
      break;
    }

    if (_mqttClient.state() == MQTT_CONNECTED)
      gs = gs + F("\",\"has2\":\"Last Publish Result : ") + (_haSendResult ? F("OK") : F("Failed"));

    break;
  }
  gs += '"';
  gs += '}';

  return gs;
};
//------------------------------------------
//code to execute during initialization and reinitialization of the app
bool WebDS18B20Bus::AppInit(bool reInit)
{
  //Stop Publish
  _publishTicker.detach();

  //Stop Convert
  _convertTicker.detach();

  //Stop MQTT Reconnect
  _mqttReconnectTicker.detach();
  if (_mqttClient.connected()) //Issue #598 : disconnect() crash if client not yet set
    _mqttClient.disconnect();

  //if MQTT used so configure it
  if (_ha.protocol == HA_PROTO_MQTT)
  {
    //setup server
    _mqttClient.setServer(_ha.hostname, _ha.mqtt.port).setCallback(std::bind(&WebDS18B20Bus::MqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));;

    //setup client used
    if (!_ha.tls)
      _mqttClient.setClient(_wifiMqttClient);
    else
    {
      //_wifiMqttClientSecure.setFingerprint(Utils::FingerPrintA2S(fpStr, ha.fingerPrint));
      _mqttClient.setClient(_wifiMqttClientSecure);
    }

    //Connect
    MqttConnect();
  }

  //cleanup DS18B20Bus
  _owInitialized = false;

  if (_ds18b20Bus)
  {
    if (_ds18b20Bus->temperatureList)
    {
      if (_ds18b20Bus->temperatureList->romCodes)
        free(_ds18b20Bus->temperatureList->romCodes);
      if (_ds18b20Bus->temperatureList->temperatures)
        free(_ds18b20Bus->temperatureList->temperatures);
      free(_ds18b20Bus->temperatureList);
    }

    delete _ds18b20Bus;
    _ds18b20Bus = NULL;
  }

  //create DS18B20Bus object
  _ds18b20Bus = new DS18B20Bus(ONEWIRE_PIN_IN, ONEWIRE_PIN_OUT);
  _ds18b20Bus->SetupTempSensors();

  _owInitialized = true;

  //Run a first Convert
  ConvertTick();

  //if no HA, then use default period for Convert
  if (_ha.protocol == HA_PROTO_DISABLED)
    //start temperature conversion ticker
    _convertTicker.attach(DEFAULT_CONVERT_PERIOD, [this]() { this->_needConvert = true; });
  else
  {
    //otherwise use Home automation configured period for Convert and Publish
    _convertTicker.attach(_ha.uploadPeriod, [this]() { this->_needConvert = true; });
    PublishTick(); //if configuration changed, publish immediately (Convert already ran just before this 'if')
    _publishTicker.attach(_ha.uploadPeriod, [this]() { this->_needPublish = true; });
  }

  return true;
};
//------------------------------------------
//Return HTML Code to insert into Status Web page
const uint8_t *WebDS18B20Bus::GetHTMLContent(WebPageForPlaceHolder wp)
{
  switch (wp)
  {
  case status:
    return (const uint8_t *)status1htmlgz;
    break;
  case config:
    return (const uint8_t *)config1htmlgz;
    break;
  default:
    return nullptr;
    break;
  };
  return nullptr;
};
//and his Size
size_t WebDS18B20Bus::GetHTMLContentSize(WebPageForPlaceHolder wp)
{
  switch (wp)
  {
  case status:
    return sizeof(status1htmlgz);
    break;
  case config:
    return sizeof(config1htmlgz);
    break;
  default:
    return 0;
    break;
  };
  return 0;
};
//------------------------------------------
//code to register web request answer to the web server
void WebDS18B20Bus::AppInitWebServer(AsyncWebServer &server, bool &shouldReboot, bool &pauseApplication)
{
  server.on("/getL", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //check DS18B20Bus is initialized
    if (!_owInitialized)
    {
      request->send(400, F("text/html"), F("Bus not Initialized"));
      return;
    }

    //list OneWire Temperature sensors
    request->send(200, F("text/json"), _ds18b20Bus->GetRomCodeListJSON());
  });

  server.on("/getT", HTTP_GET, [this](AsyncWebServerRequest *request) {
    bool requestPassed = false;
    byte romCodePassed[8];

    //check DS18B20Buses is initialized
    if (!_owInitialized)
    {
      request->send(400, F("text/html"), F("Bus not Initialized"));
      return;
    }

    char paramName[8] = "ROMCode";

    //check ROMCode param is there
    if (request->hasParam(paramName))
    {

      //get ROMCode
      const char *ROMCodeA = request->getParam(paramName)->value().c_str();
      //if it's a correct ROMCode
      if (isROMCodeString(ROMCodeA))
      {
        //Parse it
        for (byte j = 0; j < 8; j++)
          romCodePassed[j] = (Utils::AsciiToHex(ROMCodeA[j * 2]) * 0x10) + Utils::AsciiToHex(ROMCodeA[(j * 2) + 1]);
        requestPassed = true;
      }
    }

    //if no correct request passed
    if (!requestPassed)
    {
      //answer with error and return
      request->send(400, F("text/html"), F("No valid request received"));
      return;
    }

    //Read Temperature
    String temperatureJSON = _ds18b20Bus->GetTempJSON(romCodePassed);

    if (temperatureJSON.length() > 0)
      request->send(200, F("text/json"), temperatureJSON);
    else
      request->send(500, F("text/html"), F("Read sensor failed"));
  });
};

//------------------------------------------
//Run for timer
void WebDS18B20Bus::AppRun()
{
  if (_needMqttReconnect)
  {
    _needMqttReconnect = false;
    Serial.print(F("MQTT Reconnection : "));
    if (MqttConnect())
      Serial.println(F("OK"));
    else
      Serial.println(F("Failed"));
  }

  //if MQTT required but not connected and reconnect ticker not started
  if (_ha.protocol == HA_PROTO_MQTT && (!_mqttClient.connected() || _mqttClient.state() != MQTT_CONNECTED) && !_mqttReconnectTicker.active()) //Added state control because esp8266 2.5.0 WiFiClient.close don't seems to disconnect
  {
    Serial.println(F("MQTT Disconnected"));
    //set Ticker to reconnect after 20 or 60 sec (Wifi connected or not)
    _mqttReconnectTicker.once_scheduled((WiFi.isConnected() ? 20 : 60), [this]() { _needMqttReconnect = true; });
  }

  if (_ha.protocol == HA_PROTO_MQTT)
    _mqttClient.loop();
  if (_needConvert)
  {
    _needConvert = false;
    Serial.println(F("ConvertTick"));
    ConvertTick();
  }
  if (_needPublish)
  {
    _needPublish = false;
    Serial.println(F("PublishTick"));
    PublishTick();
  }
}

//------------------------------------------
//Constructor
WebDS18B20Bus::WebDS18B20Bus(char appId, String appName) : Application(appId, appName) {}