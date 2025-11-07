#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Hardware/Cpu.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

constexpr uint8_t APIC_SPURIOUS_VECTOR = 0xFF;
constexpr uint8_t APIC_TIMER_VECTOR = 0x20;
constexpr uint32_t APIC_LVT_MASK = 1 << 16;
constexpr uint32_t APIC_BASE_MSR = 0x1B;
constexpr uint32_t APIC_MSR_ENABLE = 1 << 11;
constexpr uintptr_t APIC_RANGE_SIZE = 0x1000;
constexpr uint32_t SPURIOUS = 0xF0;
constexpr uint32_t EOI = 0xB0;
constexpr uint32_t LVT_TIMER = 0x320;
constexpr uint32_t INITIAL_COUNT = 0x380;
constexpr uint32_t CURRENT_COUNT = 0x390;
constexpr uint32_t DIVIDE_CONFIG = 0x3E0;
constexpr uint32_t APIC_SVR_ENABLE = 1 << 8;
constexpr uint32_t APIC_LVT_TIMER_MODE_PERIODIC = 1 << 17;
constexpr uint32_t APIC_TIMER_DIVISOR = 0x3;

uint32_t APIC::read(uint32_t reg) {
  return *reinterpret_cast<volatile uint32_t *>(lapic_base + reg);
}

void APIC::write(uint32_t reg, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(lapic_base + reg) = value;
}

void APIC::initialize() {
  kdebug("APIC", "Initializing Local APIC...");

  if (!CPU::the().has_apic()) {
    kerror("APIC", "Local APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(APIC_BASE_MSR);
  uintptr_t apic_phys = apic_msr & 0xFFFFF000;
  kdebug("APIC", "APIC physical base detected at 0x%lx", apic_phys);

  apic_msr |= APIC_MSR_ENABLE;
  CPU::the().write_msr(APIC_BASE_MSR, apic_msr);

  for (uintptr_t offset = 0; offset < APIC_RANGE_SIZE; offset += PAGE_SIZE) {
    VirtualMemoryManager::the().map_page(
        apic_phys + offset, apic_phys + offset,
        PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
  }

  lapic_base = apic_phys;

  // Set spurious vector and enable APIC
  write(SPURIOUS, APIC_SPURIOUS_VECTOR | APIC_SVR_ENABLE);
  klog("APIC", "APIC initialized and spurious vector set to %u",
       APIC_SPURIOUS_VECTOR);

  // Calibrate timer for Strategy-compatible usage
  calibrate_timer();
}

void APIC::send_eoi(uint8_t /*irq*/) {
  if (lapic_base) {
    write(EOI, 0);
  } else {
    kdebug("APIC", "Cannot send EOI: LAPIC not mapped");
  }
}

void APIC::mask_interrupt(uint8_t irq) {
  kdebug("APIC", "Mask requested for IRQ %u (not supported on LAPIC)", irq);
}

void APIC::unmask_interrupt(uint8_t irq) {
  kdebug("APIC", "Unmask requested for IRQ %u (not supported on LAPIC)", irq);
}

void APIC::calibrate_timer() {
  if (!lapic_base)
    return;

  write(DIVIDE_CONFIG, APIC_TIMER_DIVISOR);
  write(LVT_TIMER, APIC_LVT_MASK); // Mask timer
  write(INITIAL_COUNT, 0xFFFFFFFF);

  constexpr uint64_t calib_ms = 10;
  PIT::the().sleep(calib_ms);

  uint32_t elapsed_ticks = 0xFFFFFFFF - read(CURRENT_COUNT);
  write(LVT_TIMER, APIC_LVT_MASK);
  write(INITIAL_COUNT, 0);

  apic_ticks_per_ms = elapsed_ticks / calib_ms;
  klog("APIC", "Timer calibrated: %lu ticks/ms", apic_ticks_per_ms);
}

void APIC::setup_timer(uint64_t ms) {
  if (!lapic_base || apic_ticks_per_ms == 0) {
    calibrate_timer();
  }

  uint64_t initial = apic_ticks_per_ms * ms;
  write(DIVIDE_CONFIG, APIC_TIMER_DIVISOR);
  write(LVT_TIMER, APIC_TIMER_VECTOR | APIC_LVT_TIMER_MODE_PERIODIC);
  write(INITIAL_COUNT, initial);

  klog("APIC", "Periodic timer set: %lu ms (%lu ticks)", ms, initial);
}
