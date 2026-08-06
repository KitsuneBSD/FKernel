#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Driver/Mouse/ps2_mouse.h>

void mouse_handler(uint8_t vector, InterruptFrame* frame) {
    (void)frame;
    PS2Mouse::the().irq_handler();
    HardwareInterruptManager::the().send_eoi(static_cast<uint8_t>(vector - 32));
}
