#pragma once

#include <Kernel/Ipc/notification.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/ref_counted.h>

namespace fkernel {

class PtyBuffer : public fk::memory::RefCounted<PtyBuffer> {
public:
  static constexpr size_t CAPACITY = 4096;

  PtyBuffer() = default;

  size_t read(uint8_t* dst, size_t len);
  size_t write(const uint8_t* src, size_t len);

  bool is_empty() const;
  size_t available() const;

  ipc::Notification& data_ready() { return m_notification; }

private:
  fk::containers::Vector<uint8_t> m_data;
  ipc::Notification m_notification;
};

} // namespace fkernel
