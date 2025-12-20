#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief APIC Timer implementation
 *
 * Uses APIC LVT Timer as a hardware interrupt. Ticks are counted
 * in the interrupt handler.
 */
class APICTimer : public Timer {
public:
  void initialize(uint32_t frequency) override;
};
