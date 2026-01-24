[BITS 64]

extern sys_write
extern sys_read
extern sys_mkdir
extern sys_readdir

section .text
global _start

_start:
    ; Print welcome
    mov rdi, 1
    lea rsi, [rel msg_welcome]
    mov rdx, 22
    call sys_write

.prompt:
    mov rdi, 1
    lea rsi, [rel msg_prompt]
    mov rdx, 2
    call sys_write

    ; Read input
    mov rdi, 0
    lea rsi, [rel buffer]
    mov rdx, 128
    call sys_read
    
    test rax, rax
    jz .prompt
    
    ; Echo back
    mov rdx, rax
    mov rdi, 1
    lea rsi, [rel buffer]
    call sys_write
    jmp .prompt

section .data
msg_welcome: db "FKernel Shell started", 10, 0
msg_prompt:  db "> ", 0
buffer: times 128 db 0
