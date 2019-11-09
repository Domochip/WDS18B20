#ifndef Main_h
#define Main_h

#include <arduino.h>

//DomoChip Informations
//Configuration Web Pages :
//http://IP/
//DS18B20 Request Web Pages
//http://IP/getL
//http://IP/getT?ROMCode=0A1B2C3D4E5F6071

#define APPLICATION1_HEADER "WirelessDS18B20.h"
#define APPLICATION1_NAME "WDS18B20"
#define APPLICATION1_DESC "DomoChip Wireless DS18B20"
#define APPLICATION1_CLASS WebDS18B20Bus

#define VERSION_NUMBER "3.3.3"

#define DEFAULT_AP_SSID "WirelessDS"
#define DEFAULT_AP_PSK "PasswordDS"

//Enable developper mode (fwdev webpage and SPIFFS is used)
#define DEVELOPPER_MODE 0

//Log Serial Object
#define LOG_SERIAL Serial
//Choose Log Serial Speed
#define LOG_SERIAL_SPEED 115200

//Choose Pin used to boot in Rescue Mode
//#define RESCUE_BTN_PIN 2

//construct Version text
#if DEVELOPPER_MODE
#define VERSION VERSION_NUMBER "-DEV"
#else
#define VERSION VERSION_NUMBER
#endif

#endif