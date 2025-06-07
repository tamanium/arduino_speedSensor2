#ifndef ATTINY1306_H
	#define ATTINY1306_H
	#include <arduino.h>

	class AttinyPin {
		private:
			byte num;       // ピン番号
			byte mode;      // モード
		public:
			// コンストラクタ
			AttinyPin(byte p);
			void begin(byte mode);
			byte getNum(); 
	};

	/**
	 * コンストラクタ
	 */
	AttinyPin::AttinyPin(byte p){
		this->num = p;
		this->mode = OUTPUT;
	}

	/**
	 * ピン番号取得
	 */
	byte AttinyPin::getNum(){
		return this->num;
	}

	/**
	 * 動作開始
	 *
	 * @param mode OUTPUT(0x00), INPUT(0x01), INPUT_PULLUP(0x02) (0x03はアナログ入力)
	 */
	void AttinyPin::begin(byte mode){
		if(3 < mode){
			mode = OUTPUT;
		}
		if(mode==OUTPUT || mode==INPUT || mode==INPUT_PULLUP){
			pinMode(this->num, mode);
		}
		this->mode = mode;
	}
#endif