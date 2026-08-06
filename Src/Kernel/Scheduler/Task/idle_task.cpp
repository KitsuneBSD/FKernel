#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Scheduler/Sync/task_entries.h>

using namespace fkernel;

extern "C" void idle_task_entry() {
  static bool s_init_spawned = false;

  if (!s_init_spawned) {
    s_init_spawned = true;

    Task* init = new Task();
    if (!init) {
      fk::algorithms::kfatal("IDLE", "Failed to allocate init task");
    }
    initialize_task(init, fk::ProcessId(1), "init", init_task_entry, false, 5, 1, 0, 0);
    fk::algorithms::klog("IDLE", "Init task created (PID 1)");

    init->set_heap_regions(0x10000000, 0x10000000);
    init->set_mmap_regions(0x40000000, 0x40000000);

    fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial |
                                    fk::algorithms::LogTarget::DebugFS);

    SchedulerManager::the().add_task(init);
  }

  SchedulerManager::the().idle_loop();
}
