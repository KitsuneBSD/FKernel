#include <Kernel/Memory/MemoryManager.h>
#include <Kernel/Memory/VirtualMemory/VirtualMemoryManager.h>
#include <Kernel/Memory/PhysicalMemory/PhysicalMemoryManager.h>
#include <LibFK/Core/Assertions.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

void MemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  assert(!m_is_initialized && "MemoryManager: Double initialization attempted!");
  assert(mmap && "MemoryManager: No memory map provided!");

  PhysicalMemoryManager::the().initialize(mmap);
  VirtualMemoryManager::the().initialize();

  // After make the identity-mapping, we can start the proper Hardware Services
  HardwareInterruptManager::the().set_memory_manager(true);
  TimerManager::the().set_memory_manager(true);
  ClockManager::the().set_memory_manager(true);

  m_is_initialized = true;
}

void MemoryManager::map_page(uintptr_t virt, uintptr_t phys, PageFlags flags){
  VirtualMemoryManager::the().map_page(virt, phys, flags);
}
