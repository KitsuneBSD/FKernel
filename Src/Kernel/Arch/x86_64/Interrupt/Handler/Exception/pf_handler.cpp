#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/log.h>

static void handle_demand_paging(Task*, uint64_t cr2, InterruptFrame*) {
  uintptr_t vaddr = cr2 & ~0xFFFULL;
  uintptr_t phys = PhysicalMemoryManager::the().alloc_page();

  VirtualMemoryManager::the().map_page(vaddr, phys,
                                       PageFlags::Present | PageFlags::Writable | PageFlags::User);

  fk::memory::set(reinterpret_cast<void*>(vaddr), 0, 0x1000);
}

static void handle_write_protection(Task*, uint64_t cr2) {
  uintptr_t vaddr = cr2 & ~0xFFFULL;

  auto flags_res = VirtualMemoryManager::the().get_page_flags(vaddr);
  if (flags_res.is_error())
    return;

  PageFlags current = flags_res.value();
  // error_code & 4 is always set when this is called — enforce User bit for CPL3 access
  PageFlags fixed = current | PageFlags::Writable | PageFlags::User;
  VirtualMemoryManager::the().protect_page(vaddr, fixed);
}

void page_fault_handler(uint8_t vector, InterruptFrame* frame) {
  uint64_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));

  auto* task = SchedulerManager::the().current();
  if (task && !task->is_a_kernel_task() && (frame->error_code & 4)) {
    bool not_present = !(frame->error_code & 1);
    bool write_fault = (frame->error_code & 2) != 0;

    if (not_present && task->is_address_in_allowed_regions(cr2)) {
      handle_demand_paging(task, cr2, frame);
      return;
    }

    if (write_fault && (frame->error_code & 1) && task->is_address_in_allowed_regions(cr2)) {
      handle_write_protection(task, cr2);
      return;
    }
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
    SchedulerManager::the().terminate_current(-11);
  }

  halt_forever();
}
