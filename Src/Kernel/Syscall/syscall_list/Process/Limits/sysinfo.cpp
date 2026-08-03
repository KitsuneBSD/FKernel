#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

struct sysinfo {
  long  uptime;
  unsigned long loads[3];
  unsigned long totalram;
  unsigned long freeram;
  unsigned long sharedram;
  unsigned long bufferram;
  unsigned long totalswap;
  unsigned long freeswap;
  unsigned short procs;
  unsigned long totalhigh;
  unsigned long freehigh;
  unsigned int mem_unit;
  char _f[20 - 2 * sizeof(long) - sizeof(int)];
};

extern "C" uint64_t sys_sysinfo(uint64_t info_ptr, uint64_t, uint64_t, uint64_t,
                                 uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
  if (!info_ptr) return (uint64_t)-1;
  auto* si = reinterpret_cast<sysinfo*>(info_ptr);
  uint32_t freq = TickManager::the().get_frequency();
  size_t heap_total = 0, heap_free = 0;
  MemoryManager::the().heap_stats(heap_total, heap_free);
  si->uptime    = (long)(freq > 0 ? TickManager::the().get_ticks() / freq : 0);
  si->loads[0]  = si->loads[1] = si->loads[2] = 0;
  si->totalram  = PhysicalMemoryManager::the().total_memory();
  si->freeram   = heap_free;
  si->sharedram = 0;
  si->bufferram = 0;
  si->totalswap = 0;
  si->freeswap  = 0;
  si->procs     = (unsigned short)SchedulerManager::the().process_count();
  si->totalhigh = 0;
  si->freehigh  = 0;
  si->mem_unit  = 1;
  return 0;
}
