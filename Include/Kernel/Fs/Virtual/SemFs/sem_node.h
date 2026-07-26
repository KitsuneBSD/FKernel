#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <Kernel/Ipc/notification.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class SemNode final : public Node {
public:
  static fk::core::Result<fk::RefPtr<SemNode>, fk::core::Error> create(uint32_t value, uint32_t max);

  explicit SemNode(uint32_t value, uint32_t max);

  fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
    return fk::core::Error::NotImplemented;
  }
  fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
    return fk::core::Error::NotImplemented;
  }
  size_t size() const override { return sizeof(uint32_t); }
  bool is_directory() const override { return false; }
  bool is_semaphore() const override { return true; }

  int post();
  int wait();
  int try_wait();
  int get_value() const;

  uint64_t generation() const { return m_generation; }
  const uint64_t* generation_ptr() const { return &m_generation; }
  void revoke() { ++m_generation; }

private:
  mutable fk::synchronization::Spinlock m_lock;
  uint32_t m_count;
  uint32_t m_max;
  uint64_t m_generation{0};
  ipc::Notification m_waiters;
};

}
