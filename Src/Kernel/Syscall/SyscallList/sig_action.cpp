#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Algorithms/log.h>

extern "C" {

uint64_t sys_sigaction(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                       uint64_t) {

  return 0;
}
}
