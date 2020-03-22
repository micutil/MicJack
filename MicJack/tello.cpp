#include "tello.h"
#include <WiFiUdp.h>

WiFiUDP tudp[3];//tello command
const IPAddress Tello_UDP_IPAddress{ 192,168,10,1 };
const unsigned int Tello_UDP_RemotePort = 8889;
const unsigned int Tello_UDP_LocalPort[] = { 8889, 8890, 1111 };
//unsigned int Tello_UDP_ReadPort;
const int Tello_UDP_PACKET_SIZE = UDP_TX_PACKET_MAX_SIZE;//256;
char tello_packetBuffer[Tello_UDP_PACKET_SIZE+1];
bool isTello[]={ false, false, false };
bool waitRes[]={ false, false, false };
bool showForce=false;
bool showRes=false;
bool telloSteam=false;
bool telloBreak=false;
const uint32 seqCmdDly=500;

//-------------------------------------------------
//-------------------------------------------------
// TELLO UDP start & stop
//-------------------------------------------------

bool tello_udp_start(int n, bool doCmd) {
  if(n>0&&isTello[kCmd]==false) {
    if(tello_udp_start(kCmd,true)==false) return false;
  }
  
  if(isTello[n]) {
    //if(lp==tudp.localPort()) return;
    tudp[n].stop();
    isTello[n]=false;
    //delay(seqCmdDly);
  }
  
  isTello[n]=(tudp[n].begin(Tello_UDP_LocalPort[n])==1);
  if(isTello[n]==false) {
    if(showForce) Serial.println("'Error UDP connection...");
    return false;
  }
  //delay(seqCmdDly);

  if(showForce) {
    String msg="'Tello ";
    switch(n) {
      case kCmd: msg=msg+"command"; break;
      case kState: msg="state"; break;
      case kVideo: msg="video"; break;
    }
    Serial.println(msg+" UDP started...");
  }
  
  //isTello[n]=true;
  //Tello_UDP_ReadPort=lp;
  //Breakをクリアー
  telloBreak=false;

  //Command
  if(doCmd) {
    Tello_UDP_Write("command");
    //Wait_Tello_Res(n);//delay(seqCmdDly);
  }

  if(n==2) {
    Tello_UDP_Write("streamon");
    //Wait_Tello_Res(n);//delay(seqCmdDly);
  }
  
  return true;
}

void tello_udp_stop(int n) {
  if(isTello[n]) tudp[n].stop();
  isTello[n]=false;
  waitRes[n]=false;
}

//-------------------------------------------------
//-------------------------------------------------
// TELLO read packet
//-------------------------------------------------

void Break_Tello() {
  telloBreak=true;
  //if(nowQrun) actionEmergency();
}

void Wait_Tello_Res(int n) {
  uint32 p=millis()+20000;
  while(waitRes[n]&&p>millis()&&!telloBreak) {
    Tello_UDP_Read(n);
    delay(5);
  }
  waitRes[n]=false;
}

void Tello_UDP_Read(int n) {
  unsigned int rlen;
  if((rlen = tudp[n].parsePacket())) {
    waitRes[n]=false;
    tudp[n].read(tello_packetBuffer, (rlen > Tello_UDP_PACKET_SIZE) ? Tello_UDP_PACKET_SIZE : rlen);
    if(showForce||showRes) {
      Serial.print("'");
      for (int i = 0; i < rlen; i++){
        Serial.print(tello_packetBuffer[i]);
      }
      Serial.println("");
      showRes=false;
    }
  }
  delay(1);
}

void Tello_State_Read() {
  unsigned int rlen;
  if((rlen = tudp[kState].parsePacket())) {
    waitRes[kState]=false;
    tudp[kState].read(tello_packetBuffer, (rlen > Tello_UDP_PACKET_SIZE) ? Tello_UDP_PACKET_SIZE : rlen);
    Serial.print("'");
    for (int i = 0; i < rlen; i++){
      Serial.print(tello_packetBuffer[i]);
      //delay(1);
    }
    Serial.println("");
  }
  delay(1);
}

//-------------------------------------------------
//-------------------------------------------------
// TELLO Queue & Loop
//-------------------------------------------------

telloq *tq,*tl;
bool isNewQueue=true;
bool nowQrun=false;
uint32 queueWait=0;
uint32 forceQuit=0;

const uint32 commandwait=500;
const uint32 takeoffwait=5000;
const uint32 minimumwait=50;
const uint32 maximumwait=14500;
uint32 prevRec=0;

void tello_setup() {
  initQueue();
}

void tello_loop() {

  if(isTello[kCmd]) {

    //UDP Read
    //Tello_UDP_Read(kCmd);

    //Tello Queue
    if(nowQrun) {
      if(tq!=NULL) {
        if(queueWait<millis()) {
          actionQueue();
        } else if(forceQuit<millis()) {
          actionEmergency();
        }
        //delay(1); 
      } else {
        nowQrun=false;
        if(isNewQueue==false) initQueue();
      }
      
      delay(1);
    }
    
  }

  //Read Tello state
  if(isTello[kState]) {
    Tello_State_Read();
    tello_udp_stop(kState);
  }

  /*// Videostream
  if(isTello[kVideo]) {
    Tello_Video_Read();
    //tello_udp_stop(kVideo]);
  }
  */
}

void initQueue() {
  clearQueue();
  tq=new telloq;
  tl=tq;
  isNewQueue=true;
}

void clearQueue() {
  while(tq!=NULL) {
    telloq *n=tq->n;
    delete(tq);
    tq=n;
  }
}

telloq *addQueue(String c, uint32 w) { //appendQueue & wait time
  tl->w=w;
  tl->c=c;
  tl->n=new telloq;
  tl->n->p=tl;
  tl=tl->n;
  tl->w=0;
  prevRec=millis();
  return tl->p;
}

telloq *appendQueue(String c) {
  uint32 t=millis();//3000;
  if(isNewQueue) {
    isNewQueue=false;
    addQueue("command",commandwait);
    addQueue("takeoff",takeoffwait);
  }
  if(tl->p->w==0) tl->p->w=t-prevRec;
  if(tl->p->w<minimumwait) tl->p->w=minimumwait;
  if(tl->p->w>=maximumwait) tl->p->w=maximumwait;
  //Serial.println(tl->p->w);
  tl->c=c;
  tl->n=new telloq;
  tl->n->p=tl;
  tl=tl->n;
  tl->w=0;//millis();
  prevRec=t;
  return tl->p;
}

void runQueue() {
  if(isTello[kCmd]==false) tello_udp_start(false);
  uint32 t=millis();//3000;
  telloq *tt=appendQueue("land");
  tt->w=500;
  delete(tt->n);
  tt->n=NULL;
  nowQrun=true;
}

void actionQueue() {

  telloq *n=tq->n;
  //Serial.print("'");Serial.println(tq->c);
  //Serial.print("'wait:");Serial.print(tq->w);Serial.print(" time:");Serial.print(millis());Serial.println("");
  
  nowQrun=false;//Qループ停止
  
  queueWait=tq->w+millis();       //先に街時間を設定。
  Tello_UDP_Write(tq->c);         //コマンド送信して、完了まで待つ
  forceQuit=15000+tq->w+millis(); //後に強制終了の時間をセット
  //forceQuit=queueWait+15000;
  
  nowQrun=true;//Qループ再開
  
  delete(tq);
  tq=n;
}

void actionEmergency() {
  nowQrun=false;
  initQueue();
  //land
  Tello_UDP_Write("land"); delay(100);
  //emergency
  Tello_UDP_Write("emergency");
}

//----------------------
// TELLO Queue command 
//----------------------

void Tello_Queue_Replace(String m, String a, String b) {
  m.replace(a,b);
  appendQueue(m);
}

void Tello_Queue_Command(String msg) {

  //String cs=msg;//文字をコピー
  msg.toUpperCase();//大文字に

  if(msg.startsWith("RUN")) {
    /*** QRUN ***/ runQueue();
    
  } else if(msg.startsWith("CLR")) {
    /*** QCLR ***/ actionEmergency();
    
  } else if(msg.startsWith("FL L")||msg.startsWith("FLL")) {
    /*** Flip left ***/     appendQueue("flip l");
  } else if(msg.startsWith("FL R")||msg.startsWith("FLR")) {
    /*** Flip right ***/    appendQueue("flip r");
  } else if(msg.startsWith("FL F")||msg.startsWith("FLF")) {
    /*** Flip forward ***/  appendQueue("flip f");
  } else if(msg.startsWith("FL B")||msg.startsWith("FLB")) {
    /*** Flip back ***/     appendQueue("flip b");

  } else if(msg.startsWith("L ")) {
    /*** left ***/          Tello_Queue_Replace(msg,"L ","left ");
  } else if(msg.startsWith("R ")) {
    /*** right ***/         Tello_Queue_Replace(msg,"R ","right ");
  } else if(msg.startsWith("F ")) {
    /*** forward ***/        Tello_Queue_Replace(msg,"F ","forward ");
  } else if(msg.startsWith("B ")) {
    /*** back ***/           Tello_Queue_Replace(msg,"B ","back ");

  } else if(msg.startsWith("U ")) {
    /*** up ***/             Tello_Queue_Replace(msg,"U ","up ");
  } else if(msg.startsWith("D ")) {
    /*** down ***/           Tello_Queue_Replace(msg,"D ","down ");
  } else if(msg.startsWith("TR ")) {
    /*** cw ***/             Tello_Queue_Replace(msg,"TR ","cw ");
  } else if(msg.startsWith("TL ")) {
    /*** ccw ***/            Tello_Queue_Replace(msg,"TL ","ccw ");

  } else if(msg.startsWith("GM ")) { //go
    /*** edu go  ***/        Tello_Queue_Replace(msg,"GM ","go ");
  } else if(msg.startsWith("CM ")) { //curve
    /*** edu curve  ***/     Tello_Queue_Replace(msg,"CM ","curve ");
  } else if(msg.startsWith("G ")) { //go
    /*** go  ***/            Tello_Queue_Replace(msg,"G ","go ");
  } else if(msg.startsWith("C ")) { //curveC
    /*** curve  ***/         Tello_Queue_Replace(msg,"C ","curve ");

  //-------

  } else if(msg.startsWith("L")) {
    /*** left ***/          Tello_Queue_Replace(msg,"L","left ");
  } else if(msg.startsWith("R")) {
    /*** right ***/         Tello_Queue_Replace(msg,"R","right ");
  } else if(msg.startsWith("F")) {
    /*** forward ***/        Tello_Queue_Replace(msg,"F","forward ");
  } else if(msg.startsWith("B")) {
    /*** back ***/           Tello_Queue_Replace(msg,"B","back ");

  } else if(msg.startsWith("U")) {
    /*** up ***/             Tello_Queue_Replace(msg,"U","up ");
  } else if(msg.startsWith("D")) {
    /*** down ***/           Tello_Queue_Replace(msg,"D","down ");
  } else if(msg.startsWith("TR")) {
    /*** cw ***/             Tello_Queue_Replace(msg,"TR","cw ");
  } else if(msg.startsWith("TL")) {
    /*** ccw ***/            Tello_Queue_Replace(msg,"TL","ccw ");

  } else if(msg.startsWith("GM")) { //go
    /*** edu go  ***/        Tello_Queue_Replace(msg,"GM","go ");
  } else if(msg.startsWith("CM")) { //curve
    /*** edu curve  ***/     Tello_Queue_Replace(msg,"GM","curve ");
  } else if(msg.startsWith("G")) { //go
    /*** go  ***/            Tello_Queue_Replace(msg,"G","go ");
  } else if(msg.startsWith("C")) { //curve
    /*** curve  ***/         Tello_Queue_Replace(msg,"C","curve ");
  }

}

//-----------------------------------------------
//-----------------------------------------------
// TELLO write command 
//-----------------------------------------------

void Tello_UDP_Write(String msg) {
  
  //Initialize
  if(isTello[kCmd]==false) tello_udp_start(true);
  
  //send a reply, to the IP address and port that sent us the packet we received
  //if earlier then error not joystick
  if(waitRes[kCmd]) Wait_Tello_Res(kCmd);
  
  if(telloBreak) {
    actionEmergency();
    telloBreak=false;
    return;
  }
  
  waitRes[kCmd]=true;
  tudp[kCmd].beginPacket(Tello_UDP_IPAddress,Tello_UDP_RemotePort);//udp.remoteIP(), udp.remotePort());
  tudp[kCmd].write(msg.c_str());
  tudp[kCmd].endPacket();
  if(showForce) { Serial.print("'"); Serial.println(tq->c); }
  //Serial.print("'");Serial.println(msg);
  
  Wait_Tello_Res(kCmd);
}

enum {
  kRight=0,
  kLeft,
  kForward,
  kBack,
  kUp,
  kDown,
  kTR,
  kTL
};

void Tello_RC_Write(int dir, String m, int p=1) {
  String v="";
  m=m.substring(p);
  if(m.length()==0) {
    v="100";
  } else {
    int n=m.toInt();
    v=String(n*20);
  }
  switch(dir) {
    case kRight:    Tello_UDP_Write(String("rc "+v+" 0 0 0"));  break;
    case kLeft:     Tello_UDP_Write(String("rc -"+v+" 0 0 0"));  break;
    case kForward:  Tello_UDP_Write(String("rc 0 "+v+" 0 0"));  break;
    case kBack:     Tello_UDP_Write(String("rc 0 -"+v+" 0 0"));  break;
    case kUp:       Tello_UDP_Write(String("rc 0 0 "+v+" 0"));  break;
    case kDown:     Tello_UDP_Write(String("rc 0 0 -"+v+" 0"));  break;
    case kTR:       Tello_UDP_Write(String("rc 0 0 0 "+v));  break;
    case kTL:       Tello_UDP_Write(String("rc 0 0 0 -"+v));  break;
  }
}

void Tello_Replace_Write(String m, String a, String b) {
  m.replace(a,b);
  Tello_UDP_Write(m);
}

void Tello_Direct_Command(String msg) {
  
  //if(isTello[kCmd]==false) tello_udp_start(true);
  //if(isTelloUDP[kCmd]==false) return;
  
   msg.toLowerCase();//小文字に
   //String cs=msg;
  
  //----------------------
  // Tello Direct command 
  //----------------------  
  if(msg.startsWith("cmd")) {
    /*** Command ***/ Tello_UDP_Write("command");
    
  } else if(msg.startsWith("battery?")||msg.startsWith("baro?")||
            msg.startsWith("flip ")||msg.startsWith("rc ")||
            msg.startsWith("tof?")
           ) { //forward, back, right,toがあるから
    /*** msg ***/  Tello_UDP_Write(msg);

  } else if(msg.startsWith("to")) {
    /*** Tack off ***/ Tello_UDP_Write("takeoff");
    
  } else if(msg.startsWith("ld")||msg.startsWith("land")) { //Leftがあるから
    /*** Land ***/ Tello_UDP_Write("land");
    
  } else if(msg.startsWith("emg")) {
    /*** Emergency ***/ Tello_UDP_Write("emergency");
    
  } else if(msg.startsWith("fl ")) { //Forwardがあるから
    /*** flip ***/ Tello_Replace_Write(msg,"fl ","flip ");

  } else if(msg.startsWith("n")||msg.startsWith("n")) {
  /*** rc neutral ***/ Tello_UDP_Write("rc 0 0 0 0");
  
  } else if(msg.startsWith("r")) {
    if(msg.startsWith("right ")||msg.startsWith("r ")) {
      /*** right ***/ Tello_Replace_Write(msg,"r ","right ");
    } else {
      /*** rc right ***/ Tello_RC_Write(kRight,msg);
    }
   
  } else if(msg.startsWith("l")) {
    if(msg.startsWith("left ")||msg.startsWith("l ")) {
      /*** left ***/ Tello_Replace_Write(msg,"l ","left ");
    } else {
      /*** rc left ***/ Tello_RC_Write(kLeft,msg);
    }
    
  } else if(msg.startsWith("f")) {
    if(msg.startsWith("forward ")||msg.startsWith("f ")) {
      /*** forward ***/ Tello_Replace_Write(msg,"f ","forward ");
    } else {
      /*** rc forward ***/ Tello_RC_Write(kForward,msg);
    }
   
  } else if(msg.startsWith("b")) {
    if(msg.startsWith("back ")||msg.startsWith("b ")) {
      /*** back ***/ Tello_Replace_Write(msg,"b ","back ");
    } else {
      /*** rc back ***/ Tello_RC_Write(kBack,msg);
    }
   
  } else if(msg.startsWith("u")) {
    if(msg.startsWith("up ")||msg.startsWith("u ")) {
      /*** up ***/ Tello_Replace_Write(msg,"u ","up ");
    } else {
      /*** rc up ***/ Tello_RC_Write(kUp,msg);
    }

  } else if(msg.startsWith("d")) {
    if(msg.startsWith("down ")||msg.startsWith("d ")) {
      /*** down ***/ Tello_Replace_Write(msg,"d ","down ");
    } else {
      /*** rc down ***/ Tello_RC_Write(kDown,msg);
    }

  } else if(msg.startsWith("tr ")) {
      /*** cw ***/ Tello_Replace_Write(msg,"tr ","cw ");
      
  } else if(msg.startsWith("tl ")) {
      /*** ccw ***/ Tello_Replace_Write(msg,"tl ","ccw ");

  } else if(msg.startsWith("tr")) {
      /*** rc cw ***/ Tello_RC_Write(kTR,msg,2);
      
  } else if(msg.startsWith("tl")) {
      /*** rc ccw ***/ Tello_RC_Write(kTL,msg,2);
      
  } else {
    //command, takeoff, streamon, streamoff, emergency, go, curve, cw, ccw
    Tello_UDP_Write(msg);
  }
  
}
