#include <arduino.h>

class AttinyPin {
	private:
		byte num;       // ピン番号」
		byte mode;      // モード
		bool digitalValue; // デジタル入力値
		int analogValue;   // アナログ入力値
	public:
		// コンストラクタ
		AttinyPin(int p, int mode = OUTPUT);
		void updateValue();
		byte getNum();
		bool getDigitalValue();
		byte getDigitalValue1();
		int getAnalogValue();
};

/**
 * コンストラクタ
 */
AttinyPin::AttinyPin(int p, int mode){
	this->num = byte(p);
	this->mode = byte(mode);
	this->digitalValue = false;
	this->analogValue = 0;
	if(mode <= 2) {
		pinMode(p, mode);
	}
}
byte AttinyPin::getNum(){
	return this->num;
}

/**
 * 値の更新
 */
 void AttinyPin::updateValue(){
	if(mode == INPUT|| mode == INPUT_PULLUP){
		this->digitalValue = (digitalRead(this->num) == HIGH);
	}
	else if(mode != OUTPUT){
		this->analogValue = analogRead(this->num);
	}
 }

/**
 * デジタル入力値取得
 */
 bool AttinyPin::getDigitalValue(){
	return this->digitalValue;
 }

 /**
 * デジタル入力値取得
 */
 byte AttinyPin::getDigitalValue1(){
	return this->digitalValue ? 1 : 0;
 }

/**
 * アナログ入力値取得
 */
 int AttinyPin::getAnalogValue(){
	return this->analogValue;
 }