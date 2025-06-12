section .text
global gdt_flush

gdt_flush:
    lgdt [rdi]

    push qword 0x08               ; Kernel code selector (index 1 << 3)
    lea  rax, [rel reload_cs]     ; Endereço do próximo código
    push rax
    lretq                         ; Salta para reload_cs com novo CS

reload_cs:
    mov ax, 0x10                  ; Kernel data selector (index 2 << 3)
    mov ds, ax
    mov es, ax
    mov ss, ax
    ret
