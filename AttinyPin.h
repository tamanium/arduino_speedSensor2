#include <arduino.h>

class AttinyPin {
	private:
		int num;       // ピン番号
		int mode;      // モード
	public:
		// コンストラクタ
		AttinyPin(int p);
		void begin(int mode);
		int getNum(); 
};

/**
 * コンストラクタ
 */
AttinyPin::AttinyPin(int p){
	this->num = p;
	this->mode = OUTPUT;
}

/**
 * ピン番号取得
 */
int AttinyPin::getNum(){
	return this->num;
}

/**
 * 動作開始
 *
 * @param mode OUTPUT(0x00), INPUT(0x01), INPUT_PULLUP(0x02) (0x03はアナログ入力)
 */
void AttinyPin::begin(int mode){
	if(3 < mode){
		mode = OUTPUT;
	}
	if(mode==OUTPUT || mode==INPUT || mode==INPUT_PULLUP){
		pinMode(this->num,mode);
	}
	this->mode = mode;
}