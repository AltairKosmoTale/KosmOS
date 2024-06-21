#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"

namespace pci {
	// #@@range_begin(config_addr)
	const uint16_t kConfigAddress = 0x0cf8; // CONFIG_ADDRESS 레지스터의 IO 포트 주소 (상수)
	const uint16_t kConfigData = 0x0cfc; // CONFIG_DATA 레지스터의 IO 포트 주소 (상수)
	// #@@range_end(config_addr)

	// #@@range_begin(class_code)
	struct ClassCode {
		uint8_t base, sub, interface;
		bool Match(uint8_t b) { return b == base; }
		bool Match(uint8_t b, uint8_t s) { return Match(b) && s == sub; }
		bool Match(uint8_t b, uint8_t s, uint8_t i) {
			return Match(b, s) && i == interface;
		}
	};

	struct Device {
		uint8_t bus, device, function, header_type;
		ClassCode class_code;
	};
	// #@@range_end(class_code)
	
	void WriteAddress(uint32_t address);
	void WriteData(uint32_t value);
	uint32_t ReadData();
	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
	ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

	inline uint16_t ReadVendorId(const Device& dev) {
		return ReadVendorId(dev.bus, dev.device, dev.function);
	}
	uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr);
	void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value);
	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);
	bool IsSingleFunctionDevice(uint8_t header_type);

	// #@@range_begin(var_devices)
	// ScanAllBus()로 발견된 PCI device 일람
	inline std::array<Device, 32> devices;
	// devices의 유효한 요소 수
	inline int num_device;
	/* 1. PCI 디바이스 전부 탐색 -> devices 저장
	2. 버스 0에서부터 재귀적으로 PCI 디바이스 탐색 -> devices의 선두에 채워 쓰기
	3. 발견한 디바이스 수를 num_devices에 저장 */
	Error ScanAllBus();
	// #@@range_end(var_devices)

	constexpr uint8_t CalcBarAddress(unsigned int bar_index) {
		return 0x10 + 4 * bar_index;
	}

	WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index);
}
