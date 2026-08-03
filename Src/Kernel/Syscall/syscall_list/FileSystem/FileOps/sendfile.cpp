#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Syscall/syscall_utils.h>

extern "C" uint64_t sys_sendfile(uint64_t out_fd, uint64_t in_fd, uint64_t offset_ptr,
                                  uint64_t count, uint64_t, uint64_t,
                                  [[maybe_unused]] PtRegs* regs) {
  auto* task = SchedulerManager::the().current();
  if (!task) return (uint64_t)-1;

  auto in_desc  = task->get_file_descriptor((int)in_fd);
  auto out_desc = task->get_file_descriptor((int)out_fd);
  if (!in_desc || !out_desc) return fkernel::return_error(fk::core::Error::InvalidHandle);

  uint64_t saved_offset = in_desc->offset();
  if (offset_ptr) in_desc->set_offset(*reinterpret_cast<uint64_t*>(offset_ptr));

  constexpr size_t CHUNK = 65536;
  size_t chunk = ((size_t)count < CHUNK) ? (size_t)count : CHUNK;
  uint8_t* buf = static_cast<uint8_t*>(kmalloc(chunk));
  if (!buf) return (uint64_t)-12;

  size_t total_sent = 0;
  while (total_sent < (size_t)count) {
    size_t to_read = ((size_t)count - total_sent < chunk) ? ((size_t)count - total_sent) : chunk;
    auto rres = in_desc->read(to_read, buf);
    if (rres.is_error() || rres.value() == 0) break;
    size_t nread = rres.value();
    auto wres = out_desc->write(nread, buf);
    if (wres.is_error()) break;
    total_sent += wres.value();
    if (wres.value() < nread) break;
  }

  kfree(buf);

  if (offset_ptr) {
    *reinterpret_cast<uint64_t*>(offset_ptr) = in_desc->offset();
    in_desc->set_offset(saved_offset);
  }

  return (uint64_t)total_sent;
}
