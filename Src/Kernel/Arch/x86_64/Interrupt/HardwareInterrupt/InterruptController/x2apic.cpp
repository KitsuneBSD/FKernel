#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Hardware/cpu.h>
#include <LibFK/Algorithms/log.h>

// x2APIC MSRs
constexpr uint32_t X2APIC_BASE_MSR = 0x1B;
constexpr uint32_t X2APIC_MSR_ENABLE = 1 << 11;
constexpr uint32_t X2APIC_MSR_X2APIC_MODE = 1 << 10;

constexpr uint32_t X2APIC_EOI_MSR = 0x80B;
constexpr uint32_t X2APIC_SPURIOUS_MSR = 0x80F;
constexpr uint32_t X2APIC_LVT_TIMER_MSR = 0x832;
constexpr uint32_t X2APIC_INITIAL_COUNT_MSR = 0x838;
constexpr uint32_t X2APIC_CURRENT_COUNT_MSR = 0x839;
constexpr uint32_t X2APIC_DIVIDE_CONFIG_MSR = 0x83E;

constexpr uint8_t APIC_SPURIOUS_VECTOR = 0xFF;
constexpr uint8_t APIC_TIMER_VECTOR = 0x20;
constexpr uint32_t APIC_SVR_ENABLE = 1 << 8;
constexpr uint32_t APIC_LVT_MASK = 1 << 16;
constexpr uint32_t APIC_LVT_TIMER_MODE_PERIODIC = 1 << 17;
constexpr uint32_t APIC_TIMER_DIVISOR = 0x3;

void X2APIC::initialize() {
  kdebug("x2APIC", "Initializing Local x2APIC...");

  if (!CPU::the().has_x2apic()) {
    kerror("x2APIC", "x2APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(X2APIC_BASE_MSR);
  apic_msr |= X2APIC_MSR_ENABLE | X2APIC_MSR_X2APIC_MODE;
  CPU::the().write_msr(X2APIC_BASE_MSR, apic_msr);

  CPU::the().write_msr(X2APIC_SPURIOUS_MSR,
                       APIC_SPURIOUS_VECTOR | APIC_SVR_ENABLE);
  klog("x2APIC", "x2APIC initialized and spurious vector set to %u",
       APIC_SPURIOUS_VECTOR);

  calibrate_timer();
}

void X2APIC::send_eoi(uint8_t /*irq*/) {
  CPU::the().write_msr(X2APIC_EOI_MSR, 0);
}

void X2APIC::mask_interrupt(uint8_t irq) {
  kdebug("x2APIC", "Mask requested for IRQ %u (not supported on x2APIC)", irq);
}

void X2APIC::unmask_interrupt(uint8_t irq) {
  kdebug("x2APIC", "Unmask requested for IRQ %u (not supported on x2APIC)",
         irq);
}

void X2APIC::calibrate_timer() {
  CPU::the().write_msr(X2APIC_DIVIDE_CONFIG_MSR, APIC_TIMER_DIVISOR);
  CPU::the().write_msr(X2APIC_LVT_TIMER_MSR, APIC_LVT_MASK); // Mask timer
  CPU::the().write_msr(X2APIC_INITIAL_COUNT_MSR, 0xFFFFFFFF);

  constexpr uint64_t calib_ms = 10;
  TimerManager::the().sleep(calib_ms);

  uint32_t elapsed_ticks =
      0xFFFFFFFF - CPU::the().read_msr(X2APIC_CURRENT_COUNT_MSR);
  CPU::the().write_msr(X2APIC_LVT_TIMER_MSR, APIC_LVT_MASK);
  CPU::the().write_msr(X2APIC_INITIAL_COUNT_MSR, 0);

  apic_ticks_per_ms = elapsed_ticks / calib_ms;
  klog("x2APIC", "Timer calibrated: %lu ticks/ms", apic_ticks_per_ms);
}

void X2APIC::setup_timer(uint64_t ms) {
  if (apic_ticks_per_ms == 0) {
    calibrate_timer();
  }

  uint64_t initial = apic_ticks_per_ms * ms;
  CPU::the().write_msr(X2APIC_DIVIDE_CONFIG_MSR, APIC_TIMER_DIVISOR);
  CPU::the().write_msr(X2APIC_LVT_TIMER_MSR,
                       APIC_TIMER_VECTOR | APIC_LVT_TIMER_MODE_PERIODIC);
  CPU::the().write_msr(X2APIC_INITIAL_COUNT_MSR, initial);

  klog("x2APIC", "Periodic timer set: %lu ms (%lu ticks)", ms, initial);
}
