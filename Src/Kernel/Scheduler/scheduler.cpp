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
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Loader/elf_loader.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Driver/Vga/display.h>
#include <LibFK/Algorithms/log.h>

extern CpuControlBlock g_cpu_block;

extern "C" void switch_context(uint64_t *prev_stack_ptr,
                               uint64_t next_stack_ptr);

extern "C" void enter_user_mode(uintptr_t rip, uintptr_t rsp);

extern "C" uint64_t syscall_kernel_stack;

extern "C" void init_task_entry();

extern "C" void idle_task_entry() {
  static bool s_init_spawned = false;
  
  if (APIC::the().get_id() == 0 && !s_init_spawned) {
      s_init_spawned = true;
      
      // Create Init task on CPU 0
      Task *init = new Task();
      *init = create_a_new_task(fk::ProcessId(1), "init", init_task_entry, false, 5, 1, 0, 0);

      // Set initial memory regions for demand paging
      init->resources.memory.regions.heap_start = 0x10000000;
      init->resources.memory.regions.heap_break = 0x10000000;
      init->resources.memory.regions.mmap_start = 0x40000000;
      init->resources.memory.regions.mmap_end = 0x40000000;

      // Migrate logs
      fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial |
                                      fk::algorithms::LogTarget::DebugFS);

      SchedulerManager::the().add_task(init);
      // Note: m_next_pid handling should be updated in initialize if needed, 
      // but since we hardcoded PID 1 here, we just need to ensure next generated PID is 2.
  }

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

  // Remount DevFs to ensure /dev is populated in the new root
  auto& devfs = fkernel::DevFs::the();
  // Ensure /dev exists (if RamDisk didn't create it, we might fail to mount, but usually it exists)
  // For now, assume /dev exists in initrd or VFS handles it.
  fkernel::VirtualFileSystem::the().mount("/dev", fk::RefPtr<Node>(&devfs));

  // Populate FDs 0, 1, 2 for Init process
  auto console_res = fkernel::VirtualFileSystem::the().open("/dev/tty0", O_RDWR);
  if (console_res.is_error()) {
    fk::algorithms::kerror("INIT", "Failed to open /dev/tty0 for init process!");
    return;
  }
  
  auto* current = SchedulerManager::the().current();
  current->add_file_descriptor(console_res.value()); // 0: stdin
  current->add_file_descriptor(console_res.value()); // 1: stdout
  current->add_file_descriptor(console_res.value()); // 2: stderr

  // Load init process from ELF via VFS to resolve symlinks
  auto init_dentry_res = fkernel::VirtualFileSystem::the().resolve_path("/sbin/init");
  if (init_dentry_res.is_error()) {
    fk::algorithms::kerror("INIT", "Could not find /sbin/init in VFS");
  }

  // Create new address space for init
  uintptr_t new_cr3 = VirtualMemoryManager::the().create_address_space();
  VirtualMemoryManager::the().switch_address_space(new_cr3);
  SchedulerManager::the().current()->resources.memory.cr3 = new_cr3;

  auto entry_res = fkernel::ElfLoader::load(init_dentry_res.value()->top_node());

  if (entry_res.is_error()) {
    fk::algorithms::kerror("INIT", "Failed to load /sbin/init ELF (Error %d)", (int)entry_res.error());
    while(1) asm volatile("hlt");
  }

  fkernel::ElfLoadResult elf_res = entry_res.value();
  uintptr_t entry = elf_res.entry;
  fk::algorithms::klog("INIT", "Jumping to user-mode init at %p",
                       (void *)entry);

  // Map user stack (32 KB) at the top of the user address space
  constexpr uintptr_t USER_STACK_TOP = 0x7fffffffe000;
  constexpr size_t STACK_PAGES = 8;
  for (size_t i = 0; i < STACK_PAGES; ++i) {
    uintptr_t stack_phys = PhysicalMemoryManager::the().alloc_page();
    VirtualMemoryManager::the().map_page(
        USER_STACK_TOP - (i + 1) * 0x1000, stack_phys,
        PageFlags::Present | PageFlags::Writable | PageFlags::User);
    memset(reinterpret_cast<void *>(USER_STACK_TOP - (i + 1) * 0x1000), 0,
           0x1000);
  }

  // Setup initial stack frame for Musl/BusyBox
  char *string_area = reinterpret_cast<char *>(USER_STACK_TOP) - 128;
  strcpy(string_area, "/sbin/init");
  strcpy(string_area + 32, "PATH=/bin:/sbin:/usr/bin:/usr/sbin");
  uintptr_t argv0_addr = USER_STACK_TOP - 128;
  uintptr_t envp0_addr = USER_STACK_TOP - 96;

  // Space for auxv: we need more pointers
  uintptr_t *pointers = reinterpret_cast<uintptr_t *>(string_area) - 25;
  size_t idx = 0;
  pointers[idx++] = 1;          // argc
  pointers[idx++] = argv0_addr; // argv[0]
  pointers[idx++] = 0;          // argv[1] (NULL)
  pointers[idx++] = envp0_addr; // envp[0]
  pointers[idx++] = 0;          // envp[1] (NULL)
  
  // Auxv
  pointers[idx++] = 3; pointers[idx++] = elf_res.ph_addr; // AT_PHDR
  pointers[idx++] = 4; pointers[idx++] = elf_res.ph_ent;  // AT_PHENT
  pointers[idx++] = 5; pointers[idx++] = elf_res.ph_num;  // AT_PHNUM
  pointers[idx++] = 6; pointers[idx++] = 4096;           // AT_PAGESZ
  pointers[idx++] = 0; pointers[idx++] = 0;              // AT_NULL

  uintptr_t final_rsp = reinterpret_cast<uintptr_t>(pointers);

  enter_user_mode(entry, final_rsp);

  while (true)
    asm volatile("hlt");
}

void SchedulerManager::initialize() {
  m_is_initialized = true;
  m_processor_count = 1;

  // Create Idle tasks for each CPU
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    Task *idle = new Task();
    *idle =
        create_a_new_task(fk::ProcessId(0), "idle", idle_task_entry, true, 0, 1ULL << i, 0, 0);

    m_processors[i].idle_task = idle;
    m_processors[i].current_task = nullptr;
  }

  m_next_pid = 2; // Init will be PID 1 (created by idle), next is 2

  fk::algorithms::klog(
      "SCHEDULER MANAGER",
      "Initializing SMP Scheduler Manager (round robin per CPU)...");
}

void SchedulerManager::add_task(Task *task) {
  ASSERT(task && task->is_valid());
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  task->control.lifecycle.state = TaskState::Ready;
  task->control.lifecycle.time_slice_ticks = m_default_quantum;

  uint32_t target_cpu = 0;
  if (task->control.lifecycle.cpu_affinity != 0) {
    for (uint32_t i = 0; i < 32; ++i) {
      if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
        target_cpu = i;
        break;
      }
    }
  } else {
    target_cpu = APIC::the().get_id();
  }

  if (target_cpu >= 32) target_cpu = 0;
  
  {
    fk::synchronization::ScopedLock lock(m_processors[target_cpu].run_queue_lock);
    m_processors[target_cpu].run_queue.push_back(task);
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::block_current() {
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  auto &proc = current_processor();
  if (proc.current_task) {
    Task *task = proc.current_task;
    task->control.lifecycle.state = TaskState::Blocked;
    {
      fk::synchronization::ScopedLock lock(m_lock);
      m_wait_queue.push_back(task);
    }
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
    task->control.lifecycle.state = TaskState::Blocked;
    task->control.lifecycle.terminated = true;
    {
      fk::synchronization::ScopedLock lock(m_lock);
      m_zombie_queue.push_back(task);
    }
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
    task->control.lifecycle.state = TaskState::Sleeping;
    task->control.lifecycle.wake_up_time_ticks = TickManager::the().get_ticks() + sleep_ticks;
    {
      fk::synchronization::ScopedLock lock(m_lock);
      m_sleep_queue.push_back(task);
    }
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
    if (task->control.lifecycle.state == TaskState::Running) {
      task->control.lifecycle.state = TaskState::Ready;
      {
        fk::synchronization::ScopedLock lock(proc.run_queue_lock);
        proc.run_queue.push_back(task);
      }
      proc.need_resched = true;
      schedule();
    }
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::wake_task(Task *task) {
  ASSERT(task && task->is_valid());
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  {
    fk::synchronization::ScopedLock lock(m_lock);
    if (task->control.lifecycle.state == TaskState::Blocked) {
      m_wait_queue.remove(task);
    }
  }

  task->control.lifecycle.state = TaskState::Ready;
  task->control.lifecycle.time_slice_ticks = m_default_quantum;

  uint32_t target_cpu = 0;
  for (uint32_t i = 0; i < 32; ++i) {
    if (task->control.lifecycle.cpu_affinity & (1ULL << i)) {
      target_cpu = i;
      break;
    }
  }

  {
    fk::synchronization::ScopedLock lock(m_processors[target_cpu].run_queue_lock);
    m_processors[target_cpu].run_queue.push_back(task);
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::terminate_current(int status) {
    Task* curr = this->current();
    if (!curr) return;

    curr->control.lifecycle.terminated = true;
    curr->control.lifecycle.exit_status = status;

    auto* parent = find_task(curr->control.identity.ppid);
    if (parent) {
        if (parent->control.lifecycle.vfork_waiting && 
            curr->control.lifecycle.vfork_parent_id == parent->control.identity.id) {
            parent->control.lifecycle.vfork_waiting = false;
        }
        wake_task(parent);
    }

    zombify_current();
    schedule();
}

Task *SchedulerManager::pick_next() {
  auto &proc = current_processor();

  {
    fk::synchronization::ScopedLock lock(proc.run_queue_lock);
    if (proc.run_queue.empty()) {
      proc.current_task = proc.idle_task;
    } else {
      Task *next = proc.run_queue.front();
      proc.run_queue.pop_front();

      next->control.lifecycle.state = TaskState::Running;
      next->control.lifecycle.time_slice_ticks = m_default_quantum;
      proc.current_task = next;
    }
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
    Display::the().background_flush();
    fk::synchronization::ScopedLock lock(m_lock);
    for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end();) {
      Task *task = &*it;
      ++it;

      if (task->control.lifecycle.wake_up_time_ticks <= now) {
        m_sleep_queue.remove(task);
        wake_task(task);
      }
    }
  }

  bool is_run_queue_empty = false;
  {
    fk::synchronization::ScopedLock lock(proc.run_queue_lock);
    is_run_queue_empty = proc.run_queue.empty();
  }

  if (!proc.current_task || proc.current_task == proc.idle_task || !is_run_queue_empty) {
    proc.need_resched = true;
  } else if (--proc.current_task->control.lifecycle.time_slice_ticks == 0) {
    Task *task = proc.current_task;
    task->control.lifecycle.state = TaskState::Ready;
    {
      fk::synchronization::ScopedLock lock(proc.run_queue_lock);
      proc.run_queue.push_back(task);
    }
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

    if (next_task && prev_task != next_task) {
      if (prev_task) {
          fk::algorithms::klog(
              "SCHEDULER", "Switch: %s (%p, SP=%p) -> %s (%p, SP=%p)",
              prev_task->control.identity.name.c_str(), prev_task, (void *)prev_task->resources.context.stack_pointer,
              next_task->control.identity.name.c_str(), next_task, (void *)next_task->resources.context.stack_pointer);
      } else {
          fk::algorithms::klog(
              "SCHEDULER", "Switch: (Boot) -> %s (%p, SP=%p)",
              next_task->control.identity.name.c_str(), next_task, (void *)next_task->resources.context.stack_pointer);
      }

      if (prev_task && next_task->resources.memory.cr3 != 0 && next_task->resources.memory.cr3 != prev_task->resources.memory.cr3) {
        VirtualMemoryManager::the().switch_address_space(next_task->resources.memory.cr3);
      } else if (!prev_task && next_task->resources.memory.cr3 != 0) {
        VirtualMemoryManager::the().switch_address_space(next_task->resources.memory.cr3);
      }

      if (prev_task) {
        prev_task->resources.context.user_rsp = g_cpu_block.user_rsp;
        prev_task->resources.context.saved_rip = g_cpu_block.saved_rip;
        prev_task->resources.context.saved_rflags = g_cpu_block.saved_rflags;

        prev_task->resources.context.fs_base = CPU::the().read_msr(MSR_FS_BASE);
        prev_task->resources.context.gs_base = CPU::the().read_msr(MSR_KERNEL_GS_BASE);
      }

      g_cpu_block.kernel_stack = next_task->resources.context.kernel_stack_top;
      g_cpu_block.user_rsp = next_task->resources.context.user_rsp;
      g_cpu_block.saved_rip = next_task->resources.context.saved_rip;
      g_cpu_block.saved_rflags = next_task->resources.context.saved_rflags;

      CPU::the().write_msr(MSR_FS_BASE, next_task->resources.context.fs_base);
      CPU::the().write_msr(MSR_KERNEL_GS_BASE, next_task->resources.context.gs_base);

      GDTController::the().set_kernel_stack(next_task->resources.context.kernel_stack_top);
      
      if (prev_task) {
          switch_context(&prev_task->resources.context.stack_pointer, next_task->resources.context.stack_pointer);
      } else {
          uint64_t dummy_stack_ptr;
          switch_context(&dummy_stack_ptr, next_task->resources.context.stack_pointer);
      }
    }
  }

  if (intr_state) InterruptController::the().enable_interrupt();
}

void SchedulerManager::print_all_tasks() {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    auto &proc = m_processors[i];
    if (proc.current_task) proc.current_task->print_info();
    if (proc.idle_task) proc.idle_task->print_info();
    for (auto it = proc.run_queue.begin(); it != proc.run_queue.end(); ++it) it->print_info();
  }
  for (auto it = m_wait_queue.begin(); it != m_wait_queue.end(); ++it) it->print_info();
  for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) it->print_info();
  for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end(); ++it) it->print_info();
}

Task *SchedulerManager::find_task(fk::ProcessId id) {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    auto &proc = m_processors[i];
    if (proc.current_task && proc.current_task->control.identity.id == id)
      return proc.current_task;
    if (proc.idle_task && proc.idle_task->control.identity.id == id)
      return proc.idle_task;

    {
      fk::synchronization::ScopedLock lock(proc.run_queue_lock);
      for (auto it = proc.run_queue.begin(); it != proc.run_queue.end(); ++it) {
        if (it->control.identity.id == id) return &*it;
      }
    }
  }

  {
    fk::synchronization::ScopedLock lock(m_lock);
    for (auto it = m_wait_queue.begin(); it != m_wait_queue.end(); ++it) {
      if (it->control.identity.id == id) return &*it;
    }

    for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) {
      if (it->control.identity.id == id) return &*it;
    }

    for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end(); ++it) {
      if (it->control.identity.id == id) return &*it;
    }
  }

  return nullptr;
}

Task *SchedulerManager::find_terminated_child(fk::ProcessId ppid) {
  fk::synchronization::ScopedLock lock(m_lock);
  for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) {
    Task& task = *it;
    ASSERT(task.is_valid());
    if (task.control.identity.ppid == ppid && task.control.lifecycle.terminated)
      return &task;
  }
  return nullptr;
}

Task *SchedulerManager::find_any_child(fk::ProcessId ppid) {
  for (uint32_t i = 0; i < m_processor_count; ++i) {
    auto &proc = m_processors[i];
    if (proc.current_task && proc.current_task->is_valid() && proc.current_task->control.identity.ppid == ppid)
      return proc.current_task;

    {
      fk::synchronization::ScopedLock lock(proc.run_queue_lock);
      for (auto it = proc.run_queue.begin(); it != proc.run_queue.end(); ++it) {
        if (it->is_valid() && it->control.identity.ppid == ppid) return &*it;
      }
    }
  }

  {
    fk::synchronization::ScopedLock lock(m_lock);
    for (auto it = m_wait_queue.begin(); it != m_wait_queue.end(); ++it) {
      if (it->is_valid() && it->control.identity.ppid == ppid) return &*it;
    }

    for (auto it = m_zombie_queue.begin(); it != m_zombie_queue.end(); ++it) {
      if (it->is_valid() && it->control.identity.ppid == ppid) return &*it;
    }

    for (auto it = m_sleep_queue.begin(); it != m_sleep_queue.end(); ++it) {
      if (it->is_valid() && it->control.identity.ppid == ppid) return &*it;
    }
  }

  return nullptr;
}

void SchedulerManager::reap_zombie(Task *task) {
  ASSERT(task && task->is_valid());
  bool intr_state = InterruptController::the().get_interrupt_state();
  InterruptController::the().disable_interrupt();

  {
    fk::synchronization::ScopedLock lock(m_lock);
    m_zombie_queue.remove(task);
  }
  task->invalidate();

  if (intr_state) InterruptController::the().enable_interrupt();
}
