section .rodata
    gdt_null_error_msg db "GDT flush error: GDTR pointer is NULL", 0

section .text
extern kernel_panic_log
global arch_flush_gdt
%define NEW_CS 0x08   ; Kernel code
%define NEW_DS 0x10   ; Kernel data

arch_flush_gdt:
    ; RDI = pointer para GDTR
    test rdi, rdi
    jz .null_pointer_error

    lgdt [rdi]             ; carregar a nova GDT

    mov ax, NEW_DS          ; carregar segmentos de dados
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret

.null_pointer_error:
    mov rdi, gdt_null_error_msg
    call kernel_panic_log
    ud2
