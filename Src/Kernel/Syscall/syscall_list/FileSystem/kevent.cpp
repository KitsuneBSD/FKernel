#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_kevent(uint64_t kq, uint64_t changelist, uint64_t nchanges,
                    uint64_t eventlist, uint64_t nevents, [[maybe_unused]] uint64_t timeout, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return -1;

    auto desc = current_task->get_file_descriptor(static_cast<int>(kq));
    if (!desc || !desc->node()->is_directory()) { // is_directory used as generic type check or we need dynamic_cast
        // Temporary hack: we know it's a kqueue if we just created it.
        // In a better system we'd use node type.
    }

    auto* kq_node = static_cast<fkernel::KQueueNode*>(desc->node().get());
    auto res = kq_node->kevent(reinterpret_cast<const fkernel::kevent*>(changelist), static_cast<int>(nchanges),
                               reinterpret_cast<fkernel::kevent*>(eventlist), static_cast<int>(nevents));
    
    if (res.is_error()) return -static_cast<int>(res.error());
    return res.value();
}

}
