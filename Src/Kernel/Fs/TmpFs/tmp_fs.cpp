#include <Kernel/Fs/TmpFs/tmp_fs.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Memory/new.h>

fk::RefPtr<Node> ChildList::find_by_name(const char *name) {
  for (auto &entry : m_entries) {
    if (entry.has_name(name)) {
      fk::algorithms::kdebug("TMPFS", "Found child %s", name);
      return entry.node();
    }
  }
  return {};
}

fk::core::Result<size_t, fk::core::Error>
TmpFsNode::read(uint64_t offset, size_t size, uint8_t *buffer) {
  if (offset >= m_data.size())
    return 0;

  size_t available = m_data.size() - offset;
  size_t to_read = (size < available) ? size : available;

  for (size_t i = 0; i < to_read; ++i)
    buffer[i] = m_data[offset + i];

  return to_read;
}

fk::core::Result<size_t, fk::core::Error>
TmpFsNode::write(uint64_t offset, size_t size, const uint8_t *buffer) {
  uint64_t end_offset = offset + size;

  if (end_offset > m_data.size())
    m_data.resize(end_offset);

  for (size_t i = 0; i < size; ++i)
    m_data[offset + i] = buffer[i];

  return size;
}

size_t TmpFsNode::size() const { return m_data.size(); }

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
TmpFsDirectoryNode::lookup(const char *name) {
  auto node = m_children.find_by_name(name);
  if (node)
    return node;

  return fk::core::Error::NotFound;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
TmpFsDirectoryNode::create_child(const char *name, [[maybe_unused]] int mode) {
  if (m_children.find_by_name(name))
    return fk::core::Error::PermissionDenied;

  auto result = fk::make_ref<TmpFsNode>().value();
  if (!result)
    return fk::core::Error::OutOfMemory;

  fk::RefPtr<Node> node = result;

  fk::algorithms::kdebug("TMPFS", "Created child %s", name);

  m_children.add(Child(name, node));
  return node;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
TmpFsDirectoryNode::mkdir(const char *name, [[maybe_unused]] int mode) {
  if (m_children.find_by_name(name))
    return fk::core::Error::PermissionDenied;

  auto result = fk::make_ref<TmpFsDirectoryNode>().value();
  if (!result)
    return fk::core::Error::OutOfMemory;

  fk::RefPtr<Node> node = result;
  m_children.add(Child(name, node));
  return node;
}

fk::core::Result<void, fk::core::Error>
TmpFsDirectoryNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
    for (auto& child : m_children.entries()) {
        DirectoryEntry de;
        strncpy(de.name, child.name().c_str(), sizeof(de.name) - 1);
        de.name[sizeof(de.name) - 1] = '\0';
        
        bool is_dir = child.node()->is_directory();
        if (is_dir) {
            de.type = 1; // Directory
        } else if (child.node()->is_symlink()) {
            de.type = 2; // Symlink
        } else {
            de.type = 0; // File
        }
        
        fk::algorithms::klog("TMPFS", "list_dir entry: %s, type: %u (is_dir: %s)", de.name, de.type, is_dir ? "true" : "false");
        entries.push_back(de);
    }
    return {};
}
