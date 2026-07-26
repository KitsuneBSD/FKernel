#pragma once
#include <Kernel/Fs/Virtual/TmpFs/tmp_fs_node.h>
#include <Kernel/Fs/Virtual/TmpFs/tmp_fs_child_list.h>

class TmpFsDirectoryNode final : public TmpFsNode {
public:
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* name) override;
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> create_child(const char* name, int mode) override;
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> mkdir(const char* name, int mode) override;
  virtual fk::core::Result<void, fk::core::Error> rmdir(const char* name) override;
  virtual fk::core::Result<void, fk::core::Error> unlink(const char* name) override;
  virtual fk::core::Result<void, fk::core::Error> link(const char* name, const char* target) override;
  virtual fk::core::Result<void, fk::core::Error> rename(const char* old_name, const char* new_name) override;
  virtual fk::core::Result<void, fk::core::Error> list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
  virtual bool is_directory() const override { return true; }
  void set_is_root(bool b) { if (b) m_name = ""; }

private:
  ChildList m_children;
};
