#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Ipc/capability.h>
#include <Kernel/Ipc/cspace.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

#include <Kernel/Ipc/global_endpoint_manager.h>
#include <Kernel/Ipc/notification.h>

extern "C" void task_trampoline();

extern size_t g_xsave_area_size;

static fkernel::ipc::CapabilityRights fd_flags_to_rights(int flags) {
  int acc = flags & 3;
  fkernel::ipc::CapabilityRights rights = fkernel::ipc::CapabilityRights::None;
  if (acc == 0 || acc == 2) rights = rights | fkernel::ipc::CapabilityRights::Read;
  if (acc == 1 || acc == 2) rights = rights | fkernel::ipc::CapabilityRights::Write;
  rights = rights | fkernel::ipc::CapabilityRights::Seek | fkernel::ipc::CapabilityRights::Ioctl;
  return rights;
}

void initialize_task(Task* task, fk::ProcessId id, const fk::text::fixed_string<64>& name,
                     void (*entry)(), bool kernel_task, uint8_t priority,
                     uint64_t cpu_affinity, uint64_t arg1, uint64_t arg2,
                     fkernel::scheduler::QoSClass qos) {
  if (!task) return;
  const size_t STACK_SIZE = 16 * fk::types::KiB;

  void* stack_mem = kmalloc(STACK_SIZE);
  if (!stack_mem) {
    fk::algorithms::kwarn("TASK", "initialize_task: stack allocation failed for task %lu", id.value());
    return;
  }
  uint64_t stack_top = reinterpret_cast<uint64_t>(stack_mem) + STACK_SIZE;

  // Setup initial stack for switch_context
  uint64_t* stack = reinterpret_cast<uint64_t*>(stack_top);
  *(--stack) = reinterpret_cast<uint64_t>(task_trampoline);
  *(--stack) = 0;                                  // rbx
  *(--stack) = 0;                                  // rbp
  *(--stack) = arg1;                               // r12
  *(--stack) = arg2;                               // r13
  *(--stack) = reinterpret_cast<uint64_t>(entry);  // r14
  *(--stack) = 0;                                  // r15

  // Initialize IPC CSpace
  auto* cspace = new fkernel::ipc::CSpace();
  auto* signal_notification = new fkernel::ipc::Notification();
  cspace->install(
      fkernel::ipc::Capability(signal_notification, fkernel::ipc::CapabilityType::Notification));

  // Also register in global manager for sys_kill access
  fkernel::ipc::GlobalEndpointManager::the().register_notification(id.value(), signal_notification);

  task->magic = Task::MAGIC;
  task->control.identity = {.id = id, .tgid = id, .ppid = fk::ProcessId(), .pgid = id, .sid = id, .name = name};
  task->control.lifecycle = {.state = TaskState::Ready,
                             .priority = priority,
                             .nice = 0,
                             .cpu_affinity = cpu_affinity,
                             .time_slice_ticks = 0,
                             .wake_up_time_ticks = 0,
                             .is_a_kernel_task = kernel_task,
                             .terminated = false,
                             .exit_status = 0,
                             .clear_child_tid = 0,
                             .vfork_waiting = false,
                             .vfork_parent_id = fk::ProcessId(),
                             .is_vfork_sharing_address_space = false,
                             .in_wait_queue = false,
                             .qos = qos,
                             .policy = fkernel::scheduler::SchedulingPolicy::Normal,
                             .base_priority = fkernel::scheduler::priority_for_qos(qos).value(),
                             .mlfq_level = fkernel::scheduler::qos_level(qos).default_mlfq_level.value(),
                             .cpu_time_consumed = 0,
                             .allotment_ticks = fkernel::scheduler::allotment_for_qos(qos).value(),
                             .boosted = false,
                             .original_qos = qos};

  task->resources.memory = {.cr3 = arch_read_cr3()};
  task->resources.files.cwd = "/";
  task->resources.ipc.cspace = cspace;
  task->resources.ipc.signal_notification = signal_notification;
  uint8_t* xsave_buf = nullptr;
  if (g_xsave_area_size > 512) {
    size_t alloc_size = g_xsave_area_size + 64;
    void* raw = kmalloc(alloc_size);
    if (raw) {
      xsave_buf = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(raw) + 63) & ~63ULL);
    }
  }

  task->resources.context = {
      .registers = GetContextForNewTask(reinterpret_cast<uint64_t>(stack), kernel_task, arg1, arg2),
      .stack_pointer = reinterpret_cast<uint64_t>(stack),
      .kernel_stack_top = stack_top,
      .user_rsp = 0,
      .saved_rip = 0,
      .saved_rflags = 0,
      .fs_base = 0,
      .gs_base = 0,
      .xsave_area = xsave_buf,
      .xsave_size = g_xsave_area_size};

  // Pre-initialize fx_state with valid x87/SSE defaults so the first fxrstor is safe.
  // FCW=0x037F (all exceptions masked), MXCSR=0x1F80 (default rounding, all masked).
  uint8_t* fx = task->resources.context.fx_state;
  fx[0] = 0x7F; fx[1] = 0x03; // FCW = 0x037F
  fx[24] = 0x80; fx[25] = 0x1F; // MXCSR = 0x1F80

  fk::algorithms::kdebug("TASK", "Task %lu created (%s)", id.value(), name.c_str());
}

void Task::destroy() {
  fk::algorithms::kdebug("TASK", "Destroying task %lu", control.identity.id.value());
  // Unregister signal notification from the global IPC table
  fkernel::ipc::GlobalEndpointManager::the().remove_notification(
      control.identity.id.value());

  // Delete the signal notification and capability space
  delete resources.ipc.signal_notification;
  resources.ipc.signal_notification = nullptr;
  delete resources.ipc.cspace;
  resources.ipc.cspace = nullptr;

  // Free the kernel stack (allocated with kmalloc in fork/create_a_new_task)
  static constexpr size_t KERNEL_STACK_SIZE = 16 * 1024;
  if (resources.context.kernel_stack_top) {
    kfree(reinterpret_cast<void*>(resources.context.kernel_stack_top - KERNEL_STACK_SIZE));
    resources.context.kernel_stack_top = 0;
  }

  // Free the pre-execve address space (fork clone deferred from execve)
  if (resources.memory.prev_cr3) {
    VirtualMemoryManager::the().free_address_space(resources.memory.prev_cr3);
    resources.memory.prev_cr3 = 0;
  }

  // Free the current user address space
  if (resources.memory.cr3) {
    VirtualMemoryManager::the().free_address_space(resources.memory.cr3);
    resources.memory.cr3 = 0;
  }
}

void Task::set_heap_regions(uintptr_t start, uintptr_t break_addr) {
    resources.memory.regions.heap_start = start;
    resources.memory.regions.heap_break = break_addr;
}

void Task::set_mmap_regions(uintptr_t start, uintptr_t end) {
    resources.memory.regions.mmap_start = start;
    resources.memory.regions.mmap_end = end;
}

bool Task::is_address_in_allowed_regions(uintptr_t address) const {
    for (size_t i = 0; i < resources.memory.regions.list.size(); ++i) {
        if (resources.memory.regions.list[i].contains(address)) {
            return true;
        }
    }
    if (address >= resources.memory.regions.heap_start && address < resources.memory.regions.heap_break) {
        return true;
    }
    if (address >= resources.memory.regions.mmap_start && address < resources.memory.regions.mmap_end) {
        return true;
    }
    if (address >= 0x7ffffff00000ULL && address < 0x7fffffffe000ULL) {
        return true;
    }
    auto flags_res = VirtualMemoryManager::the().get_page_flags(address & ~0xFFFULL);
    if (!flags_res.is_error()) {
        if (static_cast<uint64_t>(flags_res.value()) & static_cast<uint64_t>(PageFlags::User)) {
            return true;
        }
    }
    return false;
}

void Task::print_info() const {}

int Task::add_file_descriptor(fk::RefPtr<FileDescription> description) {
  fk::synchronization::ScopedLockIRQ lock_task(lock);

  uint32_t cap_handle = fkernel::ipc::INVALID_HANDLE;
  if (resources.ipc.cspace && description)
    cap_handle = resources.ipc.cspace->install_fd(
        description.get(), fd_flags_to_rights(description->open_flags()));

  for (size_t i = 0; i < resources.files.descriptors.size(); ++i) {
    if (!resources.files.descriptors[i]) {
      resources.files.descriptors[i] = description;
      resources.files.cap_handles[i]  = cap_handle;
      return static_cast<int>(i);
    }
  }

  if (resources.files.descriptors.is_full()) {
    fk::algorithms::kwarn("TASK", "Task %lu: FD table full!", control.identity.id.value());
    if (cap_handle != fkernel::ipc::INVALID_HANDLE && resources.ipc.cspace)
      resources.ipc.cspace->revoke_fd(cap_handle);
    return -24;
  }

  int fd = static_cast<int>(resources.files.descriptors.size());
  resources.files.descriptors.push_back(description);
  resources.files.cap_handles.push_back(cap_handle);
  return fd;
}

void Task::dump_file_descriptors() const {
  fk::synchronization::ScopedLockIRQ lock_task(lock);

  for (size_t i = 0; i < resources.files.descriptors.size(); ++i) {
    if (resources.files.descriptors[i]) {
      auto node = resources.files.descriptors[i]->node();
      const char* type = "unknown";
      if (node->is_character_device())
        type = "char";
      else if (node->is_block_device())
        type = "block";
      else if (node->is_directory())
        type = "dir";
      else
        type = "file";
      (void)type;
    }
  }
}

int Task::dup_file_descriptor(int old_fd, bool cloexec, int min_fd) {
  fk::synchronization::ScopedLockIRQ lock_task(lock);
  auto description = (old_fd < 0 || old_fd >= static_cast<int>(resources.files.descriptors.size()))
                         ? nullptr
                         : resources.files.descriptors[old_fd];
  if (!description) {
    fk::algorithms::kwarn("TASK", "dup_file_descriptor: Source FD %d not found", old_fd);
    return -1;
  }
  if (cloexec) description->set_cloexec(true);

  for (int i = min_fd; i < static_cast<int>(resources.files.descriptors.capacity()); ++i) {
    if (i >= static_cast<int>(resources.files.descriptors.size())) {
      while (static_cast<int>(resources.files.descriptors.size()) < i) {
        resources.files.descriptors.push_back({});
        resources.files.cap_handles.push_back(fkernel::ipc::INVALID_HANDLE);
      }
      uint32_t cap_handle = fkernel::ipc::INVALID_HANDLE;
      if (resources.ipc.cspace)
        cap_handle = resources.ipc.cspace->install_fd(
            description.get(), fd_flags_to_rights(description->open_flags()));
      resources.files.descriptors.push_back(description);
      resources.files.cap_handles.push_back(cap_handle);
      return static_cast<int>(resources.files.descriptors.size()) - 1;
    }

    if (!resources.files.descriptors[i]) {
      uint32_t cap_handle = fkernel::ipc::INVALID_HANDLE;
      if (resources.ipc.cspace)
        cap_handle = resources.ipc.cspace->install_fd(
            description.get(), fd_flags_to_rights(description->open_flags()));
      resources.files.descriptors[i] = description;
      resources.files.cap_handles[i]  = cap_handle;
      return i;
    }
  }

  return -1;
}

int Task::install_at(int newfd, fk::RefPtr<FileDescription> desc) {
  fk::synchronization::ScopedLockIRQ lock_task(lock);
  if (newfd < 0 || newfd >= static_cast<int>(MAX_OPEN_FILES))
    return -1;

  while (static_cast<int>(resources.files.descriptors.size()) <= newfd) {
    resources.files.descriptors.push_back({});
    resources.files.cap_handles.push_back(fkernel::ipc::INVALID_HANDLE);
  }

  uint32_t old_handle = resources.files.cap_handles[newfd];
  if (resources.files.descriptors[newfd] && resources.ipc.cspace
      && old_handle != fkernel::ipc::INVALID_HANDLE)
    resources.ipc.cspace->revoke_fd(old_handle);

  uint32_t new_handle = fkernel::ipc::INVALID_HANDLE;
  if (resources.ipc.cspace && desc)
    new_handle = resources.ipc.cspace->install_fd(
        desc.get(), fd_flags_to_rights(desc->open_flags()));

  resources.files.descriptors[newfd] = desc;
  resources.files.cap_handles[newfd]  = new_handle;
  return newfd;
}

fk::RefPtr<FileDescription> Task::get_file_descriptor(int fd) {
  if (fd < 0 || fd >= static_cast<int>(resources.files.descriptors.size()))
    return {};

  auto desc = resources.files.descriptors[fd];
  if (!desc) return {};

  if (resources.ipc.cspace && fd < static_cast<int>(resources.files.cap_handles.size())) {
    uint32_t handle = resources.files.cap_handles[fd];
    if (handle == fkernel::ipc::INVALID_HANDLE)
      return {};
    if (!resources.ipc.cspace->lookup_fd(handle))
      return {};
  }

  return desc;
}

void Task::close_file_descriptor(int fd) {
  fk::RefPtr<FileDescription> desc;
  {
    fk::synchronization::ScopedLockIRQ lock_task(lock);
    if (fd < 0 || fd >= static_cast<int>(resources.files.descriptors.size()))
      return;

    if (resources.ipc.cspace && fd < static_cast<int>(resources.files.cap_handles.size())) {
      uint32_t handle = resources.files.cap_handles[fd];
      if (handle != fkernel::ipc::INVALID_HANDLE)
        resources.ipc.cspace->revoke_fd(handle);
      resources.files.cap_handles[fd] = fkernel::ipc::INVALID_HANDLE;
    }

    desc = resources.files.descriptors[fd];
    resources.files.descriptors[fd] = nullptr;
  }
  // Call on_close() outside the task lock to avoid lock-order inversion.
  // POSIX: all advisory locks on this file held by this process are released
  // when any fd to the file is closed.
  if (desc && desc->node()) {
    desc->node()->on_close();
    desc->node()->release_all_locks_for_process(control.identity.id);
  }
}

void Task::release_all_file_locks() {
  fk::synchronization::ScopedLockIRQ lock_task(lock);
  for (size_t i = 0; i < resources.files.descriptors.size(); ++i) {
    auto desc = resources.files.descriptors[i];
    if (!desc) continue;
    auto node = desc->node();
    if (node)
      node->release_all_locks_for_process(control.identity.id);
  }
}