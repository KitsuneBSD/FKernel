global check_multiboot
global check_long_mode
global check_cpuid 

extern error

%define MULTIBOOT2_MAGIC_BOOTLOADER 0x36d76289

section .text 
bits 32 
check_multiboot:
	cmp eax, MULTIBOOT2_MAGIC_BOOTLOADER
	jne .multiboot2_fail
	ret 
.multiboot2_fail:
	mov al, 'M'
	jmp error

check_long_mode:
	mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .long_mode_fail
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .long_mode_fail
    ret
.long_mode_fail:
	mov al, 'L'
	jmp error