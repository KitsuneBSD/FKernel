section .bss
align 16
global stack_top
global stack_bottom

stack_bottom:
  resb 640 * 1024
stack_top:
