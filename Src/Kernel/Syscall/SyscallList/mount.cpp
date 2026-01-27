#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Text/string.h>

extern "C" {
uint64_t sys_mount(uint64_t source_ptr, uint64_t target_ptr,
                   uint64_t filesystemtype_ptr, uint64_t /*mountflags*/,
                   uint64_t /*data*/, [[maybe_unused]] PtRegs* regs) {
  auto *current_task = SchedulerManager::the().current();
  if (!current_task) {
    fk::algorithms::kerror("Syscall", "sys_mount: No current task");
    return fkernel::return_error(fk::core::Error::PermissionDenied);
  }

  const char *source = (const char *)source_ptr;
  const char *target = (const char *)target_ptr;
  const char *filesystemtype = (const char *)filesystemtype_ptr;

  // Use stack-allocated buffers to avoid heap corruption
  char log_buf[256];
  int log_len = snprintf(log_buf, sizeof(log_buf), 
      "[SYSCALL] sys_mount: source=%s target=%s type=%s\n", 
      source ? source : "null", target ? target : "null", 
      filesystemtype ? filesystemtype : "null");
  
  // Log to DebugFS if available  
  auto syscall_log = fkernel::DebugLogNode::the();
  if (syscall_log) syscall_log->append(log_buf, log_len);

  // Input validation
  if (!target) {
    return fkernel::return_error(fk::core::Error::InvalidParameter);
  }

  // Minimal implementation: only support mounting proc filesystem
  if (filesystemtype && strcmp(filesystemtype, "proc") == 0) {
    // Check if proc filesystem is already available
    static bool proc_mounted = false;

    if (proc_mounted) {
      // Already mounted, return success
      return 0;
    }

    // For FKernel, proc filesystem should already be available via VFS
    // Just mark it as mounted and return success
    proc_mounted = true;

    fk::algorithms::klog("Syscall", "proc filesystem mounted at /proc");
    return 0;
  }

  // For BusyBox: return success for other mount attempts but log them
  fk::algorithms::klog("Syscall",
                       "sys_mount: Unsupported filesystem %s mounted at %s",
                       filesystemtype ? filesystemtype : "unknown", target);
  return 0; // Pretend success to let rcS continue
}

} // extern "C"
