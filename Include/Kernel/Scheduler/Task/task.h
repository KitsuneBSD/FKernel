#pragma once

#include <Kernel/Fs/Vfs/file_description.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Text/fixed_string.h>

#include <Kernel/Hardware/Cpu/cpu_context.h>
#include <Kernel/Scheduler/Task/task_state.h>
#include <LibFK/Types/processId.h>
namespace fkernel {
namespace ipc {
class CSpace;
}
}

struct TaskIdentity {
    fk::ProcessId id;
    fk::ProcessId ppid;
    fk::text::fixed_string<64> name;
};

struct TaskMemory {
    uintptr_t cr3{0};
    struct Regions {
        uintptr_t heap_start{0};
        uintptr_t heap_break{0};
        uintptr_t mmap_start{0};
        uintptr_t mmap_end{0};
    } regions{};
};

struct TaskFiles {
    fk::text::fixed_string<256> cwd{"/"};
    fk::containers::static_vector<fk::RefPtr<FileDescription>, 128> descriptors;
};

struct TaskIpc {
    ::fkernel::ipc::CSpace *cspace{nullptr};
    struct SignalState {
        uint64_t mask{0};
        uint64_t pending{0};
        uintptr_t trampoline{0};
    } signals{};
};

struct Task {
  TaskIdentity identity;
  TaskMemory memory;
  TaskFiles files;
  TaskIpc ipc;

  TaskState state;
  CpuContext context;
  uint64_t stack_pointer;
  uint64_t kernel_stack_top;

  uint64_t user_rsp{0};
  uint64_t saved_rip{0};
  uint64_t saved_rflags{0};

  uint8_t priority; 
  uint64_t cpu_affinity;

  bool is_a_kernel_task{true};
  bool terminated{false};
  int exit_status{0};

  uint64_t time_slice_ticks{0};
  uint64_t wake_up_time_ticks{0};

  uintptr_t clear_child_tid{0};

  // vfork tracking
  bool vfork_waiting{false};
  fk::ProcessId vfork_parent_id;

  // x86_64 segment bases
  uint64_t fs_base{0};
  uint64_t gs_base{0};

  // vfork address space sharing
  bool is_vfork_sharing_address_space{false};

  fk::containers::IntrusiveListNode<Task> run_node;
  fk::containers::IntrusiveListNode<Task> wait_node;
  fk::containers::IntrusiveListNode<Task> sleep_node;

  int add_file_descriptor(fk::RefPtr<FileDescription> description);
  int dup_file_descriptor(int old_fd, bool cloexec = false, int min_fd = 0);
  fk::RefPtr<FileDescription> get_file_descriptor(int fd);
  void close_file_descriptor(int fd);

  void dump_file_descriptors() const;
  void print_info() const;
};

Task create_a_new_task(fk::ProcessId id, const fk::text::fixed_string<64> &name,
                       void (*entry)(), bool kernel_task, uint8_t priority,
                       uint64_t cpu_affinity, uint64_t arg1 = 0,
                       uint64_t arg2 = 0);
