#pragma once

#include <LibFK/Types/types.h>

struct Task;

namespace fkernel {
namespace ipc {

struct SharedMemoryMapping {
  Task* task;
  uintptr_t vaddr;
};

} // namespace ipc
} // namespace fkernel
