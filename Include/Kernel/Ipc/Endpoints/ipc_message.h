#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

static constexpr size_t IPC_MESSAGE_WORDS = 6;

// A synchronous IPC payload that fits entirely in registers (seL4 message
// registers). The syscall ABI currently fills words 0-3 (arg1..arg4); the
// remaining words are reserved so the receiver can observe up to six
// register-sized fields.
class IpcMessage {
  uint64_t m_words[IPC_MESSAGE_WORDS]{};

public:
  IpcMessage() = default;

  uint64_t word(size_t index) const {
    return index < IPC_MESSAGE_WORDS ? m_words[index] : 0;
  }

  void set_word(size_t index, uint64_t value) {
    if (index < IPC_MESSAGE_WORDS)
      m_words[index] = value;
  }
};

} // namespace ipc
} // namespace fkernel
