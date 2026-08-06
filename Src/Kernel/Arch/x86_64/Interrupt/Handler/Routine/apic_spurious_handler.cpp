#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>

void apic_spurious_handler([[maybe_unused]] uint8_t vector,
                           [[maybe_unused]] InterruptFrame* frame) {
    fk::algorithms::kdebug("APIC", "spurious interrupt (vector 0xFF)");
}
