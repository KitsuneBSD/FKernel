#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Segments/Tss/tss_stacks.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Fs/RamDisk/ram_disk.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Loader/elf_loader.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

extern CpuControlBlock g_cpu_block;

extern "C" void switch_context(uint64_t *prev_stack_ptr,
                               uint64_t next_stack_ptr);

extern "C" void enter_user_mode(uintptr_t rip, uintptr_t rsp);

extern "C" uint64_t syscall_kernel_stack;

extern "C" void idle_task_entry() {
  while (true) {
    asm volatile("hlt");
  }
}
SchedulerManager::SchedulerManager() {
  for (int i = 0; i < 32; ++i) {
    m_processors[i].id = i;
  }
}

extern "C" void init_task_entry() {
  fk::algorithms::klog("INIT", "Init process trampoline started.");

  auto &modules = boot::BootInfo::the().get_modules();
  if (modules.is_empty()) {
    fk::algorithms::kerror("INIT", "No RamDisk module found!");
  }

  // Assume the first module is our RamDisk
  auto &ramdisk_mod = modules[0];
  auto ramdisk_res =
      fkernel::RamDiskNode::create(ramdisk_mod.start, ramdisk_mod.end);
  if (ramdisk_res.is_error()) {
    fk::algorithms::kerror("INIT", "Failed to create RamDisk");
    return;
  }

  auto ramdisk = ramdisk_res.value();
  fkernel::VirtualFileSystem::the().mount("/", ramdisk);

  // Load init process from ELF via VFS to resolve symlinks
  auto init_dentry_res = fkernel::VirtualFileSystem::the().resolve_path("/sbin/init");
  if (init_dentry_res.is_error()) {
    fk::algorithms::kerror("INIT", "Could not find /sbin/init in VFS");
  }

  // Create new address space for init
  uintptr_t new_cr3 = VirtualMemoryManager::the().create_address_space();
  VirtualMemoryManager::the().switch_address_space(new_cr3);
  SchedulerManager::the().current()->cr3 = new_cr3;

  auto entry_res = fkernel::ElfLoader::load(init_dentry_res.value()->top_node());

  if (entry_res.is_error()) {
    fk::algorithms::kerror("INIT", "Failed to load /sbin/init ELF (Error %d)", (int)entry_res.error());
    while(1) asm volatile("hlt");
  }

  uintptr_t entry = entry_res.value();
  fk::algorithms::klog("INIT", "Jumping to user-mode init at %p",
                       (void *)entry);

  // Map user stack (16 KB) at the top of the user address space
  constexpr uintptr_t USER_STACK_TOP = 0x7fffffffe000;
  constexpr size_t STACK_PAGES = 4;
  for (size_t i = 0; i < STACK_PAGES; ++i) {
    uintptr_t stack_phys = PhysicalMemoryManager::the().alloc_page();
    VirtualMemoryManager::the().map_page(
        USER_STACK_TOP - (i + 1) * 0x1000, stack_phys,
        PageFlags::Present | PageFlags::Writable | PageFlags::User);
    memset(reinterpret_cast<void *>(USER_STACK_TOP - (i + 1) * 0x1000), 0,
           0x1000);
  }

  // Setup initial stack frame for Musl/BusyBox
  // Layout: [argc] [argv0_ptr] [argv_null] [envp0_ptr] [envp_null]
  // [string_data]

  // 1. Write the string data at the top
  char *string_area = reinterpret_cast<char *>(USER_STACK_TOP) - 128;
  strcpy(string_area, "/sbin/init");
  strcpy(string_area + 32, "PATH=/bin:/sbin:/usr/bin:/usr/sbin");
  uintptr_t argv0_addr = USER_STACK_TOP - 128;
  uintptr_t envp0_addr = USER_STACK_TOP - 96;

  // 2. Setup the pointers below the string area
  // Layout: [argc] [argv0] [NULL] [envp0] [NULL] [AT_PAGESZ] [4096] [AT_NULL]
  // [0]
  uintptr_t *pointers = reinterpret_cast<uintptr_t *>(string_area) - 9;
  pointers[0] = 1;          // argc
  pointers[1] = argv0_addr; // argv[0]
  pointers[2] = 0;          // argv[1] (NULL)
  pointers[3] = envp0_addr; // envp[0]
  pointers[4] = 0;          // envp[1] (NULL)
  pointers[5] = 6;          // AT_PAGESZ
  pointers[6] = 4096;
  pointers[7] = 0; // AT_NULL
  pointers[8] = 0;

  uintptr_t final_rsp = reinterpret_cast<uintptr_t>(pointers);

  enter_user_mode(entry, final_rsp);

  while (true)
    asm volatile("hlt");
}

void SchedulerManager::initialize() {
  m_is_initialized = true;
  m_processor_count = 1; // Start with 1, APIC/ACPI should update this later

  // Create Idle tasks for each CPU
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    Task *idle = new Task();
    *idle =
        create_a_new_task(0, "idle", idle_task_entry, true, 0, 1ULL << i, 0, 0);

    idle->gs_base = (uint64_t)&g_cpu_block;

    m_processors[i].idle_task = idle;
    m_processors[i].current_task = idle;
  }

  // Create Init task on CPU 0
  Task *init = new Task();
  *init = create_a_new_task(1, "init", init_task_entry, false, 5, 1, 0, 0);

  // x86_64: Initialize GS base to point to the CPU block for the first task
  // In a multi-core kernel, this would be the block for the specific CPU.
  init->gs_base = (uint64_t)&g_cpu_block;

  // Set initial memory regions for demand paging
  init->memory_regions.heap_start = 0x10000000;
  init->memory_regions.heap_break = 0x10000000;
  init->memory_regions.mmap_start = 0x40000000;
  init->memory_regions.mmap_end = 0x40000000;

  // Populate FDs 0, 1, 2. We use tty0 instead of console for better input support.
  auto console_res = fkernel::VirtualFileSystem::the().open("/dev/tty0", O_RDWR);
  if (console_res.is_ok()) {
    init->add_file_descriptor(console_res.value()); // 0
    init->add_file_descriptor(console_res.value()); // 1
    init->add_file_descriptor(console_res.value()); // 2
  } else {
    fk::algorithms::kerror("SCHEDULER", "Init failed to open /dev/console: %d", (int)console_res.error());
  }

  init->dump_file_descriptors();

  // Migrate logs: keep Serial and DebugFS
  fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial |
                                  fk::algorithms::LogTarget::DebugFS);

  add_task(init);
  m_next_pid = 2; // Init is PID 1

  fk::algorithms::klog(
      "SCHEDULER MANAGER",
      "Initializing SMP Scheduler Manager (round robin per CPU)...");
}

void SchedulerManager::add_task(Task *task) {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  task->state = TaskState::Ready;
  task->time_slice_ticks = m_default_quantum;

  uint32_t target_cpu = 0;
  if (task->cpu_affinity != 0) {
    for (uint32_t i = 0; i < 32; ++i) {
      if (task->cpu_affinity & (1ULL << i)) {
        target_cpu = i;
        break;
      }
    }
  } else {
    target_cpu = APIC::the().get_id();
  }

  if (target_cpu >= 32)
    target_cpu = 0;
  m_processors[target_cpu].run_queue.push_back(task);

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::block_current() {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  auto &proc = current_processor();
  if (proc.current_task) {
    Task *task = proc.current_task;
    task->state = TaskState::Blocked;
    m_wait_queue.push_back(task);
    proc.need_resched = true;
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::zombify_current() {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  auto &proc = current_processor();
  if (proc.current_task) {
    Task *task = proc.current_task;
    task->state = TaskState::Blocked;
    task->terminated = true;
    proc.need_resched = true;
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::sleep_current(uint64_t sleep_ticks) {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  auto &proc = current_processor();
  if (proc.current_task) {
    Task *task = proc.current_task;
    task->state = TaskState::Sleeping;
    task->wake_up_time_ticks = TickManager::the().get_ticks() + sleep_ticks;
    m_sleep_queue.push_back(task);
    proc.need_resched = true;
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::yield() {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  auto &proc = current_processor();
  if (proc.current_task) {
    Task *task = proc.current_task;
    // Only yield if we are currently running
    if (task->state == TaskState::Running) {
      task->state = TaskState::Ready;
      proc.run_queue.push_back(task);
      proc.need_resched = true;
      schedule(); // Switch context immediately
    }
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::wake_task(Task *task) {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  if (task->state == TaskState::Blocked) {
    m_wait_queue.remove(task);
  }

  task->state = TaskState::Ready;
  task->time_slice_ticks = m_default_quantum;

  uint32_t target_cpu = 0;
  for (uint32_t i = 0; i < 32; ++i) {
    if (task->cpu_affinity & (1ULL << i)) {
      target_cpu = i;
      break;
    }
  }

  m_processors[target_cpu].run_queue.push_back(task);

  if (intr_state) InterruptController::the().enable_interrupt();
}

Task *SchedulerManager::pick_next() {
  // pick_next is called within schedule() which is already protected
  auto &proc = current_processor();

  if (proc.run_queue.empty()) {
    proc.current_task = proc.idle_task;
  } else {
    Task *next = proc.run_queue.front();
    proc.run_queue.pop_front();

    /*
    if (next->id > 1) {
        fk::algorithms::kdebug("SCHEDULER", "Scheduling task %lu (%s)", next->id, next->name.c_str());
    }
    */

    next->state = TaskState::Running;
    next->time_slice_ticks = m_default_quantum;
    proc.current_task = next;
  }

  proc.need_resched = false;
  return proc.current_task;
}

void SchedulerManager::on_tick() {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  uint64_t now = TickManager::the().get_ticks();
  auto &proc = current_processor();

  if (proc.id == 0) {
    for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end();) {
      Task *task = &*it;
      ++it;

      if (task->wake_up_time_ticks <= now) {
        m_sleep_queue.remove(task);
        wake_task(task);
      }
    }
  }

  if (!proc.current_task || proc.current_task == proc.idle_task || !proc.run_queue.empty()) {
    proc.need_resched = true;
  } else if (--proc.current_task->time_slice_ticks == 0) {
    Task *task = proc.current_task;
    task->state = TaskState::Ready;
    proc.run_queue.push_back(task);
    proc.need_resched = true;
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

fkernel::Processor &SchedulerManager::current_processor() {
  uint32_t id = APIC::the().get_id();
  if (id < 32)
    return m_processors[id];
  return m_processors[0];
}

void SchedulerManager::schedule() {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  if (is_need_resched()) {
    auto &proc = current_processor();
    Task *prev_task = proc.current_task;
    Task *next_task = pick_next();

    if (prev_task && prev_task != next_task) {
      fk::algorithms::klog(
          "SCHEDULER", "Switch: %s (%p, SP=%p) -> %s (%p, SP=%p)",
          prev_task->name.c_str(), prev_task, (void *)prev_task->stack_pointer,
          next_task->name.c_str(), next_task, (void *)next_task->stack_pointer);

      if (next_task->cr3 != 0 && next_task->cr3 != prev_task->cr3) {
        VirtualMemoryManager::the().switch_address_space(next_task->cr3);
      }

      prev_task->user_rsp = g_cpu_block.user_rsp;
      prev_task->saved_rip = g_cpu_block.saved_rip;
      prev_task->saved_rflags = g_cpu_block.saved_rflags;

      g_cpu_block.kernel_stack = next_task->kernel_stack_top;
      g_cpu_block.user_rsp = next_task->user_rsp;
      g_cpu_block.saved_rip = next_task->saved_rip;
      g_cpu_block.saved_rflags = next_task->saved_rflags;

      CPU::the().write_msr(MSR_FS_BASE, next_task->fs_base);
      CPU::the().write_msr(MSR_KERNEL_GS_BASE, next_task->gs_base);

      GDTController::the().set_kernel_stack(next_task->kernel_stack_top);
      switch_context(&prev_task->stack_pointer, next_task->stack_pointer);
    }
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::print_all_tasks() {
  fk::algorithms::klog("SCHEDULER", "Listing tasks for %u processors",
                       m_processor_count);
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    auto &proc = m_processors[i];
    fk::algorithms::klog("SCHEDULER", "Processor %u:", proc.id);

    if (proc.current_task) {
      fk::algorithms::kdebug("SCHEDULER", " Current:");
      proc.current_task->print_info();
    }

    if (proc.idle_task) {
      fk::algorithms::kdebug("SCHEDULER", " Idle:");
      proc.idle_task->print_info();
    }

    for (auto it = proc.run_queue.begin(); it != proc.run_queue.end(); ++it) {
      Task &t = *it;
      fk::algorithms::kdebug("SCHEDULER", " RunQueue:");
      t.print_info();
    }
  }

  // global wait/sleep queues
  for (auto it = m_wait_queue.begin(); it != m_wait_queue.end(); ++it) {
    Task &t = *it;
    fk::algorithms::kdebug("SCHEDULER", " WaitQueue:");
    t.print_info();
  }

  for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) {
    Task &t = *it;
    fk::algorithms::kdebug("SCHEDULER", " ZombieQueue:");
    t.print_info();
  }

  for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end(); ++it) {
    Task &t = *it;
    fk::algorithms::kdebug("SCHEDULER", " SleepQueue:");
    t.print_info();
  }
}

Task *SchedulerManager::find_task(TaskId id) {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    auto &proc = m_processors[i];
    if (proc.current_task && proc.current_task->id == id)
      return proc.current_task;
    if (proc.idle_task && proc.idle_task->id == id)
      return proc.idle_task;

    for (auto it = proc.run_queue.begin(); it != proc.run_queue.end(); ++it) {
      Task &t = *it;
      if (t.id == id)
        return &t;
    }
  }

  for (auto it = m_wait_queue.begin(); it != m_wait_queue.end(); ++it) {
    Task &t = *it;
    if (t.id == id)
      return &t;
  }

  for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) {
    Task &t = *it;
    if (t.id == id)
      return &t;
  }

  for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end(); ++it) {
    Task &t = *it;
    if (t.id == id)
      return &t;
  }

  return nullptr;
}

Task *SchedulerManager::find_terminated_child(TaskId ppid) {
  for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) {
    if (it->ppid == ppid && it->terminated)
      return &*it;
  }
  return nullptr;
}

Task *SchedulerManager::find_any_child(TaskId ppid) {
  // Check all queues for ANY child
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    auto &proc = m_processors[i];
    if (proc.current_task && proc.current_task->ppid == ppid)
      return proc.current_task;

    for (auto it = proc.run_queue.begin(); it != proc.run_queue.end(); ++it) {
      if (it->ppid == ppid)
        return &*it;
    }
  }

  for (auto it = m_wait_queue.begin(); it != m_wait_queue.end(); ++it) {
    if (it->ppid == ppid)
      return &*it;
  }

  for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) {
    if (it->ppid == ppid)
      return &*it;
  }

  for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end(); ++it) {
    if (it->ppid == ppid)
      return &*it;
  }

  return nullptr;
}