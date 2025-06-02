/*
                   ┏━━━━┓┏━━━━┓
               VCC ┃1.  ┗┛  14┃ GND
   [GEARS] PIN_PA4 ┃2       13┃ PIN_PA3 [WINKER_L]
    [FREQ] PIN_PA5 ┃3       12┃ PIN_PA2 [WINKER_R]
   [PULSE] PIN_PA6 ┃4       11┃ PIN_PA1 [VOLTAGE]
  [SWITCH] PIN_PA7 ┃5       10┃ PIN_PA0 [UPDI]
     [---] PIN_PB3 ┃6        9┃ PIN_PB0 [SCL]
     [---] PIN_PB2 ┃7        8┃ PIN_PB1 [SDA]
                   ┗━━━━━━━━━━┛

備考：
  128x32のOLEDは動作不安定なので128x64のOLEDを使用
*/

#define FREQ_MODE
//#define DEBUG_MODE

#define INPUT_ANALOG 0x03

#define INDEX_FREQ    0
#define INDEX_PULSE   1
#define INDEX_VOLT    2
#define INDEX_GEARS   3
#define INDEX_WINKERS 4
#define INDEX_SWITCH  5

#include <Wire.h>      // I2C通信
#include "AttinyPin.h" // 自作クラスヘッダ

#ifdef DEBUG_MODE
	#include "tiny1306.h"
	#define ADDRESS_OLED 0x78
	TINY1306 oled = TINY1306(ADDRESS_OLED>>1, 128, 8);
	const int DISPLAY_INTERVAL = 16;
#else
	#define ADDRESS_ME 0x55
#endif

#ifdef FREQ_MODE
	const int INDEX_FREQTERVAL = 2000;
	//AttinyPin PULSE(PIN_PA1);
	AttinyPin PULSE(PIN_PA6);
	int freqArr[] = {
		15,55,
		105,155,
		205,255,
		305,355,
		405,455,
		505,555,
		605,655,
		705,755,
		805,855,
		905,955,
		1005
	};
	int freqArrSize = sizeof(freqArr) / sizeof(int);
	int freqArrIndex = 0;
	const int FREQ_INTERVAL =  250;
#endif 

byte regIndex = 0x00;
unsigned long counter = 0;
unsigned long pulseSpans = 0;

int data[6] = {-1,-1,-1,-1,-1,-1};
int dataSize = sizeof(data) / sizeof(int);

AttinyPin GEARS(PIN_PA4);
AttinyPin FREQ(PIN_PA5);
//AttinyPin SWITCH(PIN_PA6);
AttinyPin SWITCH(PIN_PA7);
//AttinyPin VOLTAGE(PIN_PA7);
AttinyPin VOLTAGE(PIN_PA1);
AttinyPin WINKER_L(PIN_PA3);
AttinyPin WINKER_R(PIN_PA2);

void setup() {
	delay(200);
	
	#ifdef DEBUG_MODE
		//デバッグモードの場合、ディスプレイと接続
		Wire.begin();
		Wire.setClock(400000);
		oled.init(); 
		oled.display();
		oled.clear();
		oled.setPage(0);
		oled.printlnS("Hello");
		oled.setPage(2);
		oled.printlnS("debug mode");
		delay(1000);
		oled.clear();
	#else
		// 非デバッグモードの場合、I2Cスレーブ設定
		Wire.begin(ADDRESS_ME);
		Wire.onReceive(receiveEvent);
		Wire.onRequest(requestEvent);
	#endif

	// ピン設定
	GEARS.begin(INPUT_ANALOG);    // ギア
	FREQ.begin(INPUT_PULLUP);     // 周波数
	SWITCH.begin(INPUT_PULLUP);   // スイッチ
	VOLTAGE.begin(INPUT_ANALOG);  // 電圧
	WINKER_L.begin(INPUT_PULLUP); // ウインカー左
	WINKER_R.begin(INPUT_PULLUP); // ウインカー右

	// 割り込み設定
	attachInterrupt(digitalPinToInterrupt(FREQ.getNum()), interruption, CHANGE);
	
	#ifdef FREQ_MODE
		// デバッグ用パルス出力設定
		tone(PULSE.getNum(), freqArr[freqArrIndex]);
	#endif
}

void loop(){
	static int loopNum = 0;
	static unsigned long updateTime = 0;
	static unsigned long beforePulseCount = 0;

	// システム時刻取得
	unsigned long time = millis();

	// ギアポジションADC値取得
	data[INDEX_GEARS]= analogRead(GEARS.getNum());
	// スイッチ状態取得
	data[INDEX_SWITCH] = digitalRead(SWITCH.getNum());
	// 電圧ADC値取得
	data[INDEX_VOLT] = analogRead(VOLTAGE.getNum());
	// ウインカー左右取得
	data[INDEX_WINKERS] = digitalRead(WINKER_L.getNum())<<1;
	data[INDEX_WINKERS] |= digitalRead(WINKER_R.getNum());

	// 周波数取得
	if(updateTime <= time){
		noInterrupts();
		// 累計周期(us)
		long spansTotal = pulseSpans;
		// 周期リセット
		pulseSpans = 0;
		// 前回取得からのパルスカウント
		unsigned long pulseCount = counter;
		interrupts();
		// 周波数算出
		int freqInt = int((pulseCount - beforePulseCount) * 1000000 / (2 * spansTotal));
		if(freqInt < 0){
			freqInt = 0;
		}
		// オーバーフロー防止
		data[INDEX_FREQ] = freqInt%10000;
		// 各値更新
		beforePulseCount = pulseCount;
		updateTime += FREQ_INTERVAL; 
	}

	#ifdef FREQ_MODE
		// パルス出力設定
		static unsigned long freqTime = 0;
		if(freqTime <= time){
			freqArrIndex++;
			freqArrIndex %= freqArrSize;
			tone(PULSE.getNum(), freqArr[freqArrIndex]);
			data[INDEX_PULSE] = freqArr[freqArrIndex];
			freqTime += INDEX_FREQTERVAL;
		}
	#endif

	#ifdef DEBUG_MODE
		// ディスプレイ表示
		static unsigned long dispTime = 0;
		if(dispTime <= time){
			unsigned long dStart = millis();
			/*
			String vStr = "count  :";
			vStr += String(counter);
			oled.setPage(2);
			oled.printlnS(vStr);/
			*/
			String vStr  = "freqIO :";
			vStr += convertStr(data[INDEX_FREQ]);
			vStr += "-";
			vStr += convertStr(data[INDEX_PULSE]);
			oled.setPage(3);
			oled.printlnS(vStr);
			vStr  = "voltADC:";
			vStr += convertStr(data[INDEX_VOLT]);
			oled.setPage(4);
			oled.printlnS(vStr);

			vStr  = "gearADC:";
			vStr += convertStr(data[INDEX_GEARS]);
			oled.setPage(5);
			oled.printlnS(vStr);

			vStr  = "switch :";
			int v = data[INDEX_SWITCH];
			vStr += convertStr(v);
			vStr += ":";
			vStr += (v == 1) ? "OFF" : "ON ";
			oled.setPage(6);
			oled.printlnS(vStr);

			vStr  = "winkers:";
			v = data[INDEX_WINKERS];
			vStr += convertStr(v);
			vStr += ":";
			vStr += ((v&0x01) == 0x01) ? "L" : " ";
			vStr += "-";
			vStr += ((v&0x02) == 0x02) ? "R" : " ";
			oled.setPage(7);
			oled.printlnS(vStr);

			unsigned long dDuration = millis() - dStart;

			vStr = convertStr(loopNum++);
			vStr += " ";
			vStr += convertStr(dDuration);
			oled.setPage(0);
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
	// オーバーフロー防止
	counter %= 10000000;
	// 周期[us]を取得・加算
	unsigned long time = micros();
	pulseSpans += time - beforeTime;
	beforeTime = time;
}

#ifdef DEBUG_MODE
	/**
	 * int4桁から文字列変換(右空白埋め)
	 */
	String convertStr(int v){
		String vStr = "";
		if(0 <= v && v < 10){
			vStr += "   ";
		}
		else if((10 <= v && v < 100) || (-10 < v && v < 0)){
			vStr += "  ";
		}
		else if((100 <= v && v < 1000) || (-100 < v && v <= -10)){
			vStr += " ";
		}
		vStr += String(v);
		return vStr;
	}
#else
 /**
  * I2C受診処理
  */
	void receiveEvent(int numByte){
		while(0 < Wire.available()){
			regIndex = Wire.read();
		}
	}

	/**
  * I2C要求処理
  */
	void requestEvent(){
		int sendData = -1;
		if(regIndex < dataSize){
			sendData = data[regIndex];
		}
		byte sendDataArr[2] = {byte(sendData>>8), byte(sendData&0xFF)};
		Wire.write(sendDataArr, 2);
	}
#endif