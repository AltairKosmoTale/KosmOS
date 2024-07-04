#include "acpi.hpp"

#include <cstring>
#include <cstdlib>
#include "asmfunc.h"
#include "logger.hpp"

// #@@range_begin(utils)
namespace {

// 바이트 배열 + 그 사이즈를 갖고 바이트 단위 총합 계산 기능
template <typename T>

uint8_t SumBytes(const T* data, size_t bytes) {
	return SumBytes(reinterpret_cast<const uint8_t*>(data), bytes);
}

// T가 uint8_t 인경우 함수 사용하는 측에서 캐스트 없이 호출 가능
template <>
uint8_t SumBytes<uint8_t>(const uint8_t* data, size_t bytes) {
	uint8_t sum = 0;
	for (size_t i = 0; i < bytes; ++i) {
		sum += data[i];
	}
	return sum;
}

} // namespace
// #@@range_end(utils)

namespace acpi {

// #@@range_begin(isvalid_rsdp)
bool RSDP::IsValid() const {
	// signature NUL 문자로 끝나지 않아 strcmp 불가 / strncmp로 문자 수 지정해서 비교
	if (strncmp(this->signature, "RSD PTR ", 8) != 0) {
		Log(kDebug, "invalid signature: %.8s\n", this->signature);
		return false;
	}
	if (this->revision != 2) { // xsdt_address를 읽으려면 revision이 2여야 한다.
		Log(kDebug, "ACPI revision must be 2: %d\n", this->revision);
		return false;
	}
	// checksum 계산 하위 1바이트만 남기기에, 오버플로우 무관 (1byte 단위)
	if (auto sum = SumBytes(this, 20); sum != 0) {
		Log(kDebug, "sum of 20 bytes must be 0: %d\n", sum);
		return false;
	}
	if (auto sum = SumBytes(this, 36); sum != 0) {
		Log(kDebug, "sum of 36 bytes must be 0: %d\n", sum);
		return false;
	}
	return true;
}
// #@@range_end(isvalid_rsdp)

// #@@range_begin(header_isvalid)
bool DescriptionHeader::IsValid(const char* expected_signature) const {
	// signature 확인, checksum 계산
	if (strncmp(this->signature, expected_signature, 4) != 0) {
		Log(kDebug, "invalid signature: %.4s\n", this->signature);
		return false;
	}
	if (auto sum = SumBytes(this, this->length); sum != 0) {
		Log(kDebug, "sum of %u bytes must be 0: %d\n", this->length,	sum);
		return false;
	}
	return true;
}
// #@@range_end(header_isvalid)

// #@@range_begin(xsdt)
// XSDT는 디스크립션 헤더 뒤에 각 데이터 구조에 대한 어드레스를 나열한 구조
// -> 연산자 "[]" 정의
const DescriptionHeader& XSDT::operator[](size_t i) const {
	auto entries = reinterpret_cast<const uint64_t*>(&this->header + 1);
	return *reinterpret_cast<const DescriptionHeader*>(entries[i]);
}

size_t XSDT::Count() const { // XSDT가 유지하는 데이터 구조의 어드레스 수
	return (this->header.length - sizeof(DescriptionHeader)) / sizeof(uint64_t);
}
// #@@range_end(xsdt)

const FADT* fadt; // 글로벌 변수

// #@@range_begin(wait_ms)
// 지정한 밀리초가 경과하기 기다리는 함수
void WaitMilliseconds(unsigned long msec) {
	const bool pm_timer_32 = (fadt->flags >> 8) & 1;
	const uint32_t start = IoIn32(fadt->pm_tmr_blk); // fadt->pm_tmr_blk (IO 포트에 존재)
	uint32_t end = start + kPMTimerFreq * msec / 1000; // kPMTimerFreq = 3579545
	if (!pm_timer_32) {
		end &= 0x00ffffffu;
	}

	if (end < start) { // overflow
		// 카운터가 overflow해서 0으로 돌아올 때 까지 기다리는 부분 실행 되는 곳
		while (IoIn32(fadt->pm_tmr_blk) >= start);
	}
	while (IoIn32(fadt->pm_tmr_blk) < end); // ACPI PM 타이머가 지정한 밀리초 경과하길 기다림
}
// #@@range_end(wait_ms)

// #@@range_begin(initialize_acpi)
void Initialize(const RSDP& rsdp) {
	if (!rsdp.IsValid()) {
		Log(kError, "RSDP is not valid\n");
		exit(1);
	}
	// XSDT 데이터 구조: rsdp.xsdt_address에 기록된 물리 주소에 존재
	const XSDT& xsdt = *reinterpret_cast<const XSDT*>(rsdp.xsdt_address);
	if (!xsdt.header.IsValid("XSDT")) {
		Log(kError, "XSDT is not valid\n");
		exit(1);
	}

	fadt = nullptr;
	for (int i = 0; i < xsdt.Count(); ++i) {
		const auto& entry = xsdt[i]; // DescriptionHeader 구조체 & 반환
		if (entry.IsValid("FACP")) { // FACP is the signature of FADT
			fadt = reinterpret_cast<const FADT*>(&entry);
			break;
		}
	}

	if (fadt == nullptr) {
		Log(kError, "FADT is not found\n");
		exit(1);
	}	
}
// #@@range_end(initialize_acpi)

} // namespace acpi
