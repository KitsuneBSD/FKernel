#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>
#include <LibFK/Utilities/pair.h>

namespace fkernel {

class MqueueDirNode final : public Node {
  fk::containers::Vector<fk::utilities::Pair<fk::text::String, fk::RefPtr<Node>>> m_children;

public:
  MqueueDirNode() = default;

  bool is_directory() const override { return true; }

  fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override {
    return fk::core::Error::NotADirectory;
  }
  fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
    return fk::core::Error::NotADirectory;
  }
  size_t size() const override { return 0; }

  fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
  fk::core::Result<void, fk::core::Error> list_dir(
      fk::containers::Vector<DirectoryEntry>& entries) override;

  fk::core::Result<fk::RefPtr<Node>, fk::core::Error> create_child(
      const char* name, int mode) override;

  bool remove_child(const char* name);
};

}
