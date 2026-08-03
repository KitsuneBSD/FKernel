#pragma once
#include <Kernel/Fs/Vfs/Core/node.h>

class ProcUptimeNode : public Node {
public:
  ProcUptimeNode() = default;
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return false; }
};
