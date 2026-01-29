#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include "Kernel/Hardware/Cpu/cpu_block.h"
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Ipc/cspace.h>
#include <Kernel/Ipc/notification.h>
#include <Kernel/Ipc/global_endpoint_manager.h>
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
         [[maybe_unused]] uint64_t arg5, [[maybe_unused]] uint64_t arg6,
         PtRegs* regs) {
  auto *parent = SchedulerManager::the().current();
  if (!parent)
    return fkernel::return_error(fk::core::Error::PermissionDenied);

  // 1. Create and initialize a new task structure
  Task *child = new Task();
  if (!child)
    return fkernel::return_error(fk::core::Error::OutOfMemory);

  // 2. Clone metadata
  child->identity.id = SchedulerManager::the().generate_pid();
  child->identity.ppid = parent->identity.id;
  child->identity.name = parent->identity.name;
  child->state = TaskState::Ready;
  child->priority = parent->priority;
  child->cpu_affinity = parent->cpu_affinity;
  child->is_a_kernel_task = parent->is_a_kernel_task;
  child->files.cwd = parent->files.cwd;
  child->clear_child_tid = 0;

  // 2.5 Initialize IPC CSpace for child
  child->ipc.cspace = new fkernel::ipc::CSpace();
  auto *signal_notification = new fkernel::ipc::Notification();
  child->ipc.cspace->install(fkernel::ipc::Capability(
      signal_notification, fkernel::ipc::CapabilityType::Notification));
  fkernel::ipc::GlobalEndpointManager::the().register_notification(
      child->identity.id.value(), signal_notification);

  child->time_slice_ticks = 5;
  child->wake_up_time_ticks = 0;
  child->ipc.signals.mask = parent->ipc.signals.mask;

  child->memory.regions.heap_start = parent->memory.regions.heap_start;
  child->memory.regions.heap_break = parent->memory.regions.heap_break;
  child->memory.regions.mmap_start = parent->memory.regions.mmap_start;
  child->memory.regions.mmap_end = parent->memory.regions.mmap_end;

  // 2.5. Inherit syscall return state
  child->user_rsp = regs->rsp;
  child->saved_rip = regs->rip;
  child->saved_rflags = regs->rflags;

  // Inherit user segment bases
  child->fs_base = CPU::the().read_msr(MSR_FS_BASE);
  child->gs_base = CPU::the().read_msr(MSR_KERNEL_GS_BASE);

  // 3. Clone File Descriptors
  for (size_t i = 0; i < parent->files.descriptors.size(); ++i) {
    child->files.descriptors.push_back(parent->files.descriptors[i]);
  }
  child->dump_file_descriptors();

  // 4. Setup Kernel Stack
  const size_t STACK_SIZE = 16 * 1024;
  void *child_stack_mem = kmalloc(STACK_SIZE);
  if (!child_stack_mem) {
      delete child;
      return fkernel::return_error(fk::core::Error::OutOfMemory);
  }
  child->kernel_stack_top =
      reinterpret_cast<uint64_t>(child_stack_mem) + STACK_SIZE;

  // Clone the WHOLE 16KB kernel stack
  void *parent_stack_bottom =
      reinterpret_cast<void *>(parent->kernel_stack_top - STACK_SIZE);
  memcpy(child_stack_mem, parent_stack_bottom, STACK_SIZE);

  // 5. Setup Address Space
  child->memory.cr3 = VirtualMemoryManager::the().clone_address_space(parent->memory.cr3);

  // 6. Setup child's context for switch_context
  // Calculate RSP relative to stack top
  uintptr_t parent_stack_ptr = reinterpret_cast<uintptr_t>(regs);
  uintptr_t stack_offset = parent->kernel_stack_top - parent_stack_ptr;
  uintptr_t child_stack_ptr = child->kernel_stack_top - stack_offset;

  PtRegs* child_regs = reinterpret_cast<PtRegs*>(child_stack_ptr);
  child_regs->rax = 0; // Return 0 for child in fork()

  // We need to push callee-saved registers for switch_context
  // Layout after switch_context (push rbx...r15):
  // [PtRegs]
  // [ret_addr (fork_child_trampoline)]
  // [r15]
  // [r14]
  // [r13]
  // [r12]
  // [rbp]
  // [rbx]  <- RSP
  
  uint64_t* context = reinterpret_cast<uint64_t*>(child_stack_ptr);
  *(--context) = (uint64_t)fork_child_trampoline;
  *(--context) = regs->rbx;
  *(--context) = regs->rbp;
  *(--context) = regs->r12;
  *(--context) = regs->r13;
  *(--context) = regs->r14;
  *(--context) = regs->r15;

  child->stack_pointer = reinterpret_cast<uint64_t>(context);

  // 7. Add to scheduler
  SchedulerManager::the().add_task(child);

  fk::algorithms::klog("SYSCALL", "Forked child PID %lu from parent PID %lu",
                       child->identity.id.value(), parent->identity.id.value());

  return child->identity.id.value();
}
}
