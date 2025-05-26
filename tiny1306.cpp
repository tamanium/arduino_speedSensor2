#include "Wire.h"
#include "font6x8.h"
#include "tiny1306.h"

//スケールアップのために使う関数
uint8_t extractBlock(uint8_t value, int blockIndex, int factor) {
 uint8_t result = 0b00000000;
 for (int i = 0; i < 8; i++)
 {
  uint8_t shift = (8 * blockIndex + i) / factor;
  uint8_t temp = (value >> shift) & 0b01;
  result = result | temp << i;
 }
 return result;
}

TINY1306::TINY1306(uint8_t addr, uint8_t width, uint8_t page)
{
 oledAddr = addr;//0x3C; //重要なのはこれだけ。
 oledWidth = width;//実は使わないけど入れてある [128]
 oledPages = page;//これも現状では使わない。[8] 128x64ディスプレイの場合
 charScale = 1;
}

void TINY1306::init()
{
 Wire.beginTransmission(oledAddr);
 Wire.write(0x00); // 複数バイトコマンドタグ
 Wire.write(0xC8); // [C0]/C8:上下の描画方向
 Wire.write(0xA1); // [A0]/A1:左右の描画方向
 Wire.write(0xA8); Wire.write(0x3F); // 画面の解像度 [3F]:64Line / 1F:32Line
 Wire.write(0xDA); Wire.write(0x12); // [12]:Sequential / 02:Alternative
 Wire.write(0xD3); Wire.write(0x00); // 縦方向のオフセット
 Wire.write(0x40); // 縦方向の描画開始位置
 Wire.write(0xA6); // 表示方法 - A7にすると画面反転
 Wire.write(0x8D); Wire.write(0x14);
 Wire.endTransmission();

 /*参考
       ゆるく楽しむ プログラミング＆電子工作 様
       !http://try3dcg.world.coocan.jp/note/i2c/ssd1306.html
*/
}

void TINY1306::setPageGo(uint8_t p)
{
 Wire.beginTransmission(oledAddr);
 Wire.write(0x00);
 Wire.write(0xB0 | (p & 0x07));
 Wire.write(0x00);
 Wire.write(0x10);
 Wire.endTransmission();
}

void TINY1306::setPage(uint8_t p)
{
 currentPage = p;
}

void TINY1306::setScale(uint8_t scale)
{
 charScale = scale;
}

void TINY1306::display()
{
 Wire.beginTransmission(oledAddr);
 Wire.write(0x80);
 Wire.write(0xAF);
 Wire.endTransmission();
}

void TINY1306::clear()
{
 for (uint8_t m = 0; m < oledPages; m++) {
  setPageGo(m);
  Wire.beginTransmission(oledAddr);
  Wire.write(0x40);
  for (uint8_t x = 0; x < oledWidth; x++) {
   if (Wire.write(0x00) == 0) {
    Wire.endTransmission();
    Wire.beginTransmission(oledAddr);
    Wire.write(0x40);
    Wire.write(0x00);
   }
  }
  Wire.endTransmission();
 }
}

void TINY1306::printOneChar6x8(char chr, uint8_t row)
{
 int i = ((int)chr - 32) * 6; //ASCIIコードから変換

 Wire.beginTransmission(oledAddr);
 Wire.write(0x40);

 for (int j = 0; j < 6 * charScale; j++)
 {
  uint8_t font = extractBlock(font6x8[i + j / charScale], row, charScale);
  Wire.write(font);
 }
 //128x32液晶の場合は次のように書き換えてください
 
 /*
 for (int j = 0; j < 6 * charScale / 2; j++)
 {
  uint8_t font = extractBlock(font6x8[i + 2 * j / charScale], row, charScale);
  Wire.write(font);
 }
*/
 Wire.endTransmission();
}

void TINY1306::println(char *chr)
{
 for (uint8_t j = 0; j < charScale; j++)
 {
  setPageGo(currentPage + j);
  for (uint8_t i = 0; i < strlen(chr); i++)
  {
   printOneChar6x8(chr[i], j);
  }
 }
}

void TINY1306::printlnS(String str)
{
  uint8_t len = str.length() + 1; //この+1がないと末尾が消える
  char chr[len];
  str.toCharArray(chr, len);
  println(chr);

/*参考
  HatenaBlog 戯言日記 様
  !https://doubtpad.hatenablog.com/entry/2020/09/28/015028
*/
}