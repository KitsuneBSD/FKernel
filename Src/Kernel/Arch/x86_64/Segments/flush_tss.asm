global flush_tss 

section .text 

flush_tss:
    ltr ax
    ret
