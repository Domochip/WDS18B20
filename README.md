# WirelessDS18B20

This project use a D1 Mini (ESP8266) and some DS18B20 sensors on a 1-Wire bus to provide temperatures :
 - in JSON format (answer HTTP queries)
 - by publishing to an MQTT broker

It is possible to get list of DS18B20 ROMcodes available on the bus or current temperature of a sensor by HTTP.
All sensors temperatures are published regularly on MQTT topics.


![getList](https://raw.github.com/Domochip/WirelessDS18B20/master/img/getL.jpg) ![getTemp](https://raw.github.com/Domochip/WirelessDS18B20/master/img/getT.jpg)

## Build your 1-Wire bus

In order to get a wide 1-Wire bus (one that cover a house), Dallas provides some recommendations into their application note AN148.

This project use the Improved Interface bellow that is able to support a 200m bus:

![Dallas AN148 ImprovedCPUBusInterface schema](https://raw.github.com/Domochip/WirelessDS18B20/master/img/AN148-ImprovedCPUBusInterface.jpg)

This one requires a pin for reading the bus state and another one as output to drive the bus low : I named it the "Dual Pin OneWire"

**Warning : If you build a large 1 Wire bus inside your house, keep in mind that some high voltage may appear on this one by induction. You need to have a good knowledge about electricity and associated risks!!!**

## Build your WirelessDS18B20

All files are inside schematic subfolder and has been designed with KiCad

### Schematic

![WirelessDS18B20 schematic](https://raw.github.com/Domochip/WirelessDS18B20/master/img/schematic.jpg)

### PCB

![WirelessDS18B20 PCB](https://raw.github.com/Domochip/WirelessDS18B20/master/img/pcb.jpg)![WirelessDS18B20 PCB2](https://raw.github.com/Domochip/WirelessDS18B20/master/img/pcb2.jpg)

### Code/Compile

Source code can be compiled using VisualStudioCode/Platformio and flashed onto a D1 Mini

### Print your box

Box project (Fusion 360) can be found into `box` folder

![WirelessDS18B20 Box](https://raw.github.com/Domochip/WirelessDS18B20/master/img/box.jpg)

## Run

### First Boot

During First Boot, the ESP boot in Access Point Mode

- Network SSID : `WirelessDSXXXX`
- Password : `PasswordDS`
- ESP IP : `192.168.4.1`

Connect to this network and then configure it.

### Tasmota Configuration

TODO

## Use it

### Basics

Usage (answers are in JSON format):

- `http://IP/getL` will return list of DS18B20 ROMCodes available on the 1-Wire bus number 0
- `http://IP/getT?ROMCode=0A1B2C3D4E5F6071` will return simple JSON with temperature from the sensor