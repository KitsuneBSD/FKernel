global flush_gdt 
section .text 
bits 64 
%define NEW_CS 0x08         ; Seletor do segmento de código (ring 0)
%define NEW_DS 0x10         ; Seletor do segmento de dados (ring 0)

; Input: RDI - Pointer to struct GDTR

flush_gdt:
  lgdt [rdi] ; Load the new GDT

  push qword NEW_CS ; New Code Selector
  lea  rax, [rel reload_segments]
  push rax
  lretq ; Far return to reload_segments with new segments

reload_segments:
  mov ax, NEW_DS ; New Data Selector
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; Reset Temporary Registers 
  xor rax, rax

  ; Return 
  ret


