#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <LibFK/log.h>

void nmi_handler(uint8_t vector) {
  klog("NMI", "Non-Maskable Interrupt triggered (vector %u)", vector);
}
