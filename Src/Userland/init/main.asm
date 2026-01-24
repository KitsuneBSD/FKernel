[BITS 64]

; Syscall numbers are handled by lib
extern sys_write
extern sys_read
extern sys_exit

section .text
global _start

_start:
    ; Print welcome message
    mov rdi, 1          ; fd = 1 (stdout)
    lea rsi, [rel msg]
    mov rdx, 33         ; length
    call sys_write

.loop:
    ; Read from stdin (blocking)
    mov rdi, 0          ; fd = 0 (stdin)
    lea rsi, [rsp - 512] ; use stack as buffer
    mov rdx, 512
    call sys_read

    test rax, rax
    js .exit
    jz .loop

    ; Write back to stdout
    mov rdx, rax        ; count from read
    mov rdi, 1
    lea rsi, [rsp - 512]
    call sys_write
    jmp .loop

.halt:
    hlt
    jmp .halt

.exit:
    mov rdi, 0
    call sys_exit

section .rodata
msg: db "FKernel Init Process (Ring 3)", 10, 0
