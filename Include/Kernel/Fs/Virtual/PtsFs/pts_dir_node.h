#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Utilities/pair.h>

namespace fkernel {

// Read-only directory node serving /dev/pts/.
// Slave nodes are registered here when a PTY pair is allocated.
class PtsDirNode final : public Node {
  fk::containers::Vector<fk::utilities::Pair<uint32_t, fk::RefPtr<Node>>> m_slaves;

public:
  fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
    return fk::core::Error::NotADirectory;
  }
  fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
    return fk::core::Error::NotADirectory;
  }
  size_t size() const override { return m_slaves.size(); }
  bool is_directory() const override { return true; }

  void register_slave(uint32_t index, fk::RefPtr<Node> slave);
  void unregister_slave(uint32_t index);

  fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
  fk::core::Result<void, fk::core::Error> list_dir(
      fk::containers::Vector<DirectoryEntry>& entries) override;
};

} // namespace fkernel
