global gdt_flush
section .text

gdt_flush:
  lgdt [rdi]

  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  push qword 0x08
  lea rax, [rel flush_cs]
  push rax 
  lretq
flush_cs:
  ret
