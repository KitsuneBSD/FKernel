#pragma once
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Types/Process/process_id.h>

class ProcPidExeNode : public Node {
public:
  explicit ProcPidExeNode(fk::ProcessId pid) : m_pid(pid) {}
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::IsDirectory; }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return false; }
  virtual bool is_symlink() const override { return true; }
  virtual fk::core::Result<fk::text::String, fk::core::Error> read_link() override;
private:
  fk::ProcessId m_pid;
};
