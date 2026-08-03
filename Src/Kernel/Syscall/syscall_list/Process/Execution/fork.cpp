#include "Kernel/Hardware/Cpu/cpu_block.h"
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Ipc/Capabilities/cspace.h>
#include <Kernel/Ipc/Endpoints/global_endpoint_manager.h>
#include <Kernel/Ipc/Notifications/notification.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

extern "C" {
void syscall_stub_post_dispatch();
void fork_child_trampoline();
}

extern "C" {
uint64_t sys_fork([[maybe_unused]] uint64_t arg1, [[maybe_unused]] uint64_t arg2,
                  [[maybe_unused]] uint64_t arg3, [[maybe_unused]] uint64_t arg4,
                  [[maybe_unused]] uint64_t arg5, [[maybe_unused]] uint64_t arg6, PtRegs* regs) {
  auto* parent = SchedulerManager::the().current();
  if (!parent) {
    fk::algorithms::kwarn("FORK", "fork: no current task");
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  }

  // 1. Create and initialize a new task structure
  Task* child = new Task();
  if (!child) {
    fk::algorithms::kwarn("FORK", "fork: OOM");
    return fkernel::return_error(fk::core::Error::OutOfMemory);
  }

  // 2. Clone metadata
  child->control.identity.id = SchedulerManager::the().generate_pid();
  child->control.identity.tgid = child->control.identity.id; // new process = new thread group
  child->control.identity.ppid = parent->control.identity.id;
  child->control.identity.pgid = parent->control.identity.pgid;
  child->control.identity.sid = parent->control.identity.sid;
  child->control.identity.name = parent->control.identity.name;
  child->control.lifecycle.state = TaskState::Ready;
  child->control.lifecycle.priority = parent->control.lifecycle.priority;
  child->control.lifecycle.cpu_affinity = parent->control.lifecycle.cpu_affinity;
  child->control.lifecycle.is_a_kernel_task = parent->control.lifecycle.is_a_kernel_task;
  child->control.lifecycle.qos = parent->control.lifecycle.qos;
  child->control.lifecycle.policy = parent->control.lifecycle.policy;
  child->control.lifecycle.base_priority = parent->control.lifecycle.base_priority;
  child->control.lifecycle.mlfq_level = parent->control.lifecycle.mlfq_level;
  child->control.lifecycle.cpu_time_consumed = 0;
  child->control.lifecycle.allotment_ticks = parent->control.lifecycle.allotment_ticks;
  child->control.lifecycle.boosted = false;
  child->control.lifecycle.original_qos = parent->control.lifecycle.qos;
  child->resources.files.cwd = parent->resources.files.cwd;
  child->resources.files.root = parent->resources.files.root;
  child->resources.files.mount_ns = parent->resources.files.mount_ns;
  child->control.lifecycle.clear_child_tid = 0;

  child->resources.ipc.cspace = new fkernel::ipc::CSpace();
  if (!child->resources.ipc.cspace) { delete child; return (uint64_t)-12; }
  auto* signal_notification = new fkernel::ipc::Notification();
  if (!signal_notification) { delete child->resources.ipc.cspace; delete child; return (uint64_t)-12; }
  child->resources.ipc.signal_notification = signal_notification;
  child->resources.ipc.cspace->install(
      fkernel::ipc::Capability(signal_notification, fkernel::ipc::CapabilityType::Notification));
  fkernel::ipc::GlobalEndpointManager::the().register_notification(
      child->control.identity.id.value(), signal_notification);

  child->control.lifecycle.time_slice_ticks = 5;
  child->control.lifecycle.wake_up_time_ticks = 0;

  // Clone Signal State
  child->resources.ipc.signals.blocked = parent->resources.ipc.signals.blocked;
  child->resources.ipc.signals.pending = 0; // Signals are not inherited
  child->resources.ipc.signals.trampoline = parent->resources.ipc.signals.trampoline;
  for (int i = 0; i < NSIG; ++i) {
    child->resources.ipc.signals.actions[i] = parent->resources.ipc.signals.actions[i];
  }

  child->resources.memory.regions.heap_start = parent->resources.memory.regions.heap_start;
  child->resources.memory.regions.heap_break = parent->resources.memory.regions.heap_break;
  child->resources.memory.regions.mmap_start = parent->resources.memory.regions.mmap_start;
  child->resources.memory.regions.mmap_end = parent->resources.memory.regions.mmap_end;
  for (size_t i = 0; i < parent->resources.memory.regions.list.size(); ++i) {
    child->resources.memory.regions.list.push_back(parent->resources.memory.regions.list[i]);
  }

  // 2.5. Inherit syscall return state
  child->resources.context.user_rsp = regs->rsp;
  child->resources.context.saved_rip = regs->rip;
  child->resources.context.saved_rflags = regs->rflags;

  // Inherit user segment bases
  child->resources.context.fs_base = CPU::the().read_msr(MSR_FS_BASE);
  child->resources.context.gs_base = CPU::the().read_msr(MSR_KERNEL_GS_BASE);

  // 3. Clone File Descriptors — independent caps in child CSpace, same rights as parent
  for (size_t i = 0; i < parent->resources.files.descriptors.size(); ++i) {
    auto desc = parent->resources.files.descriptors[i];
    child->resources.files.descriptors.push_back(desc);
    uint32_t child_handle = fkernel::ipc::INVALID_HANDLE;
    if (desc && parent->resources.ipc.cspace && child->resources.ipc.cspace) {
      uint32_t ph = (i < parent->resources.files.cap_handles.size())
                       ? parent->resources.files.cap_handles[i]
                       : fkernel::ipc::INVALID_HANDLE;
      child_handle = parent->resources.ipc.cspace->clone_fd(*child->resources.ipc.cspace, ph);
    }
    child->resources.files.cap_handles.push_back(child_handle);
  }
  child->dump_file_descriptors();

  // 4. Setup Kernel Stack
  const size_t STACK_SIZE = 16 * 1024;
  void* child_stack_mem = kmalloc(STACK_SIZE);
  if (!child_stack_mem) {
    fk::algorithms::kwarn("FORK", "fork: OOM");
    delete child;
    return fkernel::return_error(fk::core::Error::OutOfMemory);
  }
  child->resources.context.kernel_stack_top =
      reinterpret_cast<uint64_t>(child_stack_mem) + STACK_SIZE;

  // Clone the WHOLE 16KB kernel stack
  void* parent_stack_bottom =
      reinterpret_cast<void*>(parent->resources.context.kernel_stack_top - STACK_SIZE);
  fk::memory::copy(child_stack_mem, parent_stack_bottom, STACK_SIZE);

  // 5. Setup Address Space (CoW: shares pages, marks read-only in both)
  child->resources.memory.cr3 =
      VirtualMemoryManager::the().clone_address_space(parent->resources.memory.cr3);
  VirtualMemoryManager::the().flush_tlb();

  // 6. Setup child's context for switch_context
  uintptr_t parent_stack_ptr = reinterpret_cast<uintptr_t>(regs);
  uintptr_t stack_offset = parent->resources.context.kernel_stack_top - parent_stack_ptr;
  uintptr_t child_stack_ptr = child->resources.context.kernel_stack_top - stack_offset;

  PtRegs* child_regs = reinterpret_cast<PtRegs*>(child_stack_ptr);
  child_regs->rax = 0; // Return 0 for child in fork()

  uint64_t* context = reinterpret_cast<uint64_t*>(child_stack_ptr);
  *(--context) = (uint64_t)fork_child_trampoline;
  *(--context) = regs->rbx;
  *(--context) = regs->rbp;
  *(--context) = regs->r12;
  *(--context) = regs->r13;
  *(--context) = regs->r14;
  *(--context) = regs->r15;

  child->resources.context.stack_pointer = reinterpret_cast<uint64_t>(context);

  // Child FPU state: inherit the parent's saved x87/SSE state so the child's first
  // lazy-FPU load (xrstor64/fxrstor in the #NM handler) sees valid data. The child has
  // no xsave_area (only the 512-byte fx_state), so we copy the legacy portion of the
  // parent's save area. As a safety net, mask all x87/SSE exceptions regardless of the
  // inherited state (mirrors initialize_task).
  uint8_t* child_fx = child->resources.context.fx_state;
  uint8_t* parent_fx = static_cast<uint8_t*>(get_fpu_save_area(parent->resources.context));
  fk::memory::copy(child_fx, parent_fx, 512);
  child_fx[0] = 0x7F; child_fx[1] = 0x03;   // FCW = 0x037F (all x87 exceptions masked)
  child_fx[24] = 0x80; child_fx[25] = 0x1F; // MXCSR = 0x1F80 (all SSE exceptions masked)

  // 7. Add to scheduler
  SchedulerManager::the().add_task(child);

  fk::algorithms::klog("FORK", "fork: parent=%lu, child=%lu",
                       parent->control.identity.id.value(),
                       child->control.identity.id.value());

  // Notify kqueue watchers (EVFILT_PROC NOTE_FORK) with child pid in the low bits.
  fkernel::notify_proc_kqueue(parent, fkernel::NOTE_FORK | (child->control.identity.id.value() & 0x00FFFFFFu));

  return child->control.identity.id.value();
}
}