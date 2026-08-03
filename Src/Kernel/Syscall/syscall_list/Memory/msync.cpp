#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Memory/VirtualMemory/memory_region.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Algorithms/Logging/log.h>


extern "C" uint64_t sys_msync(uint64_t addr, uint64_t len, [[maybe_unused]] uint64_t flags, uint64_t, uint64_t, uint64_t,
                                [[maybe_unused]] PtRegs* regs) {
    auto* task = SchedulerManager::the().current();
    if (!task) return fkernel::return_error(fk::core::Error::PermissionDenied);
    if (addr & 0xFFF || len == 0) return fkernel::return_error(fk::core::Error::InvalidParameter);


    uintptr_t end = addr + ((len + 0xFFF) & ~0xFFFULL);
    auto& regions = task->resources.memory.regions.list;

    for (size_t i = 0; i < regions.size(); ++i) {
        auto& r = regions[i];
        if (!r.is_shared || !r.backing_node) continue;
        // Find overlap with the msync range
        uintptr_t ov_start = r.start > addr     ? r.start : addr;
        uintptr_t ov_end   = r.end   < end      ? r.end   : end;
        if (ov_start >= ov_end) continue;

        uint64_t file_offset = r.backing_offset + (ov_start - r.start);
        size_t   write_len   = static_cast<size_t>(ov_end - ov_start);
        const uint8_t* src   = reinterpret_cast<const uint8_t*>(ov_start);
        size_t done = 0;

        while (done < write_len) {
            size_t chunk = write_len - done;
            if (chunk > 4096) chunk = 4096;
            auto res = r.backing_node->write(file_offset + done, chunk, src + done);
            if (res.is_error()) {
                fk::algorithms::kwarn("MSYNC", "write failed at offset %lu", file_offset + done);
                return fkernel::return_error(res.error());
            }
            done += res.value();
            if (res.value() == 0) break;
        }
    }
    return 0;
}
