#ifndef __MJBOARD_H__
#define __MJBOARD_H__

#include <Arduino.h>

//#define ARDUINO_ESP8266_MODULE //ESP8266
//#define ARDUINO_ESP32_MODULE //ARDUINO_ARCH_ESP32 //ESP32 chimera board define
//#define ARDUINO_M5Stack_ESP32 //ESP32 chimera board define
//#define ARDUINO_M5StickC_ESP32
#define ARDUINO_M5StickC_PLUS_ESP32
//#define ARDUINO_M5Atom_ESP32

#ifdef ARDUINO_ESP8266_MODULE
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#else
  #include <WiFi.h>
  #include <WebServer.h>
#endif

#if defined(ARDUINO_M5Stack_ESP32)
  #define DWidth 320//extern const int DWidth;//#define XMAX 320
  #define DHeight 240//extern const int DHeight;//#define YMAX 240
  
#elif defined(ARDUINO_M5StickC_ESP32)
  #define DWidth 160//extern const int DWidth;//#define XMAX 160
  #define DHeight 80//extern const int DHeight;//#define YMAX 80

#elif defined(ARDUINO_M5StickC_PLUS_ESP32)
  #define DWidth 240//extern const int DWidth;//#define XMAX 240
  #define DHeight 135//extern const int DHeight;//#define YMAX 135
#endif

#endif //__MJBOARD_H__
