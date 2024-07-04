#pragma once
#include <cstdint>
#include <queue>
#include <vector>
#include "message.hpp"

void InitializeLAPICTimer(std::deque<Message>& msg_queue);
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

// #@@range_begin(timer)
class Timer {
 public:
	Timer(unsigned long timeout, int value);
	unsigned long Timeout() const { return timeout_; }
	int Value() const { return value_; }

 private:
	unsigned long timeout_; // 타임 아웃 시간
	int value_; // 타임아웃 시 송신할 값 저장
};
// #@@range_end(timer)

// #@@range_begin(timer_less)
// 타이머 우선순위 비교 -> 타임아웃이 길수록 우선순위가 낮다
inline bool operator<(const Timer& lhs, const Timer& rhs) {
	return lhs.Timeout() > rhs.Timeout();
}
// #@@range_end(timer_less)

// #@@range_begin(timermgr)
// 인터럽트 횟수를 세는 클래스
class TimerManager {
 public:
	TimerManager(std::deque<Message>& msg_queue);
	void AddTimer(const Timer& timer);
	void Tick();
	unsigned long CurrentTick() const { return tick_; }

 private:
	// tick_은 인터럽트 핸들러에서 변경 -> 외부 참조: 컴파일러가 값 고정시키는 것 우려: volatile
	volatile unsigned long tick_{0}; // 인터럽트 횟수 기억
	std::priority_queue<Timer> timers_{};
	std::deque<Message>& msg_queue_;
};
// #@@range_end(timermgr)

// #@@range_begin(lapic_freq)
extern TimerManager* timer_manager;
extern unsigned long lapic_timer_freq;
// 높일 수록 세밀 but OS 성능 떨어질 수 있음
// 1초에 100회 -> 10밀리초 마다 tick_ 증가 
const int kTimerFreq = 100; 
// #@@range_end(lapic_freq)

void LAPICTimerOnInterrupt();