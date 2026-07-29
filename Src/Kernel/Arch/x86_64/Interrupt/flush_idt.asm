global arch_flush_idt

section .text 
bits 64 

arch_flush_idt:
  lidt [rdi]
  ret
