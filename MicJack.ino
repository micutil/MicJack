/*
 *  MicJack

 *  MixJuice compatible IoT interface module for IchigoJam with ESP-WROOM-02
 *  
 *  CC BY Michio Ono. http://ijutilities.micutil.com
 *  
 *  *Version Information
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
 
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <FS.h>
#include <ESP8266mDNS.h>

const String MicJackVer="MicJack-1.0.1b1";
const String MJVer="MixJuice-1.2.2";
const int sleepTimeSec = 60;

String inStr = ""; // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

IPAddress staIP; // the IP address of your shield

struct APCONFIG {
  char ssid[32];
  char pass[32];
  char homepage[32];
  char softap_ssid[32];
  char softap_pass[32];
  bool kbd;
  bool useHostKbdCmd;
};
APCONFIG apcbuf;

String aplist[10];
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
ESP8266WebServer server(80);
boolean isServer=false;
String rootPage;
const char* mjname = "micjack";

/*** UDP ***/
//#define supportUDP
#ifdef supportUDP
  #define initStartUDP
  #include <WiFiUdp.h>
  WiFiUDP udp;
  unsigned int UDP_LocalPort = 10000;
  //unsigned char UDP_Minimum_Packet = 1;
  boolean isUDP=false;
  const int UDP_PACKET_SIZE = 256;
  char packetBuffer[UDP_PACKET_SIZE];
#endif

/*** SoftAP ***/
#define initStartSoftAP  
const char default_softap_ssid[] = "MicJack";
const char default_softap_pass[] = "abcd1234";
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
#define KB_CLK      13 // 0   // A4  // PS/2 CLK  IchigoJamのKBD1に接続 //21//
#define KB_DATA     16 // 15  // A5  // PS/2 DATA IchigoJamのKBD2に接続 //22//
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
  //Serial.println(command);
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
    
    uint8_t t=ijKeyMap[0][key];//Serial.println(t);
    uint8_t c=ijKeyMap[1][key];//Serial.println(c);
    
    if(t==0||t==6) {
      Serial.write(key);
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

  Serial.write(key);
  return false;
}

/************************************
 * Initial Set up
 * 
 ************************************/
void setup() {

  //Serial.begin(74880);
  Serial.begin(115200);
  delay(5000);
  Serial.println("");
  Serial.println("NEW");delay(100);
  Serial.println("CLS");delay(100);
  Serial.println("");
  Serial.println("");
  
  #ifdef useMJLED
    pinMode(connLED, OUTPUT);//1
    pinMode(postLED, OUTPUT);//2
    pinMode(getLED,  OUTPUT);//3
  #endif

  LoadAPConfig();
  
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);
  Serial.println("");
  Serial.println("'"+MJVer);
  Serial.println("'"+MicJackVer);
  Serial.println("'CC BY Michio Ono");
  Serial.println("");

  SPIFFS.begin();
  {
    Serial.printf("'SPIFFS files...\n");
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("'%s:%s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      delay(500);
    }    
    Serial.printf("\n");
  }

  /* APモード */
  softApStart();

  /* HTTP server */
  serverStart();
  
  /* 自動接続 */
  WiFi.disconnect();//一回切断した後に接続
  MJ_APLAPC();

  #ifdef initStartUDP
    udpStart();
  #endif

  #ifdef supportOTA
    OTAStart();
  #endif

#ifdef useKbd
/*  Serial.println("'Low or High");
  digitalWrite(KB_CLK,LOW);digitalWrite(KB_DATA,LOW);
  for(int i=0;i<3;i++) {
    if(digitalRead(KB_CLK)==HIGH) Serial.println("'1HIGH"); else Serial.println("'1LOW"); 
    if(digitalRead(KB_DATA)==HIGH) Serial.println("'2HIGH"); else Serial.println("'2LOW"); 
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

  Serial.println("");
  Serial.println("'Ready to go!");

}

/************************************
 * softApStart
 * Soft AP 開始
 * 
 */

void softApStart() {
  WiFi.softAP(apcbuf.softap_ssid,apcbuf.softap_pass);
  delay(500);
  mySoftAPIP = WiFi.softAPIP();
  String sn="";sn=String(apcbuf.softap_ssid);
  Serial.println("'Soft AP: "+sn);
  delay(500);
  Serial.print("'IP: ");
  Serial.println(mySoftAPIP); Serial.println("");
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
  Serial.println("'HTTP Server started...");
  Serial.println("");
  
  isServer=true;  
  
}

/************************************
 * udpStart
 * USP開始
 */
#ifdef supportUDP
void udpStart() {
  udp.begin(UDP_LocalPort);
  delay(500);
  Serial.println("'UDP started...");
  isUDP=true;
}
#endif

/************************************
 * OTAStart
 * OTA対応
 */
#ifdef supportOTA
void OTAStart() {
  /*
  ArduinoOTA.onStart([]() {
    Serial.println("'Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\n'End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("'Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("'Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("'Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("'Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("'Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("'Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("'End Failed");
  });
  */
  ArduinoOTA.begin();
  Serial.println("'OTA supported...");
}

#endif

/************************************
 * Event loop
 * 
 ************************************/

void loop() {
  
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

  while (Serial.available()) {

    if(postmode&&posttype==HTML_POST_QUEST) {
      //Quest用ポストデータ
      uint8_t q=(uint8_t)Serial.read();
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
       char inChar = (char)Serial.read();
      //Check end of line 
      if(inChar=='\n' || inChar=='\r') {
        stringComplete = true;
        if(!postmode) Serial.flush(); //読み飛ばし "OK"など
        break;
      } else {
        inStr += inChar;
      }
    }
    
  }

  if(stringComplete) {  //データあり
    doMixJuice();
  }
  
  // Check if a client has connected
  // if(isServer)
  server.handleClient();

  #ifdef supportUDP
    if(isUDP) MJ_UDP_Packet();
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
      //Serial.println(posttype);
      //Serial.println(postaddr);
      //Serial.println(postdata);
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
          //Serial.println(postdata);
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
    
  } else if(cs.startsWith("MJ APL")) {
    /*** WiFiリスト ***/ MJ_APL();
      
  } else if(cs.startsWith("MJ APS")) {
   /*** 接続確認 ***/ MJ_APS();
    
  } else if(cs.startsWith("MJ APD")) {
    /*** 切断 ***/ MJ_APD();
    
  } else if(cs.startsWith("MJ APC")) {
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
    Serial.println("LIST\n");
    
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
    /*** バージョン ***/ Serial.println("'"+MicJackVer);
    // Verで、エラー表示で、MixJuiceのバージョンを表示
    // MJVerで、MicJackのバージョンを表示
    
  } else if(cs.startsWith("MJ LIP")) {
    /*** Local IP ***/ Serial.println(staIP);
    
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
    Serial.println(analogRead(A0));

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
  
//   } else if(cs.startsWith("MJ SERVER") || cs.startsWith("MJ SVR")) {
//     /*** Sever Start ***/ serverStart();

  #ifdef supporrtUDP
  } else if(cs.startsWith("MJ UDP")) {
    /*** UDP Start ***/ MJ_UDP_Start(inStr.substring(7));
  #endif
  
  } else if(cs.startsWith("MJ ")) {
    /*** NG ***/ Serial.println("'NG: "+MJVer);
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
      return;
    }
  } else {
    tss=ssidpwd.substring(0,ps);
    tps=ssidpwd.substring(ps+1);
  }

  int apn=tss.toInt();
  if(tss.length()==1 && (apn>=0 && apn<10)) tss=aplist[apn];
  
  //Serial.println("'"+tss);
  //Serial.println("'"+tps);
  
  WiFi.begin(tss.c_str(),tps.c_str());

  //Setup時　コネクトしたように見えてしてない場合があるための処理
  int64_t timeout = millis() + 10000;
  Serial.print("'Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (timeout - millis() < 0) {
      Serial.println("");
      Serial.println("'Couldn't get a wifi connection");
      return;
    }
  }
  Serial.println("");

  if (WiFi.status() != WL_CONNECTED) { 
    Serial.println("'Couldn't get a wifi connection");
    //while(true);
  } else {
    //Station mode SSID
    Serial.println("'WiFi Station: "+tss);
    
   // if you are connected, print out info about the connection:
    //print the local IP address
    staIP = WiFi.localIP();
    Serial.print("'IP: "); Serial.println(staIP);
    
    //最後に接続した情報
    lastSSID=tss;
    lastPASS=tps;
    SaveAPConfig();
    
    #ifdef useMJLED
      digitalWrite(connLED, HIGH);
    #endif

    // Add service to MDNS-SD
    MDNS.begin(mjname);
    MDNS.addService("http", "tcp", 80);
    delay(500); 
    Serial.print("'http://");
    Serial.print(mjname);Serial.println(".local/");
    delay(500); Serial.println("");
    
  }
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
  EEPROM.begin(320);//使用するサイズを宣言
  EEPROM.get<APCONFIG>(0, apcbuf);
  CheckEEPROMdata(apcbuf.ssid); lastSSID=String(apcbuf.ssid);
  CheckEEPROMdata(apcbuf.pass); lastPASS=String(apcbuf.pass);
  CheckEEPROMdata(apcbuf.homepage); homepage=String(apcbuf.homepage);
  if(CheckEEPROMdata(apcbuf.softap_ssid)==false||CheckEEPROMdata(apcbuf.softap_pass)==false) {
    strcpy(apcbuf.softap_ssid,default_softap_ssid);
    strcpy(apcbuf.softap_pass,default_softap_pass);
  }
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
    Serial.println("'Station: "+lastSSID);
    //Serial.println("'IP: "+lastSSID);
  } else {
    lastSSID=s; SaveAPConfig();
  }
}

void SetPASS(String s) {
  int n=s.length();
  if(n<=0) {
    Serial.println("'"+lastPASS);
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
    ;
  } else {
    tss=ssidpwd.substring(0,ps);
    tps=ssidpwd.substring(ps+1);
  }
  //SetSoftAP(tss,tps);
  int n=tss.length();
  int m=tps.length();
  if(n>0&&m<8) {
    Serial.println("'Pass needs more than 8chrs..");
    delay(1500);
  } else if(n<=0||m<=0) {
    String softap_ssid="";
    softap_ssid=String(apcbuf.softap_ssid);
    Serial.println("'AP: "+softap_ssid);delay(500);
    Serial.print("'IP: "); Serial.println(mySoftAPIP);
  } else {
    tss.toCharArray(apcbuf.softap_ssid,n+1);
    tps.toCharArray(apcbuf.softap_pass,m+1);
    SaveAPConfig();
    Serial.println("'OK. ESP restart...");
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
  Serial.println("'Scan Start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    Serial.println("'No networks found");
  } else {
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      String s="'"+String(i)+": ";
      s=s+WiFi.SSID(i);//+" ("+WiFi.RSSI(i)+")";
      //Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      Serial.println(s);
      delay(100);
      if(i<10) aplist[i]=WiFi.SSID(i);
    }
    Serial.println("");
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
    Serial.println("1");
  else
    Serial.println("0");
}

/***************************************
 *  MJ APD
 *  WiFiアクセスポイントから切断
 *  
***************************************/

void MJ_APD() {
  WiFi.disconnect();
  Serial.println("'OK");
  #ifdef useMJLED
    digitalWrite(connLED, LOW);
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
  RFMode wm=WAKE_RF_DEFAULT;
  
  if(ps<0) {
    ts=(uint32_t)tss.toInt();
  } else {
    ts=(uint32_t)tss.substring(0,ps).toInt(); //Serial.println(tss.substring(0,ps));
    wm=(RFMode)tss.substring(ps+1).toInt(); //Serial.println(tss.substring(ps+1));
  }
  
  ESP.deepSleep(ts,wm);
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
  //Serial.println("'"+host);
  //Serial.println("'"+url);

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
        digitalWrite(getLED, HIGH);
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
        digitalWrite(postLED, HIGH);
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
      Serial.println("'Client Timeout !");
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
          Serial.print(line);
          Serial.print('\n');
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
                    Serial.write(v);//Serial.write((uint8_t)strtol(ab,NULL,16));
                    //String h="0"+String(HexTextToVal(a,b),HEX);
                    //Serial.print(h.substring(h.length()-2));
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
                case '\n': Serial.print('\n'); delay(spn);
                  break;
                default: Serial.print(c); delay(spw);
                  break;
              }
              break;
          }
        }
        /* もし512バイトよりすくなかったら */
        if(type==HTML_GET_QUEST) {
          while(rcvCnt<512) {
            if((rcvCnt%8)==0)
              Serial.write((uint8_t)0xFF);
            else
              Serial.write((uint8_t)0);
            rcvCnt+=1;
            delay(spw*2);
          }
        }
      }

      if(!addr.equals(homepage)) lastGET=addr;
      #ifdef useMJLED
        digitalWrite(getLED, LOW);
      #endif
      break;

    case HTML_POST:
    case HTML_POSTS:
    case HTML_POST_QUEST:
      Serial.println("'POST OK!");
      #ifdef useMJLED
        digitalWrite(postLED, LOW);
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
  //Serial.println("'"+host);
  //Serial.println("'"+url);

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
        digitalWrite(getLED, HIGH);
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
        digitalWrite(postLED, HIGH);
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
      Serial.println("'Client Timeout !");
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
          Serial.print(line);
          Serial.print('\n');
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
            Serial.print('\n');
            delay(spn);
            break;
          default:
            Serial.print(c);
            delay(spw);
            break;
          }
        }
      }
      if(!addr.equals(homepage)) lastGET=addr;
      #ifdef useMJLED
        digitalWrite(getLED, LOW);
      #endif
      break;

    case HTML_POST:
    case HTML_POSTS:
      Serial.println("'POST OK!");
      #ifdef useMJLED
        digitalWrite(postLED, LOW);
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
      digitalWrite(postLED, HIGH);
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
          uint8_t v=HexTextToVal(a,b);//Serial.print(v,HEX);
          Serial.write(v);//Serial.write((uint8_t)strtol(ab,NULL,16));//Serial.print(v,HEX);//
          //String h="0"+String(HexTextToVal(a,b),HEX);
          //Serial.print(h.substring(h.length()-2));
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
      Serial.write((uint8_t)0xFF);
    else
      Serial.write((uint8_t)0);
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
    sprintf(buf, "%2X", ar[i]);
    s += buf;
    if (i < 5) s += ':';
  }
  return s;
}

void MJ_MAC() {
  Serial.print("'");
  Serial.print("MAC Address: ");
  
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.println(mac2String(mac)); 
}

void MJ_MACADDR() {
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("'");
  Serial.println(mac2String(mac));
}

/***************************************
 *  EachPrintSerial
 *  SerialPrintを少しずつ
 *  
***************************************/
void SerialPrint(String s, int m=1) {
  int cc=s.length();
  if(cc<=m)
    Serial.println(s);
  else {
    for(int j=0; j<cc; j=j+m) {
      if(cc-j<m) {
        Serial.print(s.substring(j));
      } else {
        Serial.print(s.substring(j,j+m));
      }
      delay(spw);
    }
    Serial.println("");
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
  Serial.println("'OK");
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
    Serial.println("'Use host kbd cmd");
  else
    Serial.println("'Unuse host kbd cmd");
}

/***************************************
 *  MJ_IJKBD
 *  IchigoJam Keyboard認識のためのリセット
 *  
***************************************/

void MJ_IJKBD() {
  MJ_HOSTKBDCMD(true);
  MJ_KBD(true);
  Serial.println("RESET");
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
    Serial.println("'Keyboard Mode");
  else
    Serial.println("'UART Mode");
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
    Serial.println(v);//"'"+String(v));
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
    analogWrite(p,io);
  }
}

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
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  } else {
    if(path.indexOf("/ijcmd/")==0) {
      //Serial.println("?\""+path+"\"");
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
          Serial.println(ijc);
        }
      } else {
        Serial.println(ijc);
      }
    #else
      Serial.println(ijc);
    #endif
    
    } else if(path.indexOf("/ijkey/")==0) {
      int keyval=path.substring(7).toInt();
      if(keyval<0) {
        keyval=63;//?
      } else if(keyval>255) {
        keyval-=256;
      }
      //Serial.println(keyval);
      sendKeyCode(keyval);//Serial.write(keyval);
      
    } else if(path.indexOf("/ijctr/")==0) {
      int keyval=path.substring(7).toInt();
      Serial.write(keyval);
      delay(10);Serial.write(keyval);
      
    } else if(path.indexOf("/mjcmd/")==0) {
      inStr=path.substring(7);
      inStr.replace("%20"," ");
      inStr.replace("%3Cbr /%3E","\n");
      inStr.replace("%3Cbr%3E","\n");
      //Serial.println(inStr);
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
 *  MJ_UDP_Start
*/

void MJ_UDP_Start(String pt) {
  #ifdef supporrtUDP
  int sc=pt.length(); /* 文字数 */
  if(sc>0) UDP_LocalPort=pt.toInt();
  udpStart();
  #endif
}

/***************************************
 *  MJ_UDP_Packet
*/

void MJ_UDP_Packet() {
  #ifdef supportUDP
  unsigned int rlen;
  if((rlen = udp.parsePacket())) {
    /*
    while(rlen<UDP_Minimum_Packet-1) {
      rlen = udp.parsePacket();
    }
    */
    udp.read(packetBuffer, (rlen > UDP_PACKET_SIZE) ? UDP_PACKET_SIZE : rlen);
    for (int i = 0; i < rlen; i++){
      Serial.print(packetBuffer[i]);
      delay(1);
    }
  }
  #endif
}

/***************************************/
