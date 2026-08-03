#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Driver/Storage/Interfaces/storage_device.h>
#include <Kernel/Fs/Virtual/TmpFs/tmp_fs.h>
#include <Kernel/Fs/Vfs/Mount/auto_mounter.h>
#include <Kernel/Fs/Vfs/Core/dentry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Memory/UserAccess/user_access.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Utilities/memory.h>

static bool copy_user_string(const char* user_ptr, char* kbuf, size_t kbuf_size) {
  if (!user_ptr || !fkernel::memory::is_user_address(reinterpret_cast<uintptr_t>(user_ptr), 1))
    return false;
  size_t len = 0;
  while (len < kbuf_size - 1 && user_ptr[len] != '\0')
    len++;
  if (len >= kbuf_size - 1) return false;
  auto res = fkernel::memory::copy_from_user(kbuf, user_ptr, len + 1);
  return res.is_ok();
}

static uint64_t mount_tmpfs(const char* target) {
  auto dir = fk::adopt_ref(new TmpFsDirectoryNode());
  if (!dir) return fkernel::return_error(fk::core::Error::OutOfMemory);
  dir->set_is_root(true);
  auto res = fkernel::VirtualFileSystem::the().mount(target, fk::RefPtr<Node>(dir.ptr()));
  if (res.is_error()) return fkernel::return_error(res.error());
  return 0;
}

static uint64_t mount_device(const char* source, const char* target) {
  auto dentry_res = fkernel::VirtualFileSystem::the().resolve_path(source);
  if (dentry_res.is_error()) {
    fk::algorithms::kwarn("MOUNT", "Cannot resolve source %s", source);
    return fkernel::return_error(fk::core::Error::NotFound);
  }

  auto node = dentry_res.value()->top_node();
  if (!node || !node->is_block_device()) {
    fk::algorithms::kwarn("MOUNT", "Source %s is not a block device", source);
    return fkernel::return_error(fk::core::Error::NotADirectory);
  }

  auto* storage = static_cast<StorageDevice*>(node.ptr());
  bool ok = fkernel::AutoMounter::try_mount_at(fk::RefPtr<StorageDevice>(storage), target);
  if (!ok) return fkernel::return_error(fk::core::Error::InvalidData);
  return 0;
}

extern "C" {
uint64_t sys_mount(uint64_t source_ptr, uint64_t target_ptr, uint64_t filesystemtype_ptr,
                   uint64_t /*mountflags*/, uint64_t /*data*/, [[maybe_unused]] PtRegs* regs) {
  auto* current_task = SchedulerManager::the().current();
  if (!current_task) return fkernel::return_error(fk::core::Error::PermissionDenied);
  if (current_task->control.identity.euid != 0) return -1; // EPERM

  if (!target_ptr) return fkernel::return_error(fk::core::Error::InvalidParameter);

  char ksource[512] = {};
  char ktarget[512] = {};
  char kfstype[64] = {};

  if (!copy_user_string(reinterpret_cast<const char*>(target_ptr), ktarget, sizeof(ktarget)))
    return -14; // EFAULT

  if (source_ptr && !copy_user_string(reinterpret_cast<const char*>(source_ptr), ksource, sizeof(ksource)))
    return -14; // EFAULT

  if (filesystemtype_ptr && !copy_user_string(reinterpret_cast<const char*>(filesystemtype_ptr), kfstype, sizeof(kfstype)))
    return -14; // EFAULT

  const char* source = ksource[0] ? ksource : nullptr;
  const char* target = ktarget;
  const char* fstype = kfstype[0] ? kfstype : nullptr;

  fk::algorithms::klog("SYS_MOUNT", "source=%s target=%s type=%s",
                       source ? source : "none", target, fstype ? fstype : "none");

  if (fstype && fk::memory::compare(fstype, "tmpfs") == 0) return mount_tmpfs(target);
  if (source && source[0] == '/') return mount_device(source, target);

  return fkernel::return_error(fk::core::Error::InvalidParameter); // ENODEV: unknown fs type
}
} // extern "C"
