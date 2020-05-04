/*
 *  MicJack
WebServer
 *  MixJuice compatible IoT interface module for IchigoJam with ESP-WROOM-02
 *  
 *  CC BY Michio Ono. http://ijutilities.micutil.com
 *  
 *  *Version Information
 *  2020/ 5/ 4  ver 1.2.2b2 シリアル送信の不具合を修正
 *  2020/ 5/ 4  ver 1.2.2b1 LED信号読取り、M5StickCのキーボードモード対応
 *  2020/ 4/29  ver 1.2.1b1 Fixed UDP, CardKB for M5Stack/M5StickC
 *  2020/ 4/19  ver 1.2.0b1 ESP32 Module, M5Stack, M5StickC version
 *  2020/ 3/22  ver 1.1.0b2 UDP, TJ, FP (200307:1.1.0b1)
 *  2018/10/10  ver 1.0.1b2
 *  2018/10/ 8  ver 1.0.1b1
 *  2018/ 9/ 2  ver 1.0.0b1, 9/3 b2, 9/9 b5
 *  2017/ 4/23  ver 0.9.0
 *  2017/ 1/ 2  ver 0.8.0
 *  2016/ 5/23  ver 0.1.0 (First release）
 *  
 *  *Special thanks
 *  Some advice (robo8080さん, 笹野さん）
 *  MicJack紹介ページ（イチゴジャム レシピ：https://15jamrecipe.jimdo.com/mixjuice/micjack/ ）
 *  
 */

#include "MJBoard.h"

#ifdef ARDUINO_ESP8266_MODULE //ESP8266
  #include <ESP8266mDNS.h>
  FS qbFS = SPIFFS;
#endif

#ifdef ARDUINO_ESP32_MODULE
  #define hasDISP
  #include <ESPmDNS.h>
  const int ijLED=15;
  const int espLED=2;
  #define LED_OFF  LOW
  #define LED_ON HIGH
#endif

#ifdef ARDUINO_M5Stack_Core_ESP32
  #define useSD
  #define hasDISP
  #include <M5Stack.h>
  #include <ESPmDNS.h>
  const int DWidth=320;
  const int DHeight=240;
  const int ijLED=5;
  const int espLED=2;
  #define LED_OFF  LOW
  #define LED_ON HIGH

#endif

#include "time.h"
const String ntpServer = "ntp.jst.mfeed.ad.jp";

#ifdef ARDUINO_M5StickC_ESP32
  #define hasDISP
  #include <M5StickC.h>
  #include <ESPmDNS.h>
  const int DWidth=160;
  const int DHeight=80;
  //#include "time.h"
  //const char* ntpServer = "ntp.jst.mfeed.ad.jp";
  //RTC_TimeTypeDef RTC_TimeStruct;
  //RTC_DateTypeDef RTC_DateStruct;
  bool initRTC=false;
  const int ijLED=36;
  const int espLED=10;
  #define LED_OFF HIGH
  #define LED_ON  LOW
#endif

#if defined(ARDUINO_M5Stack_Core_ESP32)
#define FACES_ADDR     0X08
#ifdef FACES_ADDR
  #define FACES_ADDR     0X08
  #define FACES_INT      5
#endif //FACES_ADDR
#endif //ARDUINO_M5Stack_Core_ESP32

#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
#define CARDKB_ADDR 0x5F //Card Keyboard Unit
//Joy Stick Unit
//#define JOY_ADDR 0x52
#ifdef JOY_ADDR
  uint8_t x_data, px_data;
  uint8_t y_data, py_data;
  uint8_t button_data, pbutton_data;
  char data[100];
#endif //JOY_ADDR

#endif

//#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <FS.h>

#ifdef useSD
  fs::SDFS qbFS = SD;
#else
  #define useSPIFFS
  #ifndef ARDUINO_ESP8266_MODULE
  #include <SPIFFS.h>
  fs::SPIFFSFS qbFS = SPIFFS;
  #endif
#endif //useSD or useSPIFFS

#include "MJSerial.h"
//#ifndef ARDUINO_M5StickC_ESP32
MJSerial mjSer;
//#endif

const String MicJackVer="MicJack-1.2.2b2";
const String TelloJackVer="TelloJack-1.0.0b1";
const String MJVer="MixJuice-1.3.0";
const int sleepTimeSec = 60;

bool printToSub=true;
String inStr = ""; // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

IPAddress staIP; // the IP address of your shield

typedef struct sspw {
  char ssid[32];
  char pass[32];
};

typedef struct APCONFIG {
  char ssid[32];
  char pass[32];
  char homepage[32];
  char softap_ssid[32];
  char softap_pass[32];
  bool kbd;
  bool useHostKbdCmd;
  bool showForce;//show the result of tello command?;
  bool autoWait;//true=don't wait, false=wait
  bool reserve[30];
  sspw sp[8];
};
APCONFIG apcbuf;

String aplist[20];
String lastSSID="";
String lastPASS="";

String homepage="mj.micutil.com";
String lastGET=homepage;

boolean useProxy=false;
String httpProxy="";
int httpPort = 80;

int posttype;
boolean postmode=false;
enum { HTML_GET, HTML_GETS, HTML_GET_QUEST, 
       HTML_POST, HTML_POSTS, HTML_POST_QUEST };
String postaddr="";
String postdata="";
String myPostContentType="";

#include "questUtil.h"
bool questEnd=false;
uint8_t questData[768];

#define kspw  18 //20
#define kspn  18 //30
int spw=kspw;
int spn=kspn;

/*** Use ccess Status LED ***/
#define useMJLED
#ifdef useMJLED
  const int connLED=12; //Green
  const int postLED=4;  //Yellow
  const int getLED=5;   //Red
#endif

/*** Web Server ***/
#define initStartSERVER
WebServer server(80);//ESP8266WebServer server(80);
boolean isServer=false;
String rootPage;
const char* mjname = "micjack";

/*** UDP ***/
#define supportUDP
#ifdef supportUDP
  #define initStartUDP
  #include <WiFiUdp.h>
  WiFiUDP udp;
  const IPAddress udpip(192, 168, 20, 1);       // IPアドレス(ゲートウェイも兼ねる)
  const IPAddress udpsubnet(255, 255, 255, 0); // サブネットマスク
  unsigned int UDP_LocalPort = 20001;//kLocalPort
  unsigned int UDP_Read_Port = 20001;//kRmoteUdpPort
  IPAddress UDP_Write_IPAddress;
  unsigned int UDP_Write_Port;
  //unsigned char UDP_Minimum_Packet = 1;
  boolean isUDP=false;
  #ifdef ARDUINO_ESP8266_MODULE
  const int UDP_PACKET_SIZE = UDP_TX_PACKET_MAX_SIZE;//8192;//UDP_TX_PACKET_MAX_SIZE=8192;
  #else
  const int UDP_TX_PACKET_MAX_SIZE=8192;
  const int UDP_PACKET_SIZE = UDP_TX_PACKET_MAX_SIZE;//8192;
  #endif
  char packetBuffer[UDP_PACKET_SIZE+1];
#endif

#define supportTELLO
#ifdef supportTELLO
#include "tello.h"
extern bool showForce;
extern bool showRes;
#endif

/*** SoftAP ***/
#define initStartSoftAP  
//const char default_softap_ssid[] = "MicJack";
const char default_softap_pass[] = "";//"abcd1234";
IPAddress mySoftAPIP;

/*** ArduinoOTA ***/
//#define supportOTA
#ifdef supportOTA
  #include <ArduinoOTA.h>
#endif

/***************** Keybord Input ***************/
bool kbdMode=false;
#define useKbd
#define detectHostKbd //ホストからの送信データ //
#ifdef useKbd
#include <ps2dev.h>

#if defined(ARDUINO_ESP32_MODULE) || defined(ARDUINO_M5Stack_Core_ESP32)
  #define KB_CLK      21 // 0   // A4  // PS/2 CLK  IchigoJamのKBD1に接続 //21//
  #define KB_DATA     22 // 15  // A5  // PS/2 DATA IchigoJamのKBD2に接続 //22//
#elif defined(ARDUINO_M5StickC_ESP32)
  //Grove端子 SDA:IO32, SCL:IO33
  //Wire.begin(32, 33);
  #define KB_CLK      33 // 0   // A4  // PS/2 CLK  IchigoJamのKBD1に接続 //21//
  #define KB_DATA     32 // 15  // A5  // PS/2 DATA IchigoJamのKBD2に接続 //22//
#elif defined(ARDUINO_ESP8266_MODULE)
  #define KB_CLK      13 // 0   // A4  // PS/2 CLK  IchigoJamのKBD1に接続 //21//
  #define KB_DATA     16 // 15  // A5  // PS/2 DATA IchigoJamのKBD2に接続 //22//
#endif //ARDUINO_ARCH_ESP32

uint8_t enabled =0;               // PS/2 ホスト送信可能状態
PS2dev keyboard(KB_CLK, KB_DATA); // PS/2デバイス

//const uint8_t ijKeyMap[2][272] PROGMEM = {
uint8_t ijKeyMap[2][272] = {
  {
    0,0,0,0,0,0,0,0,1,1,1,0,0,1,0,1, 0,3,3,3,3,0,0,3,0,0,0,1,3,3,3,3,  //0x00 - 0x1F
    1,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,  //0x20 - 0x3F
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,  //0x40 - 0x5F
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,  //0x60 - 0x7F
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,  //0x80 - 0x9F
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,  //0xA0 - 0xBF (カナ)
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,0,  //0xC0 - 0xDF (カナ)
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,  //0xE0 - 0xFF PGC
    0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0 //for Function Key
  },
  {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x0D,0x5A,0x00,0x00,0x5A,0x00,0x13,
    0x00,0x70,0x6C,0x7D,0x7A,0x00,0x00,0x69,0x00,0x00,0x00,0x76,0x6B,0x74,0x75,0x72,
    0x29,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x52,0x4C,0x41,0x4E,0x49,0x4A,  //0x20
    0x45,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x52,0x4C,0x41,0x4E,0x49,0x4A,
    0x54,0x1C,0x32,0x21,0x23,0x24,0x2B,0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,  //0x40
    0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,0x1D,0x22,0x35,0x1A,0x5B,0x6A,0x5D,0x55,0x51,
    0x54,0x1C,0x32,0x21,0x23,0x24,0x2B,0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,  //0x60
    0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,0x1D,0x22,0x35,0x1A,0x5B,0x6A,0x5D,0x55,0x00,  
    0x45,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,  //0x80
    0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,
    0x6A,0x49,0x5B,0x5D,0x41,0x4A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //0xA0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //0xC0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x45,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,  //0xE0
    0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,0x31,0x44,0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,
    0x00,0x05,0x06,0x04,0x0C,0x03,0x0B,0x83,0x0A,0x01,0x09,0x78,0x07,0x00,0x00,0x00,
  }
};

#ifdef detectHostKbd
// PS/2 ホストにack送信
void ack() {
  while(keyboard.write(0xFA));
}

// PS/2 ホストから送信されるコマンドの処理
int keyboardcommand(int command) {
  //mjSer.println(command);
  unsigned char val;
  uint32_t tm;
  switch (command) {
  case 0xFF:
    ack();// Reset: キーボードリセットコマンド。正しく受け取った場合ACKを返す。
    //keyboard.write(0xAA);
    break;
  case 0xFE: // 再送要求
    ack();
    break;
  case 0xF6: // 起動時の状態へ戻す
    //enter stream mode
    ack();
    break;
  case 0xF5: //起動時の状態へ戻し、キースキャンを停止する
    //FM
    enabled = 0;
    ack();
    break;
  case 0xF4: //キースキャンを開始する
    //FM
    enabled = 1;
    ack();
    break;
  case 0xF3: //set typematic rate/delay : 
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  case 0xF2: //get device id : 
    ack();
    keyboard.write(0xAB);
    keyboard.write(0x83);
    break;
  case 0xF0: //set scan code set
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  case 0xEE: //echo :キーボードが接続されている場合、キーボードはパソコンへ応答（ECHO Responce）を返す。
    //ack();
    keyboard.write(0xEE);
    break;
  case 0xED: //set/reset LEDs :キーボードのLEDの点灯/消灯要求。これに続くオプションバイトでLEDを指定する。 
    ack();
    keyboard.read(&val); //do nothing with the rate
    ack();
    break;
  }
}
#endif //detectHostKbd

void breakKeyCode(uint8_t c) {
  keyboard.write(0xF0);
  keyboard.write(c);
}

#endif //useKbd

bool sendKeyCode(int key) {

#ifdef useKbd

  if(apcbuf.kbd) {
    //if(key>0xFF) key=key-0x100;

    if(key>255+16) return false;
    
    uint8_t t=ijKeyMap[0][key];//mjSer.println(t);
    uint8_t c=ijKeyMap[1][key];//mjSer.println(c);
    
    if(t==0||t==6) {
      mjSer.write(key);
      return false;
    }

    if(t==4||t==5) keyboard.write(0x11);
    if(t==2||t==5) keyboard.write(0x12);
    if(t==3) keyboard.write(0xE0);
    keyboard.write(c);
    delay(1);
    
    if(t==3) keyboard.write(0xE0);
    breakKeyCode(c);
    if(t==2||t==5) breakKeyCode(0x12);
    if(t==4||t==5) breakKeyCode(0x11);
  
    return true;
  }
  
#endif //useKbd

  mjSer.write(key);
  return false;
}

/**********************************
 * RTC / TIME
 * 
 * 
***********************************/

#ifdef ARDUINO_M5StickC_ESP32

void resetRTC(String ts) {
  // Set ntp time to local
  if(ts.length()<5) ts=ntpServer;
  configTime(9 * 3600, 0, ts.c_str());

  // Get local time
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    //M5.Lcd.print("NTP : ");
    //M5.Lcd.println(ntpServer);
 
    // Set RTC time
    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours   = timeInfo.tm_hour;
    TimeStruct.Minutes = timeInfo.tm_min;
    TimeStruct.Seconds = timeInfo.tm_sec;
    M5.Rtc.SetTime(&TimeStruct);
    
    RTC_DateTypeDef DateStruct;
    DateStruct.WeekDay = timeInfo.tm_wday;
    DateStruct.Month = timeInfo.tm_mon + 1;
    DateStruct.Date = timeInfo.tm_mday;
    DateStruct.Year = timeInfo.tm_year + 1900;
    M5.Rtc.SetData(&DateStruct);

    initRTC=true;
  }
}

void getRTC(int n) {
  //int n=s.toInt();
  
  RTC_DateTypeDef ds;
  RTC_TimeTypeDef ts;
  
  M5.Rtc.GetData(&ds);
  M5.Rtc.GetTime(&ts);

  char nichiji[50];
  switch(n) {
    case 1: sprintf(nichiji,"'%04d/n",ds.Year); break; 
    case 2: sprintf(nichiji,"'%02d/n",ds.Month); break;
    case 3: sprintf(nichiji,"'%02d/n",ds.Date); break;
    
    case 4: sprintf(nichiji,"'%02d/n",ts.Hours); break;
    case 5: sprintf(nichiji,"'%02d/n",ts.Minutes); break;
    case 6: sprintf(nichiji,"'%02d/n",ts.Seconds); break;
    
    case 7: sprintf(nichiji,"'%d/n",ds.WeekDay); break;//0=Sun,1=Mon....
    case 8: sprintf(nichiji,"'%04d/%02d/%02d\n",ds.Year,ds.Month,ds.Date); break;
    case 9: sprintf(nichiji,"'%02d:%02d:%02d\n",ts.Hours,ts.Minutes,ts.Seconds); break;
    default://case 0: 
      sprintf(nichiji,"'%04d/%02d/%02d %02d:%02d:%02d\n",ds.Year,ds.Month,ds.Date,ts.Hours,ts.Minutes,ts.Seconds);
      break;
  }
  mjSer.print(nichiji);
  //Serial2.printf(s);
}

#else

void getRTC(int n) {
  // Set ntp time to local
  //if(ts.length()<5) ts=ntpServer;
  #define JST 3600* 9

  // Get local time
  #if defined(ARDUINO_ESP8266_MODULE)
  configTime( JST, 0, "ntp.nict.jp", ntpServer.c_str());
  time_t t=time(NULL);
  struct tm *tp=localtime(&t);
  if(tp) {
    int tsHours   = tp->tm_hour;
    int tsMinutes = tp->tm_min;
    int tsSeconds = tp->tm_sec;
    
    int dsWeekDay = tp->tm_wday;
    int dsMonth = tp->tm_mon + 1;
    int dsDate = tp->tm_mday;
    int dsYear = tp->tm_year+1900;

  #else
  configTime(JST, 0, ntpServer.c_str());
  struct tm ti;//,*tp; tp=&ti;
  if(getLocalTime(&ti)) {
    //RTC_DateTypeDef ds;
    //RTC_TimeTypeDef ts;
    int tsHours   = ti.tm_hour;
    int tsMinutes = ti.tm_min;
    int tsSeconds = ti.tm_sec;
    
    int dsWeekDay = ti.tm_wday;
    int dsMonth = ti.tm_mon + 1;
    int dsDate = ti.tm_mday;
    int dsYear = ti.tm_year+1900;
  #endif

    char nichiji[50];
    switch(n) {
      case 1: sprintf(nichiji,"'%04d/n",dsYear); break;
      case 2: sprintf(nichiji,"'%02d/n",dsMonth); break;
      case 3: sprintf(nichiji,"'%02d/n",dsDate); break;
      
      case 4: sprintf(nichiji,"'%02d/n",tsHours); break;
      case 5: sprintf(nichiji,"'%02d/n",tsMinutes); break;
      case 6: sprintf(nichiji,"'%02d/n",tsSeconds); break;
      
      case 7: sprintf(nichiji,"'%d/n",dsWeekDay); break;//0=Sun,1=Mon....
      case 8: sprintf(nichiji,"'%04d/%02d/%02d\n",dsYear,dsMonth,dsDate); break;
      case 9: sprintf(nichiji,"'%02d:%02d:%02d\n",tsHours,tsMinutes,tsSeconds); break;
      default://case 0: 
        sprintf(nichiji,"'%04d/%02d/%02d %02d:%02d:%02d\n",dsYear,dsMonth,dsDate,tsHours,tsMinutes,tsSeconds);
        break;
    }
    mjSer.print(nichiji);
  }
  //Serial2.printf(s);
}

#endif /ARDUINO_M5StickC_ESP32

/************************************
 * Initial Set up
 * 
 ************************************/
void setup() {
  
  #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
    M5.begin();
  #endif

  #ifdef ARDUINO_M5StickC_ESP32
    M5.Lcd.setRotation(1); // Must be setRotation(0) for this sketch to work correctly //for M5StickC
    Serial.begin(115200);
    while (!mjSub) { ; }
    Serial2.begin(115200, SERIAL_8N1, 0, 26); // EXT_IO
    while (!mjMain) { ; }
  #endif

  #ifdef ARDUINO_ESP8266_MODULE
    printToSub=false;
  #endif
  
  #if defined(CARDKB_ADDR) || defined(JOY_ADDR) || defined(FACES_ADDR)
    Wire.begin();
  #endif 

 //mjSer.begin(74880);
  /*
  mjSer.begin(115200);
  while (!mjSer) { ; }
  #ifdef mjSub
  mjSub.begin(115200);
  while (!mjSub) { ; }
  #endif
  */
  
  //Start SPIFFS or microSD
  #ifdef useSPIFFS
    while (!qbFS.begin()) { mjSer.println("."); }// mjSer.println("SPIFFS IO failed..."); }
  #endif //useSPIFFS
  #ifdef useSD
    while (!SD.begin()) { mjSer.println("."); }//mjSer.println("SD IO failed..."); }
  #endif //useSD

  #if defined(ARDUINO_M5Stack_Core_ESP32)// || defined(ARDUINO_M5StickC_ESP32)
    tft_terminal_setup(false);
  #endif
  
  delay(2000);
  mjSer.println("");
  mjSer.println("NEW");delay(100);
  mjSer.println("CLS");delay(100);
  mjSer.println("");

  #ifdef useMJLED
    #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
      pinMode(connLED, OUTPUT);//1
      pinMode(postLED, OUTPUT);//2
      pinMode(getLED,  OUTPUT);//3
    #endif
  #endif //useMJLED

  #if defined(ARDUINO_ESP32_MODULE) || defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
    pinMode(ijLED, INPUT);//36
    pinMode(espLED, OUTPUT);//35
    digitalWrite(espLED, LED_OFF);
  #endif

  #if defined(CARDKB_ADDR) || defined(JOY_ADDR)
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);
  #endif

  #if defined(FACES_ADDR)
  pinMode(FACES_INT, INPUT_PULLUP);
  #endif

  //Load setting
  LoadAPConfig();

  //Set WiFi to station mode
  WiFi.mode(WIFI_AP_STA);//WIFI_AP_STA, WIFI_STA

  //Show MicJack Information
  mjSer.println("'"+MJVer);
  mjSer.println("'"+MicJackVer);
  mjSer.println("'"+TelloJackVer);
  mjSer.println("'CC BY Michio Ono");
  mjSer.println("");

  //#ifdef useSPIFFS or #ifdef useSD
  //SPIFFS.begin();
/*  {
    mjSer.println("'qbFS files...");//mjSer.println("'SPIFFS files...");
    File root = qbFS.open("/");//File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
      String fileName = file.name();
      size_t fileSize = file.size();
      mjSer.printfileinfo("'%s:%s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      delay(500);
      file = root.openNextFile();
    }
    mjSer.println("");
  }
*/

  
  /* Soft APモード開始 */
  softApStart();

  /* HTTP server */
  serverStart();
  
  /* ステーションモード自動接続 */
  //一回切断した後に接続
  WiFi.disconnect();
  MJ_APC("");//MJ_APLAPC();
  
  #ifdef initStartUDP
    udpStart(UDP_Read_Port);
  #endif

  #ifdef supportTELLO
    tello_setup();
  #endif
  
  #ifdef supportOTA
    OTAStart();
  #endif

#ifdef useKbd
/*  mjSer.println("'Low or High");
  digitalWrite(KB_CLK,LOW);digitalWrite(KB_DATA,LOW);
  for(int i=0;i<3;i++) {
    if(digitalRead(KB_CLK)==HIGH) mjSer.println("'1HIGH"); else mjSer.println("'1LOW"); 
    if(digitalRead(KB_DATA)==HIGH) mjSer.println("'2HIGH"); else mjSer.println("'2LOW"); 
  }
*/
  //trueでも意味がない
  apcbuf.useHostKbdCmd=false;
  
  if(apcbuf.useHostKbdCmd) {
    MJ_HOSTKBDCMD(apcbuf.useHostKbdCmd);
    
    uint32_t tm = millis();  
    while(keyboard.write(0xAA)!=0)
    {
      if ( millis() > tm+1000) {
          apcbuf.kbd=false;
          break;
      }
    }
  }
  
  /* 現在の入力モード */
  //apcbuf.kbd=false;
  MJ_KBD(apcbuf.kbd); 
  
#else
  //Keyboardモードを使わない場合
  apcbuf.kbd=false;
  
#endif

  //Serial.println(UDP_PACKET_SIZE,DEC);
  mjSer.println("'Ready to go!");
  mjSer.println("");
  
}

/************************************
 * softApStart
 * Soft AP 開始
 * 
 */

void softApStart() {
  WiFi.softAP(apcbuf.softap_ssid,apcbuf.softap_pass);
  delay(100);
  WiFi.softAPConfig(udpip,udpip,udpsubnet);
  delay(500);
  mySoftAPIP = WiFi.softAPIP();
  String sn="";sn=String(apcbuf.softap_ssid);
  mjSer.println("'Soft AP: "+sn);
  delay(500);
  mjSer.print("'IP: ");
  mjSer.println(mySoftAPIP); mjSer.println("");
}

/************************************
 * serverStart
 * Web Server 開始
 */

void serverStart() {
  
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //makeRootPage();
  server.begin();
  delay(500); 
  mjSer.println("'HTTP Server started...");
  mjSer.println("");
  
  isServer=true;  
  
}

/************************************
 * OTAStart
 * OTA対応
 */
#ifdef supportOTA
void OTAStart() {
  /*
  ArduinoOTA.onStart([]() {
    mjSer.println("'Start");
  });
  ArduinoOTA.onEnd([]() {
    mjSer.println("\n'End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    mjSer.printf("'Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    mjSer.printf("'Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) mjSer.println("'Auth Failed");
    else if (error == OTA_BEGIN_ERROR) mjSer.println("'Begin Failed");
    else if (error == OTA_CONNECT_ERROR) mjSer.println("'Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) mjSer.println("'Receive Failed");
    else if (error == OTA_END_ERROR) mjSer.println("'End Failed");
  });
  */
  ArduinoOTA.begin();
  mjSer.println("'OTA supported...");
}

#endif

/************************************
 * CardKB / FACES
 * 
 ************************************/

#if defined(CARDKB_ADDR) || defined(FACES_ADDR)
void getKeyData(int kb_add) {
  Wire.requestFrom(kb_add, 1);
  while(Wire.available())
  {
    char c = Wire.read(); // receive a byte as characterif
    if (c != 0)
    {
      switch(c) {
        case 0x0D: c=10; break;//Return
        case 0xB4: c=28; break;//left
        case 0xB7: c=29; break;//right
        case 0xB5: c=30; break;//up
        case 0xB6: c=31; break;//down
      }
      //Serial.println(c,HEX);
      #ifdef useKbd
      if(kbdMode) {
        sendKeyCode(c);
      } else {
        mjSer.print(c);
      }
      #else
        mjSer.print(c);
      #endif    
    }
  }
}
#endif

/************************************
 * Event loop
 * 
 ************************************/

#if defined(ARDUINO_ESP32_MODULE) || defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
  const int mjNum=2;
  HardwareSerial *mjSS[]={&mjMain,&mjSub};
#elif defined(ARDUINO_ESP8266_MODULE)
  const int mjNum=1;
  HardwareSerial *mjSS[]={&mjMain};
#endif

void loop() {

  #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
  M5.update();
  
  if(M5.BtnA.wasPressed()) {
    drawAPQRCode();
  }
  if(M5.BtnB.wasPressed()) {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
      tft_terminal_setup(true);
    #elif defined(ARDUINO_M5StickC_ESP32)
      if (staIP[0] >= 0x20 && staIP[0] != '?') {
        M5.Lcd.fillRect(80,0,180,80,TFT_BLACK);
        ConnectStatusLED(true,IPAddressToStr(staIP));
      }
    #endif
  }
  #endif

  #ifdef ARDUINO_M5Stack_Core_ESP32
  if(M5.BtnC.wasPressed()) {
    M5.powerOFF();
  }
  #endif
  #ifdef ARDUINO_M5StickC_ESP32
  if(M5.Axp.GetBtnPress()==2) esp_restart();
  #endif //ARDUINO_M5StickC_ESP32

  #ifdef supportOTA
    ArduinoOTA.handle();
  #endif

  #ifdef useKbd
    #ifdef detectHostKbd
    if(apcbuf.kbd) {
      if(apcbuf.useHostKbdCmd) {
        unsigned char ck;  // ホストからの送信データ
        if( (digitalRead(KB_CLK)==LOW) || (digitalRead(KB_DATA) == LOW)) {
          while(keyboard.read(&ck)) ;
          keyboardcommand(ck);
        }
      }
    }
    #endif
  #endif  

  while (mjMain.available()) {
    if(postmode&&posttype==HTML_POST_QUEST) {
      //Quest用ポストデータ
      uint8_t q=(uint8_t)mjMain.read();
      #ifndef ARDUINO_ESP8266_MODULE
        mjSub.write(q);
      #endif
      int p=(postdata.length()%16);
      if(questEnd) {
        //データ終了後は0xFF000000000000にする
        q=0; if(p==0) q=0xFF;
      } else if(p==0&&q==0xFF) {
        questEnd=true;//データ終了
      }
      questData[postdata.length()/2]=q;
      String h="0"+String(q,HEX);
      postdata += h.substring(h.length()-2);
      stringComplete = true;
      inStr="";//いちおう消しておく
    } else {
      char inChar = (char)mjMain.read();
      #ifndef ARDUINO_ESP8266_MODULE
        mjSub.write(inChar);
      #endif
      //Check end of line 
      if(inChar=='\n' || inChar=='\r') {
        stringComplete = true;
        //if(!postmode) mjSer.flush(); //読み飛ばし "OK"など
        // flashは Arduino 1.0から、読み込みを待つコマンドになったため
        // flashしないで、次の読み込みで、次の処理をすることに
        break;
      } else {
        inStr += inChar;
      }
    }
    delay(1);
  }

  /*
  for(int i=0;i<mjNum;i++) {
    if(stringComplete==false) {
      while (mjSS[i]->available()) {
        if(postmode&&posttype==HTML_POST_QUEST) {
          //Quest用ポストデータ
          uint8_t q=(uint8_t)mjSS[i]->read();
          int p=(postdata.length()%16);
          if(questEnd) {
            //データ終了後は0xFF000000000000にする
            q=0; if(p==0) q=0xFF;
          } else if(p==0&&q==0xFF) {
            questEnd=true;//データ終了
          }
          questData[postdata.length()/2]=q;
          String h="0"+String(q,HEX);
          postdata += h.substring(h.length()-2);
          stringComplete = true;
          inStr="";//いちおう消しておく
        } else {
          char inChar = (char)mjSS[i]->read();
          //Check end of line 
          if(inChar=='\n' || inChar=='\r') {
            stringComplete = true;
            //if(!postmode) ss[i]->flush(); //読み飛ばし "OK"など
            // flashは Arduino 1.0から、読み込みを待つコマンドになったため
            // flashしないで、次の読み込みで、次の処理をすることに
            break;
          } else {
            inStr += inChar;
          }
        }
        delay(1);
      }
    }
  }
  */
  if(stringComplete) {  //データあり
    doMixJuice();
  }

  #ifndef ARDUINO_ESP8266_MODULE
  while (mjSub.available()) {
    mjMain.write(mjSub.read());
  }
  #endif
  
  // Check if a client has connected
  // if(isServer)
  server.handleClient();

  #ifdef supportUDP
    if(isUDP) MJ_UDP_ReadPacket();
    tello_loop();
  #endif

  //キーボードユニット
  #ifdef CARDKB_ADDR
  getKeyData(CARDKB_ADDR);
  #endif //CARDKB_ADDR
  
  #ifdef FACES_ADDR
  if(digitalRead(FACES_INT) == LOW) {
    getKeyData(FACES_ADDR);
  }
  #endif //FACES_ADDR

  // LEDのピンを読み取ってM5StickCのLEDを点灯
  #if defined(ARDUINO_M5StickC_ESP32) ||defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_ESP32_MODULE)
    digitalWrite(espLED, (digitalRead(ijLED)==LED_ON));
  #endif
  
  // Wait a bit before scanning again
  delay(1);
}

/***************************************
 * Check MixJuice data
 * 
***************************************/

void doMixJuice() {
  stringComplete=false;
  String cs=inStr;//文字をコピー
  cs.toUpperCase();//大文字に


  if(postmode) {
    /**** POSTモード 処理 ****/
    if(cs.startsWith("MJ POST END")) {
      //mjSer.println(posttype);
      //mjSer.println(postaddr);
      //mjSer.println(postdata);
      /* POST通信開始 */
      MJ_HTML(posttype,postaddr);
      ResetPostParam();
    } else if(cs.startsWith("MJ POSTS END")) {
      /* POST通信開始 */
      MJ_HTMLS(posttype,postaddr);
      ResetPostParam();
    } else if(cs.startsWith("MJ POST CANCEL") || cs.startsWith("MJ POSTS CANCEL") ||
              cs.startsWith("MJ POST STOP") || cs.startsWith("MJ POSTS STOP") ||
              cs.startsWith("MJ POST ESC") || cs.startsWith("MJ POSTS ESC")) {
      /*POST/POSTSデータリセット*/
      ResetPostParam();
      
    } else {
      /* POSTコンテンツを記録 */
      switch(posttype) {
        case HTML_POST_QUEST:
          //postdata+=inStr;
          postdata.toUpperCase();
          //mjSer.println(postdata);
          if(postdata.length()>=1024) {
            postdata=questBinToProg(questData)+'\n'+"[HEX]"+'\n'+postdata;
            inStr="MJ POST END";
            doMixJuice();
          }
          break;
        default:
          if(inStr.endsWith("\n")) inStr=inStr.substring(0,inStr.length()-1);//最後の"\n"を外す
          postdata+=inStr;
          postdata+="\n";
          break;
      }

    }
    
  } else if(cs.startsWith("MJ APL")||cs.startsWith("TJ APL")||cs.startsWith("FP APL")) {
    /*** WiFiリスト ***/ MJ_APL();
      
  } else if(cs.startsWith("MJ APS")||cs.startsWith("TJ APS")||cs.startsWith("FP APS")) {
   /*** 接続確認 ***/ MJ_APS();
    
  } else if(cs.startsWith("MJ APD")||cs.startsWith("TJ APD")||cs.startsWith("FP APD")) {
    /*** 切断 ***/ MJ_APD();
    
  } else if(cs.startsWith("MJ APC")||cs.startsWith("TJ APC")||cs.startsWith("FP APC")) {
    /*** 接続 ***/
    MJ_APC(inStr.substring(7));
    
  } else if(cs.startsWith("MJ SOFTAP")) {
    /*** 接続 ***/
    MJ_SOFTAP(inStr.substring(10));
    
  } else if(cs.startsWith("MJ GET ")) {
    /*** GET通信 ****/
    MJ_HTML(HTML_GET,inStr.substring(7));
   
  } else if(cs.startsWith("MJ GKP ")) {
    /*** KidspodからプログラムをGET通信 ****/
    MJ_HTML(HTML_GET,"kidspod.club/mj/"+inStr.substring(7));
   
  } else if(cs.startsWith("MJ QGKP ")) {
    /*** KidspodからQuestプログラムをGET通信 ****/
    MJ_HTML(HTML_GET_QUEST,"kidspod.club/mj/"+inStr.substring(8));
   
  } else if(cs.startsWith("MJ GETS ")) {
    /**** HTMLS GET通信 ****/
    MJ_HTMLS(HTML_GETS,inStr.substring(8));
   
  } else if(cs.startsWith("MJ POST START ")) {
    /*** POST START通信 ***/
    MJ_POST_START(HTML_POST,inStr.substring(14));
    
  } else if(cs.startsWith("MJ PKP ")) {
    /*** KidspodにPOST START通信 ***/
    MJ_POST_START(HTML_POST,"kidspod.club/mj/"+inStr.substring(7));
    /*** リスト表示 ***/
    mjSer.println("LIST\n");
    
  } else if(cs.startsWith("MJ QPKP ")) {
    /*** KidspodにQuestpuroグラムをPOST START通信 ***/
    MJ_POST_START(HTML_POST_QUEST,"kidspod.club/mj/"+inStr.substring(8));

  } else if(cs.startsWith("MJ QSEND ")) {
    /*** Questデータを通信 ***/
    MJ_QSEND(inStr.substring(9));  
    
  } else if(cs.startsWith("MJ POSTS START ")) {
    /*** HTMLS POST通信 ***/
    MJ_POST_START(HTML_POSTS,inStr.substring(15));
    
  } else if(cs.startsWith("MJ SLEEP ")) {
    /*** スリープ ***/
    MJ_SLEEP(inStr.substring(9));
    
  } else if(cs.startsWith("MJ MJVER")) {
    /*** バージョン ***/ mjSer.println("'"+MicJackVer);
    // Verで、エラー表示で、MixJuiceのバージョンを表示
    // MJVerで、MicJackのバージョンを表示
    
  } else if(cs.startsWith("MJ TJVER")||cs.startsWith("TJ VER")) {
    /*** バージョン ***/ mjSer.println("'"+TelloJackVer);
    // Verで、エラー表示で、MixJuiceのバージョンを表示
    // TelloJackのバージョンを表示
    
  } else if(cs.startsWith("MJ LIP")) {
    /*** Local IP ***/ mjSer.println(staIP);
    
  } else if(cs.startsWith("MJ MACADDR")) {
    /*** Mac address ***/ MJ_MACADDR();
    
  } else if(cs.startsWith("MJ MAC")) {
    /*** Mac address ***/ MJ_MAC();
    
  } else if(cs.startsWith("MJ PROXY")) {
    /*** PROXY & PORT設定 ***/
    MJ_PROXY(inStr.substring(9));
    
  } else if(cs.startsWith("MJ PORT")) {
    /*** PORT設定 ***/
    MJ_PORT(inStr.substring(8));
  
  } else if(cs.startsWith("MJ PCT")) {
    /*** PCT ***/ SetPCT(inStr.substring(7));
  
  } else if(cs.startsWith("MJ SSID")) {
    /*** SSID ***/ SetSSID(inStr.substring(8));
  
  } else if(cs.startsWith("MJ PWD")) {
    /*** PASS ***/ SetPASS(inStr.substring(7));
  
  } else if(cs.startsWith("MJ RGA ")||cs.startsWith("TJ RGA ")) { //# SSID PWD
    /*** Registration of ssid ***/ RegistSSID(inStr.substring(7));
  
  } else if(cs.startsWith("MJ RGC ")||cs.startsWith("TJ RGC ")) { //#
    /*** Connect ssid ***/ MJ_ConnectNum(inStr.substring(7));

  } else if(cs.startsWith("MJ RGL")||cs.startsWith("TJ RGL")) {
    /*** Show ssid List***/ MJ_RegistList();

  } else if(cs.startsWith("MJ RGD ")||cs.startsWith("TJ RGD ")) { //#
    /*** Connect ssid ***/ MJ_DeleteReg(inStr.substring(7));

  } else if(cs.startsWith("MJ GETHOME")) {
    /*** HOME ***/ SetHome(inStr.substring(11),false);
  
  } else if(cs.startsWith("MJ GETSHOME")) {
    /*** HOME ***/ SetHome(inStr.substring(12),true);
  
  } else if(cs.startsWith("MJ GETLAST")) {
    /*** LAST ***/ MJ_HTML(HTML_GET,lastGET);
  
  } else if(cs.startsWith("MJ GETSLAST")) {
    /*** LAST ***/ MJ_HTMLS(HTML_GETS,lastGET);
  
  } else if(cs.startsWith("MJ SPW")) {
    /*** SPW ***/ MJ_SPW(inStr.substring(7));
  
  } else if(cs.startsWith("MJ PMODE ") || cs.startsWith("MJ PINMODE ")) {
    /*** pinMode ***/ MJ_PINMODE(inStr);
  
  } else if(cs.startsWith("MJ DWRT ") || cs.startsWith("MJ DIGITALWRITE ") || cs.startsWith("MJ OUT ")) {
    /*** digitalWrite ***/ MJ_DIGITALWRITE(inStr);
  
  } else if(cs.startsWith("MJ DREAD ") || cs.startsWith("MJ DIGITALREAD ") || cs.startsWith("MJ IN ")) {
    /*** digitalRead ***/ MJ_DIGITALREAD(inStr);
  
  } else if(cs.startsWith("MJ AWRT ") || cs.startsWith("MJ ANALOGWRITE ")) {
    /*** analogWrite ***/ MJ_ANALOGWRITE(inStr);
  
  } else if(cs.startsWith("MJ AREAD") || cs.startsWith("MJ ANA")) {
    /*** digitalRead ***/
    /* analog input 0 - 1V */
    #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
    #else
    mjSer.println(analogRead(A0));
    #endif

  } else if(cs.startsWith("MJ KBDCMD ON") || cs.startsWith("MJ KBDCMD 1")) {
    /*** Host KBD command ON ***/ MJ_HOSTKBDCMD(true);
  
  } else if(cs.startsWith("MJ KBDCMD OFF") || cs.startsWith("MJ KBDCMD 0")) {
    /*** Host KBD command OFF ***/ MJ_HOSTKBDCMD(false);

  } else if(cs.startsWith("MJ IJKBD")) {
    /*** Reset for IchigoJam Keyboard ***/ MJ_IJKBD();
  
  } else if(cs.startsWith("MJ KBD")) {
    /*** Input Mode ***/ MJ_KBD(true);
    
  } else if(cs.startsWith("MJ UART")) {
    /*** Input Mode ***/ MJ_KBD(false);
  
//} else if(cs.startsWith("MJ SERVER") || cs.startsWith("MJ SVR")) {
//  /*** Sever Start ***/ serverStart();

  #ifdef ARDUINO_M5StickC_ESP32
  } else if(cs.startsWith("MJ SETRTC")) {
    /*** RTCをリセット ***/ resetRTC(inStr.substring(10));
    
  #endif //ARDUINO_M5StickC_ESP32
  } else if(cs.startsWith("MJ GETRTC")) {
    /*** RTCデータを取得 ***/ getRTC(inStr.substring(7).toInt());

  #ifndef ARDUINO_ESP8266_MODULE
  } else if(cs.startsWith("MJ PSUB")) {
    /*** 出力先 ***/ printToSub=(inStr.substring(8).toInt()==1);
  #endif
  
  #ifdef supportUDP
  } else if(cs.startsWith("MJ UDP START")) {
    /*** UDP Start ***/ MJ_UDP_Start(inStr.substring(13));

  } else if(cs.startsWith("MJ UDP STOP")) {
    /*** UDP Start ***/ if(isUDP) {udp.stop();isUDP=false;}

  } else if(cs.startsWith("MJ UDP MSG")) {
    /*** UDP Write ***/ MJ_UDP_Write(inStr.substring(11));
    
  } else if(cs.startsWith("MJ UDP")) {
    /*** UDP Write ***/ MJ_UDP_WritePacket(inStr.substring(7));
  
  #ifdef supportTELLO
  } else if(cs.startsWith("TJ INIT")||cs.startsWith("FP INIT")||
            cs.startsWith("TJ START")||cs.startsWith("FP START")) {
    /*** UDP Start ***/ tello_udp_start(0,true);

  } else if(cs.startsWith("TJ CLOSE")||cs.startsWith("FP CLOSE")) {
    /*** Tello Command Start ***/ tello_udp_stop(0);

  } else if(cs.startsWith("TJ BREAK")||cs.startsWith("FP BREAK")) {
    /*** Tello Command Start ***/ Break_Tello();

  } else if(cs.startsWith("TJ STATE")||cs.startsWith("FP STATE")) {
    /*** Tello State Start ***/ tello_udp_start(1,false);

  } else if(cs.startsWith("TJ VIDEO")||cs.startsWith("FP VIDEO")) {
    /*** Tello Video Start ***/ tello_udp_start(2,false);

  //--------------------
  //Queue control mode
  //--------------------
  
  } else if(cs.startsWith("TJ Q")||cs.startsWith("FP Q")) {
    /*** FP Direct Command ***/ Tello_Queue_Command(inStr.substring(4));

  //--------------------------------
  // Command and Radio control mode
  //--------------------------------
      
  } else if(cs.startsWith("TJ RESON")||cs.startsWith("FP RESON")) {
    /*** force show result ***/ showForce=true; SaveAPConfig();

  } else if(cs.startsWith("TJ RESOFF")||cs.startsWith("FP RESOFF")) {
    /*** force show result ***/ showForce=false; SaveAPConfig();

  } else if(cs.startsWith("TJ S ")||cs.startsWith("FP S ")) {
    /*** FP Direct Command ***/ showRes=true;
     Tello_Direct_Command(inStr.substring(5));

  } else if(cs.startsWith("TJ ")||cs.startsWith("FP ")) {
    /*** FP Direct Command ***/ Tello_Direct_Command(inStr.substring(3));

  #endif //supportTELLO 
  #endif //supportUDP
  
  } else if(cs.startsWith("MJ ")||cs.startsWith("TJ ")||cs.startsWith("FP ")) {
    /*** NG ***/ mjSer.println("'NG: "+MJVer);
  }

  inStr="";

}

/***************************************
 * MJ APC ssid password
 * WiFiアクセスポイントに接続
 * 
***************************************/

void MJ_APC(String ssidpwd) {
  ssidpwd.replace("\ "," ");
  String tss="";
  String tps="";
  
  int sc=ssidpwd.length(); /* 文字数 */
  int ps=ssidpwd.lastIndexOf(" ",sc-1); /* スペースの位置 */
  
  if(sc<=0) {/*** 文字がない場合 ***/
    if(lastSSID.length()>0 && lastPASS.length()>0) {
      tss=lastSSID; tps=lastPASS;
    } else {
      return;
    }
  } else if(ps<0) {/*** スペースがない場合 ***/
    if(lastPASS.length()>0) {
      tss=ssidpwd; tps=lastPASS;
    } else {
      tss=ssidpwd.substring(0,ps);//return;
    }
  } else {
    tss=ssidpwd.substring(0,ps);
    tps=ssidpwd.substring(ps+1);
  }

  int apn=tss.toInt();
  if(tss.length()==1 && (apn>=0 && apn<10)) tss=aplist[apn];
  
  //mjSer.println("'"+tss);
  //mjSer.println("'"+tps);

  MJ_APC_SSID_PSSS(tss,tps);
}

void MJ_APC_SSID_PSSS(String tss, String tps) {
  WiFi.begin(tss.c_str(),tps.c_str());

  //Setup時　コネクトしたように見えてしてない場合があるための処理
  int64_t timeout = millis() + 10000;
  mjSer.print("'Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    mjSer.print(".");
    if (timeout - millis() < 0) {
      mjSer.println("");
      mjSer.println("'Couldn't get a wifi connection");
      return;
    }
  }
  mjSer.println("");

  if (WiFi.status() != WL_CONNECTED) { 
    mjSer.println("'Couldn't get a wifi connection");
    //while(true);
  } else {
    //Station mode SSID
    mjSer.println("'WiFi Station: "+tss);
    
   // if you are connected, print out info about the connection:
    //print the local IP address
    staIP = WiFi.localIP();
    mjSer.print("'IP: "); mjSer.println(staIP);
    
    //最後に接続した情報
    lastSSID=tss;
    lastPASS=tps;
    SaveAPConfig();
    
    #ifdef useMJLED
      #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
        digitalWrite(connLED, HIGH);
      #elif defined(ARDUINO_M5StickC_ESP32)
        //digitalWrite(connLED, LOW);
      #endif
      #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
        ConnectStatusLED(true,IPAddressToStr(staIP));
      #endif
    #endif

    // Add service to MDNS-SD
    MDNS.begin(mjname);
    MDNS.addService("http", "tcp", 80);
    delay(500); 
    mjSer.print("'http://");
    mjSer.print(mjname);mjSer.println(".local/");
    delay(500); mjSer.println("");

    #ifdef ARDUINO_M5StickC_ESP32
      if(initRTC==false) resetRTC("");
    #endif //ARDUINO_M5StickC_ESP32
  }
}

void MJ_RegistList() {
  int i;
  for(i=0;i<8;i++) {
    mjSer.print("'#");mjSer.print(i);mjSer.print(": ");
    if(apcbuf.sp[i].ssid[0]>0x20&&apcbuf.sp[i].ssid[0]<0x80) {
      mjSer.println(String(apcbuf.sp[i].ssid));
    } else {
      if(apcbuf.sp[i].ssid[0]>0) {
        for(int j=0;j<32;j++) {
            apcbuf.sp[i].ssid[j]=0;
            apcbuf.sp[i].pass[j]=0;
        }
        SaveAPConfig();
      }
      mjSer.println("");
    }
  }
}

void MJ_ConnectNum(String tid) {
  int id=tid.toInt();
  if(id<0||id>7) return;
  MJ_APC_SSID_PSSS(String(apcbuf.sp[id].ssid),String(apcbuf.sp[id].pass));
}

void MJ_DeleteReg(String tid) {
  int id=tid.toInt();
  if(id<0||id>7) return;
  for(int j=0;j<32;j++) {
      apcbuf.sp[id].ssid[j]=0;
      apcbuf.sp[id].pass[j]=0;
  }
  SaveAPConfig();
}

void RegistSSID(String idssidpwd) {
  idssidpwd.replace("\ "," ");
  String tid="";//0-7
  String tss="";
  String tps="";

  tid=idssidpwd.substring(0,1);
  int id=tid.toInt();
  if(id<0 || id>7) {
    //mjSer.println("'error");
    return;
  }
  
  String ssidpwd=idssidpwd.substring(2);
  int sc=ssidpwd.length(); /* 文字数 */
  int ps=ssidpwd.lastIndexOf(" ",sc-1); /* スペースの位置 */
  
  if(sc<=0) {/*** 文字がない場合 ***/
    if(lastSSID.length()>0 && lastPASS.length()>0) {
      tss=lastSSID; tps=lastPASS;
    } else {
      //mjSer.println("'error");
      return;
    }
  } else if(ps<0) {/*** スペースがない場合 ***/
    if(lastPASS.length()>0) {
      tss=ssidpwd; tps=lastPASS;
    } else {
      tss=ssidpwd.substring(0,ps);//return;
    }
  } else {
    tss=ssidpwd.substring(0,ps);
    tps=ssidpwd.substring(ps+1);
  }

  int apn=tss.toInt();
  if(tss.length()==1 && (apn>=0 && apn<10)) tss=aplist[apn];

  strcpy(apcbuf.sp[id].ssid,tss.c_str());
  strcpy(apcbuf.sp[id].pass,tps.c_str());
  SaveAPConfig();

  mjSer.print("'Saved #");mjSer.println(tid);
}

/***************************************
 * 
 * WiFiアクセスポイント
 * SSID / Password関連
 * 
***************************************/

bool CheckEEPROMdata(char *s) {
  for(int i=0; i<32; i++) {
    if(s[i]==0) {
      if(i>0) return true;
      break;
    }
    if(s[i]<0x20 || s[i]>0x7F) break;
  }
  for(int i=0; i<32; i++) { s[i]=0; }
  return false;
}

void LoadAPConfig() {
  EEPROM.begin(1024);//使用するサイズを宣言
  EEPROM.get<APCONFIG>(0, apcbuf);
  CheckEEPROMdata(apcbuf.ssid); lastSSID=String(apcbuf.ssid);
  CheckEEPROMdata(apcbuf.pass); lastPASS=String(apcbuf.pass);
  CheckEEPROMdata(apcbuf.homepage); homepage=String(apcbuf.homepage);
  
  //if(CheckEEPROMdata(apcbuf.softap_ssid)==false||CheckEEPROMdata(apcbuf.softap_pass)==false) {
    String mjssid=GetMJSoftApSSID();
    strcpy(apcbuf.softap_ssid,mjssid.c_str());//default_softap_ssid);
    
  if(CheckEEPROMdata(apcbuf.softap_pass)==false) {
    strcpy(apcbuf.softap_pass,default_softap_pass);
  }

  showForce=apcbuf.showForce;
}

void SaveAPConfig() { //String tss, String tps, String hp) {
  for(int i=0; i<32; i++) {
    apcbuf.ssid[i]=0;
    apcbuf.pass[i]=0;
    apcbuf.homepage[i]=0;
  }
  
  lastSSID.toCharArray(apcbuf.ssid,lastSSID.length()+1);
  lastPASS.toCharArray(apcbuf.pass,lastPASS.length()+1);
  homepage.toCharArray(apcbuf.homepage,homepage.length()+1);
  apcbuf.showForce=showForce;
  
  EEPROM.put<APCONFIG>(0, apcbuf);
  EEPROM.commit();//内蔵フラッシュメモリに実際に書込
}

void SetPCT(String s) {
  int n=s.length();
  if(n<=0) {
    myPostContentType="";
  } else {
    //Content-Type: application/x-www-form-urlencoded;\r\n
    myPostContentType="Content-Type: "+s+"\r\n";
  }
}

void SetSSID(String s) {
  int n=s.length();
  if(n<=0) {
    mjSer.println("'Station: "+lastSSID);
    //mjSer.println("'IP: "+lastSSID);
  } else {
    lastSSID=s; SaveAPConfig();
  }
}

void SetPASS(String s) {
  int n=s.length();
  if(n<=0) {
    mjSer.println("'"+lastPASS);
  } else {
    lastPASS=s; SaveAPConfig();
  }
}

void SetHome(String s, bool isGets) {
  int n=s.length();
  if(n<=0) {
    if(isGets) {
      MJ_HTMLS(HTML_GETS,homepage);
    } else {
      MJ_HTML(HTML_GET,homepage);
    }
  } else {
    homepage=s; SaveAPConfig();
  }
}

/***************************************
 * MJ APC ssid password
 * WiFiアクセスポイントに接続
 * 
***************************************/

void MJ_SOFTAP(String ssidpwd) {
  ssidpwd.replace("\ "," ");
  String tss="";
  String tps="";
  
  int sc=ssidpwd.length(); /* 文字数 */
  int ps=ssidpwd.lastIndexOf(" ",sc-1); /* スペースの位置 */
  
  if(sc<=0) {/*** 文字がない場合 ***/
    ;
  } else if(ps<0) {/*** スペースがない場合 ***/
    tss=sc;//でtps=""
  } else {
    tss=ssidpwd.substring(0,ps);
    tps=ssidpwd.substring(ps+1);
  }
  
  //SetSoftAP(tss,tps);
  int n=tss.length();
  int m=tps.length();
  /*
  if(n>0&&m<8) {//パスワードがなしでもOKとする
    mjSer.println("'Pass needs more than 8chrs..");
    delay(1500);
  } else */if(n<=0&&m<=0) {//引数がない場合は、情報表示
    String softap_ssid="";
    softap_ssid=String(apcbuf.softap_ssid);
    mjSer.println("'AP: "+softap_ssid);delay(500);
    mjSer.print("'IP: "); mjSer.println(mySoftAPIP);
  } else {
    tss.toCharArray(apcbuf.softap_ssid,n+1);
    tps.toCharArray(apcbuf.softap_pass,m+1);
    SaveAPConfig();
    mjSer.println("'OK. ESP restart...");
    delay(1500);
    ESP.restart();
  }
}

/***************************************
 * MJ_APL
 * WiFiアクセスポイントリストを表示
 * 
***************************************/

void MJ_APL() {
  int cc;//Bytes of SSID name
  mjSer.println("'Scan Start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    mjSer.println("'No networks found");
  } else {
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      String s="'"+String(i)+": ";
      s=s+WiFi.SSID(i);//+" ("+WiFi.RSSI(i)+")";
      //mjSer.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      mjSer.println(s);
      delay(100);
      if(i<10) aplist[i]=WiFi.SSID(i);
    }
    mjSer.println("");
  }  
}

/***************************************
 * MJ APS
 * WiFiアクセスポイントへの接続を確認
 * 戻り値が0の場合は未接続、1の場合は接続中
 * 
***************************************/

void MJ_APS() {
  if(WL_CONNECTED==WiFi.status())
    mjSer.println("1");
  else
    mjSer.println("0");
}

/***************************************
 *  MJ APD
 *  WiFiアクセスポイントから切断
 *  
***************************************/

void MJ_APD() {
  WiFi.disconnect();
  mjSer.println("'OK");
  #ifdef useMJLED
    #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
        digitalWrite(connLED, HIGH);
      #elif defined(ARDUINO_M5StickC_ESP32)
        //digitalWrite(connLED, LOW);
    #endif
    #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
      ConnectStatusLED(false);
    #endif
  #endif
}

/***************************************
 * MJ_APLAPC
 * WiFiアクセスポイントに自動ログイン
 * 
***************************************/

void MJ_APLAPC() {
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if(lastSSID.equals(WiFi.SSID(i))) {
      MJ_APC("");
      return;
    }
  }
  String ws="";
  
  for(int j=0; j<8; j++) {
    ws=apcbuf.sp[j].ssid;
    for (int i=0; i<n; ++i) {
      if(ws.equals(WiFi.SSID(i))) {
        MJ_APC_SSID_PSSS(ws,apcbuf.sp[j].pass);
        return;
      }
    }
  }
}

/***************************************
 *  MJ SLEEP sec
 *  スリープ(節電モード)
 *  
 *  WAKE_RF_DEFAULT = 0, // RF_CAL or not after deep-sleep wake up, depends on init data byte 108.
 *  WAKE_RFCAL = 1,      // RF_CAL after deep-sleep wake up, there will be large current.
 *  WAKE_NO_RFCAL = 2,   // no RF_CAL after deep-sleep wake up, there will only be small current.
 *  WAKE_RF_DISABLED = 4 // disable RF after deep-sleep wake up, just like modem sleep, there will be the smallest current.
 *  
***************************************/

void MJ_SLEEP(String tss) {

  int sc=tss.length();
  if(sc<=0) return;
  
  int ps=tss.lastIndexOf(" ",sc-1);
  uint32_t ts;

  #ifdef ARDUINO_ESP8266_MODULE
  RFMode wm=WAKE_RF_DEFAULT;
  if(ps<0) {
    ts=(uint32_t)tss.toInt();
  } else {
    ts=(uint32_t)tss.substring(0,ps).toInt(); //mjSer.println(tss.substring(0,ps));
    wm=(RFMode)tss.substring(ps+1).toInt(); //mjSer.println(tss.substring(ps+1));
  }
  
  ESP.deepSleep(ts,wm);
  #endif
}

/***************************************
 *  HexTextToValue
 *  Hex文字を数値化
 *  
***************************************/

uint8_t HexTextToVal(uint8_t a, uint8_t b) {
  //return (uint8_t)strtol(ab,NULL,16);
  uint8_t va,vb;
  if(a>='0'&&a<='9') {
    va=a-'0';
  } else if(a>='A'&&a<='F') {
    va=a-'A'+10;
  } else if(a>='a'&&a<='f') {
    va=a-'a'+10;
  } else {
    return 0;
  }
  if(b>='0'&&b<='9') {
    vb=b-'0';
  } else if(b>='A'&&b<='F') {
    vb=b-'A'+10;
  } else if(b>='a'&&b<='f') {
    vb=b-'a'+10;
  } else {
    return 0;
  }
  return (va*16+vb);
}

/*
 * 読み飛ばし
 */
bool clientReadTo(WiFiClient client, char x) {
  char c;
  bool b;
  int n;
  do {
    n+=1;
    c=client.read();
    if(c==x) b=true;
  } while(b==false||n<1024);
  return b;
}

/***************************************
 *  MJ GET / POST
 *  HTTP GET/POST 通信
 *  
***************************************/

void MJ_HTML(int type, String addr) {
  int rcvCnt=0,allCnt=0;
  bool hexStart=false;
  bool hexEnd=false;
  int sc=addr.length();
  if(sc<=0) return;

  String host="";
  String url="/";
  String prm="";
  
  int ps;
  ps=addr.indexOf("/");
  if(ps<0) {
    host=addr;
  } else {
    host=addr.substring(0,ps);  // /より前
    url=addr.substring(ps);     // /を含んで後ろ
  }
  //mjSer.println("'"+host);
  //mjSer.println("'"+url);

  //------プロキシ-----
  String httpServer=host;
  if(useProxy) httpServer=httpProxy;

  //------ポート-----
  int port=httpPort;
  ps=host.indexOf(":");
  if(ps>0) {
    host=host.substring(0,ps);  // :より前
    port=host.substring(ps+1).toInt();     // :より後ろ
  }
  
  //------せつぞく-----
  WiFiClient client;
  if(!client.connect(httpServer.c_str(), port)) {
    return;
  }
    
  //------リクエスト-----
  switch(type) {
    case HTML_GET:
    case HTML_GETS:
    case HTML_GET_QUEST:
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(getLED, HIGH);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          GetStatusLED(true,addr);
        #endif
      #endif
      client.print(String("GET ") + url + " HTTP/1.0\r\n" + 
                   "Host: " + host + "\r\n" + 
                   "User-Agent: " + MJVer + " " + MicJackVer + "\r\n" + 
                   //"Connection: close\r\n" +
                   "\r\n");
                   //"Accept: */*\r\n" + 
      break;

    case HTML_POST:
    case HTML_POSTS:
    case HTML_POST_QUEST:
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(postLED, HIGH);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          PostStatusLED(true,addr);
        #endif
      #endif

      prm=postdata;//postdata.replace("\n","\r\n");//改行を置き換え

      client.print(String("POST ") + url + " HTTP/1.0\r\n" + 
                  "Accept: */*\r\n" + 
                  "Host: " + host + "\r\n" + 
                  "User-Agent: " + MJVer + " " + MicJackVer + "\r\n" + 
                  //"Connection: close\r\n" +
                  myPostContentType +
                  "Content-Length: " + String(prm.length()) + "\r\n" +
                  "\r\n" + 
                  prm);
                  //"Content-Type: application/x-www-form-urlencoded;\r\n" +
     
      break;

    default:
      return;
  }

  //------待ち-----
  int64_t timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      mjSer.println("'Client Timeout !");
      client.stop();
      return;
    }
  }

  //------レスポンス-----
  switch(type) {
    case HTML_GET:
    case HTML_GETS:
    case HTML_GET_QUEST:
      /* ヘッダー 読み飛ばし */
      while(client.available()){
        String line = client.readStringUntil('\n');
        if(line.length()<2) break;
      }
       
      /*
       * プログラム入力時は、改行後、50msくらい,
       * 文字は、20msはあけないと、
       * IchigoJamは処理できない
      */
      if(spw<=1) {
        while(client.available()){
          String line = client.readStringUntil('\n');
          mjSer.print(line);
          mjSer.print('\n');
          delay(spn);
        }
      } else {
        while(client.available()){
          char c = client.read(); allCnt+=1;
          uint8_t a,b;
          switch(type) {
            case HTML_GET_QUEST:
              if(hexEnd) continue;
              if(allCnt==1&&(c=='0'||c=='1')) hexStart=true;
              if(hexStart) {
                if(rcvCnt<512) {
                  a=(uint8_t)c;
                  if((a>='0'&&a<='9')||(a>='A'&&a<='F')||(a>='a'&&a<='f')) {
                    b = (uint8_t)client.read();
                    uint8_t v=HexTextToVal(a,b);
                    mjSer.write(v);//mjSer.write((uint8_t)strtol(ab,NULL,16));
                    //String h="0"+String(HexTextToVal(a,b),HEX);
                    //mjSer.print(h.substring(h.length()-2));
                    if((rcvCnt%8)==0&&v==0xFF) hexEnd=true;
                    rcvCnt+=1;
                  } else {
                    //HEX文字以外のコードの場合は、以降を次の行まで読み飛ばす
                    //hexStart=clientReadTo(&client,'\n');
                    int n;
                    do {
                      n+=1;
                      c=client.read();
                      if(c=='\n') hexStart=true;
                    } while(hexStart==false||n<1024);
                  }
                }
              } else if(c=='[') {
                //後からコードの場合は、"["がキーでその行以降からプログラムとして読み込む
                //hexStart=clientReadTo(&client,'\n');
                int n;
                do {
                  n+=1;
                  c=client.read();
                  if(c=='\n') hexStart=true;
                } while(hexStart==false||n<1024);
              }
              delay(spw*2);
              break;
            default:
              switch(c){
                case 0: case '\r':
                  break;
                case '\n': mjSer.print('\n'); delay(spn);
                  break;
                default: mjSer.print(c); delay(spw);
                  break;
              }
              break;
          }
        }
        /* もし512バイトよりすくなかったら */
        if(type==HTML_GET_QUEST) {
          while(rcvCnt<512) {
            if((rcvCnt%8)==0)
              mjSer.write((uint8_t)0xFF);
            else
              mjSer.write((uint8_t)0);
            rcvCnt+=1;
            delay(spw*2);
          }
        }
      }

      if(!addr.equals(homepage)) lastGET=addr;
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(getLED, LOW);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          GetStatusLED(false);
        #endif
      #endif
      break;

    case HTML_POST:
    case HTML_POSTS:
    case HTML_POST_QUEST:
      mjSer.println("'POST OK!");
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(postLED, LOW);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          PostStatusLED(false);
        #endif
      #endif
      break;
      
    default:
      /* 読み飛ばし */
      while(client.available()){
        String line = client.readStringUntil('\r');
      }
      break;
  }

}

/***************************************
 *  MJ GETS / POSTS
 *  HTTP GETS/POSTS 通信
 *  
***************************************/

void MJ_HTMLS(int type, String addr) {
  int sc=addr.length();
  if(sc<=0) return;

  String host="";
  String url="/";
  String prm="";
  
  int ps;
  ps=addr.indexOf("/");
  if(ps<0) {
    host=addr;
  } else {
    host=addr.substring(0,ps);  // /より前
    url=addr.substring(ps);     // /を含んで後ろ
  }
  //mjSer.println("'"+host);
  //mjSer.println("'"+url);

  //------プロキシ-----
  String httpServer=host;
  if(useProxy) httpServer=httpProxy;

  //------ポート-----
  int port=httpPort;
  ps=host.indexOf(":");
  if(ps>0) {
    host=host.substring(0,ps);  // :より前
    port=host.substring(ps+1).toInt();     // :より後ろ
  }
  
  switch(type) {
    case HTML_GET:
    case HTML_POST:
      break;
    case HTML_POSTS:
    case HTML_GETS:
      if(port==80) port=443;
      break;
  }
  
  //------セキュアーなせつぞく-----
  WiFiClientSecure client;
  if(!client.connect(httpServer.c_str(), port)) {
    return;
  }

  //------リクエスト-----
  switch(type) {
    case HTML_GET:
    case HTML_GETS:
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(getLED, HIGH);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          GetStatusLED(true,addr);
        #endif
      #endif
      client.print(String("GET ") + url + " HTTP/1.0\r\n" + 
                   "Host: " + host + "\r\n" + 
                   "User-Agent: " + MJVer + " " + MicJackVer + "\r\n" + 
                   //"Connection: close\r\n" + 
                   "\r\n");
                   //"Accept: */*\r\n" + 
      break;

    case HTML_POST:
    case HTML_POSTS:
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(postLED, HIGH);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          PostStatusLED(true,addr);
        #endif
      #endif
      prm=postdata;//postdata.replace("\n","\r\n");//改行を置き換え

      client.print(String("POST ") + url + " HTTP/1.0\r\n" + 
                  "Accept: */*\r\n" + 
                  "Host: " + host + "\r\n" + 
                  "User-Agent: " + MJVer + " " + MicJackVer + "\r\n" + 
                  //"Connection: close\r\n" +
                  myPostContentType +
                  "Content-Length: " + String(prm.length()) + "\r\n" +
                  "\r\n" + 
                  prm);
                  //"Content-Type: application/x-www-form-urlencoded;\r\n" +
     
      break;

    default:
      return;
  }

  //------待ち-----
  int64_t timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      mjSer.println("'Client Timeout !");
      client.stop();
      return;
    }
  }

  //------レスポンス-----
  switch(type) {
    case HTML_GET:
    case HTML_GETS:
      /* ヘッダー 読み飛ばし */
      while(client.available()){
        String line = client.readStringUntil('\n');
        if(line.length()<2) break;
      }
       
      /*
       * プログラム入力時は、改行後、50msくらい,
       * 文字は、20msはあけないと、
       * IchigoJamは処理できない
      */
      if(spw<=1) {
        while(client.available()){
          String line = client.readStringUntil('\n');
          mjSer.print(line);
          mjSer.print('\n');
          delay(spn);
        }
      } else {
        while(client.available()){
          char c = client.read();
          switch(c){
          case 0:
            break;
          case '\r':
            break;
          case '\n':
            mjSer.print('\n');
            delay(spn);
            break;
          default:
            mjSer.print(c);
            delay(spw);
            break;
          }
        }
      }
      if(!addr.equals(homepage)) lastGET=addr;
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(getLED, LOW);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          GetStatusLED(false);
        #endif
      #endif
      break;

    case HTML_POST:
    case HTML_POSTS:
      mjSer.println("'POST OK!");
      #ifdef useMJLED
        #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
          digitalWrite(postLED, LOW);
        #endif
        #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
          PostStatusLED(false);
        #endif
      #endif
      break;
      
    default:
      /* 読み飛ばし */
      while(client.available()){
        String line = client.readStringUntil('\r');
      }
      break;
  }

}

/***************************************
 *  MJ POST START addr
 *  POST/POSTS 開始
 *  
***************************************/

void MJ_POST_START(int type, String addr) {
  int n=addr.length();
  if(n>0) {
    posttype=type;
    postaddr=addr;
    postdata="";
    postmode=true;
    questEnd=false;

    #ifdef useMJLED
      #if defined(ARDUINO_ESP8266_MODULE) || defined(ARDUINO_ESP32_MODULE)
        digitalWrite(postLED, HIGH);
      #endif
      #if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)
        PostStatusLED(true,addr);
      #endif
    #endif
    
  }
}

/***************************************
 *  MJ QSEND
 *  IchigonQuesへのデータ転送 開始
 *  
***************************************/

void MJ_QSEND(String qd) {
  int rcvCnt=0,allCnt=0;
  bool hexStart=false;
  bool hexEnd=false;

  char c;
  uint8_t a,b;
  for(int i=0;i<qd.length();i++) {
    c=(char)qd.charAt(i);allCnt+=1;
    if(hexEnd) continue;
    if(allCnt==1&&(c=='0'||c=='1')) hexStart=true;
    if(hexStart) {
      if(rcvCnt<512) {
        a=(uint8_t)c;
        if((a>='0'&&a<='9')||(a>='A'&&a<='F')||(a>='a'&&a<='f')) {
          i+=1;b=(uint8_t)qd.charAt(i);
          uint8_t v=HexTextToVal(a,b);//mjSer.print(v,HEX);
          mjSer.write(v);//mjSer.write((uint8_t)strtol(ab,NULL,16));//mjSer.print(v,HEX);//
          //String h="0"+String(HexTextToVal(a,b),HEX);
          //mjSer.print(h.substring(h.length()-2));
          if((rcvCnt%8)==0&&v==0xFF) {
            hexEnd=true;
            i=qd.length();
          }
          rcvCnt+=1;
        } else {
          //HEX文字以外のコードの場合は、以降を次の行まで読み飛ばす
          //hexStart=clientReadTo(&client,'\n');
          int n;
          do {
            n+=1;
            i+=1; c=qd.charAt(i);
            if(c=='\n') hexStart=true;
          } while(hexStart==false||n<1024);
        }
      }
    } else if(a=='[') {
      //後からコードの場合は、"["がキーでその行以降からプログラムとして読み込む
      //hexStart=clientReadTo(&client,'\n');
      int n;
      do {
        n+=1;
        i+=1; c=qd.charAt(i);
        if(c=='\n') hexStart=true;
      } while(hexStart==false||n<1024);
    }
    delay(spw*2);
  }

  /* もし512バイトよりすくなかったら */
  while(rcvCnt<512) {
    if((rcvCnt%8)==0)
      mjSer.write((uint8_t)0xFF);
    else
      mjSer.write((uint8_t)0);
    rcvCnt+=1;
    delay(spw*2);
  }
}

/***************************************
 *  MJ PROXY addr:port
 *  PROXYを設定
 *  
***************************************/
void MJ_PROXY(String prxy) {
  int ps=prxy.indexOf(":");
  String httpProxy=prxy.substring(0,ps);  // :より前
  MJ_PORT(prxy.substring(ps+1));    // :を含まないで後ろ
}

/***************************************
 *  MJ PORT port
 *  PORTを設定
 *  
***************************************/
void MJ_PORT(String prt) {
  int n=prt.length();
  if(n==0) {
    httpPort=80;
  } else {
    httpPort=prt.toInt();
  }
}

/***************************************
 *  Reset POST parameters
 *  POST用パラメータをリセット
 *  
***************************************/
void ResetPostParam() {
  postmode=false;
  postaddr="";
  postdata="";
}

/***************************************
 *  MJ_MACADDR
 *  MACアドレスを表示する
 *  
***************************************/

String mac2String(byte ar[])
{
  String s;
  for (byte i = 0; i < 6; ++i)
  {
    char buf[3];
    sprintf(buf, "%02X", ar[i]);
    s += buf;
    if (i < 5) s += ':';
  }
  return s;
}

void MJ_MAC() {
  mjSer.print("'");
  mjSer.print("MAC Address: ");
  
  byte mac[6];
  WiFi.macAddress(mac);
  mjSer.println(mac2String(mac)); 
}

void MJ_MACADDR() {
  byte mac[6];
  WiFi.macAddress(mac);
  mjSer.print("'");
  mjSer.println(mac2String(mac));
}

String GetMJSoftApSSID() {
  byte mac[6];
  WiFi.macAddress(mac);
  String s="MJ-";
  for (byte i = 0; i < 6; ++i)
  {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    s += buf;
  }
  return s;  
}

/***************************************
 *  EachPrintSerial
 *  SerialPrintを少しずつ
 *  
***************************************/
void SerialPrint(String s, int m=1) {
  int cc=s.length();
  if(cc<=m)
    mjSer.println(s);
  else {
    for(int j=0; j<cc; j=j+m) {
      if(cc-j<m) {
        mjSer.print(s.substring(j));
      } else {
        mjSer.print(s.substring(j,j+m));
      }
      delay(spw);
    }
    mjSer.println("");
  }
}

/***************************************
 *  MJ_SPW
 *  通信Deleyの値を設定
 *  
***************************************/

void MJ_SPW(String sw) {
  int sc=sw.length(); /* 文字数 */
  int ps=sw.lastIndexOf(" ",sc-1); /* スペースの位置 */
  if(sc<=0) {
    /*** 文字がない場合 ***/
    spw=kspw; spn=kspn;
 } else if(ps<0) {
    /*** スペースがない場合 ***/
    spw=sw.toInt();
    spn=spw;
  } else {
    spw=sw.substring(0,ps).toInt();
    spn=sw.substring(ps+1).toInt();
  }
  mjSer.println("'OK");
}

/***************************************
 *  MJ_WIREDKBD
 *  キーボード入力対応ボードを使っている
 *  0: 使ってない
 *  1: 使っている
 *  
***************************************/

void MJ_HOSTKBDCMD(bool m) {
  
  if(apcbuf.useHostKbdCmd!=m) {
    apcbuf.useHostKbdCmd=m;
    SaveAPConfig();
  }
  
  if(m)
    mjSer.println("'Use host kbd cmd");
  else
    mjSer.println("'Unuse host kbd cmd");
}

/***************************************
 *  MJ_IJKBD
 *  IchigoJam Keyboard認識のためのリセット
 *  
***************************************/

void MJ_IJKBD() {
  MJ_HOSTKBDCMD(true);
  MJ_KBD(true);
  mjSer.println("RESET");
}

/***************************************
 *  MJ_KBD
 *  データー入力モード
 *  0: UART
 *  1: Keyboard
 *  
***************************************/

void MJ_KBD(bool m) {
  
  if(apcbuf.kbd!=m) {
    apcbuf.kbd=m;
    SaveAPConfig();
  }
  
  if(m)
    mjSer.println("'Keyboard Mode");
  else
    mjSer.println("'UART Mode");
}

/***************************************
 *  MJ_PINMODE
 *  ESP_WROOM-02のPINモードを設定
 *  
***************************************/

void MJ_PINMODE(String pm) {
  int sc=pm.indexOf(" ",3);
  if(sc<1) return;
  pm=pm.substring(sc+1);
  sc=pm.length(); /* 文字数 */

  int ps=pm.lastIndexOf(" ",sc-1); /* スペースの位置 */
  if(sc<=0 || ps<0) {
      /*** 文字がない場合 ***/
      /*** スペースがない場合 ***/
      return;
  } else {
    //OUTPUT=1, INPUT=0, INPUT_PULLUP=2
    int p=pm.substring(0,ps).toInt();
    int m=pm.substring(ps+1).toInt();
    pinMode(p,m);
  }
}

/***************************************
 *  MJ_DIGITALWRITE
 *  ESP_WROOM-02のdigitalWriteを設定
 *  
***************************************/

void MJ_DIGITALWRITE(String pio) {
  int sc=pio.indexOf(" ",3);
  if(sc<1) return;
  pio=pio.substring(sc+1);
  sc=pio.length(); /* 文字数 */
  
  int ps=pio.lastIndexOf(" ",sc-1); /* スペースの位置 */
  if(sc<=0 || ps<0) {
      /*** 文字がない場合 ***/
      /*** スペースがない場合 ***/
      return;
  } else {
    int p=pio.substring(0,ps).toInt();
    int io=pio.substring(ps+1).toInt();
    digitalWrite(p,io);
  }
}

/***************************************
 *  MJ_DIGITALREAD
 *  ESP_WROOM-02のdigitalWriteを設定
 *  
***************************************/

void MJ_DIGITALREAD(String pr) {
  int sc=pr.indexOf(" ",3);
  if(sc<1) return;
  pr=pr.substring(sc+1);
  sc=pr.length(); /* 文字数 */
  if(sc<=0) {
      /*** 文字がない場合 ***/
      return;
  } else {
    int v=digitalRead(pr.toInt());
    mjSer.println(v);//"'"+String(v));
  }
}

/***************************************
 *  MJ_ANALOGWRITE
 *  ESP_WROOM-02のanalogWriteを設定
 *  
***************************************/

void MJ_ANALOGWRITE(String pio) {
  int sc=pio.indexOf(" ",3);
  if(sc<1) return;
  pio=pio.substring(sc+1);
  sc=pio.length(); /* 文字数 */
  
  int ps=pio.lastIndexOf(" ",sc-1); /* スペースの位置 */
  if(sc<=0 || ps<0) {
      /*** 文字がない場合 ***/
      /*** スペースがない場合 ***/
      return;
  } else {
    int p=pio.substring(0,ps).toInt();
    int io=pio.substring(ps+1).toInt();
    if(io<0) io=0; else if(io>255) io=255;
    #ifdef ARDUINO_ESP8266_MODULE
    analogWrite(p,io);
    #endif
  }
}

/***************************************
 * IP to String
 * IPを文字に
 * 
***************************************/

String IPAddressToStr(IPAddress ip) {
  char buf[13];
  sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return String(buf);
}

/***************************************
 * 
 * Draw QRcord of Station mode
 * 
****************************************/

#ifdef hasDISP

#ifdef ARDUINO_M5Stack_Core_ESP32
extern int scrdiff;
#endif

void drawAPQRCode() {
  String apip = "";
  
  if (staIP[0] >= 0x20 && staIP[0] != '?') {
    apip = "http://" + IPAddressToStr(staIP) + "/";
    
    #ifdef ARDUINO_M5Stack_Core_ESP32
      M5.Lcd.qrcode(apip,200,40+scrdiff,108,2);
    #endif
    #ifdef ARDUINO_M5StickC_ESP32
      //M5.Lcd.qrcode("http://www.m5stack.com");
      //M5.Lcd.qrcode(const char *string, uint16_t x = 50, uint16_t y = 10, uint8_t width = 220, uint8_t version = 6);
      M5.Lcd.qrcode(apip,80,0,79,2);
    #endif
    /*
    #ifdef hasOLED
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 30, apip);
        display.display();
    #endif //hasOLED
    */
  }

}

#endif hasDISP

/***************************************/
/***************************************
 * format bytes
 * サイズを文字列で返す
*/

String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

/***************************************
 * getContentType
 * リンクされてるコンテンツのタイプを取得
*/

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  else if(filename.endsWith(".ttf")) return "application/x-font-ttf";
  return "text/plain";
}

/***************************************
 * handleFileRead
 * リクエストされたファイルをSPIから読込み処理
*/

bool handleFileRead(String path){
  //DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(qbFS.exists(pathWithGz) || qbFS.exists(path)){ //if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(qbFS.exists(pathWithGz)) //if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = qbFS.open(path, "r"); //SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  } else {
    if(path.indexOf("/ijcmd/")==0) {
      //mjSer.println("?\""+path+"\"");
      String ijc=path.substring(7);
      ijc.replace("%20"," ");
      ijc.replace("%3Cbr /%3E","\n");
      ijc.replace("%3Cbr%3E","\n");
      
    #ifdef useKbd
      if(kbdMode) {        
        if(ijc.startsWith("CLS")) {
          sendKeyCode(0x101);
        } else if(ijc.startsWith("LOAD")) {
          sendKeyCode(0x102);
        } else if(ijc.startsWith("SAVE")) {
          sendKeyCode(0x103);
        } else if(ijc.startsWith("LIST")) {
          sendKeyCode(0x104);
        } else if(ijc.startsWith("RUN")) {
          sendKeyCode(0x105);
        } else {
          mjSer.println(ijc);
        }
      } else {
        mjSer.println(ijc);
      }
    #else
      mjSer.println(ijc);
    #endif
    
    } else if(path.indexOf("/ijkey/")==0) {
      int keyval=path.substring(7).toInt();
      if(keyval<0) {
        keyval=63;//?
      } else if(keyval>255) {
        keyval-=256;
      }
      //mjSer.println(keyval);
      sendKeyCode(keyval);//mjSer.write(keyval);
      
    } else if(path.indexOf("/ijctr/")==0) {
      int keyval=path.substring(7).toInt();
      mjSer.write(keyval);
      delay(10);mjSer.write(keyval);
      
    } else if(path.indexOf("/mjcmd/")==0) {
      inStr=path.substring(7);
      inStr.replace("%20"," ");
      inStr.replace("%3Cbr /%3E","\n");
      inStr.replace("%3Cbr%3E","\n");
      //mjSer.println(inStr);
      doMixJuice();
      
    } else if(path.indexOf("/qgkp/")==0) {
      inStr=path.substring(6);
      inStr.replace("%20"," ");
      doMixJuice();
      
    } else if(path.indexOf("/qpkp/")==0) {
      inStr=path.substring(6);
      inStr.replace("%20"," ");
      doMixJuice();

    } else if(path.indexOf("/qsend/")==0) {
      inStr=path.substring(7);
      inStr.replace("%20"," ");
      doMixJuice(); 
    }
}
  return false;
}

/***************************************/
/***************************************
 * udpStart
 * USP開始
 */
 
#ifdef supportUDP
void udpStart(unsigned int lp) {
  if(isUDP) {
    if(lp==UDP_Read_Port) return;//?
    udp.stop();
    isUDP=false;
    UDP_Read_Port=UDP_LocalPort;
    delay(500);
  }
  udp.begin(lp);//UDP_LocalPort
  delay(500);
  mjSer.println("'UDP started...");
  isUDP=true;
  UDP_Read_Port=lp;
}
#endif

/***************************************
 *  MJ_UDP_Start
*/

void MJ_UDP_Start(String pt) {
  #ifdef supportUDP
  unsigned int lp=UDP_LocalPort;
  int sc=pt.length(); /* 文字数 */
  if(sc>0) lp=pt.toInt();
  udpStart(lp);
  #endif
}

/***************************************
 *  MJ_UDP_Packet
*/

void MJ_UDP_ReadPacket() {  
  #ifdef supportUDP
  unsigned int rlen=udp.parsePacket();
  if(rlen) {
    /*
    while(rlen<UDP_Minimum_Packet-1) {
      rlen = udp.parsePacket();
    }
    */
    udp.read(packetBuffer, (rlen > UDP_PACKET_SIZE) ? UDP_PACKET_SIZE : rlen);
    //mjSer.print("'");
    for (int i=0; i<rlen; i++){
      #ifdef useKbd
        if(kbdMode) {
          sendKeyCode(packetBuffer[i]);
        } else {
          mjSer.print(packetBuffer[i]);
        }
      #else
        mjSer.print(packetBuffer[i]);
      #endif
      delay(1);
    }
    //mjSer.println("");
  }
  #endif
}

/***************************************
 *  MJ_UDP_Write
*/

void MJ_UDP_WritePacket(String msg) {
  int sc=msg.indexOf(" ");
  if(sc<1) {MJ_UDP_Write(msg); return;}
  IPAddress sip;
  if(!sip.fromString(msg.substring(0,sc))) {MJ_UDP_Write(msg); return;}
  //mjSer.println(msg.substring(0,sc));
  
  msg=msg.substring(sc+1);
  sc=msg.indexOf(" ");
  if(sc<1) {MJ_UDP_Write(msg); return;}
  uint16_t spt=msg.substring(0,sc).toInt();
  //mjSer.println(msg.substring(0,sc));
 
  msg=msg.substring(sc+1);
  //mjSer.println(msg);
 
  UDP_Write_IPAddress=sip;
  UDP_Write_Port=spt;
  MJ_UDP_Write(msg);

}

void MJ_UDP_Write(String msg) {
  //mjSer.println(UDP_Write_IPAddress);
  //mjSer.println(UDP_Write_Port);
  //mjSer.println(msg);
  
  // send a reply, to the IP address and port that sent us the packet we received
  udp.beginPacket(UDP_Write_IPAddress,UDP_Write_Port);//udp.remoteIP(), udp.remotePort());
  //udp.write(msg.c_str());
  udp.print(msg.c_str());
  udp.endPacket();  
}

/*
void MJ_Test(String msg) {
  int sc=msg.indexOf(" ");
  if(sc<1) {mjSer.println("Error..Is not IP.");return;}
  IPAddress sip;
  if(!sip.fromString(msg.substring(0,sc))) {mjSer.println("Error..Is not IP.");}
  //mjSer.println(msg.substring(0,sc));
  mjSer.println("This is IP.");
}
*/
/***************************************/
