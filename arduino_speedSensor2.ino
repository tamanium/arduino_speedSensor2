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
#include "AttinyPin.h"  // 自作クラスヘッダ

//#define DEBUG_MODE
#define INPUT_ANALOG 0x03
#define ADDRESS_ME 0x55
#define ADDRESS_OLED 0x78

#ifdef DEBUG_MODE
	#include "tiny1306.h"
	TINY1306 oled = TINY1306(ADDRESS_OLED >> 1, 128, 8);
	const int DISPLAY_INTERVAL = 16;
#endif


const int FREQ_INTERVAL = 250;

// 本体側も同じ定義
enum {
	INDEX_FREQ,           // パルス周波数
	INDEX_GEARS,         // ギアポジADC値
	INDEX_WINKERS,       // ウインカービット値
	INDEX_SWITCH,        // スイッチビット値
	INDEX_VOLT,          // 電圧ADC値

	DATA_SIZE,           // 配列要素数
	INDEX_A_PART = 0x40, // 電圧と出力パルス以外のデータを要求する値
	INDEX_ALL = 0xFF,    // 全てのデータを要求する値(イラナイかも)
};

AttinyPin gearN(PIN_PA5); // N
AttinyPin gear1(PIN_PA6); // 1
AttinyPin gear2(PIN_PA7); // 2
AttinyPin gear3(PIN_PB3); // 3
AttinyPin gear4(PIN_PB2); // 4
AttinyPin gears[] = {gearN, gear1, gear2, gear3, gear4};

AttinyPin voltage(PIN_PA3);  // 電圧new
AttinyPin sw(PIN_PA2);   // スイッチ
AttinyPin speed(PIN_PA1);     // 周波数new
AttinyPin winkers(PIN_PA4);  // ウインカーnew

byte regIndex = 0x00;
unsigned long counter = 0;
unsigned long pulsePeriodTotal = 0;
int data[DATA_SIZE];

void setup() {
	delay(200);

	#ifdef DEBUG_MODE
		//デバッグモードの場合、ディスプレイと接続
		Wire.begin();
		Wire.setClock(400000);
		oled.init();
		oled.display();
		oled.clear();
		printlnS(0, "Hello");
		printlnS(2, "debug mode");
		delay(1000);
		oled.clear();
	#else
		// 非デバッグモードの場合、I2Cスレーブ設定
		Wire.begin(ADDRESS_ME);
		Wire.onReceive(receiveEvent);
		Wire.onRequest(requestEvent);
	#endif

	// データバッファの初期化
	for (int i = 0; i < DATA_SIZE; i++) {
		data[i] = -2;
	}

	// ピン設定
	for(AttinyPin gear : gears){ // ギア
		gear.begin(INPUT_PULLUP);
	}
	speed.begin(INPUT_PULLUP);   // 周波数
	sw.begin(INPUT_PULLUP);      // スイッチ
	voltage.begin(INPUT_ANALOG); // 電圧
	winkers.begin(INPUT_ANALOG); // ウインカー右

	// 割り込み設定
	attachInterrupt(digitalPinToInterrupt(speed.getNum()), interruption, CHANGE);
}

void loop() {
	static unsigned long updateTime = 0;
	
	unsigned long time = millis();                      // システム時刻取得

	data[INDEX_GEARS]   = getGearData();                  // ギア状態取得
	data[INDEX_SWITCH]  = !digitalRead(sw.getNum()); // スイッチ状態取得(LOWでON)
	data[INDEX_VOLT]    = analogRead(voltage.getNum());    // 電圧ADC値取得 TODO:非デバッグ時は1分間隔くらいにする
	data[INDEX_WINKERS] = analogRead(winkers.getNum()); // ウインカーADC値取得

	// 周波数取得
	if (updateTime <= time) {
		noInterrupts();            // 割り込み処理停止
		long p = pulsePeriodTotal; // 累計周期(us)
		pulsePeriodTotal = 0;      // 周期リセット
		unsigned long c = counter; // 前回取得からのパルスカウント
		interrupts();              // 割り込み処理再開
		data[INDEX_FREQ] = calcFreq(c, p); // 周波数算出

		updateTime += FREQ_INTERVAL;
	}

	#ifdef DEBUG_MODE
		// ディスプレイ表示
		static unsigned long dispTime = 0;
		static int loopNum = 0;
		if (dispTime <= time) {
			unsigned long dStart = millis();
			int page = 3;
			char vStr[5] = "";
			
			String vStr = "freqIO :";
			vStr += sprintf(vStr, "%4d", data[INDEX_FREQ]);
			printlnS(page++, vStr);
			
			vStr = "voltADC:";
			vStr += sprintf(vStr, "%4d", data[INDEX_VOLT]);
			printlnS(page++, vStr);

			vStr = "gearADC:";
			vStr += sprintf(vStr, "%4d", data[INDEX_GEARS]);
			printlnS(page++, vStr);

			vStr = "switch :";
			vStr += sprintf(vStr, "%4d", data[INDEX_SWITCH]);
			printlnS(page++, vStr);

			vStr = "winkADC:";
			vStr += sprintf(vStr, "%4d", data[INDEX_WINKERS]);
			printlnS(page++, vStr);

			vStr = sprintf(vStr, "%4d", loopNum++);
			vStr += " ";
			vStr += sprintf(vStr, "%4d", millis() - dStart);
			printlnS(0, vStr);
			loopNum %= 10000;

			dispTime += DISPLAY_INTERVAL;
		}
	#endif
}

int getGearData(){
	int result = 0;
	int i = 0;
	for(AttinyPin gear : gears){
		result |= digitalRead(gear.getNum())<<(i++);
	}
	return result;
}

/**
 * 周波数算出
 *
 * @param counter 合計パルスカウント数
 * @param period  前回からのパルス周期積算
 */
int calcFreq(int counter, long period) {
	static int beforeCounter = 0;
	int freqInt = int((counter - beforeCounter) * 1000000 / (2 * period));
	// 0未満なら0に、0以上なら最大値9999にまとめる
	freqInt = (freqInt < 0) ? 0 : freqInt % 10000;

	beforeCounter = counter;
	return freqInt;
}

/**
 * 割り込み処理
 */
void interruption() {
	static unsigned long beforeTime = 0;
	counter++;
	// オーバーフロー防止
	counter %= 10000000;
	// 周期[us]を取得・加算
	unsigned long time = micros();
	pulsePeriodTotal += time - beforeTime;
	beforeTime = time;
}

#ifdef DEBUG_MODE
	/**
	 * 文字列表示
	 *
	 * @param p ページ
	 * @param str 文字列
	 */
	void printlnS(int p, String str){
		oled.setPage(p);
		oled.printlnS(str);
	}
#else
	/**
	 * I2C受診処理
	 */
	void receiveEvent(int numByte) {
		while (0 < Wire.available()) {
			regIndex = Wire.read();
		}
	}

	/**
	 * I2C要求処理
	 */
	void requestEvent() {
		int sendData = -8;
		if (regIndex == INDEX_A_PART) {
			byte sendDataArr[8] = {
				byte(data[INDEX_FREQ] >> 8),    byte(data[INDEX_FREQ] & 0xFF),
				byte(data[INDEX_GEARS] >> 8),   byte(data[INDEX_GEARS] & 0xFF),
				byte(data[INDEX_WINKERS] >> 8), byte(data[INDEX_WINKERS] & 0xFF),
				byte(data[INDEX_SWITCH] >> 8),  byte(data[INDEX_SWITCH] & 0xFF),
			};
			Wire.write(sendDataArr, 8);
			return;
		}
		else if (regIndex < DATA_SIZE) {
			sendData = data[regIndex];
		}
		byte sendDataArr[2] = { byte(sendData >> 8), byte(sendData & 0xFF) };
		Wire.write(sendDataArr, 2);
	}
#endif
