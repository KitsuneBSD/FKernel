section .text
global flush_gdt
%define NEW_CS 0x08   ; Kernel code
%define NEW_DS 0x10   ; Kernel data

flush_gdt:
    ; RDI = pointer para GDTR
    lgdt [rdi]             ; carregar a nova GDT

    mov ax, NEW_DS          ; carregar segmentos de dados
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret
