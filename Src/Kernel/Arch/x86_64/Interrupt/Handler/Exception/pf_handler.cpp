#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/log.h>

static PageFlags resolve_region_flags(Task* task, uintptr_t vaddr) {
  auto& regions = task->resources.memory.regions.list;
  for (size_t i = 0; i < regions.size(); ++i) {
    if (regions[i].contains(vaddr))
      return regions[i].flags;
  }
  return PageFlags::Present | PageFlags::Writable | PageFlags::User | PageFlags::ExecuteDisable;
}

static void handle_demand_paging(Task* task, uint64_t cr2, InterruptFrame*) {
  uintptr_t vaddr = cr2 & ~0xFFFULL;
  uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
  if (!phys) {
    fk::algorithms::kwarn("PF", "demand paging OOM at %p pid=%lu", (void*)cr2, task->control.identity.id.value());
    return;
  }

  PageFlags flags = resolve_region_flags(task, vaddr);
  flags = flags | PageFlags::Present | PageFlags::User;

  VirtualMemoryManager::the().map_page(vaddr, phys, flags);

  fk::memory::set(reinterpret_cast<void*>(phys + KERNEL_VIRT_BASE), 0, PAGE_SIZE);
}

static void handle_write_protection(uint64_t cr2) {
  uintptr_t vaddr = cr2 & ~0xFFFULL;

  uintptr_t phys = VirtualMemoryManager::the().translate(vaddr);
  if (!phys) return;

  uint16_t refcount = PhysicalMemoryManager::the().get_refcount(phys);

  if (refcount > 1) {
    uintptr_t new_phys = PhysicalMemoryManager::the().alloc_page();
    if (!new_phys) return;

    fk::memory::copy(reinterpret_cast<void*>(new_phys + KERNEL_VIRT_BASE),
                     reinterpret_cast<void*>(phys + KERNEL_VIRT_BASE), 0x1000);

    auto flags_res = VirtualMemoryManager::the().get_page_flags(vaddr);
    if (flags_res.is_error()) return;
    PageFlags flags = flags_res.value() | PageFlags::Writable | PageFlags::User;

    VirtualMemoryManager::the().map_page(vaddr, new_phys, flags);
    PhysicalMemoryManager::the().free_page(phys);
    return;
  }

  auto flags_res = VirtualMemoryManager::the().get_page_flags(vaddr);
  if (flags_res.is_error()) return;

  PageFlags current = flags_res.value();

  if (!(static_cast<uint64_t>(current) & static_cast<uint64_t>(PageFlags::User))) {
    uintptr_t new_phys = PhysicalMemoryManager::the().alloc_page();
    if (!new_phys) return;

    fk::memory::copy(reinterpret_cast<void*>(new_phys + KERNEL_VIRT_BASE),
                     reinterpret_cast<void*>(phys + KERNEL_VIRT_BASE), 0x1000);

    PageFlags new_flags = current | PageFlags::Writable | PageFlags::User;
    VirtualMemoryManager::the().map_page(vaddr, new_phys, new_flags);
    return;
  }

  PageFlags fixed = current | PageFlags::Writable | PageFlags::User;
  VirtualMemoryManager::the().map_page(vaddr, phys, fixed);
}

void page_fault_handler(uint8_t vector, InterruptFrame* frame) {
  uint64_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));

  auto* task = SchedulerManager::the().current();
  bool is_user = (frame->error_code & 4) != 0;
  bool not_present = !(frame->error_code & 1);
  bool write_fault = (frame->error_code & 2) != 0;

  // Handle kernel-mode write faults on present pages. With CR0.WP enabled, kernel
  // writes to CoW-shared user pages will fault. Handle them identically to user faults.
  bool should_handle = task && (!task->is_a_kernel_task() || (!is_user && write_fault && !not_present));

  if (should_handle) {
    if (not_present && task->is_address_in_allowed_regions(cr2) && (is_user || !task->is_a_kernel_task())) {
      fk::algorithms::kdebug("PF", "demand-page %p pid=%lu", (void*)cr2, task->control.identity.id.value());
      handle_demand_paging(task, cr2, frame);
      return;
    }

    if (write_fault && !not_present && task->is_address_in_allowed_regions(cr2)) {
      fk::algorithms::kdebug("PF", "cow-fault %p pid=%lu mode=%s", (void*)cr2, task->control.identity.id.value(), is_user ? "user" : "kernel");
      handle_write_protection(cr2);
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

  if (task && !task->is_a_kernel_task() && is_user) {
    fk::algorithms::kerror("PF", "User-mode Page Fault. Killing process %lu",
                           task->control.identity.id.value());
    SchedulerManager::the().terminate_current(-11);
  }

  halt_forever();
}
