bits 64

extern global_default_handler

section .text

global isr_common_stub

%macro ISR_NO_ERRCODE 2
    global %1
%1:
    push qword %2          ; push interrupt_id
    push qword 0           ; push fake error_code = 0
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 2
    global %1
%1:
    push qword %2          ; push interrupt_id
    jmp isr_common_stub
%endmacro

ISR_NO_ERRCODE isr_divide_by_zero,      0
ISR_NO_ERRCODE isr_debug,               1
ISR_NO_ERRCODE isr_nmi,                 2
ISR_NO_ERRCODE isr_breakpoint,          3
ISR_NO_ERRCODE isr_overflow,            4
ISR_NO_ERRCODE isr_bound_range,         5
ISR_NO_ERRCODE isr_invalid_opcode,      6
ISR_NO_ERRCODE isr_device_na,           7
ISR_ERRCODE    isr_double_fault,        8
ISR_NO_ERRCODE isr_coprocessor_seg,     9
ISR_ERRCODE    isr_invalid_tss,         10
ISR_ERRCODE    isr_seg_not_present,     11
ISR_ERRCODE    isr_stack_fault,         12
ISR_ERRCODE    isr_gp_fault,            13
ISR_ERRCODE    isr_page_fault,          14
ISR_NO_ERRCODE isr_reserved_15,         15
ISR_NO_ERRCODE isr_fpu_error,           16
ISR_NO_ERRCODE isr_alignment_check,     17
ISR_NO_ERRCODE isr_machine_check,       18
ISR_NO_ERRCODE isr_simd_fp,             19
ISR_NO_ERRCODE isr_virtualization,      20
ISR_NO_ERRCODE isr_reserved_21,         21
ISR_NO_ERRCODE isr_reserved_22,         22
ISR_NO_ERRCODE isr_reserved_23,         23
ISR_NO_ERRCODE isr_reserved_24,         24
ISR_NO_ERRCODE isr_reserved_25,         25
ISR_NO_ERRCODE isr_reserved_26,         26
ISR_NO_ERRCODE isr_reserved_27,         27
ISR_NO_ERRCODE isr_reserved_28,         28
ISR_NO_ERRCODE isr_reserved_29,         29
ISR_NO_ERRCODE isr_reserved_30,         30
ISR_NO_ERRCODE isr_reserved_31,         31


isr_common_stub:
    cli

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; Agora a pilha está assim (topo -> base):
    ; [rax ... r15] [error_code] [interrupt_id] [return addr do isr macro ou cpu]

    mov rdi, rsp         ; ponteiro para r15 (primeiro registro salvo manualmente)

    sub rsp, 8           ; alinhamento ABI

    call global_default_handler

    add rsp, 8

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16          ; remove error_code e interrupt_id

    sti
    iretq
