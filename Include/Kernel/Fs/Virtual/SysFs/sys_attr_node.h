#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Text/fixed_string.h>

// Read-only sysfs attribute file that returns a fixed string value.
class SysAttrNode : public Node {
public:
  explicit SysAttrNode(const char* value) : m_value(value) {}
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
  virtual size_t size() const override { return m_value.size() + 1; }
  virtual bool is_directory() const override { return false; }
private:
  fk::text::fixed_string<64> m_value;
};
