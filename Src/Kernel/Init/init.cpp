#include <Kernel/Boot/init.h>
#include <Kernel/Scheduler/scheduler.h>

void init() {
  SchedulerManager::the().initialize();
}