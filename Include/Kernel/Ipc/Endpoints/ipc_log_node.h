#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Text/string.h>

namespace fkernel {

class IpcLogNode final : public Node {
public:
  IpcLogNode();
  virtual ~IpcLogNode() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t, size_t, const uint8_t *buffer) override;
  virtual size_t size() const override;

  void append(const char *str, size_t len);
  void log_endpoint_operation(const char *operation, uint32_t sender_id, uint32_t receiver_id, uint32_t msg_info);
  void log_notification_operation(const char *operation, uint32_t task_id, uint64_t bits);
  void log_signal_delivery(uint32_t task_id, int signal, uint64_t trampoline);

  static fk::RefPtr<IpcLogNode> the();

private:
  fk::synchronization::Spinlock m_lock;
  fk::containers::Vector<uint8_t> m_buffer;
  static constexpr size_t MAX_LOG_SIZE = 64 * 1024;

  void format_log_entry(const char *type, const char *operation, uint32_t task_id,
                       const char *details = nullptr);
};

} // namespace fkernel