#include "window.hpp"
#include "logger.hpp"
#include "font.hpp"

// #@@range_begin(window_ctor)
Window::Window(int width, int height, PixelFormat shadow_format) : width_{width}, height_{height} {
	data_.resize(height);
	for (int y = 0; y < height; ++y) {
		data_[y].resize(width);
	}
	FrameBufferConfig config{};
	config.frame_buffer = nullptr;
	config.horizontal_resolution = width;
	config.vertical_resolution = height;
	config.pixel_format = shadow_format;
	if (auto err = shadow_buffer_.Initialize(config)) { // 메모리 영역 확보
		Log(kError, "failed to initialize shadow buffer: %s at %s:%d\n",
			err.Name(), err.File(), err.Line());
	}	
}
// #@@range_end(window_ctor)

// #@@range_begin(drawto)
void Window::DrawTo(FrameBuffer& dst, Vector2D<int> pos, const Rectangle<int>& area) {
	if (!transparent_color_) { // 투명색: writer.Write() 호출 // shadow 복사시 불투명
		Rectangle<int> window_area{pos, Size()};
		Rectangle<int> intersection = area & window_area;
		dst.Copy(intersection.pos, shadow_buffer_, {intersection.pos - pos, intersection.size});
		return;
	}
 
// #@@range_end(drawto)
	const auto tc = transparent_color_.value();
	auto& writer = dst.Writer();
	// #@@range_begin(limit_draw_area)
	for (int y = std::max(0, 0 - pos.y);
			 y < std::min(Height(), writer.Height() - pos.y);
			 ++y) {
		for (int x = std::max(0, 0 - pos.x);
				 x < std::min(Width(), writer.Width() - pos.x);
				 ++x) {
	// #@@range_end(limit_draw_area)
			const auto c = At(Vector2D<int>{x, y});
			if (c != tc) {
				writer.Write(pos + Vector2D<int>{x, y}, c);
			}
		}
	}
}

// #@@range_end(window_drawto)

// #@@range_begin(window_settc)
void Window::SetTransparentColor(std::optional<PixelColor> c) {
	transparent_color_ = c;
}
// #@@range_end(window_settc)

Window::WindowWriter* Window::Writer() {
	return &writer_;
}

// #@@range_begin(write)
const PixelColor& Window::At(Vector2D<int> pos) const{
	return data_[pos.y][pos.x];
}

void Window::Write(Vector2D<int> pos, PixelColor c) {
	data_[pos.y][pos.x] = c;
	shadow_buffer_.Writer().Write(pos, c);
}
// #@@range_end(write)

int Window::Width() const {
	return width_;
}

int Window::Height() const {
	return height_;
}

Vector2D<int> Window::Size() const {
	return {width_, height_};
}

// #@@range_begin(move)
// FrmaeBuffer::Move()에 처리 위임
void Window::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
	shadow_buffer_.Move(dst_pos, src);
}
// #@@range_end(move)

// #@@range_begin(utils)
namespace {
	const int kCloseButtonWidth = 16;
	const int kCloseButtonHeight = 14;
	const char close_button[kCloseButtonHeight][kCloseButtonWidth + 1] = {
		"...............@",
		".:::::::::::::$@",
		".:::::::::::::$@",
		".:::@@::::@@::$@",
		".::::@@::@@:::$@",
		".:::::@@@@::::$@",
		".::::::@@:::::$@",
		".:::::@@@@::::$@",
		".::::@@::@@:::$@",
		".:::@@::::@@::$@",
		".:::::::::::::$@",
		".:::::::::::::$@",
		".$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@",
	};

	constexpr PixelColor ToColor(uint32_t c) {
		return {
			static_cast<uint8_t>((c >> 16) & 0xff),
			static_cast<uint8_t>((c >> 8) & 0xff),
			static_cast<uint8_t>(c & 0xff)
		};
	}
}
// #@@range_end(utils)

// #@@range_begin(draw_window)
void DrawWindow(PixelWriter& writer, const char* title) {
	// 매번 writer 지정 X // PixelWriter: 추상 클래스 -> 복사 불가, 참조 캡처 가능
	auto fill_rect = [&writer](Vector2D<int> pos, Vector2D<int> size, uint32_t c) {
		FillRectangle(writer, pos, size, ToColor(c));
	};
	const auto win_w = writer.Width();
	const auto win_h = writer.Height();

	fill_rect({0, 0}, {win_w, 1}, 0xc6c6c6); // 연한 회색
	fill_rect({1, 1}, {win_w - 2, 1}, 0xffffff); // 흰색
	fill_rect({0, 0}, {1, win_h}, 0xc6c6c6); // 연한 회색
	fill_rect({1, 1}, {1, win_h - 2}, 0xffffff); // 흰색
	fill_rect({win_w - 2, 1}, {1, win_h - 2}, 0x848484); // 회색
	fill_rect({win_w - 1, 0}, {1, win_h}, 0x000000); // 검정색
	fill_rect({2, 2}, {win_w - 4, win_h - 4}, 0xc6c6c6); // 연한 회색
	fill_rect({3, 3}, {win_w - 6, 18},	0x141414); // 짙은 회색
	fill_rect({1, win_h - 2}, {win_w - 2, 1}, 0x848484); // 회색
	fill_rect({0, win_h - 1}, {win_w, 1}, 0x000000); // 검정색

	WriteString(writer, {24, 4}, title, ToColor(0xffffff)); // 흰색

	for (int y = 0; y < kCloseButtonHeight; ++y) {
		for (int x = 0; x < kCloseButtonWidth; ++x) {
			PixelColor c = ToColor(0xffffff);
			if (close_button[y][x] == '@') {
				c = ToColor(0x000000);
			} else if (close_button[y][x] == '$') {
				c = ToColor(0x848484);
			} else if (close_button[y][x] == ':') {
				c = ToColor(0xc6c6c6);
			}
			writer.Write({win_w - 5 - kCloseButtonWidth + x, 5 + y}, c);
		}
	}
}
// #@@range_end(draw_window)