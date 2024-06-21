#include "pci.hpp"

#include "asmfunc.h"

namespace {
	using namespace pci;

	// #@@range_begin(make_address)
	// CONFIG_ADDRESS용 32비트 정수를 생성
	uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr) {
		// 람다식: [캡쳐](파라미터 리스트){본체} // 간단한 함수를, 함수 내부에서 정의 하기 위함
		auto shl = [](uint32_t x, unsigned int bits) {
				return x << bits;
		};

		return shl(1, 31)	// enable bit -> CONFIG_DATA의 읽고 쓰기가 PCI 설정 공간에 전송
			| shl(bus, 16) // OR
			| shl(device, 11) // OR
			| shl(function, 8) // OR
			| (reg_addr & 0xfcu); // OR // 하위 2비트가 0이여야 한다. // 0xfc = 0b11111100
	}
	// #@@range_end(make_address)

	// #@@range_begin(add_device)
	// 발견한 PCI 디바이스를 devices에 추가한다
	// devices[num_device]에 정보를 쓰고, num_device++
	Error AddDevice(const Device& device) {
		if (num_device == devices.size()) {
			return MAKE_ERROR(Error::kFull);
		}

		devices[num_device] = device;
		++num_device;
		return MAKE_ERROR(Error::kSuccess);
	}
	// #@@range_end(add_device)

	Error ScanBus(uint8_t bus);

	// #@@range_begin(scan_function)
	// 지정된 fucntion을 devices에 추가한다.
	// 만약 PCI-PCI bridge라면, secondary bus에 대한 ScanBus를 실행한다.
	Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
		auto class_code = ReadClassCode(bus, device, function);
		auto header_type = ReadHeaderType(bus, device, function);
		Device dev{bus, device, function, header_type, class_code};
		if (auto err = AddDevice(dev)) {
			return err; // PCI 디바이스 수가 너무 많아서 넣을 수 없는 경우 -> 탐색 중지
		}

		if (class_code.Match(0x06u, 0x04u)) {
			// standard PCI-PCI bridge
			auto bus_numbers = ReadBusNumbers(bus, device, function);
			uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
			return ScanBus(secondary_bus);
		}

		return MAKE_ERROR(Error::kSuccess);
	}
	// #@@range_end(scan_function)

	// #@@range_begin(scan_device)
	// 유효한 function을 찾았다면 ScanFunction 실행
	Error ScanDevice(uint8_t bus, uint8_t device) {
		if (auto err = ScanFunction(bus, device, 0)) {
			return err;
		}
		if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) { // single function 처리
			return MAKE_ERROR(Error::kSuccess);
		}

		for (uint8_t function = 1; function < 8; ++function) { // multi function 처리
			if (ReadVendorId(bus, device, function) == 0xffffu) {
				continue;
			}
			if (auto err = ScanFunction(bus, device, function)) {
				return err;
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}
	// #@@range_end(scan_device)

	// #@@range_begin(scan_bus)
	// 유효한 디바이스를 찾았다면, ScanDevice를 실행
	Error ScanBus(uint8_t bus) {
		for (uint8_t device = 0; device < 32; ++device) {
			if (ReadVendorId(bus, device, 0) == 0xffffu) { // 0xffff: 유효하지 않은 값
				continue;
			}
			if (auto err = ScanDevice(bus, device)) {
				return err;
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}
	// #@@range_end(scan_bus)
}

namespace pci {
	// #@@range_begin(config_addr_data)
	void WriteAddress(uint32_t address) {
		IoOut32(kConfigAddress, address);
	}

	void WriteData(uint32_t value) {
		IoOut32(kConfigData, value);
	}

	uint32_t ReadData() {
		return IoIn32(kConfigData);
	}

	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() & 0xffffu;
	}
	// #@@range_end(config_addr_data)

	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() >> 16;
	}

	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x0c));
		return (ReadData() >> 16) & 0xffu;
	}

	ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x08));
		auto reg = ReadData();
		ClassCode cc;
		cc.base			 = (reg >> 24) & 0xffu;
		cc.sub				= (reg >> 16) & 0xffu;
		cc.interface	= (reg >> 8)	& 0xffu;
		return cc;
	}

	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x18));
		return ReadData();
	}

	bool IsSingleFunctionDevice(uint8_t header_type) {
		return (header_type & 0x80u) == 0;
	}

	// #@@range_begin(scan_all_bus)
	Error ScanAllBus() {
		num_device = 0;

		auto header_type = ReadHeaderType(0, 0, 0);
		// bus_0의 device_0은 host bridge -> CPU, PCI 디바이스 간 통신은 반드시 통과한다.
		// header_type = 8bit 정수 -> 7th bit가 1인 경우 "multi function"
		// = function 0 이외 기능을 가진 PCI device
		
		if (IsSingleFunctionDevice(header_type)) {
			// multi function device의 경우, host bridge 여러개 존재
			// function 0의 host bridge -> bus_0 담당,
			// function1의 host bridge -> bus_1 담당
			return ScanBus(0); // 각 bus 탐색
		}

		for (uint8_t function = 0; function < 8; ++function) {
			if (ReadVendorId(0, 0, function) == 0xffffu) {
				continue;
			}
			if (auto err = ScanBus(function)) {
				return err;
			}
		}
		return MAKE_ERROR(Error::kSuccess);
	}
	// #@@range_end(scan_all_bus)
	
	uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr) {
		WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
		return ReadData();
	}

	void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value) {
		WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
		WriteData(value);
	}

	WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index) {
		if (bar_index >= 6) {
			return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
		}

		const auto addr = CalcBarAddress(bar_index);
		const auto bar = ReadConfReg(device, addr);

		// 32 bit address
		if ((bar & 4u) == 0) {
			return {bar, MAKE_ERROR(Error::kSuccess)};
		}

		// 64 bit address
		if (bar_index >= 5) {
			return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
		}

		const auto bar_upper = ReadConfReg(device, addr + 4);
		return {
			bar | (static_cast<uint64_t>(bar_upper) << 32),
			MAKE_ERROR(Error::kSuccess)
		};
	}
}