global flush_idt

section .text 
bits 64 

flush_idt:
  lidt [rdi] ; Load the IDTR
  ret

