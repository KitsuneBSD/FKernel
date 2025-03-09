global flush_segments

section .text 
bits 64
flush_segments:
  mov rax, 0x08
  push rax
  lea rax, [reload_cs]
  push rax
  lret
reload_cs:
  ret
