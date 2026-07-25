#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/task_entries.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <LibFK/Algorithms/log.h>

using namespace fkernel;

extern "C" void idle_task_entry() {
  static int s_init_spawned = 0;

  if (APIC::the().get_id() == 0 && __sync_bool_compare_and_swap(&s_init_spawned, 0, 1)) {

    // Create Init task on CPU 0
    Task* init = new Task();
    if (!init) {
      fk::algorithms::kfatal("IDLE", "Failed to allocate init task");
    }
    *init = create_a_new_task(fk::ProcessId(1), "init", init_task_entry, false, 5, 1, 0, 0);
    fk::algorithms::klog("IDLE", "Init task created (PID 1)");

    // Set initial memory regions for demand paging
    init->set_heap_regions(0x10000000, 0x10000000);
    init->set_mmap_regions(0x40000000, 0x40000000);

    // Keep Display so kernel panics/errors remain visible on VGA alongside serial.
    fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial |
                                    fk::algorithms::LogTarget::DebugFS |
                                    fk::algorithms::LogTarget::Display);

    SchedulerManager::the().add_task(init);
  }

  arch_halt_loop();
}
