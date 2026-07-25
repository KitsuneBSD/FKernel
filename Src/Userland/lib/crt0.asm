[BITS 64]

section .bss
global environ
environ: resq 1         ; char** environ — set from envp before main()

section .text
global _start
extern main
extern sys_exit

_start:
    ; System V ABI entry point
    ; [rsp] = argc
    ; [rsp + 8] = argv[0]
    ; ...

    xor rbp, rbp        ; Mark end of stack trace

    mov rdi, [rsp]      ; Load argc into RDI
    lea rsi, [rsp + 8]  ; Load argv pointer into RSI (address of first argv pointer)

    ; Calculate envp: it's after argv (argc pointers + 1 NULL pointer)
    mov rax, rdi
    add rax, 1          ; argc + 1 (for NULL)
    shl rax, 3          ; * 8 (sizeof pointer)
    lea rdx, [rsi + rax] ; envp = argv + (argc + 1) * 8

    ; Initialize environ global
    mov [rel environ], rdx

    call main           ; main(argc, argv, envp)

    ; If main returns, exit process
    mov rdi, rax
    call sys_exit
