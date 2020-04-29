#ifndef __MJBOARD_H__
#define __MJBOARD_H__

#include <Arduino.h>

//#define ARDUINO_ESP8266_MODULE //ESP8266
//#define ARDUINO_ESP32_MODULE //ARDUINO_ARCH_ESP32 //ESP32 chimera board define
//#define ARDUINO_M5Stack_Core_ESP32 //ESP32 chimera board define
//#define ARDUINO_M5StickC_ESP32

#ifdef ARDUINO_ESP8266_MODULE
  #include <ESP8266WiFi.h>
#else
  #include <WiFi.h>
#endif

#endif //__MJBOARD_H__
