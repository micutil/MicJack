#include "MJBoard.h"
#include "MJSerial.h"

#if defined(ARDUINO_M5Stack_Core_ESP32)
  #include <M5Stack.h>
  extern fs::SDFS qbFS;
  #define YMAX 240 // Bottom of screen area
  #define XMAX 320

#elif defined(ARDUINO_M5StickC_ESP32)
  #include <M5StickC.h>
  extern fs::SPIFFSFS qbFS;
  #define YMAX 80 // Bottom of screen area
  #define XMAX 160
#endif

#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)

//======================================
// Connect status LED
//======================================

bool statusLED[4];

void InitStatusLED() {
  statusLED[0]=true;
  statusLED[1]=statusLED[2]=statusLED[3]=false;
  DrawStatusLED();
  statusLED[0]=false;
}
void ConnectStatusLED(bool b, String addr) {
  statusLED[1]=b;
  DrawStatusLED();
  #if defined(ARDUINO_M5StickC_ESP32)
  M5.Lcd.fillRect(30,2,180,16,TFT_BLACK);//16,13
  if(b) { M5.Lcd.setCursor(30,10); M5.Lcd.print(addr); }
  #endif
}
void PostStatusLED(bool b, String addr) {
  statusLED[2]=b;
  DrawStatusLED();
  #if defined(ARDUINO_M5StickC_ESP32)
  M5.Lcd.fillRect(30,29,180,16,TFT_BLACK);//16,40
  if(b) { M5.Lcd.setCursor(30,37); M5.Lcd.print(addr.substring(0,20)); }
  #endif
}
void GetStatusLED(bool b, String addr) {
  statusLED[3]=b;
  DrawStatusLED();
  #if defined(ARDUINO_M5StickC_ESP32)
  M5.Lcd.fillRect(30,57,180,16,TFT_BLACK);//16,68
  if(b) { M5.Lcd.setCursor(30,65); M5.Lcd.print(addr.substring(0,20)); }
  #endif
}

int scrdiff=0;
void DrawStatusLED() {
  
  //if(statusLED[0]) {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
    M5.Lcd.fillRect(0,scrdiff,7,7,TFT_BLACK);
    M5.Lcd.fillRect(XMAX-7,scrdiff,7,7,TFT_BLACK);
    M5.Lcd.fillRoundRect(0,scrdiff,XMAX,27,7,TFT_LIGHTGREY);
    M5.Lcd.fillRect(0,20+scrdiff,XMAX,7,TFT_LIGHTGREY);
    #elif defined(ARDUINO_M5StickC_ESP32)
    //M5.Lcd.fillRoundRect(0,0,XMAX,27,7,TFT_LIGHTGREY);
    //M5.Lcd.fillRect(0,20,XMAX,7,TFT_LIGHTGREY);
    #endif
  //}
  
  if(statusLED[1]) {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
    M5.Lcd.fillCircle(16,13+scrdiff,8,TFT_GREEN);
    #elif defined(ARDUINO_M5StickC_ESP32)
    M5.Lcd.fillCircle(13,13,8,TFT_GREEN);
    #endif
  } else {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
    M5.Lcd.fillCircle(16,13+scrdiff,8,TFT_DARKGREY);
    M5.Lcd.drawCircle(16,13,8+scrdiff,TFT_GREEN);
    #elif defined(ARDUINO_M5StickC_ESP32)
    M5.Lcd.fillCircle(13,13,8,TFT_DARKGREY);
    M5.Lcd.drawCircle(13,13,8,TFT_GREEN);
    #endif
  }

  if(statusLED[2]) {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
    M5.Lcd.fillCircle(44,13+scrdiff,8,TFT_YELLOW);
    #elif defined(ARDUINO_M5StickC_ESP32)
    M5.Lcd.fillCircle(13,40,8,TFT_YELLOW);
    #endif
  } else {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
    M5.Lcd.fillCircle(44,13+scrdiff,8,TFT_DARKGREY);
    M5.Lcd.drawCircle(44,13+scrdiff,8,TFT_YELLOW);  
    #elif defined(ARDUINO_M5StickC_ESP32)
    M5.Lcd.fillCircle(13,40,8,TFT_DARKGREY);
    M5.Lcd.drawCircle(13,40,8,TFT_YELLOW);  
    #endif
  }

  if(statusLED[3]) {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
    M5.Lcd.fillCircle(72,13+scrdiff,8,TFT_RED);
    #elif defined(ARDUINO_M5StickC_ESP32)
    M5.Lcd.fillCircle(13,68,8,TFT_RED);
    #endif
  } else {
    #if defined(ARDUINO_M5Stack_Core_ESP32)
    M5.Lcd.fillCircle(72,13+scrdiff,8,TFT_DARKGREY);
    M5.Lcd.drawCircle(72,13+scrdiff,8,TFT_RED);
    #elif defined(ARDUINO_M5StickC_ESP32)
    M5.Lcd.fillCircle(13,68,8,TFT_DARKGREY);
    M5.Lcd.drawCircle(13,68,8,TFT_RED);
    #endif
  }
  
}
#endif //defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5StickC_ESP32)


#if defined(ARDUINO_M5Stack_Core_ESP32)

TFT_eSPI tft = TFT_eSPI();  // Invoke library

// The scrolling area must be a integral multiple of TEXT_HEIGHT
int TEXT_HEIGHT=8;//#define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
#define TOP_FIXED_AREA 32// TEXT_HEIGHT*2 // Number of lines in top fixed area (lines counted from top of screen)
#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
int DspW=TEXT_HEIGHT*32;//#define XMAX 320

// The initial y coordinate of the top of the scrolling area
uint16_t yStart = 0;//TOP_FIXED_AREA;//0;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yArea = YMAX-TOP_FIXED_AREA-BOT_FIXED_AREA;
// The initial y coordinate of the top of the bottom text line

uint16_t yDraw = TOP_FIXED_AREA;//0;//YMAX - BOT_FIXED_AREA - TEXT_HEIGHT; //
//uint16_t yDraw = 0;

// Keep track of the drawing x coordinate
uint16_t xPos = 0;

// For the byte we read from the serial port
//byte data = 0;

// A few test variables used during debugging
/////boolean change_colour = 1;
/////boolean selected = 1;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the serial buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
////////int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

// ##############################################################################################
// Setup the vertical scrolling start address pointer
// ##############################################################################################
void scrollAddress(uint16_t vsp) {
  #if defined(ARDUINO_M5Stack_Core_ESP32)
  tft.writecommand(ILI9341_VSCRSADD); // Vertical scrolling pointer
  #elif defined(ARDUINO_M5StickC_ESP32)
  tft.writecommand(ST7735_VSCRDEF); // Vertical scrolling pointer
  #endif
  tft.writedata(vsp>>8);
  tft.writedata(vsp);
}

// ##############################################################################################
bool inisc=false;
// ##############################################################################################
// Call this function to scroll the display one text line
// ##############################################################################################
int scroll_line() {
  
  if(inisc==false) {
    if(yDraw+TEXT_HEIGHT>=YMAX) {
      inisc=true;
    } else {
      scrdiff=0;
      return yDraw+TEXT_HEIGHT;
    }
  }

  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line
  //tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, TFT_BLACK);
  tft.fillRect(0,yStart,320,TEXT_HEIGHT, TFT_BLACK);

  // Change the top of the scroll area
  yStart+=TEXT_HEIGHT;
  // The value must wrap around as the screen memory is a circular buffer
  if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - YMAX + BOT_FIXED_AREA);
  //if (yStart >= YMAX) yStart = TOP_FIXED_AREA;//0;
  // Now we can scroll the display
  scrollAddress(yStart);
  scrdiff=yStart;
  return  yTemp;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in portrait orientation
void setupScrollArea(uint16_t tfa, uint16_t bfa) {
  #if defined(ARDUINO_M5Stack_Core_ESP32)
  tft.writecommand(ILI9341_VSCRSADD); // Vertical scrolling pointer
  #elif defined(ARDUINO_M5StickC_ESP32)
  tft.writecommand(ST7735_VSCRDEF); // Vertical scrolling pointer
  #endif
  tft.writedata(tfa >> 8);           // Top Fixed Area line count
  tft.writedata(tfa);
  tft.writedata((YMAX-tfa-bfa)>>8);  // Vertical Scrolling Area line count
  tft.writedata(YMAX-tfa-bfa);
  tft.writedata(bfa >> 8);           // Bottom Fixed Area line count
  tft.writedata(bfa);
}

// ##############################################################################################

void tft_terminal_setup(bool chg) {

  if(chg) {
    if(TEXT_HEIGHT==8) {
      TEXT_HEIGHT=16;
    } else {
      TEXT_HEIGHT=8;
    }
  }
  
  //M5.Lcd.fillScreen(TFT_BLACK);
  
  /*
  uint8_t cardType = SD.cardType();
  
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    //return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  */
  
  // Initialise the TFT after the SD card!
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  //tft.setTextWrap(false,false);

  // Change colour for scrolling zone text
  //M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  // Setup scroll area
  //setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  setupScrollArea(0, 0);

  // Zero the array
  /////for (byte i = 0; i<18; i++) blank[i]=0;

  String fileName = "";//"IchigoJam-for-Display-1216";//+String(TEXT_HEIGHT);
  switch(TEXT_HEIGHT) {
    case 8:   fileName = "IchigoJam-for-Display-1208"; break;
    case 16:  
    default:  fileName = "IchigoJam-for-Display-1216"; break;
  }
  
  // Load the font
  tft.loadFont(fileName, qbFS); // Use font stored on SD
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 0);
  
  InitStatusLED();
  //setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
}

void tft_terminal_print(const char *s, int n) {
  //  These lines change the text colour when the serial buffer is emptied
  //  These are test lines to see if we may be losing characters
  //  Also uncomment the change_colour line below to try them
  //
  //  if (change_colour){
  //  change_colour = 0;
  //  if (selected == 1) {M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK); selected = 0;}
  //  else {M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK); selected = 1;}
  //}

  char data;
  for(int i=0;i<n;i++) {
    data = s[i];
    if (data == '\r' || data == '\n') {// || xPos>31*TEXT_HEIGHT) {//xPos>320-TEXT_HEIGHT) {
      xPos = 0;
      yDraw = scroll_line();
      tft.setCursor(xPos,yDraw);
      DrawStatusLED();
    } else if(xPos>XMAX-TEXT_HEIGHT && xPos<=31*TEXT_HEIGHT) {
      xPos+=TEXT_HEIGHT;
    } else { //if(data > 31 && data < 128) {
      if(xPos>31*TEXT_HEIGHT) {
        xPos = 0; yDraw = scroll_line();
        tft.setCursor(xPos,yDraw);
        DrawStatusLED();
      }
      //xPos += tft.drawChar(data,xPos,yDraw,2);//M5.Lcd.drawChar(data,xPos,yDraw,2);
      tft.drawGlyph(data);//tft.print((char)data);
      xPos+=TEXT_HEIGHT; tft.setCursor(xPos,yDraw);
      // blank[(18+(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT)%19]=xPos; // Keep a record of line lengths
    }
    //change_colour = 1; // Line to indicate buffer is being emptied
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

#endif //ARDUINO_M5Stack_Core_ESP32
