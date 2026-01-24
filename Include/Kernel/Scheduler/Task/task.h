#pragma once

#include <Kernel/Fs/Vfs/file_description.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Text/fixed_string.h>

#include <Kernel/Hardware/Cpu/cpu_context.h>
#include <Kernel/Scheduler/Task/task_state.h>

// TODO: Change the userID to a proper type based on UUID
using TaskId = uint64_t;

namespace fkernel {
namespace ipc {
class CSpace;
}
} // namespace fkernel

struct Task {
  TaskId id;
  TaskId ppid{0};
  fk::text::fixed_string<64> name;

  TaskState state;
  CpuContext context;
  uint64_t stack_pointer;
  uint64_t kernel_stack_top;

  uint64_t user_rsp{0};
  uint64_t saved_rip{0};
  uint64_t saved_rflags{0};

  uintptr_t cr3{0};

  uint8_t priority; // TODO: Change to enum class for priority levels
  uint64_t cpu_affinity;

  bool is_a_kernel_task{true};
  bool terminated{false};
  int exit_status{0};

  uint64_t time_slice_ticks{0};
  uint64_t wake_up_time_ticks{0};

  fk::text::fixed_string<256> cwd{"/"};

  uintptr_t clear_child_tid{0};

  struct MemoryRegions {
    uintptr_t heap_start{0};
    uintptr_t heap_break{0};
    uintptr_t mmap_start{0};
    uintptr_t mmap_end{0};
  } memory_regions{};

  // IPC and Signals
  struct SignalState {
    uint64_t mask{0};
    uint64_t pending{0};
    uintptr_t trampoline{0};
  } signal_state{};

  uintptr_t ipc_buffer_vaddr{0};
  fkernel::ipc::CSpace *cspace{nullptr};

  fk::containers::IntrusiveListNode<Task> run_node;
  fk::containers::IntrusiveListNode<Task> wait_node;
  fk::containers::IntrusiveListNode<Task> sleep_node;

  fk::containers::static_vector<fk::RefPtr<FileDescription>, 32>
      file_descriptors;

  int add_file_descriptor(fk::RefPtr<FileDescription> description);
  int dup_file_descriptor(int old_fd, bool cloexec = false, int min_fd = 0);
  fk::RefPtr<FileDescription> get_file_descriptor(int fd);
  void close_file_descriptor(int fd);

  void print_info() const;
};

Task create_a_new_task(TaskId id, const fk::text::fixed_string<64> &name,
                       void (*entry)(), bool kernel_task, uint8_t priority,
                       uint64_t cpu_affinity, uint64_t arg1 = 0,
                       uint64_t arg2 = 0);
