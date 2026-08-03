#pragma once
#include <Kernel/Fs/Vfs/Core/node.h>

// Read-only node: pass static string literal.
// Read-write node: pass writable 64-byte global buffer as both value and writable_buf.
class ProcSysStringNode : public Node {
public:
  explicit ProcSysStringNode(const char* value, char* writable_buf = nullptr)
    : m_value(value), m_writable_buf(writable_buf) {}

  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return false; }

private:
  const char* m_value;
  char* m_writable_buf;
};
