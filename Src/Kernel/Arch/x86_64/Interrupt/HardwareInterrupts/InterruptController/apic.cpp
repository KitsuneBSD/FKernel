#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic_common.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/msi_helpers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>
#include <Kernel/Memory/memory_manager.h>

APIC* g_apic_ptr = nullptr;

void APIC::write(uint32_t reg, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(lapic_base + reg) = value;
}

uint32_t APIC::read(uint32_t reg) const {
  return *reinterpret_cast<volatile uint32_t *>(lapic_base + reg);
}

void APIC::initialize() {
  if (lapic_base != 0) {
    fk::algorithms::kwarn("APIC", "APIC already initialized.");
    return;
  }

  g_apic_ptr = this;

  if (!CPU::the().has_apic()) {
    fk::algorithms::kfatal("APIC", "Local APIC not detected");
    return;
  }

  uint64_t apic_msr = CPU::the().read_msr(APIC_BASE_MSR);
  uintptr_t apic_phys = apic_msr & 0xFFFFF000;

  apic_msr |= APIC_MSR_ENABLE;
  CPU::the().write_msr(APIC_BASE_MSR, apic_msr);

  for (uintptr_t offset = 0; offset < APIC_RANGE_SIZE; offset += PAGE_SIZE) {
    MemoryManager::the().map_page(apic_phys + offset, apic_phys + offset,
                                  PageFlags::Present | PageFlags::Writable |
                                      PageFlags::WriteThrough);
  }

  lapic_base = apic_phys;

  write(APIC_REG_SPURIOUS, APIC_SPURIOUS_VECTOR | APIC_SVR_ENABLE);

  fk::algorithms::klog("APIC", "Local APIC enabled with spurious vector 0x%X",
                       APIC_SPURIOUS_VECTOR);
}

void APIC::send_eoi(uint8_t) {
  if (lapic_base)
    write(APIC_REG_EOI, 0);
}

void APIC::mask_interrupt(uint8_t irq) {
  fk::algorithms::kwarn(
      "APIC", "Mask request for IRQ %u ignored (LAPIC doesn't mask that way)",
      irq);
}

void APIC::unmask_interrupt(uint8_t irq) {
  fk::algorithms::kwarn(
      "APIC",
      "Unmask request for IRQ %u ignored (LAPIC doesn't unmask that way)", irq);
}

void APIC::calibrate_timer() {
  if (!lapic_base)
    return;

  write(APIC_REG_DIVIDE_CONFIG, APIC_TIMER_DIVISOR);
  write(APIC_REG_LVT_TIMER, APIC_LVT_MASK);
  write(APIC_REG_INITIAL_COUNT, 0xFFFFFFFF);

  constexpr uint64_t calib_ms = 10;
  TimerManager::the().sleep(calib_ms);

  uint32_t elapsed = 0xFFFFFFFF - read(APIC_REG_CURRENT_COUNT);

  write(APIC_REG_INITIAL_COUNT, 0);
  apic_ticks_per_ms = elapsed / calib_ms;

  fk::algorithms::klog("APIC", "APIC timer calibrated: %u ticks/ms",
                       apic_ticks_per_ms);
}

void APIC::setup_timer(uint64_t frequency_hz) {
  if (!lapic_base)
    initialize();

  if (apic_ticks_per_ms == 0)
    calibrate_timer();

  uint64_t interval_ms = 1000 / frequency_hz;
  uint64_t initial_ticks = apic_ticks_per_ms * interval_ms;

  write(APIC_REG_DIVIDE_CONFIG, APIC_TIMER_DIVISOR);
  write(APIC_REG_LVT_TIMER, APIC_TIMER_VECTOR | APIC_LVT_TIMER_MODE_PERIODIC);
  write(APIC_REG_INITIAL_COUNT, static_cast<uint32_t>(initial_ticks));

  fk::algorithms::klog("APIC",
                       "APIC timer armed at %llu Hz (%u ticks per period)",
                       frequency_hz, static_cast<uint32_t>(initial_ticks));
}

fk::core::Result<uint8_t, fk::core::Error>
APIC::allocate_msi_vector(const PciDevice& device) {
  return msi::allocate_msi_vector(device);
}

fk::core::Result<uint8_t, fk::core::Error>
APIC::allocate_msix_vector(const PciDevice& device, uint16_t entry) {
  constexpr uint8_t MSIX_CAP_ID = 0x11;
  uint8_t cap_ptr = device.find_capability(MSIX_CAP_ID);
  if (cap_ptr == 0)
    return fk::core::Error::NotSupported;

  uint16_t ctrl = device.read_config_word(cap_ptr + 2);
  uint16_t table_size = (ctrl & 0x7FF) + 1;
  if (entry >= table_size)
    return fk::core::Error::InvalidParameter;

  auto vector_result = msi::allocate_vector();
  if (!vector_result.is_ok())
    return vector_result.error();
  uint8_t vector = vector_result.value();

  uint32_t table_info = device.read_config_dword(cap_ptr + 4);
  uint8_t  bir        = static_cast<uint8_t>(table_info & 0x7);
  uint32_t table_off  = table_info & ~0x7U;

  uintptr_t bar_phys   = device.bar_base(bir);
  uintptr_t table_phys = bar_phys + table_off;
  uintptr_t table_virt = table_phys;

  MemoryManager::the().map_page(table_virt, table_phys,
                                PageFlags::Present | PageFlags::Writable |
                                    PageFlags::WriteThrough);

  struct MsixEntry {
    volatile uint64_t msg_addr;
    volatile uint32_t msg_data;
    volatile uint32_t vector_ctrl;
  };
  auto* msix_table = reinterpret_cast<MsixEntry*>(table_virt);
  msix_table[entry].msg_addr    = static_cast<uint64_t>(msi_address_base());
  msix_table[entry].msg_data    = vector;
  msix_table[entry].vector_ctrl = 0;

  ctrl |= (1 << 15);
  ctrl &= ~static_cast<uint16_t>(1 << 14);
  device.write_config_word(cap_ptr + 2, ctrl);

  fk::algorithms::klog("MSI_X", "Entry %u -> Vector 0x%x for %02x:%02x.%d",
                       entry, vector, device.address().bus(),
                       device.address().device(), device.address().function());
  return vector;
}

void APIC::enable_msi(uint8_t vector) { (void)vector; }
void APIC::disable_msi(uint8_t vector) { (void)vector; }

uint32_t APIC::get_id() const {
  if (!lapic_base)
    return 0;
  return (read(0x20) >> 24) & 0xFF;
}

void APIC::send_ipi(uint8_t lapic_id, uint8_t vector, uint32_t delivery_mode) {
  write(APIC_REG_ICR_HIGH, static_cast<uint32_t>(lapic_id) << 24);
  uint32_t icr_low = delivery_mode | vector;
  write(APIC_REG_ICR_LOW, icr_low);
}

void APIC::wait_ipi_delivery() {
  while (read(APIC_REG_ICR_LOW) & (1u << 12))
    asm volatile("pause");
}
