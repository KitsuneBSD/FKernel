global switch_context
global task_trampoline
global fork_child_trampoline
extern syscall_stub_post_dispatch

section .text
bits 64

; void switch_context(uint64_t* prev_stack_ptr, uint64_t next_stack_ptr)
; rdi = pointer to prev_stack_ptr
; rsi = next_stack_ptr
;
; FPU save/restore is LAZY: handled by #NM (#DeviceNotAvailable) handler.
; We set CR0.TS here so the next task traps into the #NM handler if it
; uses FPU before the scheduler has loaded its FPU state.
switch_context:
    ; Mark FPU as unavailable for the incoming task
    mov rax, cr0
    or  rax, 8          ; CR0.TS (bit 3) = 1 → trap FPU access
    mov cr0, rax

    ; Save callee-saved registers of outgoing task
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; Save current stack pointer
    mov [rdi], rsp

    ; Switch to incoming task's stack
    mov rsp, rsi

    ; Restore callee-saved registers of incoming task
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret

; Trampoline for new tasks to load arguments
; r12 = arg1, r13 = arg2, r14 = entry_point
task_trampoline:
    mov rdi, r12
    mov rsi, r13
    call r14

    ; If the task returns, spin
.loop:
    hlt
    jmp .loop

fork_child_trampoline:
    xor rax, rax ; return 0 for child
    sub rsp, 24
    ; Sync the child's fork-time user context from PtRegs into g_cpu_block so that
    ; syscall_stub_post_dispatch returns to the right user RSP/RIP/RFLAGS.
    ;
    ; After sub rsp,24: PtRegs base = rsp+24, so:
    ;   PtRegs.rip    = [rsp+128]   (base+104)
    ;   PtRegs.rflags = [rsp+136]   (base+112)
    ;   PtRegs.rsp    = [rsp+144]   (base+120)
    mov rcx, [rsp + 128]   ; PtRegs.rip
    mov [gs:16], rcx       ; g_cpu_block.saved_rip
    mov rcx, [rsp + 136]   ; PtRegs.rflags
    mov [gs:24], rcx       ; g_cpu_block.saved_rflags
    mov rcx, [rsp + 144]   ; PtRegs.rsp
    mov [gs:8], rcx        ; g_cpu_block.user_rsp
    jmp syscall_stub_post_dispatch
