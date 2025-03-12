global idt_flush
section .text

idt_flush:
  lidt [rdi]
  ret
