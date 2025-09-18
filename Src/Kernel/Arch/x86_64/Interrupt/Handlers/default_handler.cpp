#include <Kernel/Arch/x86_64/Interrupt/handlerList.h>
#include <Kernel/Arch/x86_64/Interrupt/idt_types.h>

#include <LibC/stdio.h>

static const char *exception_names[MAX_X86_64_ISR_SIZE] = {
    "Divide Error (#DE)",
    "Debug (#DB)",
    "Non-Maskable Interrupt",
    "Breakpoint (#BP)",
    "Overflow (#OF)",
    "BOUND Range Exceeded (#BR)",
    "Invalid Opcode (#UD)",
    "Device Not Available (#NM)",
    "Double Fault (#DF)",
    "Coprocessor Segment Overrun (reserved)",
    "Invalid TSS (#TS)",
    "Segment Not Present (#NP)",
    "Stack-Segment Fault (#SS)",
    "General Protection Fault (#GP)",
    "Page Fault (#PF)",
    "Reserved",
    "x87 FPU Floating-Point Error (#MF)",
    "Alignment Check (#AC)",
    "Machine Check (#MC)",
    "SIMD Floating-Point Exception (#XM)",
    "Virtualization Exception (#VE)",
    "Control Protection Exception (#CP)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"};

void default_handler([[maybe_unused]] InterruptFrame *frame,
                     [[maybe_unused]] uint8_t vector) {
  if (vector < 32) {
    kprintf("Unhandled exception: vector=%u (%s)\n", (unsigned)vector,
            exception_names[vector]);
  }

  for (;;) {
    asm volatile("hlt");
  }
}
