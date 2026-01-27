#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include "Kernel/Hardware/Cpu/cpu_block.h"
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

extern "C" {
void fork_child_trampoline();
}

extern "C" uint64_t sys_vfork([[maybe_unused]] uint64_t arg1, [[maybe_unused]] uint64_t arg2,
                             [[maybe_unused]] uint64_t arg3, [[maybe_unused]] uint64_t arg4,
                             [[maybe_unused]] uint64_t arg5, [[maybe_unused]] uint64_t arg6,
                             PtRegs* regs) {
    auto *parent = SchedulerManager::the().current();
    if (!parent) return -1;

    fk::algorithms::klog("SYSCALL", "sys_vfork: Task %lu forking...", parent->id);

    // 1. Create child task
    Task *child = new Task();
    if (!child) return -1;

    // 2. Clone metadata
    child->id = SchedulerManager::the().generate_pid();
    child->ppid = parent->id;
    child->name = parent->name;
    child->state = TaskState::Ready;
    child->priority = parent->priority;
    child->cpu_affinity = parent->cpu_affinity;
    child->is_a_kernel_task = parent->is_a_kernel_task;
    child->cwd = parent->cwd;
    child->vfork_parent_id = parent->id; // Mark as vfork child

    // 3. Clone file descriptors
    for (size_t i = 0; i < parent->file_descriptors.size(); ++i) {
        child->file_descriptors.push_back(parent->file_descriptors[i]);
    }

    // 4. Setup Kernel Stack
    const size_t STACK_SIZE = 16 * 1024;
    void *child_stack_mem = kmalloc(STACK_SIZE);
    if (!child_stack_mem) {
        delete child;
        return -1;
    }
    child->kernel_stack_top = reinterpret_cast<uint64_t>(child_stack_mem) + STACK_SIZE;
    memcpy(child_stack_mem, reinterpret_cast<void*>(parent->kernel_stack_top - STACK_SIZE), STACK_SIZE);

    // 5. Shared Address Space (vfork semantic)
    child->cr3 = parent->cr3;
    child->is_vfork_sharing_address_space = true;
    child->memory_regions = parent->memory_regions;

    // 6. Setup context
    extern CpuControlBlock g_cpu_block;
    child->user_rsp = g_cpu_block.user_rsp;
    child->saved_rip = g_cpu_block.saved_rip;
    child->saved_rflags = g_cpu_block.saved_rflags;
    child->fs_base = parent->fs_base;
    child->gs_base = parent->gs_base;

    // Calculate RSP relative to stack top
    uintptr_t parent_stack_ptr = reinterpret_cast<uintptr_t>(regs);
    uintptr_t stack_offset = parent->kernel_stack_top - parent_stack_ptr;
    uintptr_t child_stack_ptr = child->kernel_stack_top - stack_offset;

    PtRegs* child_regs = reinterpret_cast<PtRegs*>(child_stack_ptr);
    child_regs->rax = 0; // Return 0 for child

    uint64_t* context = reinterpret_cast<uint64_t*>(child_stack_ptr);
    *(--context) = (uint64_t)fork_child_trampoline;
    *(--context) = 0; // r15
    *(--context) = 0; // r14
    *(--context) = 0; // r13
    *(--context) = 0; // r12
    *(--context) = 0; // rbp
    *(--context) = 0; // rbx

    child->stack_pointer = reinterpret_cast<uint64_t>(context);

    // 7. Add child to scheduler and BLOCK parent
    SchedulerManager::the().add_task(child);
    
    fk::algorithms::klog("SYSCALL", "sys_vfork: Blocking parent %lu until child %lu execs/exits", parent->id, child->id);
    parent->vfork_waiting = true;
    SchedulerManager::the().block_current();
    SchedulerManager::the().schedule();

    return child->id;
}
