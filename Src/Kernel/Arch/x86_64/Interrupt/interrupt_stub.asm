BITS 64
GLOBAL isr0, isr1, ..., isr255  ; ou use %rep para gerar

EXTERN interrupt_dispatch

section .text

%macro ISR_STUB 2
global isr%1
isr%1:
    %if %2 = 0
      push 0
    %endif

    ; Check if we came from user mode (CS in bits 0-1 of the saved CS, which is at RSP+16 if we push RAX)
    ; But we haven't pushed anything yet besides error code.
    ; Stack: RSP+0: Error, RSP+8: RIP, RSP+16: CS
    cmp qword [rsp+16], 0x23
    jne .kernel_mode
    swapgs
.kernel_mode:

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

    push 0            ; Alignment padding
    mov rdi, %1
    lea rsi, [rsp]
    call interrupt_dispatch
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

    ; Check if we are returning to user mode
    cmp qword [rsp+16], 0x23 ; Error code is at RSP, RIP at RSP+8, CS at RSP+16
    jne .kernel_mode_return
    swapgs
.kernel_mode_return:

    add rsp, 8
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
