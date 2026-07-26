#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Ipc/notification.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class MqueueNode final : public Node {
  struct Entry {
    uint32_t prio;
    fk::containers::Vector<uint8_t> data;
  };

public:
  static fk::core::Result<fk::RefPtr<MqueueNode>, fk::core::Error> create(
      size_t max_msgs, size_t max_msg_size);

  explicit MqueueNode(size_t max_msgs, size_t max_msg_size);

  fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
    return fk::core::Error::NotImplemented;
  }
  fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
    return fk::core::Error::NotImplemented;
  }
  size_t size() const override { return m_count; }
  bool is_directory() const override { return false; }
  bool is_mqueue() const override { return true; }

  int send(const void* buf, size_t len, uint32_t prio);
  ssize_t receive(void* buf, size_t len, uint32_t* prio, bool nonblock);
  short poll() const override;

  uint64_t generation() const { return m_generation; }
  const uint64_t* generation_ptr() const { return &m_generation; }
  void revoke() { ++m_generation; }

private:
  mutable fk::synchronization::Spinlock m_lock;
  fk::containers::Vector<Entry*> m_queue;
  size_t m_max_msgs;
  size_t m_max_msg_size;
  size_t m_count{0};
  uint64_t m_generation{0};
  ipc::Notification m_readable;
  ipc::Notification m_writable;
};

}
