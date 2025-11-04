#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ioapic.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <LibFK/Algorithms/log.h>

// I/O APIC Redirection Table Entry bits
constexpr uint64_t IOAPIC_REDIR_MASKED = 1 << 16;

void IOAPIC::write(uint32_t reg, uint32_t value) {
  *reinterpret_cast<volatile uint32_t *>(ioapic_base) = reg;
  *reinterpret_cast<volatile uint32_t *>(ioapic_base + 0x10) = value;
}

uint32_t IOAPIC::read(uint32_t reg) {
  *reinterpret_cast<volatile uint32_t *>(ioapic_base) = reg;
  return *reinterpret_cast<volatile uint32_t *>(ioapic_base + 0x10);
}

void IOAPIC::initialize(uintptr_t base_addr) {
  ioapic_base = base_addr;

  // Map the I/O APIC registers.
  for (uintptr_t offset = 0; offset < PAGE_SIZE; offset += PAGE_SIZE) {
    VirtualMemoryManager::the().map_page(
        ioapic_base + offset, ioapic_base + offset,
        PageFlags::Present | PageFlags::Writable | PageFlags::WriteThrough);
  }

  uint32_t ver_reg = read(IOAPIC_REG_VER);
  uint32_t max_redir_entries = ((ver_reg >> 16) & 0xFF) + 1;

  klog("IOAPIC",
       "I/O APIC initialized at 0x%lx, GSI base %u, version %u, %u entries",
       ioapic_base, global_interrupt_base, ver_reg & 0xFF, max_redir_entries);

  for (uint32_t i = 0; i < max_redir_entries; ++i) {
    mask_irq(i);
  }
}

void IOAPIC::remap_irq(uint8_t irq, uint8_t vector, uint8_t lapic_id,
                       uint32_t flags) {
  uint64_t redirection_entry = 0;
  redirection_entry |= vector;
  redirection_entry |= (uint64_t)lapic_id << 56;
  redirection_entry |= flags;

  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + (irq * 2);
  uint32_t reg_high = IOAPIC_REG_TABLE_BASE + (irq * 2) + 1;

  write(reg_low, (uint32_t)redirection_entry);
  write(reg_high, (uint32_t)(redirection_entry >> 32));

  kdebug("IOAPIC", "Remapped IRQ %u to vector %u on LAPIC %u", irq, vector,
         lapic_id);
}

void IOAPIC::mask_irq(uint8_t irq) {
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + (irq * 2);
  uint32_t reg_high = IOAPIC_REG_TABLE_BASE + (irq * 2) + 1;

  uint64_t redirection_entry = ((uint64_t)read(reg_high) << 32) | read(reg_low);
  redirection_entry |= IOAPIC_REDIR_MASKED;

  write(reg_low, (uint32_t)redirection_entry);
  write(reg_high, (uint32_t)(redirection_entry >> 32));
}

void IOAPIC::unmask_irq(uint8_t irq) {
  uint32_t reg_low = IOAPIC_REG_TABLE_BASE + (irq * 2);
  uint32_t reg_high = IOAPIC_REG_TABLE_BASE + (irq * 2) + 1;

  uint64_t redirection_entry = ((uint64_t)read(reg_high) << 32) | read(reg_low);
  redirection_entry &= ~IOAPIC_REDIR_MASKED;

  write(reg_low, (uint32_t)redirection_entry);
  write(reg_high, (uint32_t)(redirection_entry >> 32));
}
