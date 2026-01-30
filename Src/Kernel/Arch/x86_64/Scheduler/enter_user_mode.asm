[BITS 64]

global enter_user_mode

section .rodata
    rip_error_msg db "RIP is NULL", 0
    rsp_error_msg db "RSP is NULL", 0

section .text
extern user_mode_validation_log

; void enter_user_mode(uint64_t user_rip, uint64_t user_rsp)
; rdi = user_rip
; rsi = user_rsp
enter_user_mode:
    ; Validate RIP
    test rdi, rdi
    jz .invalid_rip

    ; Validate RSP
    test rsi, rsi
    jz .invalid_rsp

    ; Swap GS to user base (0 initially)
    swapgs

    ; Push user SS, RSP, RFLAGS, CS, RIP and iretq into user context
    ; User data selector = GDT index 3 -> 0x18, with RPL=3 => 0x1B
    ; User code selector = GDT index 4 -> 0x20, with RPL=3 => 0x23

    push qword 0x1B
    push rsi
    push qword 0x202
    push qword 0x23
    push rdi
    iretq

.invalid_rip:
    mov rdi, rip_error_msg
    mov rsi, 0
    call user_mode_validation_log
    ud2

.invalid_rsp:
    mov rdi, rsp_error_msg
    mov rsi, 0
    call user_mode_validation_log
    ud2

