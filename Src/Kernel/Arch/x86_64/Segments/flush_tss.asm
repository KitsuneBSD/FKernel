global tss_flush
section .text 

tss_flush:
    mov ax, di                    
    ltr ax
    ret
