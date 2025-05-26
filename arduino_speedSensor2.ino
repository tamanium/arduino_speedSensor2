/*
                 ┏━━━━━━━━━┓
             VCC ┃1      14┃ GND
  SS  A0 PIN_PA4 ┃2      13┃ PIN_PA3 A10 SCK
      A1 PIN_PA5 ┃3      12┃ PIN_PA2 A9 MISO
      A2 PIN_PA6 ┃4      11┃ PIN_PA1 A8 MOSI
      A3 PIN_PA7 ┃5      10┃ PIN_PA0 A11 UPDI
  RxD D4 PIN_PB3 ┃6       9┃ PIN_PB0 A7 SCL
  TxD D5 PIN_PB2 ┃7       8┃ PIN_PB1 A6 SDA
                 ┗━━━━━━━━━┛

備考：
  128x32のOLEDは動作不安定なので128x64のOLEDを使用
  USB-CのD-とSCLを接続することによるプルアップは不要
*/

//#define DEBUG_MODE
#define FREQ_MODE
#define INPUT_ANALOG 0x03

#define FREQ_INPUT  0
#define FREQ_OUTPUT 1
#define ADC_IN      2
#define SHIFT_NUM   3
#define WINKERS     4

#include <Wire.h>
#include "AttinyPin.h"

#ifdef DEBUG_MODE
	#include "tiny1306.h"
	#define ADDREDD_OLED 0x78
	TINY1306 oled = TINY1306(ADDREDD_OLED>>1, 128, 8);
#else
	#define address 0x55
#endif

#ifdef FREQ_MODE
	const int FREQ_INTERVAL = 2000;
	const int pulseOutputPin = PIN_PA1;	
	int freqArr[] = {
		10,50,
		100,150,
		200,250,
		300,350,
		400,450,
		500,550,
		600,650,
		700,750,
		800,850,
		900,950,
		1000
	};
	int freqArrSize = sizeof(freqArr) / sizeof(int);
	int freqArrIndex = 0;
#endif 

const int DISPLAY_INTERVAL = 50;
const int UPDATE_INTERVAL = 250;

byte regIndex = 0x00;
unsigned long counter = 0;
unsigned long pulseSpans = 0;

int data[5] = {-1,-1,-1,-1,-1};
int dataSize = sizeof(data) / sizeof(int);

AttinyPin SHIFTS(PIN_PA4);
AttinyPin SPEED(PIN_PA5);
AttinyPin SWITCH(PIN_PA6);
AttinyPin WINKER_L(PIN_PA3);
AttinyPin WINKER_R(PIN_PA2);


void setup() {
	delay(500);
	
	// ピン設定
	SHIFTS.begin(INPUT_ANALOG);
	SPEED.begin(INPUT_PULLUP);
	SWITCH.begin(INPUT_PULLUP);
	WINKER_L.begin(INPUT_PULLUP);
	WINKER_R.begin(INPUT_PULLUP);

	// 割り込み設定
	attachInterrupt(digitalPinToInterrupt(SPEED.getNum()), interruption, CHANGE);


	#ifdef FREQ_MODE
		//デバッグ用
		tone(pulseOutputPin, freqArr[freqArrIndex]);
	#endif
	
	#ifdef DEBUG_MODE
		Wire.begin();
		oled.init(); 
		oled.display();
		oled.clear();

		oled.setPage(0);
		oled.printlnS("Hello");
		oled.setPage(2);
		oled.printlnS("debug mode");
		delay(1000);
		oled.clear();

		String msg = "";
		msg += "SF:";
		msg += String(SHIFTS.getNum()) + " ";
		msg += "SP:";
		msg += String(SPEED.getNum()) + " " ;
		msg += "SW:";
		msg += String(SWITCH.getNum()) + " ";
		oled.setPage(2);
		oled.printlnS(msg);
		
		msg  = "L:";
		msg += String(WINKER_L.getNum()) + " ";
		msg += "R:";
		msg += String(WINKER_R.getNum()) + " ";
		oled.setPage(3);
		oled.printlnS(msg);
	#else
		Wire.begin(address);
		Wire.onReceive(receiveEvent);
		Wire.onRequest(requestEvent);	
	#endif
}

void loop(){
	static int loopNum = 0;
	static unsigned long updateTime = 0;
	static unsigned long beforePulseCount = 0;

	unsigned long time = millis();
	
	if(updateTime <= time){
		noInterrupts();
		long spansTotal = pulseSpans;
		unsigned long pulseCount = counter;
		pulseSpans = 0;
		interrupts();
		int freqInt = int((pulseCount - beforePulseCount) * 1000000 / (2 *spansTotal));
		if(freqInt < 0){
			freqInt = 0;
		}
		data[FREQ_INPUT] = freqInt%10000;

		beforePulseCount = pulseCount;
		updateTime += UPDATE_INTERVAL; 
	}

	#ifdef FREQ_MODE
		static unsigned long freqTime = 0;
		if(freqTime <= time){
			freqArrIndex++;
			freqArrIndex %= freqArrSize;
			tone(pulseOutputPin, freqArr[freqArrIndex]);
			data[FREQ_OUTPUT] = freqArr[freqArrIndex];
			freqTime += FREQ_INTERVAL;
		}
	#endif

	#ifdef DEBUG_MODE
		static unsigned long dispTime = 0;
		if(dispTime <= time){
			oled.setPage(0);
			oled.printlnS(String(loopNum++));
			
			String vStr = "count  :";
			vStr += String(counter);
			oled.setPage(5);
			oled.printlnS(vStr);

			vStr  = "freq I :";
			vStr += String(data[FREQ_INPUT]);
			vStr += "   ";
			oled.setPage(6);
			oled.printlnS(vStr);

			vStr  = "freq O :";
			vStr += String(data[FREQ_OUTPUT]);
			vStr += "   ";
			oled.setPage(7);
			oled.printlnS(vStr);
			dispTime += DISPLAY_INTERVAL;
		}
	#endif
}

/**
 * 割り込み処理
 */
volatile unsigned long beforeTime = 0;
void interruption(){
	counter++;
	counter %= 10000000;
	// 波長[us]を取得・加算
	unsigned long time = micros();
	pulseSpans += time - beforeTime;
	beforeTime = time;
}
#ifndef DEBUG_MODE
	void receiveEvent(int numByte){
		while(0 < Wire.available()){
			regIndex = Wire.read();
		}
	}

	void requestEvent(){
		int sendData = -1;
		if(regIndex < dataSize){
			sendData = data[regIndex];
		}
		byte sendDataArr[2] = {byte(sendData>>8), byte(sendData&0xFF)};
		Wire.write(sendDataArr, 2);
	}

#endif