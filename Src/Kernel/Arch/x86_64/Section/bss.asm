global multiboot_magic 
global multiboot_info_ptr

global current_pml4_ptr 

global page_table_l4 
global page_table_l3 
global page_table_l2

global stack_bottom 
global stack_top

section .bss
align 8 
multiboot_magic: resd 1 
multiboot_info_ptr: resq 1
current_pml4_ptr: resq 1
align 4096
page_table_l4:
	resb 4096
page_table_l3:
	resb 4096
page_table_l2:
	resb 4096
; Per-CPU kernel stacks: MAX_CPUS=32 slots × KERNEL_STACK_SIZE=16 KiB = 512 KiB.
; Slot i top = stack_bottom + 16KiB*(i+1). BSP boot uses stack_top (slot 31).
stack_bottom:
	resb 4096 * 16 * 32
stack_top:
