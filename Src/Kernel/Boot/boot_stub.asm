section .bss 
align 16
stack_bottom:
  resb 16384 ; 16 KiB   TODO:Increase the value to 640KiB 
stack_top:

section .text 
extern kernel_main
global _start

_start:
  ; Clean the screen
  mov edi, 0xb8000
  mov eax, 0x0720
  mov ecx, 80*25
  rep stosw
  ; Jump cursor to initial position
  xor eax, eax
  mov dx, 0x3D4
  mov al, 0x0f
  out dx, al
  mov dx, 0x3D5
  out dx, al

  mov dx, 0x3D4
  mov al, 0x0E
  out dx, al 
  mov dx, 0x3D5
  out dx, al

  ; Configure the kernel stack
  mov esp, stack_top
  mov ebp, 0
  call kernel_main

.hang: hlt
  jmp .hang
.end:
