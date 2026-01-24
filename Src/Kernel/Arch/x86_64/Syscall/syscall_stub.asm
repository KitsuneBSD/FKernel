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

    ; Save user context to per-cpu data
    mov [gs:8], rsp   ; user_rsp
    mov [gs:16], rcx  ; saved_rip
    mov [gs:24], r11  ; saved_rflags

    ; Load kernel stack
    mov rsp, [gs:0]

    ; Push context for potential context switch/interruption handling
    push qword [gs:8]  ; User RSP
    push qword [gs:24] ; User RFLAGS
    push qword [gs:16] ; User RIP

    ; Save General Purpose Registers
    push rax 
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9

    ; Prepare C arguments
    push 0       ; Alignment padding
    push r9      ; Arg6
    
    mov r9, r8   ; Arg5
    mov r8, r10  ; Arg4
    mov rcx, rdx ; Arg3
    mov rdx, rsi ; Arg2
    mov rsi, rdi ; Arg1
    mov rdi, rax ; Num
    
    call syscall_dispatcher
    
global syscall_stub_post_dispatch
syscall_stub_post_dispatch:
    ; RAX now contains return value
    
    ; Cleanup Stack
    add rsp, 16  ; Remove Arg6 + Padding
    
    ; Restore Registers
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    add rsp, 8   ; discarding saved RAX
    
    ; Discard old stack context (we'll use values from GS)
    add rsp, 24
    
    ; Load return state from GS (it might have been changed by execve)
    mov rcx, [gs:16]
    mov r11, [gs:24]
    mov rsp, [gs:8]
    
    swapgs
    
    o64 sysret   ; Return to user mode (RCX->RIP, R11->RFLAGS)

section .bss
    global syscall_user_rsp
    syscall_user_rsp: resq 1
    
    global syscall_kernel_stack
    syscall_kernel_stack: resq 1
