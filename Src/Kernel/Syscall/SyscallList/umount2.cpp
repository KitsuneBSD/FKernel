#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Text/string.h>

extern "C" {
uint64_t sys_umount2(uint64_t target_ptr, uint64_t /*flags*/, [[maybe_unused]] PtRegs* regs) {
    auto *current_task = SchedulerManager::the().current();
    if (!current_task) {
        fk::algorithms::kerror("Syscall", "sys_umount2: No current task");
        return fkernel::return_error(fk::core::Error::PermissionDenied);
    }

    const char *target = (const char *)target_ptr;

// Use stack-allocated buffers to avoid heap corruption
  char log_buf[256];
  int log_len = snprintf(log_buf, sizeof(log_buf), 
      "[SYSCALL] sys_umount2: target=%s\n", 
      target ? target : "null");
  
  // Log to DebugFS
  auto syscall_log = fkernel::DebugLogNode::the();
  if (syscall_log) syscall_log->append(log_buf, log_len);

    // Input validation
    if (!target) {
        return fkernel::return_error(fk::core::Error::InvalidParameter);
    }

    // Minimal implementation: only support unmounting proc filesystem
    if (strcmp(target, "/proc") == 0) {
        static bool proc_mounted = true; // Assume it was mounted
        
        if (!proc_mounted) {
            // Not mounted, return success
            return 0;
        }
        
        // For FKernel, just mark as unmounted and return success
        proc_mounted = false;
        
        fk::algorithms::klog("Syscall", "proc filesystem unmounted from /proc");
        return 0;
    }

    // For BusyBox: return success for other unmount attempts but log them
    fk::algorithms::klog("Syscall", "sys_umount2: %s unmounted (not actually implemented)", target);
    return 0; // Pretend success to let rcS continue
}

} // extern "C"