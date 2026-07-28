#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_kevent(uint64_t kq, uint64_t changelist, uint64_t nchanges,
                    uint64_t eventlist, uint64_t nevents, uint64_t timeout_ptr,
                    [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return static_cast<uint64_t>(-1);

    auto desc = current_task->get_file_descriptor(static_cast<int>(kq));
    if (!desc) return static_cast<uint64_t>(-static_cast<int>(fk::core::Error::InvalidHandle));

    auto* kq_node = static_cast<fkernel::KQueueNode*>(desc->node().get());

    fkernel::timespec kts{};
    const fkernel::timespec* timeout_arg = nullptr;
    if (timeout_ptr) {
        if (fkernel::memory::is_user_address(timeout_ptr, sizeof(fkernel::timespec))) {
            auto copy_res = fkernel::memory::copy_from_user(&kts,
                reinterpret_cast<const void*>(timeout_ptr), sizeof(kts));
            if (copy_res.is_ok()) {
                timeout_arg = &kts;
            }
        }
    }

    auto res = kq_node->kevent(reinterpret_cast<const fkernel::kevent*>(changelist),
                                static_cast<int>(nchanges),
                                reinterpret_cast<fkernel::kevent*>(eventlist),
                                static_cast<int>(nevents),
                                timeout_arg);

    if (res.is_error()) return static_cast<uint64_t>(-static_cast<int>(res.error()));
    return static_cast<uint64_t>(res.value());
}

}
