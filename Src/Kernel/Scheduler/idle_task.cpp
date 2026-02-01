#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Scheduler/task_entries.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/InterruptController/apic.h>
#include <LibFK/Algorithms/log.h>

using namespace fkernel;

extern "C" void idle_task_entry() {
  static bool s_init_spawned = false;

  if (APIC::the().get_id() == 0 && !s_init_spawned) {
    s_init_spawned = true;

    // Create Init task on CPU 0
    Task* init = new Task();
    *init = create_a_new_task(fk::ProcessId(1), "init", init_task_entry, false, 5, 1, 0, 0);

    // Set initial memory regions for demand paging
    init->set_heap_regions(0x10000000, 0x10000000);
    init->set_mmap_regions(0x40000000, 0x40000000);

    // Migrate logs
    fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial |
                                    fk::algorithms::LogTarget::DebugFS);

    SchedulerManager::the().add_task(init);
  }

  while (true) {
    asm volatile("hlt");
  }
}
