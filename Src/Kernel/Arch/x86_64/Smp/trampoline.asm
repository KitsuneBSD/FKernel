; AP Trampoline — 16-bit real-mode → 64-bit long-mode bootstrap
; Loaded by BSP at physical address 0x8000 (vector 0x08 for SIPI).
; Data area at offset 0xF80 contains PML4, stack, entry point provided by BSP.

[bits 16]
section .text
global trampoline_start
global trampoline_end
global trampoline_data

trampoline_start:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Load temporary 32-bit GDT
    lgdt [gdt32_desc - trampoline_start + 0x8000]

    ; Enable protected mode
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    ; Far jump to flush prefetch queue → 32-bit code
    jmp dword 0x08:(protected_mode - trampoline_start + 0x8000)

[bits 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Enable PAE (CR4.PAE)
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    ; Load PML4 from data area (offset 0xF80 in page)
    mov eax, [0x8F80]
    mov cr3, eax

    ; Enable long mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or  eax, 1 << 8
    wrmsr

    ; Enable paging (CR0.PG)
    mov eax, cr0
    or  eax, 1 << 31
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64_desc - trampoline_start + 0x8000]

    ; Far jump to 64-bit code
    jmp dword 0x08:(long_mode - trampoline_start + 0x8000)

[bits 64]
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Load stack pointer from data area
    mov rsp, [0x8F88]

    ; Get entry point from data area
    mov rax, [0x8F90]

    ; Call ap_entry(cpu_index) — cpu_index at 0x8F98
    mov edi, [0x8F98]
    call rax

    cli
    hlt
    jmp $

align 16
gdt32:
    dq 0x0000000000000000  ; null
    dq 0x00CF9A000000FFFF  ; 32-bit code: base=0 limit=4G
    dq 0x00CF92000000FFFF  ; 32-bit data: base=0 limit=4G
gdt32_end:

gdt32_desc:
    dw gdt32_end - gdt32 - 1
    dd gdt32 - trampoline_start + 0x8000

align 16
gdt64:
    dq 0x0000000000000000  ; null
    dq 0x00209A0000000000  ; 64-bit code
    dq 0x0000920000000000  ; 64-bit data
gdt64_end:

gdt64_desc:
    dw gdt64_end - gdt64 - 1
    dd gdt64 - trampoline_start + 0x8000

trampoline_data:
    ; offset 0xF80 in the page — BSP fills these in:
    dq 0  ; +0x00: PML4 physical address (cr3)
    dq 0  ; +0x08: AP stack pointer (rsp)
    dq 0  ; +0x10: Entry function pointer (ap_entry)
    dd 0  ; +0x18: CPU index
    dq 0  ; +0x20: online flag (AP sets to 1 when ready)

trampoline_end:
