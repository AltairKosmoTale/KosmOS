#include <cstdint>
#include <cstddef>
#include <cstdio>
// #@@range_begin(includes)
#include "frame_buffer_config.hpp"
#include "graphics.hpp" // image 관련 코드
#include "font.hpp" // font 관련 코드
#include "console.hpp"
// #@@range_end(includes)

/* 픽셀 데이터 형식에 의존 하지 않는 "픽셀 렌더링 인터페이스",
"픽셀 데이터 형식에 따라 실제 렌더링의 구현" 분리 */

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

// #@@range_begin(console_buf)
char console_buf[sizeof(Console)];
Console* console;
// #@@range_end(console_buf)

// #@@range_begin(printk)
int printk(const char* format, ...) {
	va_list ap;
	int result;
	char s[1024];

	va_start(ap, format);
	result = vsprintf(s, format, ap);
	va_end(ap);

	console->PutString(s);
	return result;
}
// #@@range_end(printk)

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
	/* Test1. Image Rendering, Font Rendering, Use Sprintf
	// #@@range_begin(Test1) : 출력 결과: draw gray square, write all font, print string with sprinft
	
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

	// #@@range_begin(write_fonts)
	int i = 0;
	for (char c = '!'; c <= '~'; ++c, ++i) {
		WriteAscii(*pixel_writer, 8 * i, 50, c, {0, 0, 0}); // 글자 간격 8이니까, 8띄어서 출력 시도
	}
	WriteString(*pixel_writer, 0, 66, "Hello, KosmOS!", {0, 0, 0}); // 글자 높이는 16 간격 (50+16=66)
	// #@@range_end(write_fonts)	

	// #@@range_begin(sprintf)
	char buf[128];
	sprintf(buf, "1 + 1 = %d", 1 + 1);
	WriteString(*pixel_writer, 0, 82, buf, {0, 0, 0}); // 글자 높이는 16 간격 (66+16=82)
	// #@@range_end(sprintf) 
	*/ // #@@range_end(Test1) 
	
	// /* Test2. Scroll, printk function
	// #@@range_begin(Test2) : 출력 결과: write 24 lines -> scroll 3 lines -> 3~26 left
	
	// #@@range_begin(new_console)
	// displacement new 사용 -> 인스턴스 생성
	console = new(console_buf) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};
	// #@@range_end(new_console)
	
	// #@@range_begin(use_printk)
	for (int i = 0; i < 27; ++i) { // (kRows < 25) : 24 줄 작성 가능, line 0~2 scroll -> 3~26 left
		printk("printk: %d\n", i);
	}
	// #@@range_end(use_printk)
	// */ // #@@range_end(Test2)
	
	while (1) __asm__("hlt");
}
// #@@range_end(call_pixel_writer)