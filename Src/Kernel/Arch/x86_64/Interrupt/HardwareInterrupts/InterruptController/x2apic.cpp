#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic_common.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/msi_helpers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibFK/Algorithms/log.h>

void X2APIC::initialize() {
  if (m_is_initialized) {
    fk::algorithms::kdebug("X2APIC", "x2APIC already initialized.");
    return;
  }

  fk::algorithms::kdebug("X2APIC", "Initializing Local x2APIC...");

  if (!CPU::the().has_x2apic()) {
    fk::algorithms::kfatal("X2APIC", "x2APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(APIC_BASE_MSR);
  apic_msr |= APIC_MSR_ENABLE | APIC_MSR_X2APIC_MODE;
  CPU::the().write_msr(APIC_BASE_MSR, apic_msr);

  CPU::the().write_msr(X2APIC_SPURIOUS_MSR, APIC_SPURIOUS_VECTOR | APIC_SVR_ENABLE);
  fk::algorithms::klog("X2APIC",
                       "x2APIC initialized and spurious vector set to %u",
                       APIC_SPURIOUS_VECTOR);
  m_is_initialized = true;

  calibrate_timer();
}

void X2APIC::send_eoi(uint8_t /*irq*/) {
  CPU::the().write_msr(X2APIC_EOI_MSR, 0);
}

void X2APIC::mask_interrupt(uint8_t irq) {
  fk::algorithms::kdebug(
      "X2APIC", "Mask requested for IRQ %u (not supported on x2APIC)", irq);
}

void X2APIC::unmask_interrupt(uint8_t irq) {
  fk::algorithms::kdebug(
      "X2APIC", "Unmask requested for IRQ %u (not supported on x2APIC)", irq);
}

void X2APIC::calibrate_timer() {
  CPU::the().write_msr(X2APIC_DIVIDE_CONFIG_MSR, APIC_TIMER_DIVISOR);
  CPU::the().write_msr(X2APIC_LVT_TIMER_MSR, APIC_LVT_MASK);
  CPU::the().write_msr(X2APIC_INITIAL_COUNT_MSR, 0xFFFFFFFF);

  constexpr uint64_t calib_ms = 10;
  TimerManager::the().sleep(calib_ms);

  uint32_t elapsed_ticks =
      0xFFFFFFFF - static_cast<uint32_t>(CPU::the().read_msr(X2APIC_CURRENT_COUNT_MSR));
  CPU::the().write_msr(X2APIC_LVT_TIMER_MSR, APIC_LVT_MASK);
  CPU::the().write_msr(X2APIC_INITIAL_COUNT_MSR, 0);

  apic_ticks_per_ms = elapsed_ticks / calib_ms;
  fk::algorithms::klog("X2APIC", "Timer calibrated: %lu ticks/ms",
                       apic_ticks_per_ms);
}

void X2APIC::setup_timer(uint64_t frequency_hz) {
  if (apic_ticks_per_ms == 0)
    calibrate_timer();

  uint64_t interval_ms = 1000 / frequency_hz;
  uint64_t initial = apic_ticks_per_ms * interval_ms;
  CPU::the().write_msr(X2APIC_DIVIDE_CONFIG_MSR, APIC_TIMER_DIVISOR);
  CPU::the().write_msr(X2APIC_LVT_TIMER_MSR,
                       APIC_TIMER_VECTOR | APIC_LVT_TIMER_MODE_PERIODIC);
  CPU::the().write_msr(X2APIC_INITIAL_COUNT_MSR, initial);

  fk::algorithms::klog("X2APIC", "Periodic timer armed at %lu Hz (%lu ticks per period)",
                       frequency_hz, initial);
}

fk::core::Result<uint8_t, fk::core::Error>
X2APIC::allocate_msi_vector(const PciDevice& device) {
  return msi::allocate_msi_vector(device);
}

void X2APIC::enable_msi(uint8_t vector) { (void)vector; }
void X2APIC::disable_msi(uint8_t vector) { (void)vector; }

uint32_t X2APIC::get_id() const {
  return static_cast<uint32_t>(CPU::the().read_msr(X2APIC_ID_MSR) & 0xFFFFFFFF);
}

void X2APIC::send_ipi(uint8_t lapic_id, uint8_t vector, uint32_t delivery_mode) {
  uint64_t icr = (static_cast<uint64_t>(lapic_id) << 32) | delivery_mode | vector;
  CPU::the().write_msr(X2APIC_ICR_MSR, icr);
}

void X2APIC::wait_ipi_delivery() {
  asm volatile("pause");
}
