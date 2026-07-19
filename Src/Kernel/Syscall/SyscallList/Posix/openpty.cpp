#include <Kernel/Driver/Pty/pty_buffer.h>
#include <Kernel/Driver/Pty/pty_master.h>
#include <Kernel/Driver/Pty/pty_slave.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Memory/ref_ptr.h>

using namespace fkernel;

extern "C" {

// sys_openpty(int* master_fd, int* slave_fd, ...) → 0 on success
uint64_t sys_openpty(uint64_t master_ptr, uint64_t slave_ptr,
                     uint64_t, uint64_t, uint64_t, uint64_t,
                     [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return static_cast<uint64_t>(-1);

  static uint32_t s_pty_index = 0;
  uint32_t index = __sync_fetch_and_add(&s_pty_index, 1);

  auto to_slave   = fk::make_ref<PtyBuffer>().value();
  auto to_master  = fk::make_ref<PtyBuffer>().value();
  auto master_node = fk::make_ref<PtyMaster>(to_slave, to_master).value();
  auto slave_node  = fk::make_ref<PtySlave>(to_slave, to_master, index).value();

  auto mden = Dentry::create("ptm", nullptr);
  if (mden.is_error()) return static_cast<uint64_t>(-1);
  mden.value()->push_node(master_node);

  auto sden = Dentry::create(slave_node->name().c_str(), nullptr);
  if (sden.is_error()) return static_cast<uint64_t>(-1);
  sden.value()->push_node(slave_node);

  auto mdesc = fk::make_ref<FileDescription>(mden.value(), O_RDWR);
  auto sdesc = fk::make_ref<FileDescription>(sden.value(), O_RDWR);
  if (mdesc.is_error() || sdesc.is_error()) return static_cast<uint64_t>(-1);

  int mfd = task->add_file_descriptor(mdesc.value());
  if (mfd < 0) return static_cast<uint64_t>(mfd);

  int sfd = task->add_file_descriptor(sdesc.value());
  if (sfd < 0) {
    task->close_file_descriptor(mfd);
    return static_cast<uint64_t>(sfd);
  }

  auto* mfds = reinterpret_cast<int*>(master_ptr);
  auto* sfds = reinterpret_cast<int*>(slave_ptr);
  if (!mfds || !sfds) {
    task->close_file_descriptor(mfd);
    task->close_file_descriptor(sfd);
    return static_cast<uint64_t>(-14); // -EFAULT
  }

  *mfds = mfd;
  *sfds = sfd;
  return 0;
}

} // extern "C"
