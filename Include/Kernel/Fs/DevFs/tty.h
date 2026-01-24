#pragma once

#include <Kernel/Fs/Vfs/node.h>

namespace fkernel {

class TtyNode final : public Node {
public:
  TtyNode() = default;
  virtual ~TtyNode() override = default;

  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override;
  virtual fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg) override;
  virtual size_t size() const override { return 0; }
};

} // namespace fkernel
