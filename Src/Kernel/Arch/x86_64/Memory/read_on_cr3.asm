global read_on_cr3

section .text
bits 64

read_on_cr3:
    mov rax, cr3
    ret
