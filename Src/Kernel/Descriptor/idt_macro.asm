section .text
global isr_stub_table

extern exception_handler

%macro Pushaq 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro Popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

%macro IsrNoErrStub 1
isr_stub_%1:
    push 0
    push %1
    Pushaq

    mov rdi, rsp
    call exception_handler

    Popaq
    add rsp, 16
    iretq
%endmacro

%assign i 0
%rep 32
    IsrNoErrStub i
    %assign i i+1
%endrep

section .data
isr_stub_table:
%assign i 0
%rep 32
    dq isr_stub_ %+ i
    %assign i i+1
%endrep

