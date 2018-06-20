#ifndef Main_h
#define Main_h

#include <arduino.h>

#include "data\status.html.gz.h"
#include "data\config.html.gz.h"
#include "data\fw.html.gz.h"
#include "data\discover.html.gz.h"

//DomoChip Informations
//------------Compile for 1M 64K SPIFFS------------
//Configuration Web Pages :
//http://IP/
//http://IP/config
//http://IP/fw
//DS18B20 Request Web Pages
//http://IP/getL?bus0
//http://IP/getT?bus0=0A1B2C3D4E5F6071

//include Application header file
#include "WirelessDS18B20.h"

#define APPLICATION1_NAME "WDS18B20"
#define APPLICATION1_DESC "DomoChip Wireless DS18B20"
#define APPLICATION1_CLASS WebDS18B20Buses

#define VERSION_NUMBER "3.2.2"

#define DEFAULT_AP_SSID "WirelessDS"
#define DEFAULT_AP_PSK "PasswordDS"

//Choose ESP-01 version or not
//For ESP-01, Pins used are restricted
//Pin 3 (RX) = 1Wire bus input
//Pin 0 = 1Wire bus output
//Pin 2 = config Mode button
//For other models, Pin Numbers and Buses are defined through Configuration Web Page
#define ESP01_PLATFORM 1

//Enable developper mode (fwdev webpage and SPIFFS is used)
#define DEVELOPPER_MODE 0

//Choose Serial Speed
#define SERIAL_SPEED 115200

//Choose Pin used to boot in Rescue Mode
//For ESP-01, Pin 2 is used
#define RESCUE_BTN_PIN 2

//construct Version text
#if DEVELOPPER_MODE
#define VERSION_W_DEV VERSION_NUMBER "-DEV"
#else
#define VERSION_W_DEV VERSION_NUMBER
#endif

#if ESP01_PLATFORM
#define VERSION VERSION_W_DEV " (ESP-01)"
#else
#define VERSION VERSION_W_DEV
#endif

#endif

