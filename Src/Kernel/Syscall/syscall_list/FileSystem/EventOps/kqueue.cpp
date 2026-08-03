#include <Kernel/Fs/Vfs/Events/kqueue.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>

using namespace fkernel;

extern "C" {

uint64_t sys_kqueue(uint64_t, uint64_t, uint64_t, uint64_t,
                    uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* current_task = SchedulerManager::the().current();
    if (!current_task) return -1;

    auto kq_res = fkernel::KQueueNode::create();
    if (kq_res.is_error()) return -static_cast<int>(kq_res.error());

    auto dentry = Dentry::create("kqueue", nullptr).value();
    dentry->push_node(kq_res.value());

    auto desc = fk::make_ref<FileDescription>(dentry, 0).value();
    return current_task->add_file_descriptor(desc);
}

}
