global setup_page_tables 
global enable_paging

extern page_table_l2 
extern page_table_l3 
extern page_table_l4 

section .text 
bits 32
setup_page_tables:
	mov eax, page_table_l3
	or eax, 0b11 ; present, writable
	mov [page_table_l4], eax
	
  mov eax, page_table_l2
	or eax, 0b11 ; present, writable
	mov [page_table_l3], eax

	; Fill 512 huge-page (2 MiB) PDE entries by additive accumulation.
	; Starting address 0x0 | flags 0x83 (P+RW+PS, WB caching).
	; Per Intel SDM Vol.3A, PWT=1+PCD=1 simultaneously is reserved.
	; Adding 0x200000 per iteration avoids mul and the repeated OR.
	mov ecx, 0
	mov eax, 0b10000011 ; base=0x0, flags: P+RW+PS (Write-Back)
.loop:
	mov [page_table_l2 + ecx * 8], eax
	add eax, 0x00200000       ; advance by 2 MiB
	inc ecx
	cmp ecx, 512
	jne .loop

	ret

enable_paging:
	; pass page table location to cpu
	mov eax, page_table_l4
	mov cr3, eax

	; enable PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	; enable long mode
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; enable paging
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	ret


