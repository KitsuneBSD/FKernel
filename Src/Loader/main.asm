; Booting the kernel as legacy BIOS
; NOTE: This kernel is not a real kernel, it's just a simple boot loader from legacy BIOS

; Disable interrupts 
cli 

mov ax, 0x07C0
mov ds, ax
mov si, msg 
cld 

; TODO: Create a file to separe the print logic from the boot.asm
chr_loop:lodsb
  or al, al ; Check the character is null
  jz hang ; If null, jump to hang
  mov ah, 0x0E ; BIOS teletype function
  mov bh, 0 
  int 0x10
  jmp chr_loop

; Lock the kernel
hang:
  jmp hang 

msg db 'Hello FKernel', 13, 10, 0

; fill the 512 bytes of the boot sector with 0
times 510-($-$$) db 0

; Signature of the boot
db 0x55, 0xAA
