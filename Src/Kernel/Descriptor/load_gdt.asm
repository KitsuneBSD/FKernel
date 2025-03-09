global load_gdt

section .text
bits 64

load_gdt:
  lgdt [rdi]
  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  push qword 0x08
  lea rax, [rel reload_cs]
  push rax
  lretq

reload_cs:
  ret
