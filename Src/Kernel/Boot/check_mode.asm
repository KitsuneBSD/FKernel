global check_cpuid
global check_long_mode

section .text
bits 32

error:
  mov dword [0xb8000], 0x4f524f45
  mov dword [0xb8004], 0x4f3a4f52
  mov dword [0xb8008], 0x4f204f20
  mov byte [0xb800a], al
  ret

check_cpuid:
  pushfd
  pop eax
  mov ecx, eax
  xor eax, 1 << 21
  push eax
  popfd
  pushfd
  pop eax
  push ecx
  popfd
  cmp eax, ecx
  je .without_cpuid
  ret
.without_cpuid:
  mov al, 'C'
  jmp error

check_long_mode:
  mov eax, 0x80000000
  cpuid
  cmp eax, 0x80000001
  jb .without_long_mode
  
  mov eax, 0x80000001
  cpuid
  test edx, 1 << 29
  jz .without_long_mode

  ret
.without_long_mode
  mov al, 'L'
  jmp error
