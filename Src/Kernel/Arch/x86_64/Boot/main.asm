global start
global current_pml4_ptr

extern kmain

extern check_multiboot 
extern check_long_mode 

extern setup_page_tables 
extern enable_paging

extern stack_top 
extern multiboot_magic 
extern multiboot_info_ptr

extern gdtr 
extern gdt64.pointer 
extern gdt64.code_segment

extern long_mode_start

section .text
bits 32

start:
	mov esp, stack_top

	call check_multiboot

	mov [multiboot_magic], eax
  	mov [multiboot_info_ptr], ebx

	call check_long_mode

	call setup_page_tables
	call enable_paging
  	
	lgdt [gdt64.pointer]
	jmp gdt64.code_segment:long_mode_start

	hlt

; TODO: The boot flow here is very linear and uses calls that assume
;       success. Consider introducing a lightweight BootManager in C++
;       (or a structured set of checks) that can provide better
;       diagnostic messages on failure and transition to fallback
;       recovery states instead of HLT.
; FIXME: cpuid/long mode checks should be more defensive: some CPUs may
;        return unexpected feature flags; prepare a clear compatibility
;        matrix and fail gracefully with a message on serial VGA.
