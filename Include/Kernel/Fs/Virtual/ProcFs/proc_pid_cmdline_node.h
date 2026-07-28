#pragma once
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Types/process_id.h>

class ProcPidCmdlineNode : public Node {
public:
  explicit ProcPidCmdlineNode(fk::ProcessId pid) : m_pid(pid) {}
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return false; }
private:
  fk::ProcessId m_pid;
};
