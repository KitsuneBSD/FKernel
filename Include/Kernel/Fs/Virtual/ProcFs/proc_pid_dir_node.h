#pragma once
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Types/Process/process_id.h>

class ProcPidDirNode : public Node {
public:
  explicit ProcPidDirNode(fk::ProcessId pid) : m_pid(pid) {}
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return true; }
private:
  fk::ProcessId m_pid;
};
