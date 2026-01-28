#include <Kernel/Fs/Vfs/kqueue.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

extern "C" {

uint64_t sys_kqueue(uint64_t, uint64_t, uint64_t, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return -1;

    auto kq_res = fkernel::KQueueNode::create();
    if (kq_res.is_error()) return -static_cast<int>(kq_res.error());

    auto desc = fk::make_ref<FileDescription>(kq_res.value(), 0).value();
    return current_task->add_file_descriptor(desc);
}

}
