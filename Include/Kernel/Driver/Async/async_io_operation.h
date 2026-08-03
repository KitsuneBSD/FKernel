#pragma once

#include <Kernel/Driver/Async/io_completion_status.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class AsyncIoOperation {
public:
  virtual ~AsyncIoOperation() = default;

  virtual void on_interrupt(uint32_t interrupt_status) = 0;

  virtual bool is_completed() const = 0;

  virtual IoCompletionStatus get_status() const = 0;

  virtual IoCompletionStatus wait_for_completion(uint64_t timeout_ms = 5000) = 0;
};

} // namespace fkernel
