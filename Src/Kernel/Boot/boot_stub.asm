section .text
bits 32
global _start

extern kernel_main
extern stack_top

extern check_cpuid
extern check_long_mode

extern setup_page_tables
extern enable_paging

extern gdt64.code_segment
extern gdt64.pointer


clean_screen:
  mov edi, 0xb8000
  mov eax, 0x0720
  mov ecx, 80 * 25
  rep stosw

  mov dx, 0x3d4
  mov al, 0x0f
  out dx, al
  mov dx, 0x3D5
  mov al, 0x00
  out dx, al

  mov dx, 0x3d4
  mov al, 0x0E
  out dx, al
  mov dx, 0x3D5
  mov al, 0x00
  out dx, al

_start:
  call clean_screen
 
  mov esp, stack_top
  mov ebp, 0
 
  call check_cpuid
  call check_long_mode

  call setup_page_tables
  call enable_paging

  lgdt [gdt64.pointer]
  jmp gdt64.code_segment:long_mode_start

.hang:
  hlt
  jmp .hang

.end:

section .text
bits 64
long_mode_start:
  mov ax, 0
  mov ss, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  call kernel_main

