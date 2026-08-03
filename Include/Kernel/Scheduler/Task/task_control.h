#pragma once

#include <Kernel/Scheduler/Task/task_identity.h>
#include <Kernel/Scheduler/Task/task_lifecycle.h>

struct TaskControl {
  TaskIdentity identity;
  TaskLifecycle lifecycle;
};
