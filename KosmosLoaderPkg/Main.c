#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>
#include "frame_buffer_config.hpp"
#include "elf.hpp"

// #@@range_begin(struct_memory_map)
struct MemoryMap {
	UINTN buffer_size;
	VOID* buffer;
	UINTN map_size;
	UINTN map_key;
	UINTN descriptor_size;
	UINT32 descriptor_version;
};
// #@@range_end(struct_memory_map)

// #@@range_begin(get_memory_map)
EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
	if (map->buffer == NULL) {
		return EFI_BUFFER_TOO_SMALL;
	}

	map->map_size = map->buffer_size;
	return gBS->GetMemoryMap(
			&map->map_size, // in: 쓰기용 메모리 영역 크기 -> out: 실제 메모리 맵의 크기
			(EFI_MEMORY_DESCRIPTOR*)map->buffer,
			&map->map_key, // 메모리맵 식별 전용
			&map->descriptor_size,
			&map->descriptor_version);
}
// #@@range_end(get_memory_map)

// #@@range_begin(get_memory_type)
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
	switch (type) {
		case EfiReservedMemoryType: return L"EfiReservedMemoryType";
		case EfiLoaderCode: return L"EfiLoaderCode";
		case EfiLoaderData: return L"EfiLoaderData";
		case EfiBootServicesCode: return L"EfiBootServicesCode";
		case EfiBootServicesData: return L"EfiBootServicesData";
		case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
		case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
		case EfiConventionalMemory: return L"EfiConventionalMemory";
		case EfiUnusableMemory: return L"EfiUnusableMemory";
		case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
		case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
		case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
		case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
		case EfiPalCode: return L"EfiPalCode";
		case EfiPersistentMemory: return L"EfiPersistentMemory";
		case EfiMaxMemoryType: return L"EfiMaxMemoryType";
		default: return L"InvalidMemoryType";
	}
}
// #@@range_end(get_memory_type)

// #@@range_begin(save_memory_map)
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
	EFI_STATUS status;
	CHAR8 buf[256];
	UINTN len;

	CHAR8* header =
		"Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
	len = AsciiStrLen(header);
	status = file->Write(file, &len, header);
	if (EFI_ERROR(status)) {
		return status;
	}

	Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
			map->buffer, map->map_size);

	EFI_PHYSICAL_ADDRESS iter;
	int i;
	for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
		 iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
		 iter += map->descriptor_size, i++) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
		len = AsciiSPrint(
			buf, sizeof(buf),
			"%u, %x, %-ls, %08lx, %lx, %lx\n",
			i, desc->Type, GetMemoryTypeUnicode(desc->Type),
			desc->PhysicalStart, desc->NumberOfPages,
			desc->Attribute & 0xffffflu);
		status = file->Write(file, &len, buf);
		if (EFI_ERROR(status)) {
			return status;
		}
	}

	return EFI_SUCCESS;
}
// #@@range_end(save_memory_map)

// #@@range_begin(open_root_dir)
// memmap file을 쓰기 모드로 열어서 SaveMemoryMap()에 전달
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

	status = gBS->OpenProtocol(
		image_handle,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&loaded_image,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if (EFI_ERROR(status)) {
		return status;
	}		

	status = gBS->OpenProtocol(
		loaded_image->DeviceHandle,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&fs,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	if (EFI_ERROR(status)) {
		return status;
	}

	return fs->OpenVolume(fs, root);
}
// #@@range_end(open_root_dir)

// #@@range_begin(open_gop)
EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
	EFI_STATUS status;
	UINTN num_gop_handles = 0;
	EFI_HANDLE* gop_handles = NULL;
	status = gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiGraphicsOutputProtocolGuid,
		NULL,
		&num_gop_handles,
		&gop_handles);
	if (EFI_ERROR(status)) {
		return status;
	}
	
	status = gBS->OpenProtocol(
		gop_handles[0],
		&gEfiGraphicsOutputProtocolGuid,
		(VOID**)gop,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if (EFI_ERROR(status)) {
		return status;
	}
	
	FreePool(gop_handles);

	return EFI_SUCCESS;
}
// #@@range_end(open_gop)

// #@@range_begin(get_pixel_format_unicode)
const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
	switch (fmt) {
		case PixelRedGreenBlueReserved8BitPerColor:
			return L"PixelRedGreenBlueReserved8BitPerColor";
		case PixelBlueGreenRedReserved8BitPerColor:
			return L"PixelBlueGreenRedReserved8BitPerColor";
		case PixelBitMask:
			return L"PixelBitMask";
		case PixelBltOnly:
			return L"PixelBltOnly";
		case PixelFormatMax:
			return L"PixelFormatMax";
		default:
			return L"InvalidPixelFormat";
	}
}
// #@@range_end(get_pixel_format_unicode)

// #@@range_begin(halt)
void Halt(void) {
	while (1) __asm__("hlt");
}
// #@@range_end(halt)

// #@@range_begin(calc_addr_func)
void CalcLoadAddressRange(Elf64_Ehdr* ehdr, UINT64* first, UINT64* last) {
	Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
	*first = MAX_UINT64;
	*last = 0;
	for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
		if (phdr[i].p_type != PT_LOAD) continue; // phdr[i] = ith 프로그램 헤더 -> LOAD 아니면 skip
		*first = MIN(*first, phdr[i].p_vaddr); // 첫 LOAD segment의 p_vaddr
		*last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz); // 마지막 LOAD의 segment p_vaddr + p_memsz
	}
}
// #@@range_end(calc_addr_func)

// #@@range_begin(copy_segm_func)
void CopyLoadSegments(Elf64_Ehdr* ehdr) {
	Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
	for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
		if (phdr[i].p_type != PT_LOAD) continue;

		// 1. segm_in_file이 가리키는 임시 영역에서 p_vaddr이 가리키는 최종 목적지로 데이터 복사
		UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
		CopyMem((VOID*)phdr[i].p_vaddr, (VOID*)segm_in_file, phdr[i].p_filesz);
		
		// 2. segment의 메모리상 크기가 파일상의 크기보다 큰 경우, 남은 부분을 0으로 채움
		UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
		SetMem((VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
	}
}
// #@@range_end(copy_segm_func)

EFI_STATUS EFIAPI UefiMain(
	EFI_HANDLE image_handle,
	EFI_SYSTEM_TABLE *system_table) {
	EFI_STATUS status;
	Print(L"Hello, KosmOS!\n");
	
	CHAR8 memmap_buf[4096 * 4];
	struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
	status = GetMemoryMap(&memmap);
	if (EFI_ERROR(status)) {
		Print(L"failed to get memory map: %r\n", status);
		Halt();
	}

	EFI_FILE_PROTOCOL* root_dir;
	status = OpenRootDir(image_handle, &root_dir);
	if (EFI_ERROR(status)) {
		Print(L"failed to open root directory: %r\n", status);
		Halt();
	}

	EFI_FILE_PROTOCOL* memmap_file;
	status = root_dir->Open(
		root_dir, &memmap_file, L"\\memmap",
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

	if (EFI_ERROR(status)) {
		Print(L"failed to open file '\\memmap': %r\n", status);
		Print(L"Ignored.\n");
	} else {
		status = SaveMemoryMap(&memmap, memmap_file);
		if (EFI_ERROR(status)) {
			Print(L"failed to save memory map: %r\n", status);
			Halt();
		}
		status = memmap_file->Close(memmap_file);
		if (EFI_ERROR(status)) {
			Print(L"failed to close memory map: %r\n", status);
			Halt();
		}
	}
	
	// #@@range_begin(gop)
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	status = OpenGOP(image_handle, &gop);
	if (EFI_ERROR(status)) {
		Print(L"failed to open GOP: %r\n", status);
		Halt();
	}
	Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
		gop->Mode->Info->PixelsPerScanLine); // 1픽셀의 데이터 형식 (256 or 16,770,000)
	Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
		gop->Mode->FrameBufferBase, // 디스플레이 픽셀에 반영 되는 시작 주소
		gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize, // 시작 주속 + 크기
		gop->Mode->FrameBufferSize); // 크기

	UINT8* frame_buffer = (UINT8*)gop->Mode->FrameBufferBase;
	for (UINTN i = 0; i < gop->Mode->FrameBufferSize; ++i) {
		frame_buffer[i] = 255;
	}
	// #@@range_end(gop)

	// #@@range_begin(read_kernel)
	EFI_FILE_PROTOCOL* kernel_file;
	status = root_dir->Open(
		root_dir, &kernel_file, L"\\kernel.elf",
		EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status)) {
		Print(L"failed to open file '\\kernel.elf': %r\n", status);
		Halt();
	}		
	/* typedef struct{
		UINT64 Size, FileSize, PhysicalSize;
		EFI_TIME CreateTime, LastAccessTime, ModificationTime;
		UINT64 Attribute;
		CHAR16 FileName[]; 
	} EFI_FILE_INFO
	*/
	// sizeof(EFI_FILE_INFO) = Attribute까지의 크기 -> "\kernel.elf\0" (12 * char) 
	UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
	UINT8 file_info_buffer[file_info_size];
	status = kernel_file->GetInfo(
		kernel_file, &gEfiFileInfoGuid,
		&file_info_size, file_info_buffer);
	if (EFI_ERROR(status)) {
		Print(L"failed to get file information: %r\n", status);
		Halt();
	}
	
	/* Loader 개량
	1. 커널 파일을 한 번에 최종 목적지 X, 임시영역에 읽기
	2. 임시 영역에 읽은 커널 파일의 헤더를 읽어 최종 목적지 주소 범위 계산
	3. 계산된 최종 목적지로 LOAD 세그먼트 복사 & 임시 영역 제거 */
	// #@@range_begin(read_kernel)
	EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
	UINTN kernel_file_size = file_info->FileSize;

	VOID* kernel_buffer;
	// 임시 영역 확보 -> page 단위가 아닌 byte 단위로 메모리 확보 & 저장 (위치 지정은 불가)
	status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_buffer);
	if (EFI_ERROR(status)) {
		Print(L"failed to allocate pool: %r\n", status);
		Halt();
	}
	// 임시 영역 시작 주소를 받아서 파일 내용 전부를 임시 영역에 읽기
	status = kernel_file->Read(kernel_file, &kernel_file_size, kernel_buffer);
	if (EFI_ERROR(status)) {
		Print(L"error: %r", status);
		Halt();
	}
	// #@@range_end(read_kernel)

	// #@@range_begin(alloc_pages)
	Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
	UINT64 kernel_first_addr, kernel_last_addr;
	CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr); // 범위 계산
	
	/* 이제 커널의 크기를 알았기 때문에, 메모리 확보
	1. 메모리 확보 방법
	1-1. 어디라도 좋으니, 비어있는 공간에서 확보: AllocateAnyPages
	1-2. 지정한 어드레스 이후에 비어있는 공간에서 확보: AllocateMaxAddress
	1-3. 지정한 주소에서 확보: AllocateAddress [selected]
		커널은 0x100000번지에 배치를 전제로 만들었다. -> ld.lld 옵션
		so, 다른 위치에 배치시키면 정상 작동 X
	2. 확보할 메모리 영역 유형: EfiLoaderData // boot loader가 사용하기 위한 영역
	3. 크기
	4. 확보한 메모리 공간의 어드레스를 저장하기 위한 변수 지정
	포인터 넘기는 이유: AllocateAnyPages, AllocateMaxAddress의 경우, 해당 주소 접근 필요
	but now I don't need it -> AllocateAddress -> 변숫값이 변뀌는 경우가 없기 때문
	return 값이 성공 실패인 상황에서 다른 파라미터 전달: 포인터로 구현 */
	
	/* 페이지 수 = (kernel_file_size + 0xfff) / 0x1000 // 0x1000 = 4KiB
	0xfff 더해주는 이유: 페이지수를 올림 하기 위해 -> 잘려서 누락되는 것 방지 (올림) */

	UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
	status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr);
	if (EFI_ERROR(status)) {
		Print(L"failed to allocate pages: %r\n", status);
		Halt();
	}
	// #@@range_end(alloc_pages)

	// #@@range_begin(copy_segments)
	CopyLoadSegments(kernel_ehdr); // 임시 영역에서 최종 목적지로 LOAD segment 복사
	Print(L"Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addr, kernel_last_addr);
	
	status = gBS->FreePool(kernel_buffer); // 확보했던 임시 영역 해제
	if (EFI_ERROR(status)) {
		Print(L"failed to free pool: %r\n", status);
		Halt();
	}
	// #@@range_end(copy_segments)

	
	// #@@range_begin(exit_bs)
	/* ExitBootServices()는 최신 메모리 맵의 맵 키 요구
	지정된 맵 키가 최신의 메모리 맵과 연결된 맵 키가 아닌 경우 실행 실패

	실패시 다시 메모리 맵을 얻고 해당 맵 키를 사용해 재실행
	초기 메모리 맵 취득 -> 함수 호출 사이 여러 기능 사용 중 -> 첫 시도 반드시 실패
	두번째 실행 -> 실패 시 중대한 오류 이므로 무한루프 처리로 정지
	*/
	status = gBS->ExitBootServices(image_handle, memmap.map_key);
	if (EFI_ERROR(status)) {
		status = GetMemoryMap(&memmap);
		if (EFI_ERROR(status)) {
			Print(L"failed to get memory map: %r\n", status);
			Halt();
		}
		status = gBS->ExitBootServices(image_handle, memmap.map_key);
		if (EFI_ERROR(status)) {
			Print(L"Could not exit boot service: %r\n", status);
			Halt();
		}
	}
	// #@@range_end(exit_bs)

	// #@@range_begin(call_kernel)
	// ELF Spec상, Entry Point offset 24byte에서 8byte 정수로 작성
	// 함수 포인터로 캐스트 하고 호출 (유사 PC)
	// #@@range_begin(get_entry_point)
	UINT64 entry_addr = *(UINT64*)(kernel_first_addr + 24);
	// #@@range_end(get_entry_point)

	//typedef void EntryPointType(UINT64, UINT64);
	// #@@range_begin(pass_frame_buffer_config)
	struct FrameBufferConfig config = {
		(UINT8*)gop->Mode->FrameBufferBase,
		gop->Mode->Info->PixelsPerScanLine,
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		0
	};
	
	switch (gop->Mode->Info->PixelFormat) {
		case PixelRedGreenBlueReserved8BitPerColor:
			config.pixel_format = kPixelRGBResv8BitPerColor;
			break;
		case PixelBlueGreenRedReserved8BitPerColor:
			config.pixel_format = kPixelBGRResv8BitPerColor;
			break;
		default:
			Print(L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
			Halt();
	}

	typedef void __attribute__((sysv_abi)) EntryPointType(const struct FrameBufferConfig*);
	EntryPointType* entry_point = (EntryPointType*)entry_addr;
	entry_point(&config);
	// #@@range_end(pass_frame_buffer_config)
	// #@@range_end(call_kernel)
	
	Print(L"All done\n");
	
	while (1);
	return EFI_SUCCESS;
}
