#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>

#include <Kernel/Boot/Stages/early_init.h>
#include <Kernel/Boot/Stages/init.h>
#include <Kernel/Boot/boot_info.h>

#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/heap_malloc.h>

void early_init() {
  assert(boot::BootInfo::the().is_initialized() &&
         "early_init: BootInfo not initialized!");

  fk::algorithms::klog("EARLY_INIT", "--- early init start ---");

  // GDT
  fk::algorithms::klog("EARLY_INIT", "Initializing GDT...");
  GDTController::the().initialize();
  fk::algorithms::klog("EARLY_INIT", "GDT OK");

  // Heap
  fk::algorithms::klog("EARLY_INIT", "Initializing kernel heap...");
  MemoryManager::the().initialize_heap();
  if (!MemoryManager::the().is_heap_initialized())
    fk::algorithms::kfatal("EARLY_INIT", "Kernel heap failed to initialize");
  fk::algorithms::klog("EARLY_INIT", "Kernel heap ready");

  // Finalize BootInfo iterators (needs heap)
  boot::BootInfo::the().create_iterators();

  // Interrupt controller
  fk::algorithms::klog("EARLY_INIT", "Initializing interrupt controller...");
  InterruptController::the().initialize();
  fk::algorithms::klog("EARLY_INIT", "Interrupt controller OK");

  // Physical + Virtual memory managers
  fk::algorithms::klog("EARLY_INIT", "Initializing memory manager...");
  MemoryManager::the().initialize();
  if (!MemoryManager::the().is_initialized())
    fk::algorithms::kfatal("EARLY_INIT", "Memory manager failed to initialize");

  auto &pmm = PhysicalMemoryManager::the();
  size_t total_mib = pmm.total_memory() / (1024 * 1024);
  size_t raw_free   = pmm.free_memory();
  size_t free_mib   = (raw_free > total_mib * 1024ULL * 1024ULL) ? total_mib : raw_free / (1024 * 1024);
  fk::algorithms::klog("EARLY_INIT",
      "Physical memory: total=%zu MiB free=%zu MiB",
      total_mib, free_mib);

  size_t heap_total = 0, heap_free = 0;
  MemoryManager::the().heap_stats(heap_total, heap_free);
  fk::algorithms::klog("EARLY_INIT",
      "Kernel heap: total=%zu KiB free=%zu KiB",
      heap_total / 1024, heap_free / 1024);

  // ACPI
  fk::algorithms::klog("EARLY_INIT", "Initializing ACPI...");
  {
    RSDP *boot_rsdp = static_cast<RSDP *>(boot::BootInfo::the().get_acpi_info().rsdp);
    ACPIManager::the().initialize(boot_rsdp);
  }
  if (ACPIManager::the().get_madt()) {
    fk::algorithms::klog("EARLY_INIT", "ACPI: MADT found");
  } else {
    fk::algorithms::kwarn("EARLY_INIT", "ACPI: no MADT — SMP and APIC may be unavailable");
  }

  // CPU features
  fk::algorithms::klog("EARLY_INIT", "Detecting CPU features...");
  CPU::the().initialize_features();
  detect_tsc_frequency();
  fk::algorithms::klog("EARLY_INIT", "CPU: vendor='%s' brand='%s'",
      CPU::the().get_vendor().c_str(),
      CPU::the().get_brand().c_str());
  fk::algorithms::klog("EARLY_INIT", "CPU: APIC=%s x2APIC=%s",
      CPU::the().has_apic()   ? "yes" : "no",
      CPU::the().has_x2apic() ? "yes" : "no");

  fk::algorithms::klog("EARLY_INIT", "--- early init done ---");
  init();
}
