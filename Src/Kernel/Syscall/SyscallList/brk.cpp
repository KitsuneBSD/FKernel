#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

uint64_t sys_brk(uint64_t brk_addr, uint64_t, uint64_t, uint64_t, uint64_t,
                 uint64_t) {
  auto* task = SchedulerManager::the().current();
  if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);

  if (brk_addr == 0) return task->memory_regions.heap_break;

  if (brk_addr < task->memory_regions.heap_start) {
      return task->memory_regions.heap_break;
  }

  // Just update the break. Demand paging will map the pages on fault.
  task->memory_regions.heap_break = brk_addr;
  return task->memory_regions.heap_break;
}
}
