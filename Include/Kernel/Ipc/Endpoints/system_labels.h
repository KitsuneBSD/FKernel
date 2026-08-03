#pragma once

namespace fkernel {
namespace ipc {

enum SystemLabel : uint64_t {
  IPC_LABEL_SIGNAL = 0x1,
  IPC_LABEL_EXIT = 0x2,
  IPC_LABEL_FAULT = 0x3,
  IPC_LABEL_SYSCALL_PROXY = 0x4,
};

}
} // namespace fkernel
