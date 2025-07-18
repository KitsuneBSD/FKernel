global long_mode_start 
extern kmain

extern page_table_l4 
extern current_pml4_ptr 
extern multiboot_magic 
extern multiboot_info_ptr

section .text
bits 64
long_mode_start:
  lea rax, [page_table_l4]
  mov [current_pml4_ptr], rax

  mov eax, [multiboot_magic]
  mov edi, eax
  mov rsi, [multiboot_info_ptr]
  

  mov ax, 0
  mov ss, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  
  call kmain
  hlt
