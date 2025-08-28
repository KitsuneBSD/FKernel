BITS 64

extern irq_dispatch

section .text

%macro IRQ_HANDLER 1
    global irq%1_handler
irq%1_handler:
    push rbp
    mov rbp, rsp

    mov rdi, %1
    mov rsi, rbp

    call irq_dispatch

    pop rbp
    iretq
%endmacro

%assign i 0
%rep 16
    IRQ_HANDLER i
    %assign i i+1
%endrep
