#include "console.hpp"

#include <cstring>
#include "font.hpp"

// #@@range_begin(constructor)
Console::Console(PixelWriter& writer,
	const PixelColor& fg_color, const PixelColor& bg_color)
	: writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color},
		buffer_{}, cursor_row_{0}, cursor_column_{0} { // buffer Null로 초기화
}
// #@@range_end(constructor)

// #@@range_begin(put_string)
void Console::PutString(const char* s) {
	while (*s) {
		if (*s == '\n') { // \n 만나면 Newline
			Newline();
		} else if (cursor_column_ < kColumns - 1) {
			WriteAscii(writer_, 8 * cursor_column_, 16 * cursor_row_, *s, fg_color_);
			buffer_[cursor_row_][cursor_column_] = *s;
			++cursor_column_;
		}
		++s;
	}
}
// #@@range_end(put_string)

// #@@range_begin(newline)
void Console::Newline() {
	cursor_column_ = 0;
	if (cursor_row_ < kRows - 1) {
		++cursor_row_;
	} else { // 커서가 최하단에 있을 때, 표시영역 전체를 올려서 스크롤 처리
		for (int y = 0; y < 16 * kRows; ++y) { // 1. 표시영역 전체를 배격으로 채우기
			for (int x = 0; x < 8 * kColumns; ++x) {
				writer_.Write(x, y, bg_color_);
			}
		}
		for (int row = 0; row < kRows - 1; ++row) { // 2. 한줄 씩 buffer_내용 갱신, 랜더링
			memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
			WriteString(writer_, 0, 16 * row, buffer_[row], fg_color_);
		}
		memset(buffer_[kRows - 1], 0, kColumns + 1);
	}
}
// #@@range_end(newline)
