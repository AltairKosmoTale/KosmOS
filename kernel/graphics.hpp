#pragma once

// #@@range_begin(pixel_color_def)
#include "frame_buffer_config.hpp"

struct PixelColor {
	uint8_t r, g, b;
};
// #@@range_end(pixel_color_def)

// #@@range_begin(pixel_writer)
class PixelWriter {
 public:
	PixelWriter(const FrameBufferConfig& config) : config_{config} { // 생성자
	} // 프레임 버퍼의 구성 정보를 받아 클래스 멤버 변수 config_에 복사
	// FrameBufferConfig의 내용을 그대로 복사 X, 포인터를 복사하는 것
	// 즉, Write 할때, 구성 정보 전달 필요 X
	virtual ~PixelWriter() = default; // 소멸자
	virtual void Write(int x, int y, const PixelColor& c) = 0; // 순수 가상 함수 "= 0"

 protected:
	uint8_t* PixelAt(int x, int y) {
		return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
	}

 private:
	const FrameBufferConfig& config_;
};
// #@@range_end(pixel_writer)

// #@@range_begin(pixel_writer_def)
class RGBResv8BitPerColorPixelWriter : public PixelWriter {
 public:
	using PixelWriter::PixelWriter; // 부모 생성자 사용
	virtual void Write(int x, int y, const PixelColor& c) override; // override
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
 public:
	using PixelWriter::PixelWriter; // 부모 생성자 사용
	virtual void Write(int x, int y, const PixelColor& c) override; // override
};
// #@@range_end(pixel_writer_def)
