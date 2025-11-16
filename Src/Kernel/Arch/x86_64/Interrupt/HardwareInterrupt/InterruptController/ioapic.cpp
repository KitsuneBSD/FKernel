#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/ioapic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Hardware/cpu.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <LibFK/Algorithms/log.h>

// I/O APIC Redirection Table Entry bits
constexpr uint64_t IOAPIC_REDIR_MASKED = 1ULL << 16;

uint32_t IOAPIC::read(uint32_t reg) {
  *reinterpret_cast<volatile uint32_t *>(ioapic_base) = reg;
  return *reinterpret_cast<volatile uint32_t *>(ioapic_base + 0x10);
}

void IOAPIC::write(uint32_t reg, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(ioapic_base) = reg;
  *reinterpret_cast<volatile uint32_t *>(ioapic_base + 0x10) = value;
}

void IOAPIC::initialize() {
  ioapic_base = IOAPIC_ADDRESS;
  klog("IOAPIC", "Initializing IOAPIC at %p", ioapic_base);

  // Map I/O APIC registers
  VirtualMemoryManager::the().map_page(
      ioapic_base, ioapic_base,
      PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);

  uint32_t ver = read(IOAPIC_REG_VER);
  uint32_t max_entries = ((ver >> 16) & 0xFF) + 1;

  klog("IOAPIC", "Version %u, %u redirection entries", ver & 0xFF, max_entries);

  // Mask all IRQs initially
  for (uint32_t i = 0; i < max_entries; ++i) {
    mask_interrupt(i);
  }
}

void IOAPIC::mask_interrupt(uint8_t irq) {
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + irq * 2;
  uint32_t reg_high = reg_low + 1;

  uint64_t entry = ((uint64_t)read(reg_high) << 32) | read(reg_low);
  entry |= IOAPIC_REDIR_MASKED;

  write(reg_low, (uint32_t)entry);
  write(reg_high, (uint32_t)(entry >> 32));
}

void IOAPIC::unmask_interrupt(uint8_t irq) {
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + irq * 2;
  uint32_t reg_high = reg_low + 1;

  uint64_t entry = ((uint64_t)read(reg_high) << 32) | read(reg_low);
  entry &= ~IOAPIC_REDIR_MASKED;

  write(reg_low, (uint32_t)entry);
  write(reg_high, (uint32_t)(entry >> 32));
}

void IOAPIC::send_eoi([[maybe_unused]] uint8_t irq) {
  if (CPU::the().has_x2apic()) {
    X2APIC::the().send_eoi(irq);
  } else {
    APIC::the().send_eoi(irq);
  }
}

void IOAPIC::remap_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id,
                       uint32_t flags) {
  uint64_t entry = vector | ((uint64_t)lapic_id << 56) | flags;

  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + irq * 2;
  uint32_t reg_high = reg_low + 1;

  write(reg_low, (uint32_t)entry);
  write(reg_high, (uint32_t)(entry >> 32));
}
