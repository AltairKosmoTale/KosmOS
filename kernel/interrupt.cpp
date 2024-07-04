#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "timer.hpp"

// #@@range_begin(idt_array)
std::array<InterruptDescriptor, 256> idt;
// #@@range_end(idt_array)

// #@@range_begin(set_idt_entry)
void SetIDTEntry(InterruptDescriptor& desc, InterruptDescriptorAttribute attr, 
	uint64_t offset,uint16_t segment_selector) { // interrupt handler address는 3개로 분할해 설정 필요
		desc.attr = attr;
		desc.offset_low = offset & 0xffffu;
		desc.offset_middle = (offset >> 16) & 0xffffu;
		desc.offset_high = offset >> 32;
		desc.segment_selector = segment_selector;
}
// #@@range_end(set_idt_entry)

// #@@range_begin(notify_eoi)
void NotifyEndOfInterrupt() { // interrupt의 종료를 CPU에 알림
	// 0xfee00000 ~ 0xfee00400: 1024 byte범위: CPU의 레지스터가 놓여있음
	// volatile: 휘발성 변수 -> 최적화 제외 다상 -> 꼭 0xfee000b0에 작성하겠다는 의사표현
	volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
	*end_of_interrupt = 0;
}
// #@@range_end(notify_eoi)

// #@@range_begin(int_handler)
namespace {
	std::deque<Message>* msg_queue;

	__attribute__((interrupt)) // 컴파일러가 interrupt handler에 필요한 전 후 처리 삽입
	void IntHandlerXHCI(InterruptFrame* frame) {
		msg_queue->push_back(Message{Message::kInterruptXHCI});
		NotifyEndOfInterrupt();
	}
	
	// #@@range_begin(int_handler)
	__attribute__((interrupt))
	void IntHandlerLAPICTimer(InterruptFrame* frame) {
		LAPICTimerOnInterrupt();
		NotifyEndOfInterrupt();	// 처리안해 주면 두 번째 이후 인터럽트 도착 X
	}
	// #@@range_end(int_handler)
}
// #@@range_end(int_handler)

// #@@range_begin(register_handler)
void InitializeInterrupt(std::deque<Message>* msg_queue) {
	::msg_queue = msg_queue;

	// InterruptVector::kXHCI : 0x40 정의
	SetIDTEntry(idt[InterruptVector::kXHCI],
		MakeIDTAttr(DescriptorType::kInterruptGate, 0),
		reinterpret_cast<uint64_t>(IntHandlerXHCI),
		kKernelCS); // 현재 code segment 값 지정
	SetIDTEntry(idt[InterruptVector::kLAPICTimer],
		MakeIDTAttr(DescriptorType::kInterruptGate, 0),
		reinterpret_cast<uint64_t>(IntHandlerLAPICTimer),
		kKernelCS);		
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}
// #@@range_end(register_handler)