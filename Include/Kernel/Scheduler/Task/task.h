#pragma once

#include <Kernel/Scheduler/Task/task_identity.h>
#include <Kernel/Scheduler/Task/task_lifecycle.h>
#include <Kernel/Scheduler/Task/task_control.h>
#include <Kernel/Scheduler/Task/task_memory.h>
#include <Kernel/Scheduler/Task/task_files.h>
#include <Kernel/Scheduler/Task/task_ipc.h>
#include <Kernel/Scheduler/Task/task_context.h>
#include <Kernel/Scheduler/Task/task_nodes.h>
#include <Kernel/Scheduler/Task/task_resources.h>
#include <Kernel/Scheduler/Task/task_initialize.h>
#include <LibFK/Container/Sequence/intrusive_list.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <Kernel/Fs/Vfs/Core/file_description.h>

/**
 * @brief Central Task object refactored for Object Calisthenics (Rule 8)
 */
struct Task {
  static constexpr uint32_t MAGIC = 0x5441534B; // "TASK"
  uint32_t magic{MAGIC};

  // Intrusive nodes MUST be direct members for pointer-to-member templates
  fk::containers::IntrusiveListNode<Task> run_node;
  fk::containers::IntrusiveListNode<Task> wait_node;
  fk::containers::IntrusiveListNode<Task> recv_wait_node;
  fk::containers::IntrusiveListNode<Task> sleep_node;
  fk::containers::IntrusiveListNode<Task> zombie_node;

  mutable fk::synchronization::Spinlock lock;

  TaskControl control;
  TaskResources resources;

  // Helper methods
  bool is_valid() const { return magic == MAGIC; }
  void invalidate() { magic = 0; }

  CpuContext& registers() { return resources.context.registers; }
  const CpuContext& registers() const { return resources.context.registers; }

  TaskState& state() { return control.lifecycle.state; }
  TaskState state() const { return control.lifecycle.state; }

  TaskMemory& memory() { return resources.memory; }
  const TaskMemory& memory() const { return resources.memory; }

  TaskFiles& files() { return resources.files; }
  const TaskFiles& files() const { return resources.files; }

  TaskIpc& ipc() { return resources.ipc; }
  const TaskIpc& ipc() const { return resources.ipc; }

  bool is_a_kernel_task() const { return control.lifecycle.is_a_kernel_task; }
  bool has_pending_signals() const {
    return (resources.ipc.signals.pending &
            ~(resources.ipc.signals.blocked & ~resources.ipc.signals.forced_pending)) != 0ULL;
  }

  int add_file_descriptor(fk::RefPtr<FileDescription> description);
  int dup_file_descriptor(int old_fd, bool cloexec = false, int min_fd = 0);
  int install_at(int newfd, fk::RefPtr<FileDescription> desc);
  fk::RefPtr<FileDescription> get_file_descriptor(int fd);
  void close_file_descriptor(int fd);

  void set_heap_regions(uintptr_t start, uintptr_t break_addr);
  void set_mmap_regions(uintptr_t start, uintptr_t end);
  bool is_address_in_allowed_regions(uintptr_t address) const;

  void dump_file_descriptors() const;
  void print_info() const;
  void release_all_file_locks();

  void destroy();

  // Refcount for safe cross-lock access via fk::RefPtr<Task>.
  // Starts at 1 (scheduler owns). ref()/unref() must be called
  // while a scheduler lock is held to close the UAF window.
  uint32_t m_ref_count{1};
  void ref()   { __sync_fetch_and_add(&m_ref_count, 1u); }
  void unref() { if (__sync_fetch_and_sub(&m_ref_count, 1u) == 1u) delete this; }
};
