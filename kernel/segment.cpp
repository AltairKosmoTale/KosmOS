#include "segment.hpp"

#include "asmfunc.h"

// #@@range_begin(gdt_definition)
namespace { // segment.cpp 내부에서만 참조 -> namespace 사용
	std::array<SegmentDescriptor, 3> gdt;
}
// #@@range_end(gdt_definition)

// #@@range_begin(setup_segm_function)
void SetCodeSegment(SegmentDescriptor& desc,
					DescriptorType type,
					unsigned int descriptor_privilege_level,
					uint32_t base,
					uint32_t limit) {
	desc.data = 0;

	desc.bits.base_low = base & 0xffffu;
	desc.bits.base_middle = (base >> 16) & 0xffu;
	desc.bits.base_high = (base >> 24) & 0xffu;

	desc.bits.limit_low = limit & 0xffffu;
	desc.bits.limit_high = (limit >> 16) & 0xfu;

	desc.bits.type = type; // x86_descriptor.hpp에 정의 (kReadWrite = 2, kExecuteRead = 10)
	desc.bits.system_segment = 1; // 1: code & data segment
	desc.bits.descriptor_privilege_level = descriptor_privilege_level; // DPL: 현재 권한 레벨 (CPL)
	desc.bits.present = 1; // 1일때 유효
	desc.bits.available = 0; // OS가 자유롭게 사용 가능한지 여부
	desc.bits.long_mode = 1; // 1일때 64비트 모드
	desc.bits.default_operation_size = 0; // should be 0 when long_mode == 1
	desc.bits.granularity = 1; // limit = 4KiB 단위 해석
}

void SetDataSegment(SegmentDescriptor& desc,
					DescriptorType type,
					unsigned int descriptor_privilege_level,
					uint32_t base,
					uint32_t limit) {
	SetCodeSegment(desc, type, descriptor_privilege_level, base, limit);
	desc.bits.long_mode = 0; // 예약 필드; 반드시 0으로 설정
	desc.bits.default_operation_size = 1; // 32-bit stack segment // for syscall과 일관성 유지: 1
}

void SetupSegments() {
	gdt[0].data = 0; // NULL descriptor: 사용 X -> 0으로 채움
	SetCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0, 0, 0xfffff);
	SetDataSegment(gdt[2], DescriptorType::kReadWrite, 0, 0, 0xfffff);
	LoadGDT(sizeof(gdt) - 1, reinterpret_cast<uintptr_t>(&gdt[0])); // GDT 위치, 사이즈 CPU에 등록
}
// #@@range_end(setup_segm_function)
