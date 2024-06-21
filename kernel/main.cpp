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

#include "logger.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
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
		/* 
		void* operator new(size_t size, void* buf) {
			return buf;
		}
		void operator delete(void* obj) noexcept {
		}
// #@@range_end(placement_new) */

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

	// #@@range_begin(receive_event)
	while (1) {
		// data -> event 형태로 xHC에 Stack -> ProcessEvent로 처리 (polling 기법)
		if (auto err = ProcessEvent(xhc)) { 
			Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
					err.Name(), err.File(), err.Line());
		}
	}
	// #@@range_end(receive_event)

	while (1) __asm__("hlt");
}

extern "C" void __cxa_pure_virtual() {
	while (1) __asm__("hlt");
}