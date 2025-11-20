/*
ピン配置
                        ┏━━━━┓┏━━━━┓
                    VCC ┃1.  ┗┛  14┃ GND
      [winkers] PIN_PA4 ┃2       13┃ PIN_PA3 [voltage]
   (yet)[gearN] PIN_PA5 ┃3       12┃ PIN_PA2 [sw]
   (yet)[gear1] PIN_PA6 ┃4       11┃ PIN_PA1 [speed]
   (yet)[gear2] PIN_PA7 ┃5       10┃ PIN_PA0 [UPDI]
   (yet)[gear3] PIN_PB3 ┃6        9┃ PIN_PB0 [SCL]
   (yet)[gear4] PIN_PB2 ┃7        8┃ PIN_PB1 [SDA]
                        ┗━━━━━━━━━━┛

備考：
  128x32のOLEDは動作不安定なので128x64のOLEDを使用
*/

#include <Wire.h>       // I2C通信
//#include "AttinyPin.h"  // 自作クラスヘッダ

//#define DEBUG_MODE
#define MY_ADDRESS 0x55

#ifdef DEBUG_MODE
	#include "tiny1306.h"
	#define ADDRESS_OLED 0x78
	TINY1306 oled = TINY1306(ADDRESS_OLED >> 1, 128, 8);
	const int DISPLAY_INTERVAL = 16;
#endif

const int FREQ_INTERVAL = 250;

// インデックス定数（本体側も同じ定義）
enum {
	FREQ,      // パルス周波数
	GEAR,      // ギアポジADC値
	WNKR,      // ウインカービット値
	BUTN,      // スイッチビット値
	VOLT,      // 電圧ADC値
	DATA_SIZE, // 配列要素数
};

// ピン定義
enum {
	gearN  = PIN_PA7,
	gear1  = PIN_PB2,
	gear2  = PIN_PA6,
	gear3  = PIN_PA5,
	gear4  = PIN_PB3,

	voltage = PIN_PA3,
	sw      = PIN_PA2,
	speed   = PIN_PA1,
	winkers = PIN_PA4
};

// ギア用ピン定義
constexpr uint8_t gears[] = {gearN, gear1, gear2, gear3, gear4};
// パルス取得用カウンタ
unsigned long counter = 0;
// パルス所得用合計カウンタ
unsigned long pulsePeriodTotal = 0;
// データ格納用配列
int data[DATA_SIZE];

void setup() {
	delay(200);

	#ifdef DEBUG_MODE
		//デバッグモード初期設定
		debugInit();
	#else
		// 非デバッグモードの場合、I2Cスレーブ設定
		normalInit();
	#endif


	//ギアポジション
	for(uint8_t gear : gears) {
		pinMode(gear, INPUT_PULLUP);
	}
	// 周波数
	pinMode(speed, INPUT_PULLUP);
	// スイッチ
	pinMode(sw, INPUT_PULLUP);
	// 割り込み設定
	attachInterrupt(digitalPinToInterrupt(speed), interruption, CHANGE);
}

void loop() {
	static unsigned long updateTime = 0; // 周波数取得時刻
	
	unsigned long sysTime = millis();    // システム時刻取得

	data[GEAR] = getGearData();       // ギア状態取得
	data[BUTN] = !digitalRead(sw);    // スイッチ状態取得(LOWでON)
	data[VOLT] = analogRead(voltage); // 電圧ADC値取得
	data[WNKR] = analogRead(winkers); // ウインカーADC値取得
	// 周波数取得
	if (updateTime <= sysTime) {
		noInterrupts();            // 割り込み処理停止
		unsigned long  p = pulsePeriodTotal; // 累計周期(us)
		pulsePeriodTotal = 0;      // 周期リセット
		unsigned long c = counter; // 前回取得からのパルスカウント
		interrupts();              // 割り込み処理再開
		data[FREQ] = calcFreq(c, p); // 周波数算出
		updateTime += FREQ_INTERVAL;
	}

	#ifdef DEBUG_MODE
		// デバッグ表示
		displayDebug(sysTime);
	#endif
}

/**
 * ギアポジション取得
 * @return uint8_t型 ビットマップ[N,1,2,3,4,]
 */
uint8_t getGearData(){
	uint8_t result = 0;
	for(uint8_t i=0; i<sizeof(gears)/sizeof(uint8_t); i++) {
		result |= digitalRead(gears[i]) << i;
	}
	return result;
}

/**
 * 周波数算出
 *
 * @param counter 合計パルスカウント数
 * @param period  前回からのパルス周期積算
 */
int calcFreq(unsigned long counter, unsigned long period) {
	static unsigned long beforeCounter = 0;
	// 0の場合、処理スキップ
	if(period == 0) return 0;
	unsigned long freqInt = (counter - beforeCounter) * 1000000UL / (2 * period);
	beforeCounter = counter;
	// 最大9999
	return (freqInt < 10000) ? freqInt : 9999;
}

/**
 * 割り込み処理
 */
void interruption() {
	static unsigned long beforeTime = 0;
	counter++;
	// 周期[us]を取得・加算
	unsigned long time = micros();
	pulsePeriodTotal += time - beforeTime;
	beforeTime = time;
}

#ifdef DEBUG_MODE

	/**
	 * デバッグモード初期設定
	 */
	void debugInit() {
		Wire.begin();
		Wire.setClock(400000);
		oled.init();
		oled.display();
		oled.clear();
		printlnS(0, "Hello");
		printlnS(2, "debug mode");
		delay(1000);
		oled.clear();
	}

	/**
	 * 表示処理
	 */
	void displayDebug(unsigned long sysTime) {
		static unsigned long displayTime = 0;
		static int loopNum = 0;
		if (displayTime <= sysTime) {
			unsigned long dStart = millis();
			int page = 0;
			int tmpData[DATA_SIZE+2];
			memcpy(tmpData, data, DATA_SIZE);
			tmpData[DATA_SIZE] = loopNum++;
			tmpData[DATA_SIZE+1] = millis() - dStart;

			char msgCharsArr[7][9] = {
				"freqIO :",
				"gearADC:",
				"winkADC:",
				"switch :",
				"voltADC:",

				"counter:",
				"msec   :"
			};
				
			char valueChars[4];
			for(int i=0;i<DATA_SIZE+2;i++){
				char msgChars[13];
				memcpy(msgChars, msgCharsArr, 8);
				sprintf(valueChars, "%4d", tmpData[i]);
				strncpy(&msgChars[8], valueChars, 4);
				oled.setPage(page++);
				oled.println(msgChars);
			}

			loopNum %= 10000;
			displayTime += DISPLAY_INTERVAL;
		}
	}

	/**
	 * 文字列表示
	 *
	 * @param p ページ
	 * @param charPtr char配列ポインタ
	 */
	void printlnS(int p, const char* charPtr){
		oled.setPage(p);
		oled.printlnS(charPtr);
	}
#else

	/**
	 * 通常モード初期設定
	 */
	void normalInit() {
		// I2C通信設定
		Wire.begin(MY_ADDRESS);
		Wire.onReceive(receiveEvent);
		Wire.onRequest(requestEvent);
	}

	/**
	 * I2C受信処理
	 */
	void receiveEvent(int numByte) {
		while (0 < Wire.available()) {
			uint8_t dummy = Wire.read();
		}
	}

	/**
	 * I2C要求処理（全データ送信）
	 */
	void requestEvent() {
		uint8_t sendDataArr[DATA_SIZE*2] = {
			uint8_t(data[FREQ] >> 8), uint8_t(data[FREQ] & 0xFF),
			uint8_t(data[GEAR] >> 8), uint8_t(data[GEAR] & 0xFF),
			uint8_t(data[WNKR] >> 8), uint8_t(data[WNKR] & 0xFF),
			uint8_t(data[BUTN] >> 8), uint8_t(data[BUTN] & 0xFF),
			uint8_t(data[VOLT] >> 8), uint8_t(data[VOLT] & 0xFF),
		};
		Wire.write(sendDataArr, DATA_SIZE*2);
	}
#endif
