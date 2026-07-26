#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/global_endpoint_manager.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

// Clone flags (x86-64 Linux ABI)
static constexpr uint64_t CLONE_VM       = 0x00000100;
static constexpr uint64_t CLONE_FILES    = 0x00000400;
static constexpr uint64_t CLONE_SIGHAND  = 0x00000800;
[[maybe_unused]] static constexpr uint64_t CLONE_THREAD   = 0x00010000;
static constexpr uint64_t CLONE_SETTLS   = 0x00080000;
static constexpr uint64_t CLONE_PARENT_SETTID  = 0x00100000;
static constexpr uint64_t CLONE_CHILD_CLEARTID = 0x00200000;
static constexpr uint64_t CLONE_CHILD_SETTID   = 0x01000000;

extern "C" {
void fork_child_trampoline();
}

extern "C" uint64_t sys_clone(uint64_t flags, uint64_t child_stack,
                               uint64_t parent_tidptr, uint64_t child_tidptr,
                               uint64_t tls, uint64_t,
                               PtRegs* regs) {
    auto* parent = SchedulerManager::the().current();
    if (!parent) return fkernel::return_error(fk::core::Error::PermissionDenied);

    Task* child = new Task();
    if (!child) return fkernel::return_error(fk::core::Error::OutOfMemory);

    child->control.identity.id   = SchedulerManager::the().generate_pid();
    child->control.identity.ppid = parent->control.identity.id;
    child->control.identity.name = parent->control.identity.name;
    child->control.lifecycle.state           = TaskState::Ready;
    child->control.lifecycle.priority        = parent->control.lifecycle.priority;
    child->control.lifecycle.cpu_affinity    = parent->control.lifecycle.cpu_affinity;
    child->control.lifecycle.is_a_kernel_task = false;
    child->control.lifecycle.clear_child_tid  = 0;
    child->control.lifecycle.qos              = parent->control.lifecycle.qos;
    child->control.lifecycle.policy           = parent->control.lifecycle.policy;
    child->control.lifecycle.base_priority    = parent->control.lifecycle.base_priority;
    child->control.lifecycle.mlfq_level       = parent->control.lifecycle.mlfq_level;
    child->control.lifecycle.cpu_time_consumed = 0;
    child->control.lifecycle.allotment_ticks  = parent->control.lifecycle.allotment_ticks;
    child->control.lifecycle.boosted          = false;
    child->control.lifecycle.original_qos     = parent->control.lifecycle.qos;
    child->resources.files.cwd = parent->resources.files.cwd;

    child->resources.ipc.cspace = new fkernel::ipc::CSpace();
    if (!child->resources.ipc.cspace) { delete child; return (uint64_t)-12; }

    auto* sig_notification = new fkernel::ipc::Notification();
    if (!sig_notification) { delete child->resources.ipc.cspace; delete child; return (uint64_t)-12; }
    child->resources.ipc.signal_notification = sig_notification;
    child->resources.ipc.cspace->install(
        fkernel::ipc::Capability(sig_notification, fkernel::ipc::CapabilityType::Notification));
    fkernel::ipc::GlobalEndpointManager::the().register_notification(
        child->control.identity.id.value(), sig_notification);

    child->resources.ipc.signals.blocked   = parent->resources.ipc.signals.blocked;
    child->resources.ipc.signals.pending   = 0;
    child->resources.ipc.signals.trampoline = parent->resources.ipc.signals.trampoline;
    if (flags & CLONE_SIGHAND) {
        for (int i = 0; i < NSIG; ++i)
            child->resources.ipc.signals.actions[i] = parent->resources.ipc.signals.actions[i];
    }

    if (flags & CLONE_FILES) {
        for (size_t i = 0; i < parent->resources.files.descriptors.size(); ++i)
            child->resources.files.descriptors.push_back(parent->resources.files.descriptors[i]);
    }

    const size_t STACK_SIZE = 16 * 1024;
    void* child_kstack = kmalloc(STACK_SIZE);
    if (!child_kstack) { delete child; return fkernel::return_error(fk::core::Error::OutOfMemory); }
    child->resources.context.kernel_stack_top =
        reinterpret_cast<uint64_t>(child_kstack) + STACK_SIZE;

    if (flags & CLONE_VM) {
        child->resources.memory.cr3 = parent->resources.memory.cr3;
        child->control.lifecycle.is_vfork_sharing_address_space = true;
    } else {
        fk::memory::copy(child_kstack,
               reinterpret_cast<void*>(parent->resources.context.kernel_stack_top - STACK_SIZE),
               STACK_SIZE);
        child->resources.memory.cr3 =
            VirtualMemoryManager::the().clone_address_space(parent->resources.memory.cr3);
    }

    child->resources.memory.regions.heap_start = parent->resources.memory.regions.heap_start;
    child->resources.memory.regions.heap_break = parent->resources.memory.regions.heap_break;
    child->resources.memory.regions.mmap_start = parent->resources.memory.regions.mmap_start;
    child->resources.memory.regions.mmap_end   = parent->resources.memory.regions.mmap_end;
    for (size_t i = 0; i < parent->resources.memory.regions.list.size(); ++i)
        child->resources.memory.regions.list.push_back(parent->resources.memory.regions.list[i]);

    child->resources.context.saved_rip    = regs->rip;
    child->resources.context.saved_rflags = regs->rflags;
    child->resources.context.gs_base = CPU::the().read_msr(MSR_KERNEL_GS_BASE);

    if (flags & CLONE_SETTLS) {
        child->resources.context.fs_base = tls;
    } else {
        child->resources.context.fs_base = CPU::the().read_msr(MSR_FS_BASE);
    }

    if (flags & CLONE_CHILD_CLEARTID)
        child->control.lifecycle.clear_child_tid = child_tidptr;

    if (flags & CLONE_VM) {
        // Thread path: use provided child_stack for user RSP
        uint64_t thread_rsp = child_stack ? child_stack : regs->rsp;
        child->resources.context.user_rsp = thread_rsp;

        uintptr_t ksp = child->resources.context.kernel_stack_top;
        PtRegs* child_regs = reinterpret_cast<PtRegs*>(ksp - sizeof(PtRegs));
        *child_regs = *regs;
        child_regs->rax = 0;
        child_regs->rsp = thread_rsp;

        uint64_t* ctx = reinterpret_cast<uint64_t*>(child_regs);
        *(--ctx) = reinterpret_cast<uint64_t>(fork_child_trampoline);
        *(--ctx) = regs->rbx;
        *(--ctx) = regs->rbp;
        *(--ctx) = regs->r12;
        *(--ctx) = regs->r13;
        *(--ctx) = regs->r14;
        *(--ctx) = regs->r15;
        child->resources.context.stack_pointer = reinterpret_cast<uint64_t>(ctx);
    } else {
        // Fork path: inherit parent kernel stack layout
        child->resources.context.user_rsp = regs->rsp;
        uintptr_t parent_sp = reinterpret_cast<uintptr_t>(regs);
        uintptr_t offset = parent->resources.context.kernel_stack_top - parent_sp;
        uintptr_t child_sp = child->resources.context.kernel_stack_top - offset;

        PtRegs* child_regs = reinterpret_cast<PtRegs*>(child_sp);
        child_regs->rax = 0;

        uint64_t* ctx = reinterpret_cast<uint64_t*>(child_sp);
        *(--ctx) = reinterpret_cast<uint64_t>(fork_child_trampoline);
        *(--ctx) = regs->rbx;
        *(--ctx) = regs->rbp;
        *(--ctx) = regs->r12;
        *(--ctx) = regs->r13;
        *(--ctx) = regs->r14;
        *(--ctx) = regs->r15;
        child->resources.context.stack_pointer = reinterpret_cast<uint64_t>(ctx);
    }

    if (flags & CLONE_PARENT_SETTID) {
        uint32_t child_pid = static_cast<uint32_t>(child->control.identity.id.value());
        fkernel::memory::copy_to_user(
            reinterpret_cast<void*>(parent_tidptr), &child_pid, sizeof(child_pid));
    }

    if (flags & CLONE_CHILD_SETTID) {
        uint32_t child_pid = static_cast<uint32_t>(child->control.identity.id.value());
        fkernel::memory::copy_to_user(
            reinterpret_cast<void*>(child_tidptr), &child_pid, sizeof(child_pid));
    }

    SchedulerManager::the().add_task(child);

    fk::algorithms::klog("CLONE", "clone: parent=%lu child=%lu flags=%lx",
                          parent->control.identity.id.value(),
                          child->control.identity.id.value(), flags);

    return child->control.identity.id.value();
}
