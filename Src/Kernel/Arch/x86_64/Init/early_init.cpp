#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/init.h>
#include <Kernel/Boot/memory_map.h>
#include <Kernel/Boot/early_init.h>

#include <Kernel/Hardware/Cpu.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

#include <LibFK/Algorithms/log.h>

void early_init([[maybe_unused]] const multiboot2::TagMemoryMap *mmap)
{
  klog("EARLY_INIT", "Start early init (multiboot2)");

  GDTController::the().initialize();
  InterruptController::the().initialize();
  PhysicalMemoryManager::the().initialize(mmap);
  VirtualMemoryManager::the().initialize();

  if (CPU::the().has_apic())
  {
    APIC::the().enable();
    APIC::the().calibrate_timer();

    InterruptController::the().register_interrupt(apic_timer_handler, 32);
    APIC::the().setup_timer(1);
  }

  init();
}

void early_init_from_view(MemoryMapView const& view)
{
  klog("EARLY_INIT", "Start early init (memory map view)");

  GDTController::the().initialize();
  InterruptController::the().initialize();

  // Convert MemoryMapView to a temporary multiboot2::TagMemoryMap-like
  // structure expected by PhysicalMemoryManager. For now create a
  // small adaptor on the stack.
  struct TmpMMap {
    uint32_t entry_size;
    uint32_t entry_version;
    // We'll allocate an array dynamically if needed; to keep simple, we'll
    // only support small maps here.
    multiboot2::TagMemoryMap::Entry entries[64];
  } tmp;

  size_t n = view.count;
  if (n > 64) n = 64; // truncate for now
  tmp.entry_size = sizeof(multiboot2::TagMemoryMap::Entry);
  tmp.entry_version = 1;
  for (size_t i = 0; i < n; ++i) {
    tmp.entries[i].base_addr = view.entries[i].base_addr;
    tmp.entries[i].length = view.entries[i].length;
    tmp.entries[i].type = view.entries[i].type;
  }

  PhysicalMemoryManager::the().initialize(reinterpret_cast<const multiboot2::TagMemoryMap *>(&tmp));
  VirtualMemoryManager::the().initialize();

  if (CPU::the().has_apic())
  {
    APIC::the().enable();
    APIC::the().calibrate_timer();

    InterruptController::the().register_interrupt(apic_timer_handler, 32);
    APIC::the().setup_timer(1);
  }

  init();
}

void early_init_from_uefi(void const* efi_mmap, size_t entry_count)
{
  // TODO: Implement full EFI memory map parsing. For now, create a
  // MemoryMapView that assumes entries are already in the simple format
  // MemoryMapEntry and forward to early_init_from_view.
  auto *entries = reinterpret_cast<const MemoryMapEntry *>(efi_mmap);
  MemoryMapView view{entries, entry_count};
  early_init_from_view(view);
}
