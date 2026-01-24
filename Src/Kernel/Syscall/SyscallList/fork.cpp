#include "Kernel/Hardware/Cpu/cpu_block.h"
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/heap_malloc.h>

extern "C" {
void syscall_stub_post_dispatch();
void fork_child_trampoline();
}

extern "C" {
uint64_t
sys_fork([[maybe_unused]] uint64_t arg1, [[maybe_unused]] uint64_t arg2,
         [[maybe_unused]] uint64_t arg3, [[maybe_unused]] uint64_t arg4,
         [[maybe_unused]] uint64_t arg5, [[maybe_unused]] uint64_t arg6) {
  auto *parent = SchedulerManager::the().current();
  if (!parent)
    return -1;

  // 1. Create and initialize a new task structure
  Task *child = static_cast<Task *>(kmalloc(sizeof(Task)));
  if (!child)
    return -1;

  // Explicitly initialize intrusive nodes to null
  child->run_node.prev = nullptr;
  child->run_node.next = nullptr;
  child->wait_node.prev = nullptr;
  child->wait_node.next = nullptr;
  child->sleep_node.prev = nullptr;
  child->sleep_node.next = nullptr;

  // 2. Clone metadata
  child->id = SchedulerManager::the().generate_pid();
  child->ppid = parent->id;
  child->name = parent->name;
  child->state = TaskState::Ready;
  child->priority = parent->priority;
  child->cpu_affinity = parent->cpu_affinity;
  child->is_a_kernel_task = parent->is_a_kernel_task;
  child->cwd = parent->cwd;
  child->clear_child_tid = 0;
  child->time_slice_ticks = 5;
  child->wake_up_time_ticks = 0;
  child->signal_state.mask = parent->signal_state.mask;

  child->memory_regions.heap_start = parent->memory_regions.heap_start;
  child->memory_regions.heap_break = parent->memory_regions.heap_break;
  child->memory_regions.mmap_start = parent->memory_regions.mmap_start;
  child->memory_regions.mmap_end = parent->memory_regions.mmap_end;

  // 2.5. Inherit syscall return state
  extern CpuControlBlock g_cpu_block;
  child->user_rsp = g_cpu_block.user_rsp;
  child->saved_rip = g_cpu_block.saved_rip;
  child->saved_rflags = g_cpu_block.saved_rflags;

  // 3. Clone File Descriptors
  for (size_t i = 0; i < parent->file_descriptors.size(); ++i) {
    child->file_descriptors.push_back(parent->file_descriptors[i]);
  }

  // 4. Setup Kernel Stack
  const size_t STACK_SIZE = 16 * 1024;
  void *child_stack_mem = kmalloc(STACK_SIZE);
  child->kernel_stack_top =
      reinterpret_cast<uint64_t>(child_stack_mem) + STACK_SIZE;

  // Clone the WHOLE 16KB kernel stack
  void *parent_stack_bottom =
      reinterpret_cast<void *>(parent->kernel_stack_top - STACK_SIZE);
  memcpy(child_stack_mem, parent_stack_bottom, STACK_SIZE);

  // 5. Setup Address Space
  child->cr3 = VirtualMemoryManager::the().clone_address_space(parent->cr3);

  // 6. Setup child's context for switch_context
  uintptr_t child_stack_base = child->kernel_stack_top - 96;
  uint64_t *child_context_stack =
      reinterpret_cast<uint64_t *>(child_stack_base);

  *(--child_context_stack) = (uint64_t)fork_child_trampoline;
  *(--child_context_stack) = 0; // r15
  *(--child_context_stack) = 0; // r14
  *(--child_context_stack) = 0; // r13
  *(--child_context_stack) = 0; // r12
  *(--child_context_stack) = 0; // rbp
  *(--child_context_stack) = 0; // rbx

  child->stack_pointer = reinterpret_cast<uint64_t>(child_context_stack);

  // 7. Add to scheduler
  SchedulerManager::the().add_task(child);

  fk::algorithms::klog("SYSCALL", "Forked child PID %lu from parent PID %lu",
                       child->id, parent->id);

  return child->id;
}
}
