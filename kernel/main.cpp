#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <vector>
// #@@range_begin(includes)
#include "frame_buffer_config.hpp"
#include "graphics.hpp" // image 관련 코드
#include "mouse.hpp"
#include "font.hpp" // font 관련 코드
#include "console.hpp"
#include "pci.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "queue.hpp"

#include "logger.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
// #@@range_end(includes)

const PixelColor kDesktopBGColor{42, 42, 42};
const PixelColor kDesktopFGColor{0, 240, 0};

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

// #@@range_begin(mouse_observer)
char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor* mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
	mouse_cursor->MoveRelative({displacement_x, displacement_y});
}
// #@@range_end(mouse_observer)

// #@@range_begin(switch_echi2xhci)
void SwitchEhci2Xhci(const pci::Device& xhc_dev) {
	bool intel_ehc_exist = false;
	for (int i = 0; i < pci::num_device; ++i) {
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
				0x8086 == pci::ReadVendorId(pci::devices[i])) {
			intel_ehc_exist = true;
			break;
		}
	}
	if (!intel_ehc_exist) {
		return;
	}

	uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc); // USB3PRM
	pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports); // USB3_PSSEN
	uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4); // XUSB2PRM
	pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports); // XUSB2PR
	Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
			superspeed_ports, ehci2xhci_ports);
}
// #@@range_end(switch_echi2xhci)

// #@@range_begin(xhci_handler)
usb::xhci::Controller* xhc;

// #@@range_begin(queue_message)
struct Message {
	enum Type {
		kInterruptXHCI,
	} type;
};

ArrayQueue<Message>* main_queue;
// #@@range_end(queue_message)

// #@@range_begin(xhci_handler)
__attribute__((interrupt)) // 컴파일러가 interrupt handler에 필요한 전 후 처리 삽입
void IntHandlerXHCI(InterruptFrame* frame) {
	/*while (xhc->PrimaryEventRing()->HasFront()) {
		if (auto err = ProcessEvent(*xhc)) {
			Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
					err.Name(), err.File(), err.Line());
		}
	} ProcessEvent 1회 처리시 -> USB로부터 수신한 데이터 해석, MouseObserver 호출, 렌더링 
	interrupt handler 처리 시간이 길어지면, interrupt 처리 동안 다른 interrupt 못받을 확률이 높아짐
	동적 메모리 사용하지 않는 Queue(FIFO)를 구현해 해결 */
	main_queue->Push(Message{Message::kInterruptXHCI});	
	NotifyEndOfInterrupt();
}
// #@@range_end(xhci_handler)

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
	
	const int kFrameWidth = frame_buffer_config.horizontal_resolution;
	const int kFrameHeight = frame_buffer_config.vertical_resolution;

	// #@@range_begin(draw_desktop)
	FillRectangle(*pixel_writer,
		{0, 0},
		{kFrameWidth, kFrameHeight - 30},
		kDesktopBGColor);
	FillRectangle(*pixel_writer,
		{0, kFrameHeight - 30},
		{kFrameWidth, 30},
		{1, 1, 1});

	console = new(console_buf) Console{
		*pixel_writer, kDesktopFGColor, kDesktopBGColor
	};
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
	// #@@range_end(draw_desktop)
	
	SetLogLevel(kWarn);

	// #@@range_begin(new_mouse_cursor)
	mouse_cursor = new(mouse_cursor_buf) MouseCursor{
		pixel_writer, kDesktopBGColor, {300, 200}
	};
	// #@@range_end(new_mouse_cursor)

	std::array<Message, 32> main_queue_data;
	ArrayQueue<Message> main_queue{main_queue_data};
	::main_queue = &main_queue;
  
	auto err = pci::ScanAllBus();
	Log(kDebug, "ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_device; ++i) {
		const auto& dev = pci::devices[i];
		auto vendor_id = pci::ReadVendorId(dev);
		auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
		Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n",
				dev.bus, dev.device, dev.function,
				vendor_id, class_code, dev.header_type);
	}

	// #@@range_begin(find_xhc)
	// Intel 제품을 우선으로 해서 xHC 찾기
	// 하나로 제한 -> 전체 구조를 단순하게 유지 가능 -> intel 제품쪽이 메인 controller 가능성 high
	pci::Device* xhc_dev = nullptr;
	for (int i = 0; i < pci::num_device; ++i) {
		// 0x0c: serial bus controller 전체, 0x03: usb controller, 0x30: xHCI
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
			xhc_dev = &pci::devices[i];

			if (0x8086 == pci::ReadVendorId(*xhc_dev)) { // intel 제품 벤더 ID
				break;
			}
		}
	}

	if (xhc_dev) {
		Log(kInfo, "xHC has been found: %d.%d.%d\n",
				xhc_dev->bus, xhc_dev->device, xhc_dev->function);
	}
	// #@@range_end(find_xhc)

	// #@@range_begin(load_idt)
	const uint16_t cs = GetCS(); // 현재 code segment 값 get
	// InterruptVector::kXHCI : 0x40 정의
	SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
							reinterpret_cast<uint64_t>(IntHandlerXHCI), cs); // 현재 code segment 값 지정
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
	// #@@range_end(load_idt)

	// #@@range_begin(configure_msi)
	// BSP (Bootstrap Processor) : 최초로 동작하는 Core
	const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
	pci::ConfigureMSIFixedDestination(
			*xhc_dev, bsp_local_apic_id, // bsp_local_apic_id = Destinamtion ID (해당 core에 대해)
			pci::MSITriggerMode::kLevel,
			pci::MSIDeliveryMode::kFixed,
			InterruptVector::kXHCI, 0); // 지정한 interrupt 발생시키라는 설정
	// #@@range_end(configure_msi)
	
	// #@@range_begin(read_bar)
	/* xHCI Spec상, xHC를 제어하는 레지스터: MMIO -> memory address space 어딘가에 register 존재
	MMIO address: PCI configuration Space의 BAR0에 기재 ->
	BAR0 ~ BAR5까지 6개의 32bit 폭 (64 표현시 2개 사용) ->
	ReadBar: 지정된 BAR, 후속 BAR 읽어 결합한 주소 반환 ->
	최종적으로 xhc_bar = xHC의 MMIO 지정 64bit address 설정 */
	const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
	Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
	// 하위 4bit 플래그 마스크 -> get MMIO Base address
	const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
	Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);
	// #@@range_end(read_bar)

	// #@@range_begin(init_xhc)
	// BAR0 값을 이용해 xHC 초기화 (xHC reset, 동작에 필요한 설정)
	usb::xhci::Controller xhc{xhc_mmio_base};

	if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
		// Intel 제품 2.0 EHCI(default), 3.0 xHCI 둘다 탑재 -> 설정 필요
		SwitchEhci2Xhci(*xhc_dev); 
	}
	{
		auto err = xhc.Initialize();
		Log(kDebug, "xhc.Initialize: %s\n", err.Name());
	}

	Log(kInfo, "xHC starting\n");
	xhc.Run(); // xHC 동작 (PC에 연결된 USB의 기기 인식 순차적으로 진행)
	// #@@range_end(init_xhc)

	::xhc = &xhc;
	__asm__("sti");
	
	// #@@range_begin(configure_port)
	usb::HIDMouseDriver::default_observer = MouseObserver;

	for (int i = 1; i <= xhc.MaxPorts(); ++i) {
		auto port = xhc.PortAt(i);
		Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

		if (port.IsConnected()) {
			if (auto err = ConfigurePort(xhc, port)) {
				Log(kError, "failed to configure port: %s at %s:%d\n",
						err.Name(), err.File(), err.Line());
				continue;
			}
		}
	}
	// #@@range_end(configure_port)
	
	// #@@range_begin(event_loop)
	while (true) {
		// #@@range_begin(get_front_message)
		__asm__("cli"); // CPU interrupt flag to 0 // 외부 interrupt 차단 (race condition 차단 효과 / 완벽 X)
		if (main_queue.Count() == 0) {
			__asm__("sti\n\thlt");
			continue;
		}

		Message msg = main_queue.Front();
		main_queue.Pop();
		__asm__("sti"); // CPU interrupt flag to 1 // 외부 interrupt 승인
		// #@@range_end(get_front_message)

		switch (msg.type) {
		case Message::kInterruptXHCI:
			while (xhc.PrimaryEventRing()->HasFront()) {
				if (auto err = ProcessEvent(xhc)) {
					Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
							err.Name(), err.File(), err.Line());
				}
			}
			break;
		default:
			Log(kError, "Unknown message type: %d\n", msg.type);
		}
	}
	// #@@range_end(event_loop)
}

extern "C" void __cxa_pure_virtual() {
	while (1) __asm__("hlt");
}