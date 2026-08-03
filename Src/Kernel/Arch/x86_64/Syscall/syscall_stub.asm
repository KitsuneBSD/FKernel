[BITS 64]

global syscall_stub
extern syscall_dispatcher

section .text

syscall_stub:
    ; On entry:
    ; RCX = RIP (Next instruction after syscall)
    ; R11 = RFLAGS
    ; RAX = Syscall Number
    ; RDI, RSI, RDX, R10, R8, R9 = Arguments 1-6

    swapgs ; Switch to kernel GS

    ; Save user context and switch to kernel stack BEFORE any bounds check.
    ; The invalid_syscall_handler must also run on the kernel stack (Bug 24).
    mov [gs:8], rsp    ; user_rsp
    mov [gs:16], rcx   ; saved_rip
    mov [gs:24], r11   ; saved_rflags
    mov rsp, [gs:0]    ; switch to kernel stack

    ; Validation: Check if syscall number is within bounds
    cmp rax, 512
    jae invalid_syscall_handler

    ; Push context for potential context switch/interruption handling
    push qword [gs:8]  ; User RSP
    push qword [gs:24] ; User RFLAGS
    push qword [gs:16] ; User RIP

    ; Save General Purpose Registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    push rax 
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9

    ; Prepare C arguments
    ; RAX contains syscall number, RDI..R9 contains args 1-6
    lea r11, [rsp] ; Pointer to PtRegs
    
    sub rsp, 8   ; Align stack to 16 bytes
    push r11     ; Arg8: Pointer to PtRegs
    push r9      ; Arg7: Arg6
    
    mov r9, r8   ; Arg5
    mov r8, r10  ; Arg4
    mov rcx, rdx ; Arg3
    mov rdx, rsi ; Arg2
    mov rsi, rdi ; Arg1
    mov rdi, rax ; Num
    
    sti
    call syscall_dispatcher
    cli
    
global syscall_stub_post_dispatch
syscall_stub_post_dispatch:
    ; RAX now contains return value
    
    ; Cleanup Stack
    add rsp, 24  ; Remove Arg7 + Arg8 + Padding
    
    ; Restore Registers
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    add rsp, 8   ; discarding saved RAX
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    
    ; Discard old stack context (we'll use values from GS)
    add rsp, 24
    
    ; Load return state from GS (it might have been changed by execve)
    mov rcx, [gs:16]
    mov r11, [gs:24]
    mov rsp, [gs:8]
    
    swapgs
    
        o64 sysret   ; Return to user mode (RCX->RIP, R11->RFLAGS)
    
    
    
    extern syscall_validation_log
    
    invalid_syscall_handler:
    
        ; Restore user GS before calling log or returning (optional but safer if log panics)
    
        ; Actually, we are in kernel GS here (swapgs happened).
    
        ; We need to log and return error -ENOSYS (which is -38)
    
        
    
        ; Save volatile registers before calling C function
    
        push rax
    
        push rdi
    
        push rsi
    
        push rdx
    
        push rcx
    
        push r8
    
        push r9
    
        push r10
    
        push r11
    
        
    
        mov rdi, rax ; Pass syscall number
    
        call syscall_validation_log
    
        
    
        pop r11
    
        pop r10
    
        pop r9
    
        pop r8
    
        pop rcx
    
        pop rdx
    
        pop rsi
    
        pop rdi
    
        pop rax
    
        
    
        mov rax, -38 ; -ENOSYS
    
        swapgs       ; Restore user GS
    
        o64 sysret
    
    
    
