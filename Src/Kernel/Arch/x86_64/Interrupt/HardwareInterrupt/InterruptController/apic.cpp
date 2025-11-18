#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Hardware/cpu.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <LibFK/Algorithms/log.h>

constexpr uint8_t APIC_SPURIOUS_VECTOR = 0xFF;
constexpr uint8_t APIC_TIMER_VECTOR = 0x20;

constexpr uint32_t APIC_BASE_MSR = 0x1B;
constexpr uint32_t APIC_MSR_ENABLE = 1 << 11;
constexpr uintptr_t APIC_RANGE_SIZE = 0x1000;

constexpr uint32_t REG_SPURIOUS = 0xF0;
constexpr uint32_t REG_EOI = 0xB0;
constexpr uint32_t REG_LVT_TIMER = 0x320;
constexpr uint32_t REG_INITIAL_COUNT = 0x380;
constexpr uint32_t REG_CURRENT_COUNT = 0x390;
constexpr uint32_t REG_DIVIDE_CONFIG = 0x3E0;

constexpr uint32_t APIC_SVR_ENABLE = 1 << 8;
constexpr uint32_t APIC_LVT_MASK = 1 << 16;
constexpr uint32_t APIC_LVT_TIMER_MODE_PERIODIC = 1 << 17;

// Divisor base (1, 2, 4, 8, 16, 32, 64, 128)
constexpr uint32_t APIC_TIMER_DIVISOR = 0x3; // divide by 16

void APIC::write(uint32_t reg, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(lapic_base + reg) = value;
}

uint32_t APIC::read(uint32_t reg) {
  return *reinterpret_cast<volatile uint32_t *>(lapic_base + reg);
}

void APIC::initialize() {
  fk::algorithms::kdebug("APIC", "Initializing Local APIC...");

  if (!CPU::the().has_apic()) {
    fk::algorithms::kerror("APIC", "Local APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(APIC_BASE_MSR);
  uintptr_t apic_phys = apic_msr & 0xFFFFF000;

  fk::algorithms::kdebug("APIC", "APIC physical base detected at 0x%lx",
                         apic_phys);

  // Enable APIC via MSR
  apic_msr |= APIC_MSR_ENABLE;
  CPU::the().write_msr(APIC_BASE_MSR, apic_msr);

  // Map LAPIC region (1 page)
  for (uintptr_t offset = 0; offset < APIC_RANGE_SIZE; offset += PAGE_SIZE) {
    VirtualMemoryManager::the().map_page(
        apic_phys + offset, apic_phys + offset,
        PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
  }

  lapic_base = apic_phys;

  // Enable LAPIC and set spurious interrupt vector
  write(REG_SPURIOUS, APIC_SPURIOUS_VECTOR | APIC_SVR_ENABLE);

  fk::algorithms::klog("APIC", "Local APIC enabled with spurious vector 0x%X",
                       APIC_SPURIOUS_VECTOR);
}

void APIC::send_eoi(uint8_t) {
  if (lapic_base)
    write(REG_EOI, 0);
}

void APIC::mask_interrupt(uint8_t irq) {
  fk::algorithms::kdebug(
      "APIC", "Mask request for IRQ %u ignored (LAPIC doesn't mask that way)",
      irq);
}

void APIC::unmask_interrupt(uint8_t irq) {
  fk::algorithms::kdebug(
      "APIC",
      "Unmask request for IRQ %u ignored (LAPIC doesn't unmask that way)", irq);
}

void APIC::calibrate_timer() {
  if (!lapic_base)
    return;

  write(REG_DIVIDE_CONFIG, APIC_TIMER_DIVISOR);
  write(REG_LVT_TIMER, APIC_LVT_MASK); // Temporarily mask timer
  write(REG_INITIAL_COUNT, 0xFFFFFFFF);

  // Espera 10ms usando fonte externa (PIT, HPET ou delay calibrado)
  constexpr uint64_t calib_ms = 10;
  TimerManager::the().sleep(calib_ms);

  uint32_t elapsed = 0xFFFFFFFF - read(REG_CURRENT_COUNT);

  write(REG_INITIAL_COUNT, 0);
  apic_ticks_per_ms = elapsed / calib_ms;

  fk::algorithms::klog("APIC", "APIC timer calibrated: %u ticks/ms",
                       apic_ticks_per_ms);
}

void APIC::setup_timer(uint64_t ms) {
  if (!lapic_base)
    initialize();

  if (apic_ticks_per_ms == 0)
    calibrate_timer();

  uint64_t interval_ms = 1000 / ms;
  uint64_t initial_ticks = apic_ticks_per_ms * interval_ms;

  write(REG_DIVIDE_CONFIG, APIC_TIMER_DIVISOR);
  write(REG_LVT_TIMER, APIC_TIMER_VECTOR | APIC_LVT_TIMER_MODE_PERIODIC);
  write(REG_INITIAL_COUNT, static_cast<uint32_t>(initial_ticks));

  fk::algorithms::klog("APIC",
                       "APIC timer armed at %u Hz (%u ticks per period)", ms,
                       (uint32_t)initial_ticks);
}
