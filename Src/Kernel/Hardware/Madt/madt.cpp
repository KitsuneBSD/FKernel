#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Madt/madt.h>
#include <Kernel/Hardware/Madt/madt_entries.h>
#include <Kernel/Hardware/Madt/madt_lapic.h>
#include <Kernel/Hardware/Madt/madt_ioapic.h>
#include <Kernel/Hardware/Madt/madt_interrupt_source_override.h>
#include <Kernel/Hardware/Madt/madt_lapic_override.h>
#include <LibFK/Algorithms/log.h>

void ACPIManager::initialize_madt() {
  m_madt = (Madt *)find_table("APIC");
  if (!m_madt) {
    fk::algorithms::kwarn("ACPI", "MADT table not found!");
    return;
  }

  fk::algorithms::klog("MADT", "MADT found at %p", m_madt);
  fk::algorithms::klog("MADT", "MADT signature: %.4s", &m_madt->header.signature);
  fk::algorithms::klog("MADT", "MADT length: %u bytes", m_madt->header.length);
  fk::algorithms::klog("MADT", "Local APIC address: %p", (void *)(uintptr_t)m_madt->lapic_address);
  fk::algorithms::klog("MADT", "MADT flags: 0x%x", m_madt->flags);
  process_madt_entries();
}

void ACPIManager::process_madt_entries() {
  if (!m_madt) {
    return;
  }

  uint8_t *entries_start = m_madt->entries;
  uint8_t *entries_end = (uint8_t *)m_madt + m_madt->header.length;
  uint32_t entry_count = 0;

  fk::algorithms::klog("MADT", "Processing MADT entries...");

  for (uint8_t *p = entries_start; p < entries_end; ) {
    MadtEntry *entry = (MadtEntry *)p;
    
    switch (entry->type) {
      case 0: { // Processor Local APIC
        auto *lapic = (MADT_LAPIC *)entry;
        fk::algorithms::klog("MADT", "  Entry %u: Processor Local APIC (ACPI ID: %u, APIC ID: %u, flags: 0x%x)",
                             entry_count, lapic->acpi_processor_id, lapic->apic_id, lapic->flags);
        break;
      }
      case 1: { // I/O APIC
        auto *ioapic = (MADT_IOAPIC *)entry;
        fk::algorithms::klog("MADT", "  Entry %u: I/O APIC (ID: %u, Address: %p, GSI Base: %u)",
                             entry_count, ioapic->ioapic_id, (void *)(uintptr_t)ioapic->address, ioapic->gsi_base);
        break;
      }
      case 2: { // Interrupt Source Override
        auto *override = (MADT_InterruptSourceOverride *)entry;
        fk::algorithms::klog("MADT", "  Entry %u: Interrupt Source Override (Bus: %u, IRQ: %u, GSI: %u, Flags: 0x%x)",
                             entry_count, override->bus_source, override->irq_source, override->gsi, override->flags);
        break;
      }
      case 3: // NMI Source
        fk::algorithms::klog("MADT", "  Entry %u: NMI Source (type %u, length %u)",
                             entry_count, entry->type, entry->length);
        break;
      case 4: // Local APIC NMI
        fk::algorithms::klog("MADT", "  Entry %u: Local APIC NMI (type %u, length %u)",
                             entry_count, entry->type, entry->length);
        break;
      case 5: { // Local APIC Address Override
        auto *lapic_override = (MADT_LAPIC_OVERRIDE *)entry;
        fk::algorithms::klog("MADT", "  Entry %u: Local APIC Address Override (Address: %p)",
                             entry_count, (void *)lapic_override->lapic_address);
        break;
      }
      default:
        fk::algorithms::klog("MADT", "  Entry %u: Unknown type %u (length %u)",
                             entry_count, entry->type, entry->length);
        break;
    }

    entry_count++;
    p += entry->length;
  }

  fk::algorithms::klog("MADT", "Total MADT entries processed: %u", entry_count);
}
