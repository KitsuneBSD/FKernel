#include <Kernel/Boot/init.h>
#include <Kernel/Scheduler/Scheduler.h>

void init() {
  SchedulerManager::the().initialize();
}