#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/hash_map.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Text/string.h>

#include <LibFK/Utilities/pair.h>

class Child {
  fk::text::String m_name;
  fk::RefPtr<Node> m_node;

public:
  Child(fk::text::String name, fk::RefPtr<Node> node)
      : m_name(name), m_node(node) {}

  bool has_name(const char *name) const { return m_name == name; }
  const fk::text::String& name() const { return m_name; }
  fk::RefPtr<Node> node() const { return m_node; }
};

class ChildList {
  fk::containers::Vector<Child> m_entries;

public:
  fk::RefPtr<Node> find_by_name(const char *name);
  void add(Child child) { m_entries.push_back(child); }
  const fk::containers::Vector<Child>& entries() const { return m_entries; }
};

class TmpFsNode : public Node {
public:
  virtual ~TmpFsNode() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) override;

  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t *buffer) override;

  virtual size_t size() const override;

protected:
  fk::containers::Vector<uint8_t> m_data;
};

class TmpFsDirectoryNode final : public TmpFsNode {
public:
  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  lookup(const char *name) override;

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  create_child(const char *name, int mode) override;

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  mkdir(const char *name, int mode) override;

  virtual fk::core::Result<void, fk::core::Error>
  list_dir(fk::containers::Vector<DirectoryEntry>& entries) override;
  
  virtual bool is_directory() const override { return true; }
  virtual fk::text::String get_path() const override { return m_is_root ? "/" : ""; }

private:
  ChildList m_children;
  bool m_is_root { false };

public:
  void set_is_root(bool b) { m_is_root = b; }
};

