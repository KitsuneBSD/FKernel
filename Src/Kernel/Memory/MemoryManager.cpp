#include <Kernel/Memory/MemoryManager.h>
#include <Kernel/Memory/VirtualMemory/VirtualMemoryManager.h>
#include <Kernel/Memory/PhysicalMemory/PhysicalMemoryManager.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

void MemoryManager::initialize(const multiboot2::TagMemoryMap *mmap) {
  if (m_is_initialized) {
    fk::algorithms::kwarn("MEMORY MANAGER", "Already initialized.");
    return;
  }

  PhysicalMemoryManager::the().initialize(mmap);
  VirtualMemoryManager::the().initialize();

  fk::algorithms::klog("MEMORY MANAGER", "Memory Manager initialized");
  m_is_initialized = true;
}
