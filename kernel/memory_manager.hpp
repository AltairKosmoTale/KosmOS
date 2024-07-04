#pragma once
#include <array>
#include <limits>
#include "error.hpp"
#include "memory_map.hpp"
// #@@range_begin(frame_id)
// C++ 11에서 도입된 사용자 정의 리터럴:
// operator"" suffix (타입 연산자) 정의 오버로드
namespace {
	constexpr unsigned long long operator""_KiB(unsigned long long kib) {
		return kib * 1024;
	}

	constexpr unsigned long long operator""_MiB(unsigned long long mib) {
		return mib * 1024_KiB;
	}

	constexpr unsigned long long operator""_GiB(unsigned long long gib) {
		return gib * 1024_MiB;
	}
}

static const auto kBytesPerFrame{4_KiB}; // 4096 상수 전달

class FrameID {
 public:
	explicit FrameID(size_t id) : id_{id} {}
	size_t ID() const { return id_; }
	void* Frame() const { return reinterpret_cast<void*>(id_ * kBytesPerFrame); }

 private:
	size_t id_;
};

static const FrameID kNullFrame{std::numeric_limits<size_t>::max()}; // page frame 발견 실패 시 반환값
// #@@range_end(frame_id)

// #@@range_begin(bitmap_memory_manager)
class BitmapMemoryManager {
 public:
	// MemoryManager가 다룰 수 있는 최대 용량
	static const auto kMaxPhysicalMemoryBytes{128_GiB};
	// 최대 용량 까지 다루기 위해 필요한 프레임 수
	static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};
	// 비트맵 배열의 요소 타입
	using MapLineType = unsigned long;
	// 비트맵 배열 한개의 요소 비트 수 = 프레임 수
	static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};
	// 인스턴스 초기화
	BitmapMemoryManager();
	// 요구된 프레임 수의 영역을 확보해 시작 주소 ID를 반환
	WithError<FrameID> Allocate(size_t num_frames);
	Error Free(FrameID start_frame, size_t num_frames);
	void MarkAllocated(FrameID start_frame, size_t num_frames);
	/* MemoryManager가 다루는 메모리 범위 설정
	SetMemoryRange() 메소드 호출 이후에는 Allocate에 의한 메모리 할당은 설정된 범위 내에서만 수행 */ 
	void SetMemoryRange(FrameID range_begin, FrameID range_end);

 private:
	// alloc_map_ : 1 page frame을 1 bit로 나타내는 비트맵
	std::array<MapLineType, kFrameCount / kBitsPerMapLine> alloc_map_;
	FrameID range_begin_;
	FrameID range_end_;
	bool GetBit(FrameID frame) const;
	void SetBit(FrameID frame, bool allocated);
};
// #@@range_end(bitmap_memory_manager)

void InitializeMemoryManager(const MemoryMap& memory_map);