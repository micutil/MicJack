#ifndef __TELLO_H__
#define __TELLO_H__

#include <Arduino.h>
#include "MJSerial.h"

//-------------
// Fundermental

enum {
  kCmd=0,
  kState,
  kVideo  
};

bool tello_udp_start(int n, bool doCmd=false);
void tello_udp_stop(int n);

void Break_Tello();
void Wait_Tello_Res(int n);
void Tello_UDP_Read(int n);

//-------
// Queue

typedef struct telloq {
  uint32_t w;
  String c;
  telloq *n;
  telloq *p;
} ;

void tello_setup();
void tello_loop();

void initQueue();
void clearQueue();
telloq *addQueue(String c, uint32_t w);
telloq *appendQueue(String c);
void runQueue();
void actionQueue();
void actionEmergency();

void Tello_Queue_Command(String msg);

//---------
// Command

void Tello_UDP_Write(String msg);
void Tello_Direct_Command(String msg);

extern MJSerial mjSer;

#endif //__TELLO_H__
