#pragma once

#include "graphics.hpp"

/* 화면 하단 도달시, 한줄 씩 스크롤 기능 필요
문자열 처음부터 살펴보다가, 줄바꿈 문자열을 만나면, (X좌표 = 0, Y좌표 += 16)
스크롤: Complex: 화면을 한번 채워서 메시지 지우고, 한줄씩 내린 후, 다음 메시지 랜더링
	-> 스크롤 전까지 표시된 메시지 기억 필요 */
class Console {
	public:
		static const int kRows = 25, kColumns = 80;
		Console(PixelWriter& writer, const PixelColor& fg_color, const PixelColor& bg_color);
		void PutString(const char* s);

	private:
		void Newline();
		PixelWriter& writer_;
		const PixelColor fg_color_, bg_color_;
		char buffer_[kRows][kColumns + 1];
		int cursor_row_, cursor_column_;
};