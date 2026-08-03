#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Logging/log.h>

extern "C" void print_stack_trace();

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

  auto flags_res = VirtualMemoryManager::the().get_page_flags(vaddr);
  if (flags_res.is_error()) return;
  PageFlags current = flags_res.value();

  uint16_t refcount = PhysicalMemoryManager::the().get_refcount(phys);

  // CoW break.  Never add the User bit here: if the page was a supervisor
  // (kernel) RO page, remapping it User would be a privilege escalation (M5).
  // Preserve the current user-ness and just make the page writable.
  if (refcount > 1) {
    uintptr_t new_phys = PhysicalMemoryManager::the().alloc_page();
    if (!new_phys) return;

    fk::memory::copy(reinterpret_cast<void*>(new_phys + KERNEL_VIRT_BASE),
                     reinterpret_cast<void*>(phys + KERNEL_VIRT_BASE), 0x1000);

    PageFlags new_flags = current | PageFlags::Writable;
    VirtualMemoryManager::the().map_page(vaddr, new_phys, new_flags);
    PhysicalMemoryManager::the().free_page(phys);
    return;
  }

  PageFlags fixed = current | PageFlags::Writable;
  VirtualMemoryManager::the().map_page(vaddr, phys, fixed);
}

static bool is_user_page(uintptr_t vaddr) {
  auto flags_res = VirtualMemoryManager::the().get_page_flags(vaddr & ~0xFFFULL);
  if (flags_res.is_error()) return false;
  return (static_cast<uint64_t>(flags_res.value()) & static_cast<uint64_t>(PageFlags::User)) != 0;
}

void page_fault_handler(uint8_t vector, InterruptFrame* frame) {
  uint64_t cr2;
  asm volatile("mov %%cr2, %0" : "=r"(cr2));

  auto* task = SchedulerManager::the().current();
  bool is_user = (frame->error_code & 4) != 0;
  bool not_present = !(frame->error_code & 1);
  bool write_fault = (frame->error_code & 2) != 0;
  bool ac_flag = (frame->rflags & (1ULL << 18)) != 0; // RFLAGS.AC: set by STAC (arch_smap_begin)

  if (!task) {
    fk::algorithms::kexception("Page Fault", "no current task CR2=%p error=0x%lx RIP=%p",
                               (void*)cr2, (uint64_t)frame->error_code, (void*)frame->rip);
    print_stack_trace();
    halt_forever();
  }

  // User-mode fault: demand paging, CoW break, or SIGSEGV with the faulting address.
  if (is_user) {
    if (not_present && task->is_address_in_allowed_regions(cr2)) {
      fk::algorithms::kdebug("PF", "demand-page %p pid=%lu", (void*)cr2, task->control.identity.id.value());
      handle_demand_paging(task, cr2, frame);
      return;
    }

    if (write_fault && !not_present && task->is_address_in_allowed_regions(cr2)) {
      fk::algorithms::kdebug("PF", "cow-fault %p pid=%lu", (void*)cr2, task->control.identity.id.value());
      handle_write_protection(cr2);
      return;
    }

    siginfo_t si{};
    si.si_signo = SIGSEGV;
    si.si_code  = not_present ? SEGV_MAPERR : SEGV_ACCERR;
    si.si_addr  = cr2;
    fk::algorithms::kwarn("PF", "User-mode Page Fault at %p pid=%lu code=%d RIP=%p err=0x%lx -> SIGSEGV",
                          (void*)cr2, task->control.identity.id.value(), (int)si.si_code,
                          (void*)frame->rip, (uint64_t)frame->error_code);
    fkernel::ipc::SignalDelivery::send_signal(task, SIGSEGV, &si, /*force=*/true);
    return;
  }

  // Kernel-mode fault. The only legitimate case is the kernel writing to user memory
  // through copy_to_user/copy_from_user inside an arch_smap_begin() window — that path
  // runs with RFLAGS.AC set. Without AC set, a supervisor fault on a user page is a bug.
  if (!task->is_a_kernel_task()) {
    if (not_present && ac_flag && task->is_address_in_allowed_regions(cr2)) {
      fk::algorithms::kdebug("PF", "kernel demand-page %p pid=%lu (AC)", (void*)cr2, task->control.identity.id.value());
      handle_demand_paging(task, cr2, frame);
      return;
    }

    if (write_fault && !not_present && ac_flag && is_user_page(cr2)) {
      fk::algorithms::kdebug("PF", "kernel cow-fault %p pid=%lu (AC)", (void*)cr2, task->control.identity.id.value());
      handle_write_protection(cr2);
      return;
    }
  }

  // Unhandled kernel fault — a genuine kernel bug. Dump context and halt.
  fk::algorithms::kexception(
      "Page Fault", "vector=%u error=0x%lx (%s, %s, %s %s) RIP=%p RSP=%p CR2=%p PID=%lu AC=%d",
      (unsigned)vector, (uint64_t)frame->error_code,
      (frame->error_code & 1) ? "Present" : "Not Present",
      (frame->error_code & 2) ? "Write" : "Read", (frame->error_code & 4) ? "User" : "Kernel",
      (frame->error_code & 16) ? "Instruction Fetch" : "Data Access", (void*)frame->rip,
      (void*)frame->rsp, (void*)cr2, task->control.identity.id.value(), ac_flag ? 1 : 0);
  fk::algorithms::kexception(
      "Page Fault", "  rax=%p rbx=%p rcx=%p rdx=%p",
      (void*)frame->rax, (void*)frame->rbx, (void*)frame->rcx, (void*)frame->rdx);
  fk::algorithms::kexception(
      "Page Fault", "  rsi=%p rdi=%p rbp=%p r8=%p",
      (void*)frame->rsi, (void*)frame->rdi, (void*)frame->rbp, (void*)frame->r8);
  fk::algorithms::kexception(
      "Page Fault", "  r9=%p r10=%p r11=%p r12=%p r13=%p r14=%p r15=%p",
      (void*)frame->r9, (void*)frame->r10, (void*)frame->r11,
      (void*)frame->r12, (void*)frame->r13, (void*)frame->r14, (void*)frame->r15);

  print_stack_trace();
  halt_forever();
}
