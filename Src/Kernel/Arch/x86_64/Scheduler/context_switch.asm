global switch_context
global task_trampoline
global fork_child_trampoline
extern syscall_stub_post_dispatch

section .text
bits 64

; void switch_context(uint64_t* prev_stack_ptr, uint64_t next_stack_ptr,
;                     void* prev_fx, void* next_fx)
; rdi = pointer to prev_stack_ptr
; rsi = next_stack_ptr
; rdx = prev_fx (16-byte aligned, always valid)
; rcx = next_fx (16-byte aligned, always valid)
switch_context:
    ; Save FPU/SSE state for outgoing task
    fxsave [rdx]

    ; Save callee-saved registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; Save current stack pointer
    mov [rdi], rsp

    ; Switch to new stack
    mov rsp, rsi

    ; Restore callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ; Restore FPU/SSE state for incoming task
    fxrstor [rcx]

    ret

; Trampoline for new tasks to load arguments
; r12 = arg1, r13 = arg2, r14 = entry_point
task_trampoline:
    mov rdi, r12
    mov rsi, r13
    call r14
    
    ; If the task returns, we are in trouble. 
    ; In a real kernel, we would call exit() here.
.loop:
    hlt
    jmp .loop

fork_child_trampoline:
    xor rax, rax ; return 0 for child
    ; The syscall_stub_post_dispatch expects to cleanup 24 bytes of arguments/padding
    ; from the stack before restoring registers. We must match this layout.
    sub rsp, 24
    jmp syscall_stub_post_dispatch
