#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerController/hpet.h>
#include <Kernel/Hardware/acpi.h>
#include <Kernel/MemoryManager/MemoryManager.h>
#include <LibFK/Algorithms/log.h>

uint64_t HPETTimer::read_reg(uint64_t reg) {
  return m_hpet_regs[reg / sizeof(uint64_t)];
}

void HPETTimer::write_reg(uint64_t reg, uint64_t value) {
  m_hpet_regs[reg / sizeof(uint64_t)] = value;
}

void HPETTimer::initialize(uint32_t frequency) {
  auto hpet_table = ACPIManager::the().find_table("HPET");
  if (!hpet_table) {
    fk::algorithms::kerror("HPET", "HPET table not found!");
    return;
  }

  // The HPET table contains the base address of the HPET registers.
  // This is a simplified representation.
  uintptr_t hpet_phys_addr = 0; // Get this from hpet_table parsing
  // For now, let's assume a common address for QEMU for demonstration
  hpet_phys_addr = 0xFED00000;

  MemoryManager::the().map_page(hpet_phys_addr, hpet_phys_addr,
                                PageFlags::Present | PageFlags::Writable |
                                    PageFlags::WriteThrough);
  m_hpet_regs = reinterpret_cast<volatile uint64_t *>(hpet_phys_addr);

  m_counter_period = read_reg(GENERAL_CAPABILITIES_ID) >> 32;

  // 1. Disable HPET
  write_reg(GENERAL_CONFIGURATION, read_reg(GENERAL_CONFIGURATION) & ~1ULL);

  // 2. Reset main counter
  write_reg(MAIN_COUNTER_VALUE, 0);

  // 3. Configure Timer 0 for periodic mode
  uint64_t timer_config = read_reg(TIMER0_CONFIGURATION);
  timer_config |= (1ULL << 3); // Periodic mode
  timer_config |= (1ULL << 6); // Enable timer interrupt
  write_reg(TIMER0_CONFIGURATION, timer_config);

  // 4. Set frequency
  m_frequency = frequency;
  uint64_t comparator_ticks =
      (1000000000000000ULL / m_counter_period) / frequency;
  write_reg(TIMER0_COMPARATOR, comparator_ticks);
  // Set the comparator again to ensure it's set for periodic mode
  write_reg(TIMER0_COMPARATOR, read_reg(MAIN_COUNTER_VALUE) + comparator_ticks);

  // 5. Enable HPET
  write_reg(GENERAL_CONFIGURATION, read_reg(GENERAL_CONFIGURATION) | 1ULL);

  fk::algorithms::klog("TIMER", "Initializing HPET Timer at %u Hz", frequency);
}
