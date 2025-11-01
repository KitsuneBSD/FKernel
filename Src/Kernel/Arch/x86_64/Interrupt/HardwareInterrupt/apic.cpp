#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Hardware/Cpu.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

auto cpu_instance = CPU::the();

void APIC::enable() {
  kdebug("APIC", "Enabling Local APIC...");

  if (!CPU::the().has_apic()) {
    kerror("APIC", "Local APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(APIC_BASE_MSR);
  uintptr_t apic_phys = apic_msr & 0xFFFFF000;
  kdebug("APIC", "APIC physical base detected at 0x%lx", apic_phys);

  apic_msr |= APIC_ENABLE;
  CPU::the().write_msr(APIC_BASE_MSR, apic_msr);
  kdebug("APIC", "APIC enabled via MSR");

  for (uintptr_t offset = 0; offset < APIC_RANGE_SIZE; offset += PAGE_SIZE) {
    VirtualMemoryManager::the().map_page(
        apic_phys + offset, apic_phys + offset,
        PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
    kdebug("APIC", "Mapped APIC page at 0x%lx", apic_phys + offset);
  }

  lapic = reinterpret_cast<local_apic *>(apic_phys);

  lapic->spurious = APIC_SPURIOUS | APIC_SVR_ENABLE;
  kdebug("APIC", "Spurious vector set and APIC enabled in SVR");

  klog("APIC", "Mapped APIC range [0x%lx - 0x%lx]", apic_phys,
       apic_phys + APIC_RANGE_SIZE);
}

void APIC::calibrate_timer() {
  if (!lapic) {
    kdebug("APIC", "Cannot calibrate timer: LAPIC not mapped");
    return;
  }

  kdebug("APIC", "Calibrating APIC timer...");

  lapic->divide_config = APIC_TIMER_DIVISOR;

  constexpr uint32_t test_count = 0x100000;
  lapic->lvt_timer = 0x10000; // timer mascarado
  lapic->initial_count = test_count;

  constexpr uint64_t calib_ms = 1; // 1 ms
  PIT::the().sleep(calib_ms);

  uint32_t elapsed_ticks = test_count - lapic->current_count;
  apic_ticks_per_ms = elapsed_ticks / calib_ms;

  kdebug("APIC", "Timer calibration complete: %lu ticks/ms", apic_ticks_per_ms);
  klog("APIC", "Timer calibrated: %lu ticks/ms", apic_ticks_per_ms);
}

void APIC::setup_timer(uint64_t ms) {
  if (!lapic) {
    kdebug("APIC", "Cannot setup timer: LAPIC not mapped");
    return;
  }

  if (apic_ticks_per_ms == 0) {
    kdebug("APIC", "APIC ticks per ms not set, calibrating timer first");
    calibrate_timer();
  }

  uint64_t initial = apic_ticks_per_ms * ms;

  lapic->divide_config = APIC_TIMER_DIVISOR;
  lapic->lvt_timer = 0x20 | APIC_LVT_TIMER_MODE_PERIODIC;
  lapic->initial_count = initial;

  kdebug("APIC", "APIC timer configured for %lu ms (%lu ticks)", ms, initial);
  klog("APIC", "Timer set: %lu ms (%lu ticks)", ms, initial);
}

void APIC::send_eoi() {
  if (lapic) {
    lapic->eoi = 0;
    kdebug("APIC", "EOI sent to LAPIC");
  } else {
    kdebug("APIC", "Cannot send EOI: LAPIC not mapped");
  }
}
