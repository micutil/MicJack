#ifndef __MJSERIAL_H__
#define __MJSERIAL_H__

#include "MJBoard.h"

extern bool printToSub;

#ifdef ARDUINO_ESP32_MODULE
  #define mjMain  Serial2 //Default RX=GPIO16, TX=GPIO17
  #define mjSub   Serial  //Default RX=GPIO3,  TX=GPIO1
  
#elif defined(ARDUINO_M5Stack_Core_ESP32)
  #include<M5Stack.h>
  #define mjMain Serial2 //Default RX=GPIO16, TX=GPIO17
  #define mjSub  Serial  //Default RX=GPIO3,  TX=GPIO1
  #define mjLcd  M5.Lcd
  
#elif defined(ARDUINO_M5StickC_ESP32)
  #include<M5StickC.h>
  #define mjMain  Serial2 //Default RX=GPIO16, TX=GPIO17
  #define mjSub  Serial  //Default RX=GPIO3,  TX=GPIO1
  #define mjLcd  M5.Lcd

#elif defined(ARDUINO_ESP8266_MODULE) //ESP8266
  #define mjMain  Serial //Default RX=GPIO3, TX=GPIO1
  
#endif

#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
void InitStatusLED();
void ConnectStatusLED(bool b, String addr="");
void PostStatusLED(bool b, String addr="");
void GetStatusLED(bool b, String addr="");
void DrawStatusLED();
#endif

#if defined(ARDUINO_M5Stack_Core_ESP32)
void tft_terminal_setup(bool chg);
void tft_terminal_print(const char *s, int n);
#endif //defined(ARDUINO_M5Stack_Core_ESP32)

enum {
  k2M,
  k2B,
  k2S
};

class MJSerial {
  public:
  MJSerial() {
  
    #if defined(ARDUINO_ESP32_MODULE) || defined(ARDUINO_M5Stack_Core_ESP32) //ARDUINO_ARCH_ESP32
      mjMain.begin(115200); while (!mjMain) { ; }
      mjSub.begin(115200); while (!mjSub) { ; }
    #elif defined(ARDUINO_M5StickC_ESP32)
      //mjMain.begin(115200, SERIAL_8N1, 0, 26); // EXT_IO
      //while (!mjMain) { ; }
      //mjSub.begin(115200);
      //while (!mjSub) { ; }
    #endif

    #ifdef ARDUINO_ESP8266_MODULE
      mjMain.begin(115200); while (!mjMain) { ; }
    #endif
  }
  void print(String s, int type=k2B) {
    if(type<k2S) mjMain.print(s);
    if(type>k2M&&printToSub) mjSub.print(s);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    tft_terminal_print(s.c_str(),s.length());
    #endif
  }
  void println(String s, int type=k2B) {
    if(type<k2S) mjMain.println(s);
    if(type>k2M&&printToSub) mjSub.println(s);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    s=s+String("\n");
    tft_terminal_print(s.c_str(),s.length());
    #endif
  }
  void print(char* s, int type=k2B) {
    if(type<k2S) mjMain.print(s);
    if(type>k2M&&printToSub) mjSub.print(s);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    tft_terminal_print(s,strlen(s));
    #endif
  }
  void println(char* s, int type=k2B) {
    if(type<k2S) mjMain.println(s);
    if(type>k2M&&printToSub) mjSub.println(s);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    char c[128];for(int i=0;i<128;i++) c[i]=0;
    int n=sprintf(c,"%s\n",s);
    tft_terminal_print(c,n);
    #endif
  }
  void print(char s, int type=k2B) {
    if(type<k2S) mjMain.print(s);
    if(type>k2M&&printToSub) mjSub.print(s);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    String t=String(s);
    tft_terminal_print(t.c_str(),t.length());
    #endif 
  }
  void println(char s, int type=k2B) {
    if(type<k2S) mjMain.println(s);
    if(type>k2M&&printToSub) mjSub.println(s);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    String t=String(s)+String("\n");
    tft_terminal_print(t.c_str(),t.length());//mjLcd.println(s);
    #endif
  }
  void printfileinfo(const char * format, const char * filename, const char * filesize) {
    //void printf(const char * format, ...) {
    mjMain.printf(format, filename, filesize);
    if(printToSub) mjSub.printf(format, filename, filesize);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    char c[128];for(int i=0;i<128;i++) c[i]=0;
    int n=sprintf(c, format, filename, filesize);
    tft_terminal_print(c,n);
    #endif
  }
  void println(IPAddress s, int type=k2B) {
    if(type<k2S) mjMain.println(s);
    if(type>k2M&&printToSub) mjSub.println(s);
    #ifdef ARDUINO_M5Stack_Core_ESP32
    char c[128];for(int i=0;i<128;i++) c[i]=0;
    int n=sprintf(c, "%d.%d.%d.%d\n",s[0],s[1],s[2],s[3]);
    tft_terminal_print(c,n);
    #endif
  }
  //size_t HardwareSerial::print(val);
  //size_t HardwareSerial::print(val, format);
  //size_t HardwareSerial::println(val);
  //size_t HardwareSerial::println(val, format);

  void write(uint8_t s, int type=k2B) {
    if(type<k2S) mjMain.write(s);
    if(type>k2M&&printToSub) mjSub.write(s);
    //mjMain.write(s);
  }
  //size_t HardwareSerial::write(uint8_t c);
  //size_t HardwareSerial::write(const char *str);
  //size_t HardwareSerial::write(const char *buffer, size_t size);
  //size_t HardwareSerial::write(const uint8_t *buffer, size_t size);
};

#endif //__MJSERIAL_H__
