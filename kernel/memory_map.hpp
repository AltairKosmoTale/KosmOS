#pragma once
#include <stdint.h>

/* // #@@range_begin(struct_memory_map)
struct MemoryMap { 
	UINTN buffer_size;
	VOID* buffer;
	UINTN map_size;
	UINTN map_key;
	UINTN descriptor_size;
	UINT32 descriptor_version;
}; // #@@range_end(struct_memory_map) */

struct MemoryMap {
	unsigned long long buffer_size;
	void* buffer;
	unsigned long long map_size;
	unsigned long long map_key;
	unsigned long long descriptor_size;
	uint32_t descriptor_version;
};

struct MemoryDescriptor {
	uint32_t type;
	uintptr_t physical_start;
	uintptr_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
};

// Main.c에서 읽을 때는 비활성화
#ifdef __cplusplus
enum class MemoryType {
	kEfiReservedMemoryType,
	kEfiLoaderCode,
	kEfiLoaderData,
	kEfiBootServicesCode,
	kEfiBootServicesData,
	kEfiRuntimeServicesCode,
	kEfiRuntimeServicesData,
	kEfiConventionalMemory,
	kEfiUnusableMemory,
	kEfiACPIReclaimMemory,
	kEfiACPIMemoryNVS,
	kEfiMemoryMappedIO,
	kEfiMemoryMappedIOPortSpace,
	kEfiPalCode,
	kEfiPersistentMemory,
	kEfiMaxMemoryType
};

inline bool operator==(uint32_t lhs, MemoryType rhs) {
	return lhs == static_cast<uint32_t>(rhs);
}

inline bool operator==(MemoryType lhs, uint32_t rhs) {
	return rhs == lhs;
}

// #@@range_begin(is_available)
inline bool IsAvailable(MemoryType memory_type) {
	return
		memory_type == MemoryType::kEfiBootServicesCode ||
		memory_type == MemoryType::kEfiBootServicesData ||
		memory_type == MemoryType::kEfiConventionalMemory;
}

const int kUEFIPageSize = 4096;
// #@@range_end(is_available)

#endif
