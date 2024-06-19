#include <cstdint>
#include <cstddef>
#include "frame_buffer_config.hpp"

/* 픽셀 데이터 형식에 의존 하지 않는 "픽셀 렌더링 인터페이스",
"픽셀 데이터 형식에 따라 실제 렌더링의 구현" 분리 */

// #@@range_begin(write_pixel)
struct PixelColor {
	uint8_t r, g, b;
};

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

// #@@range_begin(derived_pixel_writer)
class RGBResv8BitPerColorPixelWriter : public PixelWriter {
 public:
	using PixelWriter::PixelWriter; // 부모 생성자 사용
	// Override
	virtual void Write(int x, int y, const PixelColor& c) override {
		auto p = PixelAt(x, y);
		p[0] = c.r;
		p[1] = c.g;
		p[2] = c.b;
	}
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
 public:
	using PixelWriter::PixelWriter; // 부모 생성자 사용
	// Override
	virtual void Write(int x, int y, const PixelColor& c) override {
		auto p = PixelAt(x, y);
		p[0] = c.b;
		p[1] = c.g;
		p[2] = c.r;
	}
};
// #@@range_end(derived_pixel_writer)

// #@@range_begin(placement_new)
// 파라미터를 취하는 new: "displacement new"
/* 일반적인 new: 파라미터 X, 힙영역에 생성
new, malloc() 차이: 클래스 생성자의 호출 여부
malloc(): 초기화 되지 않은 데이터가 들어간 상태로 메모리 영역 반환
new: 메모리 역역 확보 후 생성자 호출: 컴파일러 -> 생성자 호출을 위한 명령어를 삽입
- 프로그래머가 명시적으로 생성자 호출 X: new 연산자 사용
- 힙 영역에 확보 -> OS에서 메모리 지원 필요
메모리 X -> displacement new: 메모리 영역 확보 X, 파라미터로 지정한 영역상에 인스턴스 생성
: 메모리 관리 필요 없는 "배열" 사용시 원하는 크기 메모리 영역 확보 가능 */

// operator overloading -> 새로운 new 생성시, 새로운 delete pair 필요
void* operator new(size_t size, void* buf) {
	return buf;
}

void operator delete(void* obj) noexcept {
}
// #@@range_end(placement_new)

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

// #@@range_begin(call_pixel_writer)
extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
	switch (frame_buffer_config.pixel_format) {
		case kPixelRGBResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
			RGBResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
		case kPixelBGRResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)
			BGRResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
	}
	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
			pixel_writer->Write(x, y, {255, 255, 255});
		}
	}
	for (int x = 0; x < 200; ++x) {
		for (int y = 0; y < 200; ++y) {
			pixel_writer->Write(x, y, {100, 100, 100});
		}
	}
	while (1) __asm__("hlt");
}
// #@@range_end(call_pixel_writer)