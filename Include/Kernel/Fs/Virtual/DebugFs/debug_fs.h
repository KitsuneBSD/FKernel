#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Text/string.h>

#include <Kernel/Fs/Virtual/DebugFs/Node/debug_log_node.h>
#include <Kernel/Fs/Virtual/DebugFs/Node/syscall_log_node.h>
#include <Kernel/Ipc/Endpoints/ipc_log_node.h>

namespace fkernel {

class DebugFsNode final : public Node {
public:
  DebugFsNode();
  virtual ~DebugFsNode() override = default;

  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t,
                                                         uint8_t *) override {
    return fk::core::Error::IsDirectory;
  }
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t, size_t, const uint8_t *) override {
    return fk::core::Error::PermissionDenied;
  }
  virtual size_t size() const override { return 0; }

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  lookup(const char *name) override;
  virtual fk::core::Result<void, fk::core::Error>
  list_dir(fk::containers::Vector<DirectoryEntry> &entries) override;
  virtual bool is_directory() const override { return true; }

  static fk::RefPtr<DebugFsNode> the();
};

} // namespace fkernel
