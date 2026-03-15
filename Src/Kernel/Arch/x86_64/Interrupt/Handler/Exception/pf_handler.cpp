#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

static void handle_demand_paging(Task*, uint64_t cr2, InterruptFrame*) {
  uintptr_t vaddr = cr2 & ~0xFFFULL;
  uintptr_t phys = PhysicalMemoryManager::the().alloc_page();

  // Map as RW User
  VirtualMemoryManager::the().map_page(vaddr, phys,
                                       PageFlags::Present | PageFlags::Writable | PageFlags::User);

  // Zero the page
  memset(reinterpret_cast<void*>(vaddr), 0, 0x1000);
}

void page_fault_handler(uint8_t vector, InterruptFrame* frame) {
  uint64_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));

  auto* task = SchedulerManager::the().current();
  if (task && !task->is_a_kernel_task() && task->is_address_in_allowed_regions(cr2)) {
    handle_demand_paging(task, cr2, frame);
    return;
  }

  fk::algorithms::kexception(
      "Page Fault", "vector=%u error=0x%lx (%s, %s, %s %s) RIP=%p RSP=%p CR2=%p PID=%lu",
      (unsigned)vector, (uint64_t)frame->error_code,
      (frame->error_code & 1) ? "Present" : "Not Present",
      (frame->error_code & 2) ? "Write" : "Read", (frame->error_code & 4) ? "User" : "Kernel",
      (frame->error_code & 16) ? "Instruction Fetch" : "Data Access", (void*)frame->rip,
      (void*)frame->rsp, (void*)cr2, task ? task->control.identity.id.value() : 0);

  if (task && !task->is_a_kernel_task() && (frame->error_code & 4)) {
    fk::algorithms::kerror("PF", "User-mode Page Fault. Killing process %lu",
                           task->control.identity.id.value());
    SchedulerManager::the().terminate_current(-11); // SIGSEGV = 11
  }

  halt_forever();
}
