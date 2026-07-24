#include <Kernel/Fs/ProcFs/proc_self_node.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

using namespace fk::core;

fk::core::Result<fk::text::String, fk::core::Error> ProcSelfNode::read_link() {
  auto* task = SchedulerManager::the().current();
  if (!task) return fk::core::Error::NotFound;
  char buf[32];
  snprintf(buf, sizeof(buf), "/proc/%lu", task->control.identity.id.value());
  return fk::text::String(buf);
}
