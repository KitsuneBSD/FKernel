#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/global_endpoint_manager.h>
#include <Kernel/Ipc/notification.h>

extern "C" void task_trampoline();

Task create_a_new_task(fk::ProcessId id, const fk::text::fixed_string<64>& name, void (*entry)(),
                       bool kernel_task, uint8_t priority, uint64_t cpu_affinity, uint64_t arg1,
                       uint64_t arg2) {
  const size_t STACK_SIZE = 16 * fk::types::KiB;

  void* stack_mem = kmalloc(STACK_SIZE);
  uint64_t stack_top = reinterpret_cast<uint64_t>(stack_mem) + STACK_SIZE;

  // Setup initial stack for switch_context
  uint64_t* stack = reinterpret_cast<uint64_t*>(stack_top);
  *(--stack) = reinterpret_cast<uint64_t>(task_trampoline);
  *(--stack) = 0;                                 // rbx
  *(--stack) = 0;                                 // rbp
  *(--stack) = arg1;                              // r12
  *(--stack) = arg2;                              // r13
  *(--stack) = reinterpret_cast<uint64_t>(entry); // r14
  *(--stack) = 0;                                 // r15

  // Initialize IPC CSpace
  auto* cspace = new fkernel::ipc::CSpace();
  auto* signal_notification = new fkernel::ipc::Notification();
  cspace->install(
      fkernel::ipc::Capability(signal_notification, fkernel::ipc::CapabilityType::Notification));

  // Also register in global manager for sys_kill access
  fkernel::ipc::GlobalEndpointManager::the().register_notification(id.value(), signal_notification);

  Task task;
  task.magic = Task::MAGIC;
  task.control.identity = {.id = id, .ppid = fk::ProcessId(), .name = name};
  task.control.lifecycle = {.state = TaskState::Ready,
                            .priority = priority,
                            .cpu_affinity = cpu_affinity,
                            .time_slice_ticks = 0,
                            .wake_up_time_ticks = 0,
                            .is_a_kernel_task = kernel_task,
                            .terminated = false,
                            .exit_status = 0,
                            .clear_child_tid = 0,
                            .vfork_waiting = false,
                            .vfork_parent_id = fk::ProcessId(),
                            .is_vfork_sharing_address_space = false};

  task.resources.memory = {.cr3 = read_on_cr3()};
  task.resources.files.cwd = "/";
  task.resources.ipc.cspace = cspace;
  task.resources.ipc.signal_notification = signal_notification;
  task.resources.context = {
      .registers = GetContextForNewTask(reinterpret_cast<uint64_t>(stack), kernel_task, arg1, arg2),
      .stack_pointer = reinterpret_cast<uint64_t>(stack),
      .kernel_stack_top = stack_top,
      .user_rsp = 0,
      .saved_rip = 0,
      .saved_rflags = 0,
      .fs_base = 0,
      .gs_base = 0};

  return task;
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
    if (address >= resources.memory.regions.heap_start && address < resources.memory.regions.heap_break) {
        return true;
    }
    if (address >= resources.memory.regions.mmap_start && address < resources.memory.regions.mmap_end) {
        return true;
    }
    if (address >= 0x7ffffff00000ULL && address < 0x7fffffffe000ULL) {
        return true;
    }
    return false;
}

void Task::print_info() const {}

int Task::add_file_descriptor(fk::RefPtr<FileDescription> description) {
  fk::synchronization::ScopedLockIRQ lock_task(lock);
  for (size_t i = 0; i < resources.files.descriptors.size(); ++i) {
    if (!resources.files.descriptors[i]) {
      resources.files.descriptors[i] = description;

      return static_cast<int>(i);
    }
  }

  // No empty slots? Add to the end.
  if (resources.files.descriptors.is_full()) {
    fk::algorithms::kwarn("TASK", "Task %lu: FD table full!", control.identity.id.value());
    return -24; // -EMFILE
  }

  int fd = static_cast<int>(resources.files.descriptors.size());
  resources.files.descriptors.push_back(description);

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
      (void)type; // Suppress unused variable warning
    }
  }
}

int Task::dup_file_descriptor(int old_fd, [[maybe_unused]] bool cloexec, int min_fd) {
  fk::synchronization::ScopedLockIRQ lock_task(lock);
  auto description = (old_fd < 0 || old_fd >= static_cast<int>(resources.files.descriptors.size()))
                         ? nullptr
                         : resources.files.descriptors[old_fd];
  if (!description) {
    fk::algorithms::kwarn("TASK", "dup_file_descriptor: Source FD %d not found", old_fd);
    return -1; // EBADF
  }

  for (int i = min_fd; i < static_cast<int>(resources.files.descriptors.capacity()); ++i) {
    if (i >= static_cast<int>(resources.files.descriptors.size())) {
      while (static_cast<int>(resources.files.descriptors.size()) < i) {
        resources.files.descriptors.push_back({});
      }
      resources.files.descriptors.push_back(description);
      int new_fd = resources.files.descriptors.size() - 1;
      return new_fd;
    }

    if (!resources.files.descriptors[i]) {
      resources.files.descriptors[i] = description;
      return i;
    }
  }

  return -1; // EMFILE
}

fk::RefPtr<FileDescription> Task::get_file_descriptor(int fd) {
  if (fd < 0 || fd >= static_cast<int>(resources.files.descriptors.size())) {
    return {};
  }
  return resources.files.descriptors[fd];
}

void Task::close_file_descriptor(int fd) {
  fk::synchronization::ScopedLockIRQ lock_task(lock);
  if (fd < 0 || fd >= static_cast<int>(resources.files.descriptors.size())) {
    return;
  }
  resources.files.descriptors[fd] = nullptr;
}