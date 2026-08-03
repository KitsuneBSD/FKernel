#pragma once

#include <Kernel/Scheduler/Task/task_memory.h>
#include <Kernel/Scheduler/Task/task_files.h>
#include <Kernel/Scheduler/Task/task_ipc.h>
#include <Kernel/Scheduler/Task/task_context.h>

struct TaskResources {
  TaskMemory memory;
  TaskFiles files;
  TaskIpc ipc;
  TaskContext context;
};
