#pragma once 

#include <Kernel/Arch/x86_64/Interrupt/interrupt_type.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/array.h>

class InterruptController {
private:
    array<IInterruptHandler*, MAX_X86_64_INTERRUPTS_LENGTH> m_handlers;
public:
    InterruptController();
    static InterruptController& the();


    void initialize();

    IInterruptHandler* get_handler(uint8_t interrupt_number);

    void register_handler(uint8_t interrupt_number, IInterruptHandler* handler);
    void unregister_handler(uint8_t interrupt_number);
};   