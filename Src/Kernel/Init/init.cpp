#include <Kernel/Boot/init.h>
#include <Kernel/Scheduler/scheduler.h>

void init() {
  SchedulerManager::the().initialize();
  SchedulerManager::the().current()->print_info();
}