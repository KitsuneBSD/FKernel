#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/global_endpoint_manager.h>
#include <Kernel/Ipc/notification.h>

extern "C" void task_trampoline();

Task create_a_new_task(TaskId id, const fk::text::fixed_string<64> &name,
                       void (*entry)(), bool kernel_task, uint8_t priority,
                       uint64_t cpu_affinity, uint64_t arg1, uint64_t arg2) {
  const size_t STACK_SIZE = 16 * fk::types::KiB;

  void *stack_mem = kmalloc(STACK_SIZE);
  uint64_t stack_top = reinterpret_cast<uint64_t>(stack_mem) + STACK_SIZE;

  // ... (context setup code) ...
  uint64_t *stack = reinterpret_cast<uint64_t *>(stack_top);
  *(--stack) = reinterpret_cast<uint64_t>(task_trampoline);
  *(--stack) = 0;                                 // rbx
  *(--stack) = 0;                                 // rbp
  *(--stack) = arg1;                              // r12
  *(--stack) = arg2;                              // r13
  *(--stack) = reinterpret_cast<uint64_t>(entry); // r14
  *(--stack) = 0;                                 // r15

  // Initialize IPC CSpace
  auto *cspace = new fkernel::ipc::CSpace();
  auto *signal_notification = new fkernel::ipc::Notification();
  cspace->install(fkernel::ipc::Capability(
      signal_notification, fkernel::ipc::CapabilityType::Notification));

  // Also register in global manager for sys_kill access
  fkernel::ipc::GlobalEndpointManager::the().register_notification(
      id, signal_notification);

  Task task{
      .id = id,
      .name = name,
      .state = TaskState::Ready,
      .context = GetContextForNewTask(reinterpret_cast<uint64_t>(stack),
                                      kernel_task, arg1, arg2),
      .stack_pointer = reinterpret_cast<uint64_t>(stack),
      .kernel_stack_top = stack_top,
      .cr3 = read_on_cr3(),
      .priority = priority,
      .cpu_affinity = cpu_affinity,
      .is_a_kernel_task = kernel_task,
      .cspace = cspace,
      .run_node = {},
      .wait_node = {},
      .sleep_node = {},
      .file_descriptors = {},
  };

  return task;
}

void Task::print_info() const {
  fk::algorithms::kdebug("TASK INFO",
                         "Task ID: %lu, Name: %s, State: %u, Priority: %u, CPU "
                         "Affinity: %lu, Is Kernel Task: %s",
                         id, name.c_str(), static_cast<uint8_t>(state),
                         priority, cpu_affinity,
                         is_a_kernel_task ? "Yes" : "No");
}

int Task::add_file_descriptor(
    fk::RefPtr<FileDescription> description) {
  for (size_t i = 0; i < file_descriptors.size(); ++i) {
    if (!file_descriptors[i]) {
      file_descriptors[i] = description;
      return static_cast<int>(i);
    }
  }

  // No empty slots? Add to the end.
  int fd = static_cast<int>(file_descriptors.size());
  file_descriptors.push_back(description);
  return fd;
}

int Task::dup_file_descriptor(int old_fd, [[maybe_unused]] bool cloexec, int min_fd) {
  auto description = get_file_descriptor(old_fd);
  if (!description) {
    fk::algorithms::kwarn("TASK", "dup_file_descriptor: Source FD %d not found", old_fd);
    return -1; // EBADF
  }

  // Find first available FD >= min_fd
  for (int i = min_fd; i < static_cast<int>(file_descriptors.capacity()); ++i) {
    if (i >= static_cast<int>(file_descriptors.size())) {
      // Pad with NULLs to fill the gap up to i
      while (static_cast<int>(file_descriptors.size()) < i) {
          file_descriptors.push_back({});
      }
      // Fix: Take a local copy before push_back because if the vector
      // reallocates, the reference to file_descriptors[old_fd] becomes invalid.
      // The `description` variable is already a local copy, so we use that.
      file_descriptors.push_back(description);
      int new_fd = file_descriptors.size() - 1;

      fk::algorithms::klog("TASK", "dup_file_descriptor: Duplicated FD %d -> %d (new slot %d)", old_fd, new_fd, i);
      return new_fd;
    }

    if (!file_descriptors[i]) {
      file_descriptors[i] = description;
      fk::algorithms::klog("TASK", "dup_file_descriptor: Duplicated FD %d -> %d (existing empty slot)", old_fd, i);
      return i;
    }
  }

  return -1; // EMFILE
}

fk::RefPtr<FileDescription> Task::get_file_descriptor(int fd) {
  if (fd < 0 || fd >= static_cast<int>(file_descriptors.size())) {
    return {};
  }
  return file_descriptors[fd];
}

void Task::close_file_descriptor(int fd) {
  if (fd < 0 || fd >= static_cast<int>(file_descriptors.size())) {
    return;
  }
  file_descriptors[fd] = nullptr;
}
