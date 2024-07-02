#include "timer.hpp"

namespace {
	const uint32_t kCountMax = 0xffffffffu;
	// 인터럽트 발생 방법 설정
	volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
	volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
	volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
	// 카운터의 감소 스피드 설정
	volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);
}

void InitializeLAPICTimer() {
	divide_config = 0b1011; // divide 1:1
	// 인터럽스 사용 X, 1회 타임 아웃 되면 타이머 동작 종료
	lvt_timer = (0b001 << 16) | 32; // masked, one-shot
}

void StartLAPICTimer() {
	initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed() {
	return kCountMax - current_count;
}

void StopLAPICTimer() {
	initial_count = 0;
}
