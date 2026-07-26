#pragma once
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>

extern char g_proc_hostname[64];
extern char g_proc_domainname[64];

class ProcSysNode : public Node {
public:
  ProcSysNode() = default;
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
  virtual size_t size() const override { return 0; }
  virtual bool is_directory() const override { return true; }
};
