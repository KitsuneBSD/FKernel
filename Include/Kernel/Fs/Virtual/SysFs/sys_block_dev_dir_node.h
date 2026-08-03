#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Text/fixed_string.h>
#include <LibFK/Types/types.h>

// /sys/block/<name>/ — per-device directory with size and sector_size attributes.
class SysBlockDevDirNode : public Node {
public:
  SysBlockDevDirNode(const char* dev_name, uint64_t size_bytes, uint32_t sector_sz);
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return true; }
  virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
private:
  fk::text::fixed_string<32> m_dev_name;
  fk::text::fixed_string<24> m_size_str;
  fk::text::fixed_string<12> m_sector_size_str;
};
