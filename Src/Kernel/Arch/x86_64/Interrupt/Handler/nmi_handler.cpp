#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/log.h>

void nmi_handler([[maybe_unused]] uint8_t vector) {
  kerror("NMI", "Non-Maskable Interrupt triggered (vector %u)", vector);
  for (;;) {
    asm volatile("hlt");
  }
}
