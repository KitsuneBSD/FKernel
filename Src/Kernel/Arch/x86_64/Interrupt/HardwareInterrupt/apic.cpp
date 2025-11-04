#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Hardware/Cpu.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

// Spurious interrupt vector
constexpr uint8_t APIC_SPURIOUS_VECTOR = 0xFF;
// APIC timer interrupt vector
constexpr uint8_t APIC_TIMER_VECTOR = 0x20; // IRQ 0
// LVT mask bit
constexpr uint32_t APIC_LVT_MASK = 1 << 16;

uint32_t APIC::read(uint32_t reg) {
  return *reinterpret_cast<volatile uint32_t *>(lapic_base + reg);
}

void APIC::write(uint32_t reg, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(lapic_base + reg) = value;
}

void APIC::enable() {
  kdebug("APIC", "Enabling Local APIC...");

  if (!CPU::the().has_apic()) {
    kerror("APIC", "Local APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(APIC_BASE_MSR);
  uintptr_t apic_phys = apic_msr & 0xFFFFF000;
  kdebug("APIC", "APIC physical base detected at 0x%lx", apic_phys);

  apic_msr |= APIC_MSR_ENABLE;
  CPU::the().write_msr(APIC_BASE_MSR, apic_msr);
  kdebug("APIC", "APIC enabled via MSR");

  for (uintptr_t offset = 0; offset < APIC_RANGE_SIZE; offset += PAGE_SIZE) {
    VirtualMemoryManager::the().map_page(
        apic_phys + offset, apic_phys + offset,
        PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
    kdebug("APIC", "Mapped APIC page at 0x%lx", apic_phys + offset);
  }

  lapic_base = apic_phys;

  // Set spurious vector and enable APIC
  write(SPURIOUS, APIC_SPURIOUS_VECTOR | APIC_SVR_ENABLE);
  kdebug("APIC", "Spurious vector set to %u and APIC enabled in SVR",
         APIC_SPURIOUS_VECTOR);

  klog("APIC", "Mapped APIC range [0x%lx - 0x%lx]", apic_phys,
       apic_phys + APIC_RANGE_SIZE);
}

void APIC::calibrate_timer() {
  if (!lapic_base) {
    kdebug("APIC", "Cannot calibrate timer: LAPIC not mapped");
    return;
  }

  kdebug("APIC", "Calibrating APIC timer...");

  write(DIVIDE_CONFIG, APIC_TIMER_DIVISOR);

  constexpr uint32_t test_count = 0xFFFFFFFF;
  write(LVT_TIMER, APIC_LVT_MASK); // Mask timer LVT entry
  write(INITIAL_COUNT, test_count);

  constexpr uint64_t calib_ms = 10; // Calibrate over 10 ms for better precision
  PIT::the().sleep(calib_ms);

  uint32_t elapsed_ticks = test_count - read(CURRENT_COUNT);
  write(LVT_TIMER, APIC_LVT_MASK); // Mask timer again
  write(INITIAL_COUNT, 0);         // Stop timer

  apic_ticks_per_ms = elapsed_ticks / calib_ms;

  klog("APIC", "Timer calibrated: %lu ticks/ms", apic_ticks_per_ms);
}

void APIC::setup_timer(uint64_t ms) {
  if (!lapic_base) {
    kdebug("APIC", "Cannot setup timer: LAPIC not mapped");
    return;
  }

  if (apic_ticks_per_ms == 0) {
    kdebug("APIC", "APIC ticks per ms not set, calibrating timer first");
    calibrate_timer();
  }

  uint64_t initial = apic_ticks_per_ms * ms;

  write(DIVIDE_CONFIG, APIC_TIMER_DIVISOR);
  write(LVT_TIMER, APIC_TIMER_VECTOR | APIC_LVT_TIMER_MODE_PERIODIC);
  write(INITIAL_COUNT, initial);

  klog("APIC", "Timer set: %lu ms (%lu ticks)", ms, initial);
}

void APIC::send_eoi() {
  if (lapic_base) {
    write(EOI, 0);
  } else {
    kdebug("APIC", "Cannot send EOI: LAPIC not mapped");
  }
}
