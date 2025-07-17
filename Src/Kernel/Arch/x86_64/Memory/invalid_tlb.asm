global invalid_tlb

section .text
invalid_tlb:
    invlpg [rdi]
    ret
