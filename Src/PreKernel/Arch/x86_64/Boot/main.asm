global start
extern check_multiboot
extern check_long_mode
extern setup_page_tables
extern enable_paging
extern long_mode_start

extern stack_top
extern multiboot_magic
extern multiboot_info_ptr

extern gdt64.code_segment
extern gdt64.pointer

section .prekernel.text
bits 32

start:
    mov esp, stack_top

    call check_multiboot
    mov [multiboot_magic], eax
    mov [multiboot_info_ptr], ebx

    call check_long_mode

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start

.halt:
    hlt
    jmp .halt
