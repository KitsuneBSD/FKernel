section .text
global gdt_flush

gdt_flush:
    lgdt [rdi]

    push qword 0x08
    lea  rax, [rel reload_cs]
    push rax
    lretq

reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    ret
