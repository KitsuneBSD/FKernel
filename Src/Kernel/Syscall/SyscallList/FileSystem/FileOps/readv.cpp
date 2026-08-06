#include <LibFK/Utilities/memory.h>

#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

struct iovec {
    void* iov_base;
    size_t iov_len;
};

static constexpr size_t READV_IOV_MAX = 1024;
static constexpr size_t READV_MAX_BYTES = 0x7ffff000;

extern "C" uint64_t sys_readv(uint64_t fd, uint64_t iov_ptr, uint64_t iovcnt,
                               uint64_t, uint64_t, uint64_t, [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return (uint64_t)-1;

    if (iovcnt == 0) return 0;
    if (iovcnt > READV_IOV_MAX) return (uint64_t)-22;
    if (!fkernel::memory::is_user_address(iov_ptr, iovcnt * sizeof(iovec))) return (uint64_t)-14;

    auto desc = task->get_file_descriptor((int)fd);
    if (!desc) return (uint64_t)-9;

    iovec* k_iov = static_cast<iovec*>(kmalloc(iovcnt * sizeof(iovec)));
    if (!k_iov) return (uint64_t)-12; // ENOMEM

    auto iov_copy = fkernel::memory::copy_from_user(k_iov, reinterpret_cast<const void*>(iov_ptr),
                                                     iovcnt * sizeof(iovec));
    if (iov_copy.is_error()) { kfree(k_iov); return (uint64_t)-14; }

    size_t total_size = 0;
    for (uint64_t i = 0; i < iovcnt; ++i) {
        if (k_iov[i].iov_len > READV_MAX_BYTES - total_size) { kfree(k_iov); return (uint64_t)-22; }
        total_size += k_iov[i].iov_len;
    }
    if (total_size == 0) { kfree(k_iov); return 0; }

    uint8_t* buf = static_cast<uint8_t*>(kmalloc(total_size));
    if (!buf) { kfree(k_iov); return (uint64_t)-12; }

    auto res = desc->read(total_size, buf);
    if (res.is_error()) { kfree(k_iov); kfree(buf); return fkernel::return_error(res.error()); }

    size_t nread = res.value();
    size_t offset = 0;
    for (uint64_t i = 0; i < iovcnt && offset < nread; ++i) {
        size_t chunk = k_iov[i].iov_len;
        if (chunk > nread - offset) chunk = nread - offset;
        if (chunk == 0) continue;
        if (!fkernel::memory::is_user_address(reinterpret_cast<uint64_t>(k_iov[i].iov_base), chunk)) {
            kfree(k_iov); kfree(buf); return (uint64_t)-14;
        }
        auto copy = fkernel::memory::copy_to_user(k_iov[i].iov_base, buf + offset, chunk);
        if (copy.is_error()) { kfree(k_iov); kfree(buf); return (uint64_t)-14; }
        offset += chunk;
    }

    kfree(k_iov);
    kfree(buf);
    return (uint64_t)nread;
}
