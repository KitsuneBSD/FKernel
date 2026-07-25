#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Fs/DebugFs/Node/debug_log_node.h>
#include <Kernel/Syscall/syscall.h>
#include <LibFK/Utilities/memory.h>

static constexpr int SYSLOG_ACTION_READ_ALL    = 3;
static constexpr int SYSLOG_ACTION_READ_CLEAR  = 4;
static constexpr int SYSLOG_ACTION_SIZE_BUFFER = 9;
static constexpr int SYSLOG_ACTION_SIZE_UNREAD = 10;

extern "C" uint64_t sys_syslog(uint64_t type, uint64_t bufp, uint64_t len,
                                uint64_t, uint64_t, uint64_t,
                                [[maybe_unused]] PtRegs* regs) {
  auto log = fkernel::DebugLogNode::the();
  if (!log) return 0;

  switch ((int)type) {
  case SYSLOG_ACTION_READ_ALL:
  case SYSLOG_ACTION_READ_CLEAR: {
    if (!bufp || len == 0) return (uint64_t)-1;
    auto* dst = reinterpret_cast<char*>(bufp);
    size_t log_size = log->size();
    size_t copy_size = log_size < (size_t)len ? log_size : (size_t)len;
    log->read(0, copy_size, reinterpret_cast<uint8_t*>(dst));
    return (uint64_t)copy_size;
  }
  case SYSLOG_ACTION_SIZE_BUFFER:
  case SYSLOG_ACTION_SIZE_UNREAD:
    return (uint64_t)log->size();
  default:
    return 0;
  }
}
