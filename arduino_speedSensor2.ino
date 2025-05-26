/*
                  _________
             VCC |1      14| GND
  SS  A0 PIN_PA4 |2      13| PIN_PA3 A10 SCK
      A1 PIN_PA5 |3      12| PIN_PA2 A9 MISO
      A2 PIN_PA6 |4      11| PIN_PA1 A8 MOSI
      A3 PIN_PA7 |5      10| PIN_PA0 A11 UPDI
  RxD D4 PIN_PB3 |6       9| PIN_PB0 A7 SCL
  TxD D5 PIN_PB2 |7       8| PIN_PB1 A6 SDA
                 |_________|
*/

#define DEBUG_MODE

#include <Wire.h>
#include "AttinyPin.h"
#ifdef DEBUG_MODE
  #include "tiny1306.h"
#endif


#ifdef DEBUG_MODE
  #define ADDREDD_OLED 0x78
  TINY1306 oled = TINY1306(ADDREDD_OLED>>1, 128, 8);
#endif

AttinyPin WINKER_L(PIN_PA4, INPUT_PULLUP);
AttinyPin WINKER_R(PIN_PA5, INPUT_PULLUP);
AttinyPin SPEED(PIN_PA6, INPUT_PULLUP);
AttinyPin SWTCH(PIN_PA7, INPUT_PULLUP);
AttinyPin SHIFT_N(PIN_PB3, INPUT_PULLUP);
AttinyPin SHIFT_1(PIN_PB2, INPUT_PULLUP);

AttinyPin SHIFT_2(PIN_PA3, INPUT_PULLUP);
AttinyPin SHIFT_3(PIN_PA2, INPUT_PULLUP);
AttinyPin SHIFT_4(PIN_PA1, INPUT_PULLUP);

// ピン構造体の構造隊
AttinyPin pinArr[9] = {
  WINKER_L,
  WINKER_R,
  SPEED,
  SWTCH,
  SHIFT_N,
  SHIFT_1,
  SHIFT_2,
  SHIFT_3,
  SHIFT_4
};


void setup() {
  delay(500);
  
  //for(AttinyPin pin : pinArr){
  //  pinMode(pin.num, INPUT_PULLUP);
  //}

  #ifdef DEBUG_MODE
    Wire.begin();
    oled.init(); 
    oled.display();
    oled.clear();
    //oled.setScale(2);
  #endif

  #ifdef DEBUG_MODE
    oled.setPage(0);
    oled.printlnS("Hello");
    oled.setPage(2);
    oled.printlnS("debug");
    delay(1000);
    oled.clear();
  #endif
}

void loop(){
  static int loopNum = 0;


  for(AttinyPin pin : pinArr){
    pin.updateValue();
  }

  oled.clear();

  oled.setPage(0);
  oled.printlnS(String(loopNum++));
  
  oled.setPage(2);
  String msg = "";
  msg += String(WINKER_L.getNum()) + ":" + String(WINKER_L.getDigitalValue1()) + " ";
  msg += String(WINKER_R.getNum()) + ":" + String(WINKER_R.getDigitalValue1()) + "  ";
  msg += String(SPEED.getNum()) + ":" + String(SPEED.getDigitalValue1()) + " ";
  msg += String(SWTCH.getNum()) + ":" + String(SWTCH.getDigitalValue1()) + " ";
  msg += String(SHIFT_N.getNum()) + ":" + String(SHIFT_N.getDigitalValue1());
  oled.printlnS(msg);

  oled.setPage(4);
  msg = "";
  msg += String(SHIFT_1.getNum()) + ":" + String(SHIFT_1.getDigitalValue1()) + " ";
  msg += String(SHIFT_2.getNum()) + ":" + String(SHIFT_2.getDigitalValue1()) + " ";
  msg += String(SHIFT_3.getNum()) + ":" + String(SHIFT_3.getDigitalValue1()) + " ";
  msg += String(SHIFT_4.getNum()) + ":" + String(SHIFT_4.getDigitalValue1()) + " ";
  oled.printlnS(msg);
  
  oled.display();
  delay(2000);
}
