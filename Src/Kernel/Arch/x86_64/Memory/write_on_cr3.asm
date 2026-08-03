section .rodata
    cr3_align_error_msg db "CR3 write error: Address not 4KiB aligned", 0

section .text
extern kernel_panic_log
global arch_write_cr3

; Replace the current PML4 / PDPT table by writing to CR3
; rdi = pointer to new table (must be 4KiB aligned)
; Note: cli/sti removed — CR3 write is atomic; unconditional sti breaks IF=0 callers.
arch_write_cr3:
    ; verify 4KiB alignment
    test rdi, 0xFFF
    jnz .alignment_error

    ; mask out lower 12 bits (safety)
    and rdi, 0xFFFFFFFFFFFFF000

    ; load CR3
    mov cr3, rdi
    ret

.alignment_error:
    mov rdi, cr3_align_error_msg
    call kernel_panic_log
    ud2                     ; invalid instruction triggers #UD
