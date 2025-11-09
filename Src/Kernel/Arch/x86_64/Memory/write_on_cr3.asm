section .text
global write_on_cr3

; Replace the current PML4 / PDPT table by writing to CR3
; rdi = pointer to new table (must be 4KiB aligned)
write_on_cr3:
    cli                     ; disable interrupts

    ; verify 4KiB alignment
    test rdi, 0xFFF
    jnz .ud2_exception

    ; mask out lower 12 bits (safety)
    and rdi, 0xFFFFFFFFFFFFF000

    ; load CR3
    mov cr3, rdi

    sti                     ; re-enable interrupts
    ret

.ud2_exception:
    ud2                     ; invalid instruction triggers #UD
