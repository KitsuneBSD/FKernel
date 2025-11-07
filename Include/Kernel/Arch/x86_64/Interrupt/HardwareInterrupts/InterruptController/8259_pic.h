#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <LibFK/Types/types.h>

/**
 * @brief Intel 8259 Programmable Interrupt Controller (PIC)
 *
 */
class PIC8259 : public HardwareInterrupt {
private:
    static uint16_t get_irr();
    static uint16_t get_isr();

public:
    void initialize() override;
    void mask_interrupt(uint8_t irq) override;
    void unmask_interrupt(uint8_t irq) override;
    void send_eoi(uint8_t irq) override;

    /**
     * @brief Disable 8259PIC 
     */
    void disable();
};
