/*
 * Quest program comverter
 * English
 */

#ifndef questUtil_h
#define questUtil_h

#include "Arduino.h"

struct questCmd {
  uint8_t id;
  uint8_t argtype1;
  uint8_t argtype2;
  uint8_t argtype3;
  int16_t arg1;
  int16_t arg2;
  int16_t arg3;
};

String getBoxNum(uint8_t t);
String argStr(uint8_t t, uint8_t a);
String questBinToProg(uint8_t *qd);

#endif /* questUtil_h */
