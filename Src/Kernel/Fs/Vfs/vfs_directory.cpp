#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/dentry.h>
#include <Kernel/Fs/Vfs/file_description.h>

#include <LibFK/Container/unordered_set.h>
#include <LibFK/Text/string.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<void, fk::core::Error>
VirtualFileSystem::readdir(const char *path, fk::containers::Vector<DirectoryEntry> &entries) {
  fk::synchronization::ScopedLockIRQ lock(m_lock);
  auto dentry_res = resolve_path_unlocked(path);
  if (dentry_res.is_error()) return dentry_res.error();

  fk::containers::UnorderedSet<fk::text::String> seen;
  auto add = [&](const DirectoryEntry& entry) {
      if (seen.insert(fk::text::String(entry.name)))
          entries.push_back(entry);
  };

  auto dentry = dentry_res.value();
  for (auto& node : dentry->nodes()) {
      fk::containers::Vector<DirectoryEntry> layer_entries;
      (void)node->list_dir(layer_entries);
      for (auto& entry : layer_entries) add(entry);
  }

  dentry->for_each_child([&](const fk::RefPtr<Dentry>& child) {
      DirectoryEntry de;
      fk::memory::copy_n(de.name, child->name().c_str(), sizeof(de.name) - 1);
      auto* top = child->top_node().ptr();
      if (top->is_directory())      de.type = 1;
      else if (top->is_symlink())   de.type = 2;
      else if (top->is_pipe())      de.type = 3;
      else                          de.type = 0;
      add(de);
  });

  return {};
}

fk::core::Result<size_t, fk::core::Error>
VirtualFileSystem::readdir(fk::RefPtr<FileDescription> description, uint8_t *buffer, size_t max_bytes) {
  if (!buffer) return fk::core::Error::InvalidParameter;
  fk::synchronization::ScopedLockIRQ lock(m_lock);

  fk::containers::Vector<DirectoryEntry> entries;
  fk::containers::UnorderedSet<fk::text::String> seen;
  auto add = [&](const DirectoryEntry& entry) {
      if (seen.insert(fk::text::String(entry.name)))
          entries.push_back(entry);
  };

  DirectoryEntry dot; fk::memory::copy_string(dot.name, "."); dot.type = 1; add(dot);
  DirectoryEntry dotdot; fk::memory::copy_string(dotdot.name, ".."); dotdot.type = 1; add(dotdot);

  auto dentry = description->dentry();
  for (auto& node : dentry->nodes()) {
      fk::containers::Vector<DirectoryEntry> layer_entries;
      (void)node->list_dir(layer_entries);
      for (auto& entry : layer_entries) add(entry);
  }

  dentry->for_each_child([&](const fk::RefPtr<Dentry>& child) {
      DirectoryEntry de;
      fk::memory::copy_n(de.name, child->name().c_str(), sizeof(de.name) - 1);
      auto* top = child->top_node().ptr();
      if (top->is_directory())      de.type = 1;
      else if (top->is_symlink())   de.type = 2;
      else if (top->is_pipe())      de.type = 3;
      else                          de.type = 0;
      add(de);
  });

  uint64_t bytes_written = 0;
  uint64_t start_idx = description->offset();

  for (size_t i = start_idx; i < entries.size(); ++i) {
    auto &entry = entries[i];
    size_t name_len = fk::memory::length(entry.name);
    size_t reclen = (offsetof(linux_dirent64, d_name) + name_len + 1 + 7) & ~7;

    if (bytes_written + reclen > max_bytes) break;

    auto *dirent = reinterpret_cast<linux_dirent64 *>(buffer + bytes_written);
    dirent->d_ino = i + 1; dirent->d_off = i + 1; dirent->d_reclen = static_cast<uint16_t>(reclen);
    uint8_t dt_type =
        (entry.type == 0) ? DT_REG :
        (entry.type == 1) ? DT_DIR :
        (entry.type == 2) ? DT_LNK :
        (entry.type == 3) ? DT_FIFO :
        DT_UNKNOWN;
    dirent->d_type = dt_type;
    fk::memory::copy(dirent->d_name, entry.name, name_len);
    dirent->d_name[name_len] = '\0';

    bytes_written += reclen;
    description->set_offset(i + 1);
  }

  return bytes_written;
}


} // namespace fkernel
