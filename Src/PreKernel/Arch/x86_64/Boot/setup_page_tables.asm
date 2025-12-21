global setup_page_tables
global enable_paging

extern page_table_l2
extern page_table_l3
extern page_table_l4

section .prekernel.text
bits 32

; @brief Configure a initial page table mapping the first 1 GiB of memory
setup_page_tables:
    ; PML4[0] -> PML3
    mov eax, page_table_l3
    or eax, 0b11         ; present + RW
    mov [page_table_l4], eax

    ; PML3[0] -> PML2
    mov eax, page_table_l2
    or eax, 0b11
    mov [page_table_l3], eax

    ; PML2[0..511] -> 2MiB pages
    xor ecx, ecx
.loop:
    mov eax, ecx
    shl eax, 21           ; cada entry = 2 MiB
    or eax, 0x83B         ; present + rw + huge + cd + wt
    mov [page_table_l2 + ecx*8], eax
    inc ecx
    cmp ecx, 512
    jne .loop
    ret

enable_paging:
    ; load PML4
    mov eax, page_table_l4
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; enable LME (EFER.MSR)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8         ; LME
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31        ; PG
    mov cr0, eax

    ret
