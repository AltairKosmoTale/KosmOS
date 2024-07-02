#include "memory_manager.hpp"

BitmapMemoryManager::BitmapMemoryManager()
	: alloc_map_{}, range_begin_{FrameID{0}}, range_end_{FrameID{kFrameCount}} {
}

// #@@range_begin(allocate)
WithError<FrameID> BitmapMemoryManager::Allocate(size_t num_frames) {
	size_t start_frame_id = range_begin_.ID();
	while (true) {
		size_t i = 0;
		for (; i < num_frames; ++i) {
			if (start_frame_id + i >= range_end_.ID()) {
				return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
			}
			if (GetBit(FrameID{start_frame_id + i})) {
				// "start_frame_id + i" 에 있는 frame은 할당을 건너 뜀
				break;
			}
		}
		if (i == num_frames) {
			// num_frames 만큼의 frame을 발견했다.
			MarkAllocated(FrameID{start_frame_id}, num_frames);
			return {
				FrameID{start_frame_id},
				MAKE_ERROR(Error::kSuccess),
			};
		}
		// 다음 frame 부터 제 탐색
		start_frame_id += i + 1;
	}
}
// #@@range_end(allocate)

// #@@range_begin(free)
Error BitmapMemoryManager::Free(FrameID start_frame, size_t num_frames) {
	for (size_t i = 0; i < num_frames; ++i) {
		SetBit(FrameID{start_frame.ID() + i}, false);
	}
	return MAKE_ERROR(Error::kSuccess);
}
// #@@range_end(free)

// #@@range_begin(mark_allocated)
void BitmapMemoryManager::MarkAllocated(FrameID start_frame, size_t num_frames) {
	for (size_t i = 0; i < num_frames; ++i) {
		SetBit(FrameID{start_frame.ID() + i}, true);
	}
}
// #@@range_end(mark_allocated)

// #@@range_begin(set_memory_range)
void BitmapMemoryManager::SetMemoryRange(FrameID range_begin, FrameID range_end) {
	range_begin_ = range_begin;
	range_end_ = range_end;
}
// #@@range_end(set_memory_range)

// #@@range_begin(get_set_bit)
bool BitmapMemoryManager::GetBit(FrameID frame) const {
	auto line_index = frame.ID() / kBitsPerMapLine;
	auto bit_index = frame.ID() % kBitsPerMapLine;

	return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame, bool allocated) {
	auto line_index = frame.ID() / kBitsPerMapLine;
	auto bit_index = frame.ID() % kBitsPerMapLine;

	if (allocated) {
		alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bit_index);
	} else {
		alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bit_index);
	}
}
// #@@range_end(get_set_bit)

// #@@range_begin(set_program_break)
extern "C" caddr_t program_break, program_break_end;

Error InitializeHeap(BitmapMemoryManager& memory_manager) {
	const int kHeapFrames = 64 * 512; // 64 * 512 * 4 KiB = 128 MiB
	const auto heap_start = memory_manager.Allocate(kHeapFrames);
	if (heap_start.error) {
		return heap_start.error;
	}

	program_break = reinterpret_cast<caddr_t>(heap_start.value.ID() * kBytesPerFrame);
	program_break_end = program_break + kHeapFrames * kBytesPerFrame;
	return MAKE_ERROR(Error::kSuccess);
}
// #@@range_end(set_program_break)