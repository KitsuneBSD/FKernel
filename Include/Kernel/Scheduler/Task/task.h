#pragma once

#include <Kernel/Fs/Vfs/file_description.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/mount_namespace.h>
#include <LibFK/Container/intrusive_list.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Text/fixed_string.h>
#include <LibFK/Types/process_id.h>
#include <LibFK/Types/virtual_address.h>
#include <LibFK/Synchronization/spinlock.h>
#include <Kernel/Hardware/Cpu/cpu_context.h>
#include <Kernel/Scheduler/Task/task_state.h>
#include <Kernel/Scheduler/qos.h>
#include <Kernel/Posix/signal_defs.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <LibFK/Container/vector.h>

namespace fkernel::ipc {
    class CSpace;
    class Notification;
}

namespace fkernel {
    class SignalFdNode;
}

namespace fkernel::scheduler {
    struct Turnstile;
}

/**
 * @brief Task Identity information
 */
struct TaskIdentity {
    fk::ProcessId id;
    fk::ProcessId ppid;
    fk::ProcessId pgid;
    fk::ProcessId sid;
    fk::text::fixed_string<64> name;
    uint32_t umask{0022};
    bool is_session_leader{false};
    uint32_t uid{0};
    uint32_t gid{0};
    uint32_t euid{0};
    uint32_t egid{0};
    uint32_t suid{0};
    uint32_t sgid{0};
    uint32_t supplementary_gids[16]{};
    uint32_t ngroups{0};
};

/**
 * @brief Task Memory regions and address space
 */
struct TaskMemory {
    uintptr_t cr3{0};
    uintptr_t prev_cr3{0};
    struct {
        uintptr_t heap_start{0};
        uintptr_t heap_break{0};
        uintptr_t mmap_start{0};
        uintptr_t mmap_end{0};
        fk::containers::Vector<::fkernel::MemoryRegion> list;
    } regions{};
};

static constexpr size_t MAX_OPEN_FILES = 1024;

/**
 * @brief Task Files and filesystem state
 */
struct TaskFiles {
    fk::text::fixed_string<256> cwd{"/"};
    fk::RefPtr<fkernel::Dentry> root;
    fk::RefPtr<fkernel::MountNamespace> mount_ns;
    fk::containers::static_vector<fk::RefPtr<FileDescription>, MAX_OPEN_FILES> descriptors;
    // CSpace cap_handles deferred — heap corruption investigation needed
};

/**
 * @brief Task IPC and signal state
 */
struct TaskIpc {
    ::fkernel::ipc::CSpace *cspace{nullptr};
    struct {
        uint32_t pending{0};
        uint32_t blocked{0};
        sigaction actions[NSIG];
        uintptr_t trampoline{0};
    } signals{};
    struct {
        void* ss_sp{nullptr};
        size_t ss_size{0};
        int ss_flags{0}; // SS_DISABLE=2, SS_ONSTACK=0
    } alt_stack{};
    ::fkernel::ipc::Notification *signal_notification{nullptr};
    ::fkernel::SignalFdNode *signal_fd{nullptr};
    ::fkernel::scheduler::Turnstile *pending_turnstile{nullptr};
    ::fkernel::scheduler::Turnstile *active_turnstile{nullptr};
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
    alignas(16) uint8_t fx_state[512]{};
    uint8_t* xsave_area{nullptr};
    size_t   xsave_size{0};
};

inline void* get_fpu_save_area(TaskContext& ctx) {
    if (ctx.xsave_area)
        return ctx.xsave_area;
    return ctx.fx_state;
}

/**
 * @brief Task Scheduling and Lifecycle state
 */
struct TaskLifecycle {
    TaskState state;
    uint8_t priority;
    int8_t nice{0};
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
    bool in_wait_queue{false};
    struct {
        uint64_t remaining_ticks{0};   // current timer value in ticks
        uint64_t interval_ticks{0};    // reload value (0 = one-shot)
        int signo{14};                 // signal to deliver (SIGALRM=14)
        bool active{false};
    } itimers[3]{};  // 0=ITIMER_REAL, 1=ITIMER_VIRTUAL, 2=ITIMER_PROF

    fkernel::scheduler::QoSClass qos{fkernel::scheduler::QoSClass::Default};
    fkernel::scheduler::SchedulingPolicy policy{fkernel::scheduler::SchedulingPolicy::Normal};
    uint8_t base_priority{0};
    uint8_t mlfq_level{0};
    uint64_t cpu_time_consumed{0};
    uint64_t allotment_ticks{0};
    bool boosted{false};
    fkernel::scheduler::QoSClass original_qos{fkernel::scheduler::QoSClass::Default};
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
    fk::containers::IntrusiveListNode<Task> recv_wait_node;
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
    bool has_pending_signals() const { return (resources.ipc.signals.pending & ~resources.ipc.signals.blocked) != 0; }

    int add_file_descriptor(fk::RefPtr<FileDescription> description);
    int dup_file_descriptor(int old_fd, bool cloexec = false, int min_fd = 0);
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

Task create_a_new_task(fk::ProcessId id, const fk::text::fixed_string<64> &name,
                       void (*entry)(), bool kernel_task, uint8_t priority,
                       uint64_t cpu_affinity, uint64_t arg1 = 0,
                       uint64_t arg2 = 0,
                       fkernel::scheduler::QoSClass qos = fkernel::scheduler::QoSClass::Default);