#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Core/Assertions.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

#include <Kernel/Clock/clock_interrupt.h>

void MemoryManager::initialize() {
  assert(!m_is_initialized && "MemoryManager: Double initialization attempted!");
  assert(boot::BootInfo::the().is_initialized() && "MemoryManager: BootInfo not initialized!");

  PhysicalMemoryManager::the().initialize();
  VirtualMemoryManager::the().initialize();

  HardwareInterruptManager::the().set_memory_manager(true);
  TimerManager::the().set_memory_manager(true);
  ClockManager::the().set_memory_manager(true);

  m_is_initialized = true;
}

void MemoryManager::map_page(uintptr_t virt, uintptr_t phys, PageFlags flags){
  VirtualMemoryManager::the().map_page(virt, phys, flags);
}
