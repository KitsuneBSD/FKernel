#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <LibFK/log.h>

InterruptController::InterruptController()
    : m_handlers{}
{
    for (auto& handler : m_handlers) {
        handler = nullptr;
    }
}

void InterruptController::initialize()
{
    klog("INTERRUPT_CONTROLLER", "Initialized interrupt controller");
}

InterruptController& InterruptController::the()
{
    static InterruptController controller;
    return controller;
}

void InterruptController::register_handler(uint8_t interrupt_number, IInterruptHandler* handler)
{
    m_handlers[interrupt_number] = handler;
}

void InterruptController::unregister_handler(uint8_t interrupt_number)
{
    m_handlers[interrupt_number] = nullptr;
}

IInterruptHandler* InterruptController::get_handler(uint8_t interrupt_number)
{
    return m_handlers[interrupt_number];
}