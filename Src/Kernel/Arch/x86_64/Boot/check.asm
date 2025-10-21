global check_multiboot
global check_long_mode
global check_cpuid 

extern error

section .text
bits 32
; TODO/FIXME: Entry point check — verify calling convention and registers preserved.
; Review behavior when called from different CPU modes and ensure this code is
; safe to run in the early boot environment.
check_multiboot:
	cmp eax, 0x36d76289
	jne .no_multiboot
	ret
.no_multiboot:
	mov al, "M"
	jmp error

; TODO/FIXME: CPUID check — ensure flags manipulation preserves processor state
; correctly and that this sequence is compatible with interrupt/exception contexts.
; Consider using safer register save/restore idioms if this runs in sensitive contexts.
check_cpuid:
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 1 << 21
	push eax
	popfd
	pushfd
	pop eax
	push ecx
	popfd
	cmp eax, ecx
	je .no_cpuid
	ret
.no_cpuid:
	mov al, "C"
	jmp error

; TODO/FIXME: Long mode feature check — validate CPUID usage and ensure the bit
; tested is correct across vendors. Consider fallbacks or clearer error reporting.
check_long_mode:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb .no_long_mode

	mov eax, 0x80000001
	cpuid
	test edx, 1 << 29
	jz .no_long_mode
	
	ret
.no_long_mode:
	mov al, "L"
	jmp error


