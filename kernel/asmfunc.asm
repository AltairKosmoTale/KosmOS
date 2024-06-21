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

global GetCS  ; uint16_t GetCS(void);
GetCS:
    xor eax, eax  ; also clears upper 32 bits of rax
    mov ax, cs
    ret

; #@@range_begin(load_idt_function)
; IDT 크기, IDT가 배치된 main memory 주소 get -> lidt로 CPU에 등록
; memory 구조 : offset 0 -> uint16_t(IDT 사이즈 -1) / offset 2 -> uint64_t(IDT의 시작 address)
global LoadIDT  ; void LoadIDT(uint16_t limit, uint64_t offset);
LoadIDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10 ; stack 에서 10 byte 확보
    mov [rsp], di  ; limit (2byte)
    mov [rsp + 2], rsi  ; offset (8byte)
    lidt [rsp]
    mov rsp, rbp
    pop rbp
    ret
; #@@range_end(load_idt_function)