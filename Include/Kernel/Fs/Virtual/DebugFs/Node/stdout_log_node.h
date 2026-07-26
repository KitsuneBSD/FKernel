#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>

namespace fkernel {

class StdoutLogNode final : public Node {
public:
  StdoutLogNode();
  virtual ~StdoutLogNode() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t, size_t, const uint8_t *buffer) override;
  virtual size_t size() const override;

  void append(const char *str, size_t len);

  static fk::RefPtr<StdoutLogNode> the();

private:
  fk::containers::Vector<uint8_t> m_buffer;
  static constexpr size_t MAX_LOG_SIZE = 256 * 1024; // 256KB log buffer
};

} // namespace fkernel
