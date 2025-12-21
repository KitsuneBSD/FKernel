global long_mode_start

extern kmain
extern stack_top
extern current_pml4_ptr
extern page_table_l4

extern multiboot_magic
extern multiboot_info_ptr

section .prekernel.text
bits 64

long_mode_start:
    mov ax, 0x10
    mov ss, ax
    mov rsp, stack_top
    and rsp, -16

    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rdi, rdi
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    lea rax, [page_table_l4]
    mov [current_pml4_ptr], rax

    mov rdi, qword [multiboot_magic]
    mov rsi, qword [multiboot_info_ptr]

    call kmain

.hang:
    hlt
    jmp .hang
