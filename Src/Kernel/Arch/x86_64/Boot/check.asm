global check_multiboot
global check_long_mode
global check_cpuid 

extern error

%define MULTIBOOT2_MAGIC_BOOTLOADER 0x36d76289

check_multiboot:
	cmp eax, MULTIBOOT2_MAGIC_BOOTLOADER
	jne .multiboot2_fail
	ret 
.multiboot2_fail:
	mov al, 'M'
	jmp error

check_cpuid:
	push eax 
	push ebx
	push ecx
	push edx

	mov eax, 1
	cpuid
	test 1 << 29
	jz .cpuid_fail

	pop edx
	pop ecx
	pop ebx
	pop eax
	ret
.cpuid_fail:
	mov al, 'C'
	jmp error

check_long_mode:
	mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .fail_long
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .fail_long
    ret
.long_mode_fail:
	mov al, 'L'
	jmp error