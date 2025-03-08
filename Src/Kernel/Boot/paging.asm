global setup_page_tables
global enable_paging

section .bss
align 4096
page_table_l4:
  resb 4096
page_table_l3:
  resb 4096
page_table_l2:
  resb 4096

section .text
bits 32
setup_page_tables:
  mov eax, page_table_l3
  or eax, 0b11
  mov [page_table_l4], eax

  mov eax, page_table_l2
  or eax, 0b11
  mov [page_table_l2], eax

  mov ecx, 0
.loop:
  mov eax, 0x200000
  mul ecx
  or eax, 0b10000011
  mov [page_table_l2 + ecx * 8], eax

  inc ecx 
  cmp ecx, 512
  jne .loop

  ret

enable_paging:
  mov eax, page_table_l4
  mov cr3, eax

  mov eax, cr4
  or eax, 1 << 5
  mov cr4, eax

  mov ecx, 0xC0000080
  rdmsr
  or eax, 1 << 8
  wrmsr

  mov eax, cr0
  or eax, 1 << 31
  mov cr0, eax

  ret
