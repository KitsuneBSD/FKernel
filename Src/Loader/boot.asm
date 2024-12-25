; Booting the kernel as legacy BIOS

; Disable interrupts 
cli 

; Lock the kernel
hang:
  jmp hang 

; fill the 512 bytes of the boot sector with 0
times 510-($-$$) db 0

; Signature of the boot 
db 0x55, 0xAA
