#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Task/task.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/global_endpoint_manager.h>
#include <Kernel/Ipc/notification.h>

extern "C" void task_trampoline();

Task create_a_new_task(fk::ProcessId id, const fk::text::fixed_string<64> &name,
                       void (*entry)(), bool kernel_task, uint8_t priority,
                       uint64_t cpu_affinity, uint64_t arg1, uint64_t arg2) {
  const size_t STACK_SIZE = 16 * fk::types::KiB;

  void *stack_mem = kmalloc(STACK_SIZE);
  uint64_t stack_top = reinterpret_cast<uint64_t>(stack_mem) + STACK_SIZE;

  // Setup initial stack for switch_context
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
      id.value(), signal_notification);

  Task task{
      .identity = { .id = id, .ppid = fk::ProcessId(), .name = name },
      .memory = { .cr3 = read_on_cr3(), .regions = {} },
      .files = { .cwd = "/", .descriptors = {} },
      .ipc = { .cspace = cspace, .signals = {} },
      .state = TaskState::Ready,
      .context = GetContextForNewTask(reinterpret_cast<uint64_t>(stack),
                                      kernel_task, arg1, arg2),
      .stack_pointer = reinterpret_cast<uint64_t>(stack),
      .kernel_stack_top = stack_top,
      .user_rsp = 0,
      .saved_rip = 0,
      .saved_rflags = 0,
      .priority = priority,
      .cpu_affinity = cpu_affinity,
      .is_a_kernel_task = kernel_task,
      .terminated = false,
      .exit_status = 0,
      .time_slice_ticks = 0,
      .wake_up_time_ticks = 0,
      .clear_child_tid = 0,
      .vfork_waiting = false,
      .vfork_parent_id = fk::ProcessId(),
      .fs_base = 0,
      .gs_base = 0,
      .is_vfork_sharing_address_space = false,
      .run_node = {},
      .wait_node = {},
      .sleep_node = {},
  };

  return task;
}

void Task::print_info() const {
  fk::algorithms::kdebug("TASK INFO",
                         "Task ID: %lu, Name: %s, State: %u, Priority: %u, CPU "
                         "Affinity: %lu, Is Kernel Task: %s",
                         identity.id.value(), identity.name.c_str(), static_cast<uint8_t>(state),
                         priority, cpu_affinity,
                         is_a_kernel_task ? "Yes" : "No");
}

int Task::add_file_descriptor(
    fk::RefPtr<FileDescription> description) {
  for (size_t i = 0; i < files.descriptors.size(); ++i) {
    if (!files.descriptors[i]) {
      files.descriptors[i] = description;
      fk::algorithms::klog("TASK", "Task %lu: Added FD %zu -> %s", identity.id.value(), i, description->node()->name().c_str());
      return static_cast<int>(i);
    }
  }

  // No empty slots? Add to the end.
  if (files.descriptors.is_full()) {
    fk::algorithms::kwarn("TASK", "Task %lu: FD table full!", identity.id.value());
    return -24; // -EMFILE
  }

  int fd = static_cast<int>(files.descriptors.size());
  files.descriptors.push_back(description);
  fk::algorithms::klog("TASK", "Task %lu: Added FD %d -> %s", identity.id.value(), fd, description->node()->name().c_str());
  return fd;
}

void Task::dump_file_descriptors() const {
    fk::algorithms::klog("TASK", "FD table for Task %lu (%s):", identity.id.value(), identity.name.c_str());
    for (size_t i = 0; i < files.descriptors.size(); ++i) {
        if (files.descriptors[i]) {
            auto node = files.descriptors[i]->node();
            const char* type = "unknown";
            if (node->is_character_device()) type = "char";
            else if (node->is_block_device()) type = "block";
            else if (node->is_directory()) type = "dir";
            else type = "file";

            fk::algorithms::klog("TASK", "  [%zu] -> %s (type: %s, ptr: %p)", 
                                 i, node->name().c_str(), type, node.get());
        }
    }
}

int Task::dup_file_descriptor(int old_fd, [[maybe_unused]] bool cloexec, int min_fd) {
  auto description = get_file_descriptor(old_fd);
  if (!description) {
    fk::algorithms::kwarn("TASK", "dup_file_descriptor: Source FD %d not found", old_fd);
    return -1; // EBADF
  }

  // Find first available FD >= min_fd
  for (int i = min_fd; i < static_cast<int>(files.descriptors.capacity()); ++i) {
    if (i >= static_cast<int>(files.descriptors.size())) {
      // Pad with NULLs to fill the gap up to i
      while (static_cast<int>(files.descriptors.size()) < i) {
          files.descriptors.push_back({});
      }
      files.descriptors.push_back(description);
      int new_fd = files.descriptors.size() - 1;

      fk::algorithms::klog("TASK", "dup_file_descriptor: Duplicated FD %d -> %d (new slot %d)", old_fd, new_fd, i);
      return new_fd;
    }

    if (!files.descriptors[i]) {
      files.descriptors[i] = description;
      fk::algorithms::klog("TASK", "dup_file_descriptor: Duplicated FD %d -> %d (existing empty slot)", old_fd, i);
      return i;
    }
  }

  return -1; // EMFILE
}

fk::RefPtr<FileDescription> Task::get_file_descriptor(int fd) {
  if (fd < 0 || fd >= static_cast<int>(files.descriptors.size())) {
    return {};
  }
  return files.descriptors[fd];
}

void Task::close_file_descriptor(int fd) {
  if (fd < 0 || fd >= static_cast<int>(files.descriptors.size())) {
    return;
  }
  files.descriptors[fd] = nullptr;
}
