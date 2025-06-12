global tss_flush
section .text 

tss_flush:
    mov ax, di                    ; TSS selector
    ltr ax
    ret
