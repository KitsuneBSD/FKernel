BITS 64
extern interrupt_dispatch

section .text

%macro ISR_STUB 2
GLOBAL isr%1
isr%1:
    %if %2 = 0
        push 0
    %endif

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, %1
    mov rsi, rsp

    sub rsp, 8          ; alinhamento 16 bytes
    call interrupt_dispatch
    add rsp, 8

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    %if %2 = 0
        add rsp, 8
    %endif

    iretq
%endmacro

; Vectors which push an error code automatically: 8,10,11,12,13,14,17
%assign i 0
%rep 256
  ; check if i is in the error-code list
  %assign has_err 0
  %if i = 8
    %assign has_err 1
  %endif
  %if i = 10
    %assign has_err 1
  %endif
  %if i = 11
    %assign has_err 1
  %endif
  %if i = 12
    %assign has_err 1
  %endif
  %if i = 13
    %assign has_err 1
  %endif
  %if i = 14
    %assign has_err 1
  %endif
  %if i = 17
    %assign has_err 1
  %endif
  ISR_STUB i, has_err
  %assign i i+1
  %undef has_err
%endrep
