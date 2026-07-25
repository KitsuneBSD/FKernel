#pragma once

#include <LibFK/Container/vector.h>
#include <LibFK/Core/error.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>

#include <Kernel/Fs/Vfs/definitions.h>
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

  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size,
                                                         uint8_t* buffer) = 0;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size,
                                                          const uint8_t* buffer) = 0;
  virtual size_t size() const = 0;

  // Called on each open(); device nodes override to do lazy hardware init.
  virtual fk::core::Result<void, fk::core::Error> on_open() { return {}; }

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error> lookup(const char* /*name*/) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  create_child([[maybe_unused]] const char* name, [[maybe_unused]] int mode) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
  mkdir([[maybe_unused]] const char* name, [[maybe_unused]] int mode) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<void, fk::core::Error> symlink([[maybe_unused]] const char* name,
                                                          [[maybe_unused]] const char* target) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<void, fk::core::Error> rmdir([[maybe_unused]] const char* name) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<void, fk::core::Error> unlink([[maybe_unused]] const char* name) {
    return fk::core::Error::NotImplemented;
  }

  virtual fk::core::Result<void, fk::core::Error> link([[maybe_unused]] const char* name,
                                                       [[maybe_unused]] const char* target) {
    return fk::core::Error::NotImplemented;
  }

  virtual fk::core::Result<void, fk::core::Error> rename([[maybe_unused]] const char* old_name,
                                                         [[maybe_unused]] const char* new_name) {
    return fk::core::Error::NotImplemented;
  }

  virtual fk::core::Result<void, fk::core::Error>
  list_dir([[maybe_unused]] fk::containers::Vector<DirectoryEntry>& entries) {
    return fk::core::Error::NotADirectory;
  }

  virtual fk::core::Result<int, fk::core::Error> ioctl([[maybe_unused]] uint64_t request,
                                                       [[maybe_unused]] uint64_t arg) {
    return fk::core::Error::NotImplemented;
  }

  virtual fk::core::Result<void, fk::core::Error> truncate([[maybe_unused]] uint64_t size) {
    return fk::core::Error::NotImplemented;
  }

  virtual fk::core::Result<void, fk::core::Error> fsync() {
    return {};
  }

  virtual bool is_directory() const { return false; }
  virtual bool is_symlink() const { return false; }
  virtual bool is_block_device() const { return false; }
  virtual bool is_character_device() const { return false; }
  virtual bool is_pipe() const { return false; }
  virtual bool is_socket() const { return false; }
  virtual bool is_eventfd() const { return false; }
  virtual bool is_timerfd() const { return false; }
  virtual bool is_signalfd() const { return false; }
  virtual short poll() const { return POLLIN | POLLOUT; }

  virtual fk::core::Result<fk::text::String, fk::core::Error> read_link() {
    return fk::core::Error::NotASymlink;
  }

  virtual fk::text::String get_path() const {
    if (!m_parent)
      return m_name.is_empty() ? "/" : m_name;
    fk::text::String parent_path = m_parent->get_path();
    if (parent_path == "/")
      return "/" + m_name;
    return parent_path + "/" + m_name;
  }

  void set_name(fk::text::String name) { m_name = fk::types::move(name); }
  const fk::text::String& name() const { return m_name; }

  void set_parent(fk::RefPtr<Node> parent) { m_parent = fk::types::move(parent); }
  fk::RefPtr<Node> parent() const { return m_parent; }

  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;

  Node() : m_inode(allocate_inode()) {}

  uint64_t inode() const { return m_inode; }

  uint32_t node_mode() const {
    if (m_mode != 0) return m_mode;
    if (is_directory()) return 0040755u;
    if (is_symlink()) return 0120777u;
    if (is_character_device()) return 0020666u;
    if (is_block_device()) return 0060660u;
    return 0100644u;
  }

  uint32_t node_uid() const { return m_uid; }
  uint32_t node_gid() const { return m_gid; }

  void set_permissions(uint32_t new_mode) {
    uint32_t type_bits = node_mode() & 0xF000u;
    m_mode = type_bits | (new_mode & 07777u);
  }

  void set_owner(uint32_t uid, uint32_t gid) {
    m_uid = uid;
    m_gid = gid;
  }

protected:
  fk::text::String m_name;
  fk::RefPtr<Node> m_parent;
  uint32_t m_mode{0};
  uint32_t m_uid{0};
  uint32_t m_gid{0};

private:
  uint64_t m_inode;

  static uint64_t allocate_inode() {
    static uint64_t s_next_inode = 1;
    return __sync_fetch_and_add(&s_next_inode, 1);
  }
};
