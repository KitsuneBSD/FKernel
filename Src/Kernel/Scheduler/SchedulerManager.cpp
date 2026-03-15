#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/task_entries.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Synchronization/interrupt_disabler.h>

extern CpuControlBlock g_cpu_block;
extern "C" void switch_context(uint64_t* prev_stack_ptr, uint64_t next_stack_ptr);

SchedulerManager::SchedulerManager() {
  for (int i = 0; i < 32; ++i) {
    m_processors[i].id = i;
  }
}

void SchedulerManager::initialize() {
  m_is_initialized = true;
  m_processor_count = 1;

  for (uint32_t i = 0; i < m_processor_count; ++i) {
    Task* idle = new Task();
    *idle = create_a_new_task(fk::ProcessId(0), "idle", idle_task_entry, true, 0, 1ULL << i, 0, 0);
    m_processors[i].idle_task = idle;
    m_processors[i].current_task = nullptr;
  }

  m_next_pid = 2;
  fk::algorithms::klog("SCHEDULER MANAGER", "Initializing SMP Scheduler Manager...");
}

Task* SchedulerManager::pick_next() {
  auto& proc = current_processor();
  {
    fk::synchronization::ScopedLock lock(proc.run_queue_lock);
    if (proc.run_queue.empty()) {
      proc.current_task = proc.idle_task;
    } else {
      Task* next = proc.run_queue.front();
      proc.run_queue.pop_front();
      next->control.lifecycle.state = TaskState::Running;
      next->control.lifecycle.time_slice_ticks = m_default_quantum;
      proc.current_task = next;
    }
  }
  proc.need_resched = false;
  return proc.current_task;
}

fkernel::Processor& SchedulerManager::current_processor() {
  uint32_t id = APIC::the().get_id();
  if (id < 32)
    return m_processors[id];
  return m_processors[0];
}

static void switch_address_space_if_needed(Task* prev_task, Task* next_task) {
  if (next_task->resources.memory.cr3 != 0 &&
      (prev_task == nullptr ||
       next_task->resources.memory.cr3 != prev_task->resources.memory.cr3)) {
    VirtualMemoryManager::the().switch_address_space(next_task->resources.memory.cr3);
  }
}

static void save_previous_task_context(Task* prev_task) {
  if (prev_task) {
    prev_task->resources.context.user_rsp = g_cpu_block.user_rsp;
    prev_task->resources.context.saved_rip = g_cpu_block.saved_rip;
    prev_task->resources.context.saved_rflags = g_cpu_block.saved_rflags;
    prev_task->resources.context.fs_base = CPU::the().read_msr(MSR_FS_BASE);
    prev_task->resources.context.gs_base = CPU::the().read_msr(MSR_KERNEL_GS_BASE);
  }
}

static void load_next_task_context(Task* next_task) {
  g_cpu_block.kernel_stack = next_task->resources.context.kernel_stack_top;
  g_cpu_block.user_rsp = next_task->resources.context.user_rsp;
  g_cpu_block.saved_rip = next_task->resources.context.saved_rip;
  g_cpu_block.saved_rflags = next_task->resources.context.saved_rflags;
  CPU::the().write_msr(MSR_FS_BASE, next_task->resources.context.fs_base);
  CPU::the().write_msr(MSR_KERNEL_GS_BASE, next_task->resources.context.gs_base);
  GDTController::the().set_kernel_stack(next_task->resources.context.kernel_stack_top);
}

void SchedulerManager::schedule() {
  fk::synchronization::ScopedInterruptDisabler intr_disabler;
  if (!is_need_resched())
    return;

  auto& proc = current_processor();
  Task* prev_task = proc.current_task;
  Task* next_task = pick_next();
  if (next_task == prev_task)
    return;

  switch_address_space_if_needed(prev_task, next_task);
  save_previous_task_context(prev_task);
  load_next_task_context(next_task);

  if (prev_task) {
    switch_context(&prev_task->resources.context.stack_pointer,
                   next_task->resources.context.stack_pointer);
  } else {
    uint64_t dummy;
    switch_context(&dummy, next_task->resources.context.stack_pointer);
  }
}
