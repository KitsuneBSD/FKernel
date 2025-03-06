MULTIBOOT2_MAGIC equ 0xe85250d6
MULTIBOOT2_ARCH equ 0 ; protected mode i386

section .multiboot_header
header_start:
	dd MULTIBOOT2_MAGIC
	dd MULTIBOOT2_ARCH
	dd header_end - header_start
	dd 0x100000000 - (MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + (header_end - header_start))

	; end tag
	dw 0
	dw 0
	dd 8
header_end:
