global write_cr3

section .text 

write_cr3:
  mov cr3, rdi 
  ret
