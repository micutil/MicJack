/*
 * Quest program comverter
 * English
 */

#include "questUtil.h"

String boxName[8]={"#","A","B","C","D","E","F","G"};

String getBoxNum(uint8_t t) {
  return "Box"+boxName[t];
}

String argStr(uint8_t t, uint8_t a) {
  if(t) return getBoxNum(a);
  return String(a);
}

String questBinToProg(uint8_t *qd) {
  int i,j,k;
  String pgm="";
  String cmd="";
  questCmd qc;
  for(i=0;i<64;i++) {
    j=i*8;
    qc.id=qd[j];
    k=qd[j+1];
    qc.argtype1=(k&3);
    qc.argtype2=((k&12)>>2);
    qc.argtype3=((k&48)>>4);
    qc.arg1=(int16_t)(qd[j+3]*256+qd[j+2]);
    qc.arg2=(int16_t)(qd[j+5]*256+qd[j+4]);
    qc.arg3=(int16_t)(qd[j+7]*256+qd[j+6]);

    switch(qc.id) {
      case 1://LED
        if(qc.argtype1) {
          cmd="Turn "+getBoxNum(qc.arg1)+" LED";
        } else {
          cmd="Turn off LED";
          if(qc.arg1==1) cmd="Turn on LED";
        }
        break;
      case 2://OUT
        cmd="Set OUT "+argStr(qc.argtype1,qc.arg1);
        break;
      case 3://PWM
        cmd="Set PWM"+String(qc.arg1)+", ";
        cmd=cmd+argStr(qc.argtype2,qc.arg2);
        break;
      case 4://WAIT
        cmd="Wait ";
        if(qc.arg2==0 ) {
          if(qc.arg1==500) {
            cmd=cmd+"0.5 sec";
          } else if(qc.arg1==1000) {
            cmd=cmd+"1 sec";
          } else if(qc.arg1==2000) {
            cmd=cmd+"2 sec";
          }
        } else {
          if(qc.argtype1) {
            cmd=cmd+getBoxNum(qc.arg1);
          } else {
            cmd=cmd+String(qc.arg1)+" times";
          }
        }
        break;
      case 5://GOTO
        cmd="Go To "+String(qc.arg1+1);
        break;
      case 6://IN
        cmd="Check IN"+String(qc.arg1);
        break;
      case 7://ANA
        cmd="Check ANA"+String(qc.arg1);
        break;
      case 8://BTN
        cmd="Check ";
        switch(qc.arg1) {
          case 0: cmd=cmd+"BUTTON"; break;
          case 1: cmd=cmd+"UP"; break;
          case 2: cmd=cmd+"DOWN"; break;
          case 3: cmd=cmd+"LEFT"; break;
          case 4: cmd=cmd+"RIGHT"; break;
          case 5: cmd=cmd+"ENTER"; break;
        }
        break;
      case 9://LET
        cmd=getBoxNum(qc.arg1)+" = "+argStr(qc.argtype2,qc.arg2);
        break;
      case 0xA://ADD
        cmd=getBoxNum(qc.arg1)+" + "+argStr(qc.argtype2,qc.arg2);
        break;
      case 0xB://SUB
        cmd=getBoxNum(qc.arg1)+" - "+argStr(qc.argtype2,qc.arg2);
        break;
      case 0xC://MUL
        cmd=getBoxNum(qc.arg1)+" * "+argStr(qc.argtype2,qc.arg2);
        break;
      case 0xD://DIV
        cmd=getBoxNum(qc.arg1)+" / "+argStr(qc.argtype2,qc.arg2);
        break;
      case 0xE://MOD
        cmd=getBoxNum(qc.arg1)+" % "+argStr(qc.argtype2,qc.arg2);
        break;
      case 0xF://Random
        cmd="Random "+argStr(qc.argtype1,qc.arg1);
        break;
      case 0x10://GOIF
        cmd="If #";
        switch(qc.arg2) {
          case 0: cmd=cmd+"="; break;
          case 1: cmd=cmd+">"; break;
          case 2: cmd=cmd+"<"; break;
          case 3: cmd=cmd+"!="; break;
        }
        cmd=cmd+String(qc.arg3)+" go "+String(qc.arg1+1);
        break;
      case 0x11://END
        cmd="END";
        break;
      case 0x12://CLS
        cmd="Clear screen";
        break;
      case 0x13://Print
        if(qc.argtype1) {
          cmd="Print "+getBoxNum(qc.arg1);
        } else {
          cmd="Print '#"+argStr(qc.argtype1,qc.arg1)+"'";
        }
        break;
      case 0x14://LC
        cmd="Set("+argStr(qc.argtype1,qc.arg1)+", "+argStr(qc.argtype2,qc.arg2)+")";
        break;
      case 0x15://SCR
        cmd="Get("+argStr(qc.argtype1,qc.arg1)+", "+argStr(qc.argtype2,qc.arg2)+")";
        break;
      case 0x16://LRUN
        cmd="Go JOURNY"+String(qc.arg1);
        break;
      case 0x17://BEEP
        cmd="Beep";
        break;
      case 0x18://CLT
        cmd="Reset clock";
        break;
      case 0x19://TICK
        cmd="Check clock";
        break;
      case 0x1A://OUTP
        cmd="Turn ";
        if(qc.argtype2) {
          cmd=cmd+getBoxNum(qc.arg2)+" ";
        } else {
          if(qc.arg2) {
            cmd=cmd+"on ";
          } else {
            cmd=cmd+"off ";
          }
        }
        cmd=cmd+"OUT"+String(qc.arg1);
        break;
      case 0xFF:
        return pgm;
        break;
      default:
        cmd="";
        break;
    }
    
    pgm=pgm+cmd+'\n';//String(i+1)+" "+

  }

  return pgm;
}
