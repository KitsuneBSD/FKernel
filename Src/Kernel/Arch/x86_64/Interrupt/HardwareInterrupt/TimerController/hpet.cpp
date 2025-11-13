#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <LibFK/Algorithms/log.h>
#include <Kernel/Hardware/ACPI/acpi.h>

// HPET Registers
namespace HPET {
    constexpr uint64_t GENERAL_CAPABILITIES_ID = 0x00;
    constexpr uint64_t GENERAL_CONFIGURATION = 0x10;
    constexpr uint64_t MAIN_COUNTER_VALUE = 0xF0;
    constexpr uint64_t TIMER0_CONFIGURATION = 0x100;
    constexpr uint64_t TIMER0_COMPARATOR = 0x108;
}

uint64_t HPETTimer::read_reg(uint64_t reg) {
    return m_hpet_regs[reg / sizeof(uint64_t)];
}

void HPETTimer::write_reg(uint64_t reg, uint64_t value) {
    m_hpet_regs[reg / sizeof(uint64_t)] = value;
}

void HPETTimer::initialize(uint32_t frequency) {
    // FIXME: We need a proper ACPI parser to get the HPET address.
    // For now, we assume it's at a known location if found.
    auto hpet_table = ACPI::the().find_table("HPET");
    if (!hpet_table) {
        kerror("HPET", "HPET table not found!");
        return;
    }

    // The HPET table contains the base address of the HPET registers.
    // This is a simplified representation.
    uintptr_t hpet_phys_addr = 0; // Get this from hpet_table parsing
    // For now, let's assume a common address for QEMU for demonstration
    hpet_phys_addr = 0xFED00000;

    VirtualMemoryManager::the().map_page(hpet_phys_addr, hpet_phys_addr, PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
    m_hpet_regs = reinterpret_cast<volatile uint64_t*>(hpet_phys_addr);

    m_counter_period = read_reg(HPET::GENERAL_CAPABILITIES_ID) >> 32;

    // 1. Disable HPET
    write_reg(HPET::GENERAL_CONFIGURATION, read_reg(HPET::GENERAL_CONFIGURATION) & ~1ULL);

    // 2. Reset main counter
    write_reg(HPET::MAIN_COUNTER_VALUE, 0);

    // 3. Configure Timer 0 for periodic mode
    uint64_t timer_config = read_reg(HPET::TIMER0_CONFIGURATION);
    timer_config |= (1ULL << 3); // Periodic mode
    timer_config |= (1ULL << 6); // Enable timer interrupt
    write_reg(HPET::TIMER0_CONFIGURATION, timer_config);

    // 4. Set frequency
    m_frequency = frequency;
    uint64_t comparator_ticks = (1000000000000000ULL / m_counter_period) / frequency;
    write_reg(HPET::TIMER0_COMPARATOR, comparator_ticks);
    // Set the comparator again to ensure it's set for periodic mode
    write_reg(HPET::TIMER0_COMPARATOR, read_reg(HPET::MAIN_COUNTER_VALUE) + comparator_ticks);


    // 5. Enable HPET
    write_reg(HPET::GENERAL_CONFIGURATION, read_reg(HPET::GENERAL_CONFIGURATION) | 1ULL);

    klog("TIMER", "Initializing HPET Timer at %u Hz", frequency);
}

void HPETTimer::sleep(uint64_t ms) {
    uint64_t start_counter = read_reg(HPET::MAIN_COUNTER_VALUE);
    uint64_t ticks_to_wait = (ms * 1000000000000ULL) / m_counter_period;
    uint64_t target_counter = start_counter + ticks_to_wait;

    while (read_reg(HPET::MAIN_COUNTER_VALUE) < target_counter) {
        asm volatile("pause");
    }
    // TODO: Replace busy-wait with a proper task scheduling mechanism.
}
