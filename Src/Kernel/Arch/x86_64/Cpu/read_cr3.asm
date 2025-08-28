global read_cr3

section .text 

read_cr3:
  mov rax, cr3
  ret
