BITS 64
GLOBAL isr0, isr1, ..., isr255  ; ou use %rep para gerar

EXTERN interrupt_dispatch

section .text

%macro ISR_STUB 2
global isr%1
isr%1:
    ; For exceptions that do not push an error code, we push a dummy 0
    ; so that the stack layout always contains an error_code field.
    ; The second macro parameter should be 1 if the CPU already pushes an
    ; error code for this vector, or 0 otherwise.
    %if %2 = 0
      push 0
    %endif

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
    %if %2 = 0
      add rsp, 8    ; remove dummy error code
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
