BITS 64
GLOBAL isr0, isr1, ..., isr255  ; ou use %rep para gerar

EXTERN interrupt_dispatch

section .text

%macro ISR_STUB 1
global isr%1
isr%1:
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

    mov rdi, %1
    lea rsi, [rsp]    ; frame_ptr
    call interrupt_dispatch

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
    iretq
%endmacro

%assign i 0
%rep 256
  ISR_STUB i
  %assign i i+1
%endrep
