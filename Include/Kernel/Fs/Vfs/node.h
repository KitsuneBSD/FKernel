#pragma once

#include <LibFK/Container/vector.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Memory/ref_counted.h>
#include <LibFK/Memory/ref_ptr.h>

struct DirectoryEntry {
  char name[256];
  uint32_t type; // 1 = DIR, 2 = SYM, 0 = REG
};

class Node : public fk::memory::RefCounted<Node> {
public:
  virtual ~Node() override = default;

  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) = 0;
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t *buffer) = 0;
  virtual size_t size() const = 0;

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  lookup(const char * /*name*/) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  create_child([[maybe_unused]] const char *name, [[maybe_unused]] int mode) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  mkdir([[maybe_unused]] const char *name, [[maybe_unused]] int mode) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<void, fk::core::Error>
  symlink([[maybe_unused]] const char *name, [[maybe_unused]] const char *target) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<void, fk::core::Error>
  list_dir([[maybe_unused]] fk::containers::Vector<DirectoryEntry> &entries) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<int, fk::core::Error>
  ioctl([[maybe_unused]] uint64_t request, [[maybe_unused]] uint64_t arg) {
    return fk::core::Error::NotImplemented;
  }

  virtual bool is_directory() const { return false; }
  virtual bool is_symlink() const { return false; }
  virtual bool is_block_device() const { return false; }
  virtual bool is_character_device() const { return false; }
  virtual fk::core::Result<fk::text::String, fk::core::Error> read_link() {
    return fk::core::Error::NotASymlink;
  }

  virtual fk::text::String get_path() const {
    if (!m_parent) return m_name.is_empty() ? "/" : m_name;
    fk::text::String parent_path = m_parent->get_path();
    if (parent_path == "/") return "/" + m_name;
    return parent_path + "/" + m_name;
  }

  void set_name(fk::text::String name) { m_name = fk::types::move(name); }
  const fk::text::String& name() const { return m_name; }

  void set_parent(fk::RefPtr<Node> parent) { m_parent = fk::types::move(parent); }
  fk::RefPtr<Node> parent() const { return m_parent; }

  Node(const Node &) = delete;
  Node &operator=(const Node &) = delete;

  // Allow default constructor
  Node() = default;

protected:
  fk::text::String m_name;
  fk::RefPtr<Node> m_parent;
};
