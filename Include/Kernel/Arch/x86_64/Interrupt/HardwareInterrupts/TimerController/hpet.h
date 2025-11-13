#pragma once

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <LibFK/Types/types.h>

class HPETTimer : public Timer {
private:
    uint64_t m_ticks = 0;
    uint32_t m_frequency = 0;
    volatile uint64_t* m_hpet_regs = nullptr;
    uint64_t m_counter_period = 0; // in femtoseconds (10^-15)

    uint64_t read_reg(uint64_t reg);
    void write_reg(uint64_t reg, uint64_t value);

public:
    void initialize(uint32_t frequency) override;
    void increment_ticks() override { m_ticks++; }
    uint64_t get_ticks() override { return m_ticks; }
    void sleep(uint64_t ms) override;
};
