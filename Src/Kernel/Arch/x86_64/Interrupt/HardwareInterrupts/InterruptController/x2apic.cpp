#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Pci/pci_device.h>
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
  if (m_is_initialized) {
    fk::algorithms::kdebug("x2APIC", "x2APIC already initialized.");
    return;
  }

  fk::algorithms::kdebug("x2APIC", "Initializing Local x2APIC...");

  if (!CPU::the().has_x2apic()) {
    fk::algorithms::kerror("x2APIC", "x2APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(X2APIC_BASE_MSR);
  apic_msr |= X2APIC_MSR_ENABLE | X2APIC_MSR_X2APIC_MODE;
  CPU::the().write_msr(X2APIC_BASE_MSR, apic_msr);

  CPU::the().write_msr(X2APIC_SPURIOUS_MSR,
                       APIC_SPURIOUS_VECTOR | APIC_SVR_ENABLE);
  fk::algorithms::klog("x2APIC",
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
      "x2APIC", "Mask requested for IRQ %u (not supported on x2APIC)", irq);
}

void X2APIC::unmask_interrupt(uint8_t irq) {
  fk::algorithms::kdebug(
      "x2APIC", "Unmask requested for IRQ %u (not supported on x2APIC)", irq);
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
  fk::algorithms::klog("x2APIC", "Timer calibrated: %lu ticks/ms",
                       apic_ticks_per_ms);
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

  fk::algorithms::klog("x2APIC", "Periodic timer set: %lu ms (%lu ticks)", ms,
                       initial);
}

extern uint8_t g_next_msi_vector;

fk::core::Result<uint8_t, fk::core::Error> X2APIC::allocate_msi_vector(const PciDevice& device) {
    uint8_t msi_ptr = device.find_capability(0x05); // MSI Capability ID
    if (msi_ptr == 0) return fk::core::Error::NotImplemented;

    uint8_t vector = g_next_msi_vector++;
    if (vector >= 0xF0) return fk::core::Error::OutOfMemory;

    uint32_t msi_addr = static_cast<uint32_t>(
        CPU::the().read_msr(X2APIC_BASE_MSR) & 0xFFFFF000ULL);
    device.write_config_dword(msi_ptr + 4, msi_addr);

    uint16_t msg_ctrl = device.read_config_word(msi_ptr + 2);
    if (msg_ctrl & (1 << 7)) { // 64-bit support
        device.write_config_dword(msi_ptr + 8, 0);
        device.write_config_word(msi_ptr + 12, vector);
    } else {
        device.write_config_word(msi_ptr + 8, vector);
    }

    msg_ctrl |= 0x01;
    device.write_config_word(msi_ptr + 2, msg_ctrl);

    fk::algorithms::klog("MSI", "Enabled MSI (Vector 0x%x) via x2APIC for device %02x:%02x.%d", 
                         vector, device.address().bus(), device.address().device(), 
                         device.address().function());

    return vector;
}

void X2APIC::enable_msi(uint8_t vector) { (void)vector; }
void X2APIC::disable_msi(uint8_t vector) { (void)vector; }
