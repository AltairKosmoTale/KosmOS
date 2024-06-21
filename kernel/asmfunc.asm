; asmfunc.asm
;
; System V AMD64 Calling Convention
; Registers: RDI, RSI, RDX, RCX, R8, R9

; IO address space != Memory Address space
; PCI address space -> IO address space에 연결
; IO address space는 C++에서 접근 불가 -> asm으로 구현된 함수를 호출하게끔 구현

bits 64
section .text

global IoOut32 ; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
	mov dx, di ; dx = addr ; di = rdi's 하위 16비트 -> 파라미터 addr 값
	mov eax, esi ; eax = data ; esi = rsi's 하위 32비트 -> 파라미터 data 값
	out dx, eax ; dx에 설정된 IO port address에 eax에 설정된 32 비트 정수 write
	ret

global IoIn32 ; uint32_t IoIn32(uint16_t addr);
IoIn32:
	mov dx, di ; dx = addr
	in eax, dx ; System V AMD64 사양 -> rax: 함수의 반환 값
	ret
