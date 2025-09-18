BITS 64
GLOBAL isr0, isr1, ..., isr31  ; ou use %rep para gerar

EXTERN isr_dispatcher

section .text

%macro ISR_STUB 1
global isr%1
isr%1:
    push rax
    mov rdi, %1
    call isr_dispatcher
    pop rax
    iretq
%endmacro

%assign i 0
%rep 32
  ISR_STUB i
  %assign i i+1
%endrep
