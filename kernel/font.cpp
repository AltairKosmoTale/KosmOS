#include "font.hpp"

// 가로 8픽셀, 세로 16픽셀

// #@@range_begin(font_text_bin)
extern const uint8_t _binary_font_text_bin_start;
extern const uint8_t _binary_font_text_bin_end;
extern const uint8_t _binary_font_text_bin_size;

const uint8_t* GetFont(char c) {
	auto index = 16 * static_cast<unsigned int>(c);
	if (index >= reinterpret_cast<uintptr_t>(&_binary_font_text_bin_size)) {
		return nullptr;
	}
	return &_binary_font_text_bin_start + index;
}
// #@@range_end(font_text_bin)

// #@@range_begin(write_ascii)
void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color) {
	const uint8_t* font = GetFont(c);
	if (font == nullptr) {
		return;
	}
	for (int dy = 0; dy < 16; ++dy) {
		for (int dx = 0; dx < 8; ++dx) {
			if ((font[dy] << dx) & 0x80u) {
			// dy[]를 dx에 맞게 Left Shift하여 맨 왼쪽값이 1이면 출력
			// 0x80u = 0b10000000
				writer.Write(x + dx, y + dy, color);
			}
		}
	}
}
// #@@range_end(write_ascii)

// #@@range_begin(write_string)
void WriteString(PixelWriter& writer, int x, int y, const char* s, const PixelColor& color) {
	for (int i = 0; s[i] != '\0'; ++i) {
		WriteAscii(writer, x + 8 * i, y, s[i], color);
	}
}
// #@@range_end(write_string)