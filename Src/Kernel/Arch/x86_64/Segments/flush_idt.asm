global flush_idt
extern idtp

section .text
flush_idt:
    lidt [rdi]
    ret
