// #@@range_begin(includes)
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <vector>
#include <deque>
#include <limits>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "usb/xhci/xhci.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "message.hpp"
#include "timer.hpp"
#include "acpi.hpp"
// #@@range_end(includes)

// #@@range_begin(measure_printk)
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
// #@@range_end(measure_printk)

std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;
void InitializeMainWindow() {
	main_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
	DrawWindow(*main_window->Writer(), "Hello Window");

	main_window_layer_id = layer_manager->NewLayer()
		.SetWindow(main_window)
		.SetDraggable(true)
		.Move({300, 100})
		.ID();

	layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

std::deque<Message>* main_queue; // 인터럽트

// #@@range_begin(main_new_stack)
alignas(16) uint8_t kernel_main_stack[1024 * 1024]; // 배열 시작 주소 16의 배수로 배치 보장, 1byte 요소, 1MB 스택

// #@@range_begin(main_function)
extern "C" void KernelMainNewStack(
	const FrameBufferConfig& frame_buffer_config_ref,
	const MemoryMap& memory_map_ref,
	const acpi::RSDP& acpi_table) {
	MemoryMap memory_map{memory_map_ref};
// #@@range_end(main_new_stack)
	InitializeGraphics(frame_buffer_config_ref);
	InitializeConsole();
	// here KosmOS Ascii
	printk("Welcome to KosmOS!\n");
	printk(" /$$   /$$                                    /$$$$$$   /$$$$$$ \n");
	printk("| $$  /$$/                                   /$$__  $$ /$$__  $$\n");
	printk("| $$ /$$/   /$$$$$$   /$$$$$$$ /$$$$$$/$$$$ | $$  : $$| $$  :__/\n");
	printk("| $$$$$/   /$$__  $$ /$$_____/| $$_  $$_  $$| $$  | $$|  $$$$$$ \n");
	printk("| $$  $$  | $$  : $$|  $$$$$$ | $$ : $$ : $$| $$  | $$ :____  $$\n");
	printk("| $$:  $$ | $$  | $$ :____  $$| $$ | $$ | $$| $$  | $$ /$$  : $$\n");
	printk("| $$ :  $$|  $$$$$$/ /$$$$$$$/| $$ | $$ | $$|  $$$$$$/|  $$$$$$/\n");
	printk("|__/  :__/ :______/ |_______/ |__/ |__/ |__/ :______/  :______/ \n");
	SetLogLevel(kWarn);
	// #@@range_end(draw_desktop)

	InitializeSegmentation();
	InitializePaging();
	InitializeMemoryManager(memory_map);
	::main_queue = new std::deque<Message>(32);
	InitializeInterrupt(main_queue);

	InitializePCI();
	usb::xhci::Initialize();

	InitializeLayer();
	InitializeMainWindow();
	InitializeMouse();
	layer_manager->Draw({{0, 0}, ScreenSize()});
	// #@@range_begin(add_sample_timer)
	acpi::Initialize(acpi_table);
	InitializeLAPICTimer(*main_queue);
	timer_manager->AddTimer(Timer(200, 2));
	timer_manager->AddTimer(Timer(600, -1));
	// #@@range_end(add_sample_timer)
// #@@range_end(main_function)	
	
	// #@@range_begin(make_counter)
	char str[128];
	// #@@range_end(make_counter)
	
	// #@@range_begin(event_loop)
	while (true) {
		// #@@range_begin(show_count)
		// #@@range_begin(draw_window_layer)
		__asm__("cli"); // tick_ 값 변경을 방지 하기 위한 cli & sti // 비동기 처리는 어려운 주제
		const auto tick = timer_manager->CurrentTick();
		__asm__("sti");
		sprintf(str, "%010lu", tick);
		FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
		WriteString(*main_window->Writer(), {24, 28}, str, {0, 0, 0});
		layer_manager->Draw(main_window_layer_id); // 1회 이후, Draw(Layer_id)
		
		// #@@range_begin(get_front_message)
		__asm__("cli"); // CPU interrupt flag to 0 // 외부 interrupt 차단 (race condition 차단 효과 / 완벽 X)
		if (main_queue->size() == 0) {
			__asm__("sti\n\thlt");
			continue;
		}
		// #@@range_end(draw_window_layer)
		// #@@range_end(show_count)
		
		Message msg = main_queue->front();
		main_queue->pop_front();
		__asm__("sti"); // CPU interrupt flag to 1 // 외부 interrupt 승인
		// #@@range_end(get_front_message)

		// #@@range_begin(process_event)
		switch (msg.type) {
		case Message::kInterruptXHCI:
			usb::xhci::ProcessEvents();
			break;
		// #@@range_begin(timer_event)
		case Message::kTimerTimeout:
			printk("Timer: timeout = %lu, value = %d\n",
					msg.arg.timer.timeout, msg.arg.timer.value);
			if (msg.arg.timer.value > 0) {
				timer_manager->AddTimer(Timer(
						msg.arg.timer.timeout + 100, msg.arg.timer.value + 1));
			}
			break;
		// #@@range_end(timer_event)
		// #@@range_end(process_event)
		default:
			Log(kError, "Unknown message type: %d\n", msg.type);
		}
	}
	// #@@range_end(event_loop)
}

extern "C" void __cxa_pure_virtual() {
	while (1) __asm__("hlt");
}