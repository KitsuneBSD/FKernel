global arch_flush_tss 

section .text 


global arch_flush_tss
section .text
arch_flush_tss:
    mov ax, di
    ltr ax
    ret
