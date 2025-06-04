#ifndef tiny1306_h
	#define tiny1306_h

	class TINY1306 {
		private:
			void printOneChar6x8(char chr, uint8_t row);
			uint8_t currentPage;
			uint8_t charScale;
			void setPageGo(uint8_t p);
			uint8_t oledAddr;
			uint8_t oledWidth; // OLEDの幅(ピクセル数)
			uint8_t oledPages; // OLEDのページ数
		public:
			TINY1306(uint8_t addr, uint8_t width, uint8_t page);
			void init();
			void display();
			void clear();
			void printlnS(String str);
			void println(char *chr);
			void setScale(uint8_t scl);
			void setPage(uint8_t p);
	};
#endif