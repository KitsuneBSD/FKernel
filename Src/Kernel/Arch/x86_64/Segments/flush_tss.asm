global flush_tss 

section .text 


global flush_tss
section .text
flush_tss:
    mov ax, di
    ltr ax
    ret
