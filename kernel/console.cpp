#include "console.hpp"

#include <cstring>
#include "font.hpp"
#include "layer.hpp"

// #@@range_begin(constructor)
Console::Console(const PixelColor& fg_color, const PixelColor& bg_color)
	: writer_{nullptr}, window_{}, fg_color_{fg_color}, bg_color_{bg_color},
		buffer_{}, cursor_row_{0}, cursor_column_{0}, layer_id_{0} { // buffer Null로 초기화
}

// #@@range_end(constructor)

// #@@range_begin(put_string)
void Console::PutString(const char* s) {
	while (*s) {
		if (*s == '\n') { // \n 만나면 Newline
			Newline();
		} else if (cursor_column_ < kColumns - 1) {
			WriteAscii(*writer_, Vector2D<int>{8 * cursor_column_, 16 * cursor_row_}, *s, fg_color_);			
			buffer_[cursor_row_][cursor_column_] = *s;
			++cursor_column_;
		}
		++s;
	}
	// #@@range_begin(draw_specific_layer)
	if (layer_manager) {
		layer_manager->Draw(layer_id_);
	}
	// #@@range_end(draw_specific_layer)
}
// #@@range_end(put_string)

// #@@range_begin(console_setwriter)
void Console::SetWriter(PixelWriter* writer) {
	if (writer == writer_) {
		return;
	}
	writer_ = writer;
	window_.reset();
	Refresh();
}
// #@@range_end(console_setwriter)

// #@@range_begin(set_window)
// for 콘솔이 PixelWriter 대신 Window를 사용하게 되는 타이밍에 적절히 전환
void Console::SetWindow(const std::shared_ptr<Window>& window) {
	if (window == window_) {
		return;
	}
	window_ = window;
	writer_ = window->Writer();
	Refresh(); // 콘솔 전체를 다시 그릴 필요
}
// #@@range_end(set_window)

// #@@range_begin(set_layer_id)
void Console::SetLayerID(unsigned int layer_id) {
	layer_id_ = layer_id;
}

unsigned int Console::LayerID() const {
	return layer_id_;
}
// #@@range_end(set_layer_id)

// #@@range_begin(newline)
void Console::Newline() {
	cursor_column_ = 0;
	if (cursor_row_ < kRows - 1) {
		++cursor_row_;
		return;
	}

	if (window_) {
		// 2nd ~ nth line 까지 이미지 이동
		Rectangle<int> move_src{{0, 16}, {8 * kColumns, 16 * (kRows - 1)}};
		window_->Move({0, 0}, move_src);
		// 마지막 줄 직사각형으로 렌더링
		FillRectangle(*writer_, {0, 16 * (kRows - 1)}, {8 * kColumns, 16}, bg_color_);
	} else {
		FillRectangle(*writer_, {0, 0}, {8 * kColumns, 16 * kRows}, bg_color_);
		for (int row = 0; row < kRows - 1; ++row) {
			memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
			WriteString(*writer_, Vector2D<int>{0, 16 * row}, buffer_[row], fg_color_);
		}
		memset(buffer_[kRows - 1], 0, kColumns + 1);
	}
}
// #@@range_end(newline)

// #@@range_begin(console_refresh)
void Console::Refresh() {
	FillRectangle(*writer_, {0, 0}, {8 * kColumns, 16 * kRows}, bg_color_);
	for (int row = 0; row < kRows; ++row) {
		WriteString(*writer_, Vector2D<int>{0, 16 * row}, buffer_[row], fg_color_);
	}
}
// #@@range_end(console_refresh)

Console* console;

namespace {
	char console_buf[sizeof(Console)];
}

void InitializeConsole() {
	console = new(console_buf) Console{
		kDesktopFGColor, kDesktopBGColor
	};
	console->SetWriter(screen_writer);
}