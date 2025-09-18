BITS 64
GLOBAL irq0, irq1, ..., irq15  ; ou use %rep para gerar

EXTERN irq_dispatcher

section .text

%macro IRQ_STUB 1
global irq%1
irq%1:
    push rax
    mov rdi, %1
    call irq_dispatcher
    pop rax
    iretq
%endmacro

%assign i 0
%rep 16
  IRQ_STUB i
  %assign i i+1
%endrep
