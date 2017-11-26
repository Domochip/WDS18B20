#ifndef WirelessDS18B20_h
#define WirelessDS18B20_h


//J6B Informations
//------------Compile for 1M 64K SPIFFS------------
//Configuration Web Pages
//http://IP/fw
//http://IP/config
//http://IP/status
//DS18B20 Request Web Pages
//http://IP/getList?bus=0
//http://IP/getTemp?bus=0&ROMCode=0A1B2C3D4E5F6071


#define VERSION_NUMBER "3.0"

//Enable developper mode (fwdev webpage and SPIFFS is used)
#define DEVELOPPER_MODE 0


//Choose ESP-01 version or not
//For ESP-01, Pins used are restricted
//Pin 3 (RX) = 1Wire bus input
//Pin 0 = 1Wire bus output
//Pin 2 = config Mode button
//For other models, Pin Numbers and Buses are defined through Configuration Web Page
#define ESP01_PLATFORM 1

//Choose Serial Speed
#define SERIAL_SPEED 115200

//Choose Pin used to boot in Rescue Mode
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


