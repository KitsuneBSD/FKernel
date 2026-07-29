#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic_common.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/x2apic.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/Smp/ap_entry.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/cpu_block.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/qos.h>
#include <Kernel/Scheduler/task_entries.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Synchronization/interrupt_disabler.h>
#include <LibFK/Utilities/memory.h>

using namespace fkernel::scheduler;

extern "C" void switch_context(uint64_t* prev_stack_ptr, uint64_t next_stack_ptr);
extern "C" void trampoline_start();
extern "C" void trampoline_end();
extern "C" uint64_t stack_bottom;

SchedulerManager::SchedulerManager() {
  for (int i = 0; i < 32; ++i) {
    m_processors[i].id = i;
  }
}

void SchedulerManager::initialize() {
  if (m_is_initialized) return;

  if (ACPIManager::the().is_initialized()) {
    uint32_t detected = ACPIManager::the().cpu_count();
    m_processor_count = fk::CpuCount((detected > 0) ? detected : 1);
  } else {
    m_processor_count = fk::CpuCount(1);
  }

  for (uint32_t i = 0; i < m_processor_count.value(); ++i) {
    Task* idle = new Task();
    initialize_task(idle, fk::ProcessId(0), "idle", idle_task_entry, true, 0, 1ULL << i, 0, 0,
                    QoSClass::Background);
    m_processors[i].idle_task = idle;
    m_processors[i].current_task = nullptr;
  }

  m_next_pid = 2;
  fk::algorithms::klog("SCHEDULER", "MLFQ Scheduler initialized (%d levels, %d CPUs)",
                       MLFQ_LEVELS, m_processor_count.value());
  m_is_initialized = true;
}

Task* SchedulerManager::steal_task(fk::CpuCount stealing_cpu) {
  uint32_t busiest_cpu = stealing_cpu.value();
  size_t max_tasks = 1;
  for (uint32_t i = 0; i < m_processor_count.value(); ++i) {
    if (i == stealing_cpu.value()) continue;
    fk::synchronization::ScopedLockIRQ peek_lock(m_processors[i].run_queue_lock);
    size_t count = m_processors[i].run_queue_total_size();
    if (count > max_tasks) {
      max_tasks = count;
      busiest_cpu = i;
    }
  }
  if (busiest_cpu == stealing_cpu.value()) return nullptr;
  fk::synchronization::ScopedLockIRQ lock(m_processors[busiest_cpu].run_queue_lock);

  uint32_t stealer_id = stealing_cpu.value();
  for (int level = MLFQ_LEVELS - 1; level >= 0; --level) {
    auto& mlfq = m_processors[busiest_cpu].run_queues[level];
    if (mlfq.queue.empty()) continue;
    if (!mlfq.can_serve_cpu(stealer_id)) continue; // O(1) affinity bitmap check
    for (auto it = mlfq.queue.begin(); it != mlfq.queue.end(); ++it) {
      Task* task = &*it;
      if (task->control.lifecycle.cpu_affinity != 0 &&
          !(task->control.lifecycle.cpu_affinity & (1ULL << stealer_id)))
        continue;
      mlfq.dequeue(task);
      task->control.lifecycle.time_slice_ticks = mlfq.quantum_ticks.value();
      return task;
    }
  }
  return nullptr;
}

Task* SchedulerManager::pick_next() {
  auto& proc = current_processor();
  uint32_t cpu_id = proc.id;
  {
    fk::synchronization::ScopedLock lock(proc.run_queue_lock);
    for (uint32_t level = 0; level < MLFQ_LEVELS; ++level) {
      auto& mlfq = proc.run_queues[level];
      if (mlfq.queue.empty()) continue;
      if (!mlfq.can_serve_cpu(cpu_id)) continue; // O(1) skip via affinity bitmap
      for (auto it = mlfq.queue.begin(); it != mlfq.queue.end(); ++it) {
        Task* task = &*it;
        if (task->control.lifecycle.cpu_affinity != 0 &&
            !(task->control.lifecycle.cpu_affinity & (1ULL << cpu_id)))
          continue;
        mlfq.dequeue(task);
        task->control.lifecycle.state = TaskState::Running;
        task->control.lifecycle.time_slice_ticks = mlfq.quantum_ticks.value();
        proc.current_task = task;
        proc.need_resched = false;
        return proc.current_task;
      }
    }
  }
  Task* stolen = steal_task(fk::CpuCount(proc.id));
  if (stolen) {
    stolen->control.lifecycle.state = TaskState::Running;
    proc.current_task = stolen;
    proc.need_resched = false;
    return proc.current_task;
  }
  proc.current_task = proc.idle_task;
  proc.need_resched = false;
  return proc.current_task;
}

fkernel::Processor& SchedulerManager::current_processor() {
  if (!m_is_initialized)
    return m_processors[0];
  uint64_t id = get_current_cpu_id();
  if (id >= (uint64_t)m_processor_count.value())
    id = 0;
  return m_processors[id];
}

static void switch_address_space_if_needed(Task* prev_task, Task* next_task) {
  if (next_task->resources.memory.cr3 != 0 &&
      (prev_task == nullptr ||
       next_task->resources.memory.cr3 != prev_task->resources.memory.cr3)) {
    VirtualMemoryManager::the().switch_address_space(next_task->resources.memory.cr3);
  }
}

static void save_previous_task_context(Task* prev_task) {
  if (!prev_task) return;
  CpuControlBlock& blk = current_cpu_block();
  prev_task->resources.context.user_rsp = blk.user_rsp;
  prev_task->resources.context.saved_rip = blk.saved_rip;
  prev_task->resources.context.saved_rflags = blk.saved_rflags;
  prev_task->resources.context.fs_base = CPU::the().read_msr(MSR_FS_BASE);
  prev_task->resources.context.gs_base = CPU::the().read_msr(MSR_KERNEL_GS_BASE);
}

static void load_next_task_context(Task* next_task) {
  CpuControlBlock& blk = current_cpu_block();
  blk.kernel_stack = next_task->resources.context.kernel_stack_top;
  blk.user_rsp = next_task->resources.context.user_rsp;
  blk.saved_rip = next_task->resources.context.saved_rip;
  blk.saved_rflags = next_task->resources.context.saved_rflags;
  CPU::the().write_msr(MSR_FS_BASE, next_task->resources.context.fs_base);
  CPU::the().write_msr(MSR_KERNEL_GS_BASE, next_task->resources.context.gs_base);
  GDTController::the().set_kernel_stack(next_task->resources.context.kernel_stack_top);
  // KPTI: update per-CPU CR3 pointers for syscall entry/exit CR3 swap.
  blk.kernel_cr3 = next_task->resources.memory.cr3;
  blk.user_cr3   = next_task->resources.memory.user_cr3;
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

  if (prev_task && prev_task != proc.idle_task &&
      prev_task->control.lifecycle.state == TaskState::Running) {
    prev_task->control.lifecycle.state = TaskState::Ready;
    uint8_t level = prev_task->control.lifecycle.mlfq_level;
    fk::synchronization::ScopedLock lock(proc.run_queue_lock);
    proc.run_queues[level].enqueue(prev_task);
  }

  switch_address_space_if_needed(prev_task, next_task);
  save_previous_task_context(prev_task);
  load_next_task_context(next_task);

  // Lazy FPU: save outgoing task's FPU state if it currently owns the FPU registers.
  if (prev_task && prev_task == current_processor().last_fpu_task) {
    void* area = get_fpu_save_area(prev_task->resources.context);
    arch_fpu_save(area);
  }

  if (prev_task) {
    switch_context(&prev_task->resources.context.stack_pointer,
                   next_task->resources.context.stack_pointer);
  } else {
    uint64_t dummy;
    switch_context(&dummy, next_task->resources.context.stack_pointer);
  }
}

void SchedulerManager::start_aps() {
  if (m_processor_count.value() <= 1) {
    fk::algorithms::klog("SCHEDULER", "Uniprocessor system, no APs to start");
    return;
  }

  auto* tramp_start_ptr = reinterpret_cast<const char*>(&trampoline_start);
  auto* tramp_end_ptr = reinterpret_cast<const char*>(&trampoline_end);
  size_t trampoline_size = tramp_end_ptr - tramp_start_ptr;
  fk::memory::copy(reinterpret_cast<void*>(fkernel::smp::TRAMPOLINE_PHYS_ADDR),
                   tramp_start_ptr, trampoline_size);

  auto* data = reinterpret_cast<fkernel::smp::TrampolineData*>(
      fkernel::smp::TRAMPOLINE_PHYS_ADDR + 0xF80);

  for (uint32_t i = 1; i < m_processor_count.value(); ++i) {
    uint8_t apic_id = ACPIManager::the().cpu_apic_id(i);

    GDTController::the().init_per_cpu(i);

    data->pml4_phys = VirtualMemoryManager::the().get_kernel_cr3();
    data->stack_ptr = reinterpret_cast<uint64_t>(&stack_bottom) + KERNEL_STACK_SIZE * (i + 1);
    data->entry_point = 0;
    data->cpu_index = i;
    data->online_flag = 0;
    __sync_synchronize();

    void (*ap_entry_fn)(uint32_t) = nullptr;
    asm volatile("lea ap_entry(%%rip), %0" : "=r"(ap_entry_fn));
    data->entry_point = reinterpret_cast<uint64_t>(ap_entry_fn);

    if (CPU::the().has_x2apic()) {
      X2APIC::the().send_ipi(apic_id, 0, IPI_INIT);
    } else {
      APIC::the().send_ipi(apic_id, 0, IPI_INIT);
    }
    TickManager::the().sleep(10);

    uint8_t sipi_vector = static_cast<uint8_t>(fkernel::smp::TRAMPOLINE_PHYS_ADDR >> 12);
    for (int attempt = 0; attempt < 2; ++attempt) {
      if (CPU::the().has_x2apic()) {
        X2APIC::the().send_ipi(apic_id, sipi_vector, IPI_STARTUP);
      } else {
        APIC::the().send_ipi(apic_id, sipi_vector, IPI_STARTUP);
      }
      TickManager::the().sleep(1);
    }

    for (int timeout = 0; timeout < 500; ++timeout) {
      __sync_synchronize();
      if (data->online_flag == 1)
        break;
      TickManager::the().sleep(10);
    }
    if (data->online_flag)
      fk::algorithms::klog("SCHEDULER", "CPU %u (APIC %u) online", i, apic_id);
    else
      fk::algorithms::kwarn("SCHEDULER", "CPU %u (APIC %u) failed to start", i, apic_id);
  }
}

void SchedulerManager::idle_loop() {
  for (;;) {
    arch_cpu_idle();
    schedule();
  }
}
