global arch_read_cr3

section .text
bits 64

arch_read_cr3:
    mov rax, cr3
    ret
