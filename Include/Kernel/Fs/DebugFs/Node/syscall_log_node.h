#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>

namespace fkernel {

class SyscallLogNode final : public Node {
public:
  SyscallLogNode();
  virtual ~SyscallLogNode() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t, size_t, const uint8_t *buffer) override;
  virtual size_t size() const override;

  void append(const char *str, size_t len);

  static fk::RefPtr<SyscallLogNode> the();

private:
  fk::containers::Vector<uint8_t> m_buffer;
  static constexpr size_t MAX_LOG_SIZE = 128 * 1024; // 128KB log buffer
};

} // namespace fkernel
