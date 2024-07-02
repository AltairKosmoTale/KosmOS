#include "window.hpp"
#include "logger.hpp"

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

// #@@range_begin(window_drawto)
void Window::DrawTo(FrameBuffer& dst, Vector2D<int> position) {
	if (!transparent_color_) { // 투명색: writer.Write() 호출 // shadow 복사시 불투명
		dst.Copy(position, shadow_buffer_); // 면적이 작아서 큰 영향 X
		return;
	}

	const auto tc = transparent_color_.value();
	auto& writer = dst.Writer();
	for (int y = 0; y < Height(); ++y) {
		for (int x = 0; x < Width(); ++x) {
			const auto c = At(Vector2D<int>{x, y});
			if (c != tc) {
				writer.Write(position + Vector2D<int>{x, y}, c);
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

// #@@range_begin(move)
// FrmaeBuffer::Move()에 처리 위임
void Window::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
	shadow_buffer_.Move(dst_pos, src);
}
// #@@range_end(move)