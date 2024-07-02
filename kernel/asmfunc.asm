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

; #@@range_begin(load_gdt)
global LoadGDT	; void LoadGDT(uint16_t limit, uint64_t offset);
LoadGDT:
	push rbp
	mov rbp, rsp
	sub rsp, 10 ; 10 byte 영역 확보
	mov [rsp], di ; 2 byte 영역에 DI 내용 복사 (첫 파라미터: rdi 하위 16 bit)
	mov [rsp + 2], rsi	; offset ; GDT 시작 주소 (rsi: 메모리 이동 or 비교 시 출발지 주소)
	lgdt [rsp] ; GDT 설정할 수 있는 Intel Architecture 특수 명령 ; C++ 불가
	; limit, offset을 GDTR 레지스터에 설정 (80 bit -> 10 byte)
	mov rsp, rbp
	pop rbp
	ret
; #@@range_end(load_gdt)

; #@@range_begin(set_cs)
global SetCSSS	; void SetCSSS(uint16_t cs, uint16_t ss);
SetCSSS:
	push rbp
	mov rbp, rsp
	mov ss, si
	mov rax, .next
	push rdi ; CS ; CS가 가리키는 디스크립터의 설정 내용에 따라 엑세스 권한 검사 수행
	push rax ; RIP
	o64 retf ; far return: far call(다른 세그먼트로 jmp)의 복귀 / 스택에서 값 얻어 CS, RIP 설정
	; retf default: 32 비트 so, o64로 64비트 값 가져오게 설정
.next:
	mov rsp, rbp
	pop rbp
	ret
; #@@range_end(set_cs)

; #@@range_begin(set_dsall)
global SetDSAll	; void SetDSAll(uint16_t value);
SetDSAll: ; 파라미터 복사 ; 0을 전달해서 Null Descriptor 가리키도록 설정
	mov ds, di
	mov es, di
	mov fs, di
	mov gs, di
	ret
; #@@range_end(set_dsall)

; #@@range_begin(set_cr3)
global SetCR3	; void SetCR3(uint64_t value);
SetCR3:
	mov cr3, rdi
	ret
; #@@range_end(set_cr3)

; #@@range_begin(set_main_stack)
extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
	mov rsp, kernel_main_stack + 1024 * 1024
	call KernelMainNewStack ; 새로운 stack에는 KernelMain()의 복귀 주소 X
.fin: ; 돌아올일 없지만 만일의 경우를 위한 loop
	hlt
	jmp .fin
; #@@range_end(set_main_stack)