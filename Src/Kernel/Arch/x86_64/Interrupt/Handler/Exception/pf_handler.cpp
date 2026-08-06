#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Scheduler/Core/scheduler.h>

static constexpr uint64_t PF_STORM_WINDOW = 10;   // 10 ticks = 100ms at 100 Hz
static constexpr uint64_t PF_STORM_LIMIT  = 500;  // >500 faults per 100ms → kill task

extern "C" void print_stack_trace();

static void handle_demand_paging(Task* task, uint64_t cr2, InterruptFrame*, bool is_user_fault) {
  uintptr_t vaddr = cr2 & ~0xFFFULL;
  uintptr_t phys = PhysicalMemoryManager::the().alloc_page();
  if (!phys) {
    fk::algorithms::kwarn("PF", "demand paging OOM at %p pid=%lu, killing task",
                          (void*)cr2, task->control.identity.id.value());
    SchedulerManager::the().kill_current_from_exception(SIGSEGV);
    return; // [[noreturn]] — defensive: kill must be the terminal statement
  }

  uint8_t* page_virt = reinterpret_cast<uint8_t*>(phys + KERNEL_VIRT_BASE);
  fk::memory::set(page_virt, 0, PAGE_SIZE);

  // Single O(N) scan: derive flags and handle file-backing in one pass.
  // Default flags used when vaddr falls outside all tracked regions (e.g., brk heap).
  // User faults always get the User bit — brk extends heap_break without a MemoryRegion
  // entry, so the region scan misses heap pages. Without User the CPU will fault on every
  // user-mode access to the page, creating an infinite protection-fault loop.
  PageFlags flags = PageFlags::Present | PageFlags::Writable | PageFlags::ExecuteDisable;
  if (is_user_fault) flags = flags | PageFlags::User;
  auto& regions = task->resources.memory.regions.list;
  for (size_t i = 0; i < regions.size(); ++i) {
    auto& region = regions[i];
    if (!region.contains(vaddr)) continue;
    flags = region.flags | PageFlags::Present;
    if (is_user_fault) flags = flags | PageFlags::User;
    if (region.backing_node) {
      uint64_t file_offset = region.backing_offset + (vaddr - region.start);
      region.backing_node->read(file_offset, PAGE_SIZE, page_virt);
    }
    break;
  }

  VirtualMemoryManager::the().map_page(vaddr, phys, flags);
}

static void handle_write_protection(uint64_t cr2, bool is_user_fault) {
  uintptr_t vaddr = cr2 & ~0xFFFULL;

  uintptr_t phys = VirtualMemoryManager::the().translate(vaddr);
  if (!phys) return;

  auto flags_res = VirtualMemoryManager::the().get_page_flags(vaddr);
  if (flags_res.is_error()) return;
  PageFlags current = flags_res.value();

  // For user-mode write faults, always ensure the User bit. A demand-paged heap page
  // (from brk) may have been mapped without User if no MemoryRegion covered it;
  // without User the remap would still be supervisor-only, causing the same fault again.
  if (is_user_fault) current = current | PageFlags::User;

  uint16_t refcount = PhysicalMemoryManager::the().get_refcount(phys);

  if (refcount > 1) {
    uintptr_t new_phys = PhysicalMemoryManager::the().alloc_page();
    if (!new_phys) {
      fk::algorithms::kwarn("PF", "CoW break OOM at %p, killing task", (void*)cr2);
      SchedulerManager::the().kill_current_from_exception(SIGSEGV);
      return; // [[noreturn]] — defensive: kill must be the terminal statement
    }

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
    // Per-task page-fault rate limit (R3): kill before a fault storm consumes the CPU.
    auto& mem = task->resources.memory.regions;
    uint64_t now = TickManager::the().get_ticks();
    if (now - mem.pf_window_ticks >= PF_STORM_WINDOW) {
      mem.pf_count = 0;
      mem.pf_window_ticks = now;
    }
    if (++mem.pf_count > PF_STORM_LIMIT) {
      fk::algorithms::kwarn("PF", "Fault storm: pid=%lu %lu faults in %lu ticks — killing",
                            task->control.identity.id.value(), mem.pf_count, PF_STORM_WINDOW);
      SchedulerManager::the().kill_current_from_exception(SIGSEGV);
    }

    if (not_present && task->is_address_in_allowed_regions(cr2)) {
      fk::algorithms::kdebug("PF", "demand-page %p pid=%lu", (void*)cr2, task->control.identity.id.value());
      handle_demand_paging(task, cr2, frame, /*is_user_fault=*/true);
      return;
    }

    if (write_fault && !not_present && task->is_address_in_allowed_regions(cr2)) {
      fk::algorithms::kdebug("PF", "cow-fault %p pid=%lu", (void*)cr2, task->control.identity.id.value());
      handle_write_protection(cr2, /*is_user_fault=*/true);
      return;
    }

    siginfo_t si{};
    si.si_signo = SIGSEGV;
    si.si_code  = not_present ? SEGV_MAPERR : SEGV_ACCERR;
    si.si_addr  = cr2;
    fk::algorithms::kwarn("PF", "User-mode Page Fault at %p pid=%lu code=%d RIP=%p err=0x%lx -> SIGSEGV",
                          (void*)cr2, task->control.identity.id.value(), (int)si.si_code,
                          (void*)frame->rip, (uint64_t)frame->error_code);
    if (not_present) {
      uintptr_t hw_cr3 = arch_read_cr3() & ~0xFFFULL;
      uintptr_t task_cr3 = task->resources.memory.cr3;
      fk::algorithms::kwarn("PF", "  CR3: hw=%p task=%p %s",
                            (void*)hw_cr3, (void*)task_cr3,
                            hw_cr3 == task_cr3 ? "match" : "MISMATCH");
      uintptr_t walk = cr2 & ~0xFFFULL;
      auto* pml4 = reinterpret_cast<uint64_t*>(hw_cr3);
      size_t i4 = (walk >> 39) & 0x1FF;
      size_t i3 = (walk >> 30) & 0x1FF;
      size_t i2 = (walk >> 21) & 0x1FF;
      size_t i1 = (walk >> 12) & 0x1FF;
      fk::algorithms::kwarn("PF", "  PML4[%zu]=%p", i4, (void*)pml4[i4]);
      if (pml4[i4] & 1) {
        auto* pdpt = reinterpret_cast<uint64_t*>(pml4[i4] & ~0xFFFULL);
        fk::algorithms::kwarn("PF", "  PDPT[%zu]=%p", i3, (void*)pdpt[i3]);
        if ((pdpt[i3] & 1) && !(pdpt[i3] & (1ULL<<7))) {
          auto* pd = reinterpret_cast<uint64_t*>(pdpt[i3] & ~0xFFFULL);
          fk::algorithms::kwarn("PF", "  PD[%zu]=%p", i2, (void*)pd[i2]);
          if ((pd[i2] & 1) && !(pd[i2] & (1ULL<<7))) {
            auto* pt = reinterpret_cast<uint64_t*>(pd[i2] & ~0xFFFULL);
            fk::algorithms::kwarn("PF", "  PT[%zu]=%p", i1, (void*)pt[i1]);
          }
        }
      }
    }
    fkernel::ipc::SignalDelivery::send_signal(task, SIGSEGV, &si, /*force=*/true);
    return;
  }

  // Kernel-mode fault. The only legitimate case is the kernel writing to user memory
  // through copy_to_user/copy_from_user inside an arch_smap_begin() window — that path
  // runs with RFLAGS.AC set. Without AC set, a supervisor fault on a user page is a bug.
  //
  // Exception: when the CPU has no SMAP (e.g. QEMU's default CPU model, SMAP=0), STAC
  // is unavailable and RFLAGS.AC is never set by copy_to_user/copy_from_user. Without
  // SMAP the kernel is free to touch user pages at the hardware level, so any supervisor
  // fault on a user page is CoW/demand-paging recovery, not a kernel bug. Require AC only
  // when SMAP is actually present.
  bool smap_present = CPU::the().has_smap();
  bool kmode_access_authorized = ac_flag || !smap_present;
  if (!task->is_a_kernel_task()) {
    if (not_present && kmode_access_authorized && task->is_address_in_allowed_regions(cr2)) {
      fk::algorithms::kdebug("PF", "kernel demand-page %p pid=%lu (AC)", (void*)cr2, task->control.identity.id.value());
      handle_demand_paging(task, cr2, frame, /*is_user_fault=*/false);
      return;
    }

    if (write_fault && !not_present && kmode_access_authorized && is_user_page(cr2)) {
      fk::algorithms::kdebug("PF", "kernel cow-fault %p pid=%lu (AC)", (void*)cr2, task->control.identity.id.value());
      handle_write_protection(cr2, /*is_user_fault=*/false);
      return;
    }
  }

  // Kernel-mode fault while running a user task: kill it, don't halt the kernel.
  if (!task->is_a_kernel_task()) {
    fk::algorithms::kwarn("PF", "kmode PF CR2=%p ec=0x%lx RIP=%p",
        (void*)cr2, (uint64_t)frame->error_code, (void*)frame->rip);
    fk::algorithms::kwarn("PF", "  np=%d wr=%d ac=%d pid=%lu -> kill",
        (int)not_present, (int)write_fault, (int)ac_flag,
        task->control.identity.id.value());
    SchedulerManager::the().kill_current_from_exception(SIGSEGV);
  }

  // Unhandled kernel fault in a kernel task — a genuine kernel bug. Dump context and halt.
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
