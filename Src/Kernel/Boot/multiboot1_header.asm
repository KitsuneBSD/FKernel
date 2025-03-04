; Declaring some constants needed to run in multiboot version 1

; Align loaded modules on page boundaries
MULTIBOOT_ALIGN equ 1 << 0

; Provides a memory map
MULTIBOOT_MEMORY_INFO equ 1 << 1

; This is a flag 'field' used in multiboot
MULTIBOOT_FLAGS equ MULTIBOOT_ALIGN | MULTIBOOT_MEMORY_INFO

; Magic number used to identify the header as multiboot compliant
MULTIBOOT_MAGIC equ 0x1BADB002

; Checksum used to prove we are multiboot compliant
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; Declare a multiboot header as section
section .multiboot_header
align 4
dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

; Create a stack to with 16 KiB following the SYSV ABI
; NOTE: Try change the value to 64 * 1024 or 512 * 1024
section .bss 
stack_bottom:
  resb 16384 ; 16 * 1024 
stack_top:

section .text
extern kernel_main
; Declare _start as a function symbol with given symbol size.
global _start:function (_start.end - _start)

_start:
  mov esp, stack_top
  call kernel_main
  cli

.hang: hlt
  jmp .hang

.end:
