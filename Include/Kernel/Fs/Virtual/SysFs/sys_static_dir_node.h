#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>

// Empty static sysfs directory (used for /sys/class/, /sys/fs/).
class SysStaticDirNode : public Node {
public:
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return true; }
  virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>&) override { return {}; }
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char*) override { return fk::core::Error::NotFound; }
};
