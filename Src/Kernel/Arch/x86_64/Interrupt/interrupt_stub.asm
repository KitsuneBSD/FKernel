BITS 64
GLOBAL isr0, isr1, ..., isr255  ; ou use %rep para gerar

EXTERN interrupt_dispatch

section .text

%macro ISR_STUB 1
global isr%1
isr%1:
    mov rdi, %1
    call interrupt_dispatch
    iretq
%endmacro

%assign i 0
%rep 256
  ISR_STUB i
  %assign i i+1
%endrep
