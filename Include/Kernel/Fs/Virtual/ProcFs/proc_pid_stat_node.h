#pragma once
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>

class ProcPidStatNode : public Node {
public:
  explicit ProcPidStatNode(uint64_t pid) : m_pid(pid) {}
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
  virtual size_t size() const override { return m_cached.size(); }
  virtual bool is_directory() const override { return false; }
private:
  uint64_t m_pid;
  fk::containers::Vector<uint8_t> m_cached;
  void ensure_cached();
};
