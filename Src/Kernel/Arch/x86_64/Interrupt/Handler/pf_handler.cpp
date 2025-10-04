#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <LibC/stdint.h>
#include <LibFK/Algorithms/log.h>

void page_fault_handler([[maybe_unused]] uint8_t vector,
                        InterruptFrame *frame) {
  uint64_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));

  kexception("PAGE FAULT", "Page Fault interrupt: vector=%u", (unsigned)vector);
  kexception("PAGE FAULT", "RIP=%lx CS=%lx RFLAGS=%lx", frame->rip, frame->cs,
             frame->rflags);
  kexception("PAGE FAULT", "RSP=%lx SS=%lx", frame->rsp, frame->ss);
  kexception("PAGE FAULT", "Fault addr (CR2)=%p, error=%lx", cr2,
             frame->error_code);

  if (frame->error_code & 1)
    kexception("PAGE FAULT", "  -> Page not present");
  else
    kexception("PAGE FAULT", "  -> Protection violation");

  if (frame->error_code & 2)
    kexception("PAGE FAULT", "  -> Write access");
  else
    kexception("PAGE FAULT", "  -> Read access");

  if (frame->error_code & 4)
    kexception("PAGE FAULT", "  -> From user-mode");
  if (frame->error_code & 8)
    kexception("PAGE FAULT", "  -> Reserved bit violation");
  if (frame->error_code & 16)
    kexception("PAGE FAULT", "  -> Instruction fetch");

  // Fault não recuperável — entra em loop
  for (;;) {
    asm volatile("cli;hlt");
  }
}
