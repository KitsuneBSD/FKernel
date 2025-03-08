global gdt64.pointer
global gdt64.code_segment

section .rodata
gdt64:
  dq 0; // Null Descriptor
.code_segment: equ $ - gdt64
  dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
  dw $ - gdt64 - 1
  dq gdt64
