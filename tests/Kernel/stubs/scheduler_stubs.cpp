// Minimal SchedulerManager stubs for host-side unit tests.
//
// path_resolver.cpp calls SchedulerManager::the().current() to look up the
// current task's CWD and chroot.  In the uninitialized test environment,
// current() must return nullptr so the resolver falls back to the VFS root.
//
// We provide only the constructor and current_processor() here.  All other
// SchedulerManager methods are left undefined (the linker will complain only
// if they are actually referenced from test code).

#include <Kernel/Scheduler/Core/scheduler.h>

namespace fkernel {

SchedulerManager::SchedulerManager() {}

Processor& SchedulerManager::current_processor() {
    return m_processors[0];
}

} // namespace fkernel
