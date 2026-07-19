#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Driver/Storage/storage_device.h>
#include <Kernel/Fs/TmpFs/tmp_fs.h>
#include <Kernel/Fs/Vfs/auto_mounter.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Memory/retain_ptr.h>

static bool is_no_device_fstype(const char* fstype) {
  if (!fstype) return true;
  if (strcmp(fstype, "proc") == 0) return true;
  if (strcmp(fstype, "sysfs") == 0) return true;
  if (strcmp(fstype, "devtmpfs") == 0) return true;
  if (strcmp(fstype, "devpts") == 0) return true;
  if (strcmp(fstype, "cgroup") == 0) return true;
  if (strcmp(fstype, "cgroup2") == 0) return true;
  return false;
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
    fk::algorithms::kwarn("sys_mount", "Cannot resolve source %s", source);
    return fkernel::return_error(fk::core::Error::NotFound);
  }

  auto node = dentry_res.value()->top_node();
  if (!node || !node->is_block_device()) {
    fk::algorithms::kwarn("sys_mount", "Source %s is not a block device", source);
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

  const char* source = reinterpret_cast<const char*>(source_ptr);
  const char* target = reinterpret_cast<const char*>(target_ptr);
  const char* fstype = reinterpret_cast<const char*>(filesystemtype_ptr);

  if (!target) return fkernel::return_error(fk::core::Error::InvalidParameter);

  fk::algorithms::klog("sys_mount", "source=%s target=%s type=%s",
                       source ? source : "none", target, fstype ? fstype : "none");

  if (fstype && strcmp(fstype, "tmpfs") == 0) return mount_tmpfs(target);
  if (is_no_device_fstype(fstype)) return 0;
  if (source && source[0] == '/') return mount_device(source, target);

  return 0;
}
} // extern "C"
