global check_multiboot
global check_long_mode

extern error

%define MULTIBOOT2_MAGIC_BOOTLOADER 0x36d76289

section .prekernel.text
bits 32

; @brief Check if booted by a Multiboot2-compliant bootloader
check_multiboot:
    cmp eax, MULTIBOOT2_MAGIC_BOOTLOADER
    jne .fail
    ret
.fail:
    mov al, 'M'
    jmp error

; @brief Check if CPU supports Long Mode (64-bit mode)
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .fail
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29      ; bit LM
    jz .fail
    ret
.fail:
    mov al, 'L'
    jmp error
