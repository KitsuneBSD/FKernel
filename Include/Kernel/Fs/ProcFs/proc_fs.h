#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Container/vector.h>

class ProcFsNode : public Node {
public:
  ProcFsNode() = default;
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::NotADirectory; }
  virtual size_t size() const override { return 0; }
  virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
  virtual bool is_directory() const override { return true; }
};

class ProcPartitionsNode : public Node {
public:
  ProcPartitionsNode() = default;
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
  virtual size_t size() const override { return m_cached.size(); }
  virtual bool is_directory() const override { return false; }

private:
  fk::containers::Vector<uint8_t> m_cached;
  void ensure_cached();
};

class ProcProcessNode : public Node {
public:
  ProcProcessNode(uint64_t pid) : m_pid(pid) {}
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override { return fk::core::Error::PermissionDenied; }
  virtual size_t size() const override { return m_cached.size(); }
  virtual bool is_directory() const override { return true; }

private:
  uint64_t m_pid;
  fk::containers::Vector<uint8_t> m_cached;
  void ensure_cached();
};
