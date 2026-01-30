#pragma once

#include <Kernel/Fs/Vfs/file_description.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Text/fixed_string.h>
#include <LibFK/Types/processId.h>
#include <LibFK/Types/virtualAddress.h>
#include <LibFK/Synchronization/spinlock.h>
#include <Kernel/Hardware/Cpu/cpu_context.h>
#include <Kernel/Scheduler/Task/task_state.h>

namespace fkernel::ipc {
    class CSpace;
}

/**
 * @brief Task Identity information
 */
struct TaskIdentity {
    fk::ProcessId id;
    fk::ProcessId ppid;
    fk::text::fixed_string<64> name;
};

/**
 * @brief Task Memory regions and address space
 */
struct TaskMemory {
    uintptr_t cr3{0};
    struct {
        uintptr_t heap_start{0};
        uintptr_t heap_break{0};
        uintptr_t mmap_start{0};
        uintptr_t mmap_end{0};
    } regions{};
};

/**
 * @brief Task Files and filesystem state
 */
struct TaskFiles {
    fk::text::fixed_string<256> cwd{"/"};
    fk::containers::static_vector<fk::RefPtr<FileDescription>, 128> descriptors;
};

/**
 * @brief Task IPC and signal state
 */
struct TaskIpc {
    ::fkernel::ipc::CSpace *cspace{nullptr};
    struct {
        uint64_t mask{0};
        uint64_t pending{0};
        uintptr_t trampoline{0};
    } signals{};
};

/**
 * @brief Task Execution context (CPU registers, stacks)
 */
struct TaskContext {
    CpuContext registers;
    uint64_t stack_pointer;
    uint64_t kernel_stack_top;
    uint64_t user_rsp{0};
    uint64_t saved_rip{0};
    uint64_t saved_rflags{0};
    uint64_t fs_base{0};
    uint64_t gs_base{0};
};

/**
 * @brief Task Scheduling and Lifecycle state
 */
struct TaskLifecycle {
    TaskState state;
    uint8_t priority;
    uint64_t cpu_affinity;
    uint64_t time_slice_ticks{0};
    uint64_t wake_up_time_ticks{0};
    bool is_a_kernel_task{true};
    bool terminated{false};
    int exit_status{0};
    uintptr_t clear_child_tid{0};
    bool vfork_waiting{false};
    fk::ProcessId vfork_parent_id;
    bool is_vfork_sharing_address_space{false};
};

/**
 * @brief Intrusive nodes for scheduler queues
 */
struct TaskNodes {
    fk::containers::IntrusiveListNode<struct Task> run;
    fk::containers::IntrusiveListNode<struct Task> wait;
    fk::containers::IntrusiveListNode<struct Task> sleep;
    fk::containers::IntrusiveListNode<struct Task> zombie;
};

/**
 * @brief Central Task object refactored for Object Calisthenics (Rule 8)
 */
struct Task {
    static constexpr uint32_t MAGIC = 0x5441534B; // "TASK"
    uint32_t magic{MAGIC};

    // Intrusive nodes MUST be direct members for pointer-to-member templates
    fk::containers::IntrusiveListNode<Task> run_node;
    fk::containers::IntrusiveListNode<Task> wait_node;
    fk::containers::IntrusiveListNode<Task> sleep_node;
    fk::containers::IntrusiveListNode<Task> zombie_node;

    mutable fk::synchronization::Spinlock lock;

    struct Control {
        TaskIdentity identity;
        TaskLifecycle lifecycle;
    } control;

    struct Resources {
        TaskMemory memory;
        TaskFiles files;
        TaskIpc ipc;
        TaskContext context;
    } resources;

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