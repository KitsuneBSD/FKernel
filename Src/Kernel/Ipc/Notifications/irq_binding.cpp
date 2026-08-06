#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Types/Ipc/notification_bits.h>

#include <Kernel/Ipc/Notifications/irq_binding.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>

namespace fkernel {
namespace ipc {

Endpoint* IrqBinding::s_endpoints[256];

fk::core::Result<void, fk::core::Error> IrqBinding::install(uint8_t vector, Endpoint* ep) {
    if (vector < 32) return fk::core::Error::InvalidParameter;
    if (s_endpoints[vector]) return fk::core::Error::AlreadyExists;

    s_endpoints[vector] = ep;
    InterruptController::the().register_interrupt(on_irq, vector);
    fk::algorithms::kdebug("IRQ_BINDING", "bound vector %u to endpoint %p", vector, ep);
    return {};
}

void IrqBinding::remove(uint8_t vector) {
    if (!s_endpoints[vector]) return;
    s_endpoints[vector] = nullptr;
    InterruptController::the().register_interrupt(nullptr, vector);
}

void IrqBinding::on_irq(uint8_t vector, [[maybe_unused]] InterruptFrame* frame) {
    HardwareInterruptManager::the().send_eoi(vector);
    if (s_endpoints[vector])
        s_endpoints[vector]->signal(fk::NotificationBits(1));
}

} // namespace ipc
} // namespace fkernel
