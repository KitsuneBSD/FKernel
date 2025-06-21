global idt_flush
extern idtp

section .text
idt_flush:
    lidt [rdi]
    ret
