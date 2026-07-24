#include <Kernel/Fs/TmpFs/tmp_fs.h>
#include <LibFK/Utilities/memory.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Memory/new.h>

fk::RefPtr<Node> ChildList::find_by_name(const char* name) {
  for (auto& entry : m_entries) {
    if (entry.has_name(name)) {
      fk::algorithms::kdebug("TMPFS", "Found child %s", name);
      return entry.node();
    }
  }
  return {};
}

fk::core::Result<size_t, fk::core::Error> TmpFsNode::read(uint64_t offset, size_t size,
                                                          uint8_t* buffer) {
  if (offset >= m_data.size())
    return 0;

  size_t available = m_data.size() - offset;
  size_t to_read = (size < available) ? size : available;

  fk::memory::copy(buffer, m_data.begin() + offset, to_read);

  return to_read;
}

fk::core::Result<size_t, fk::core::Error> TmpFsNode::write(uint64_t offset, size_t size,
                                                           const uint8_t* buffer) {
  uint64_t end_offset = offset + size;

  if (end_offset > m_data.size())
    m_data.resize(end_offset);

  fk::memory::copy(m_data.begin() + offset, buffer, size);

  return size;
}

size_t TmpFsNode::size() const {
  return m_data.size();
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error> TmpFsDirectoryNode::lookup(const char* name) {
  auto node = m_children.find_by_name(name);
  if (node)
    return node;

  return fk::core::Error::NotFound;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
TmpFsDirectoryNode::create_child(const char* name, [[maybe_unused]] int mode) {
  if (m_children.find_by_name(name))
    return fk::core::Error::PermissionDenied;

  auto result = fk::make_ref<TmpFsNode>().value();
  if (!result)
    return fk::core::Error::OutOfMemory;

  fk::RefPtr<Node> node = result;
  node->set_name(name);
  node->set_parent(this);

  fk::algorithms::kdebug("TMPFS", "Created child %s", name);

  m_children.add(Child(name, node));
  return node;
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
TmpFsDirectoryNode::mkdir(const char* name, [[maybe_unused]] int mode) {
  if (m_children.find_by_name(name))
    return fk::core::Error::PermissionDenied;

  auto result = fk::make_ref<TmpFsDirectoryNode>().value();
  if (!result)
    return fk::core::Error::OutOfMemory;

  fk::RefPtr<Node> node = result;
  node->set_name(name);
  node->set_parent(this);

  m_children.add(Child(name, node));
  return node;
}

fk::core::Result<void, fk::core::Error>
TmpFsDirectoryNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  for (auto& child : m_children.entries()) {
    DirectoryEntry de;
    fk::memory::copy_n(de.name, child.name().c_str(), sizeof(de.name) - 1);
    de.name[sizeof(de.name) - 1] = '\0';

    if (child.node()->is_directory()) {
      de.type = 1; // Directory
    } else if (child.node()->is_symlink()) {
      de.type = 2; // Symlink
    } else {
      de.type = 0; // File
    }

    entries.push_back(de);
  }
  return {};
}

fk::core::Result<void, fk::core::Error> TmpFsDirectoryNode::rmdir(const char* name) {
  auto node = m_children.find_by_name(name);
  if (!node)
    return fk::core::Error::NotFound;

  if (!node->is_directory())
    return fk::core::Error::NotADirectory;

  auto* target_dir = static_cast<TmpFsDirectoryNode*>(node.ptr());
  if (target_dir && target_dir->m_children.size() > 0)
    return fk::core::Error::DirectoryNotEmpty;

  if (!m_children.remove_by_name(name))
    return fk::core::Error::PermissionDenied;

  return {};
}

fk::core::Result<void, fk::core::Error> TmpFsDirectoryNode::unlink(const char* name) {
  auto node = m_children.find_by_name(name);
  if (!node)
    return fk::core::Error::NotFound;

  if (node->is_directory())
    return fk::core::Error::IsDirectory;

  if (!m_children.remove_by_name(name))
    return fk::core::Error::PermissionDenied;

  return {};
}

fk::core::Result<void, fk::core::Error> TmpFsDirectoryNode::link(const char* name,
                                                                 const char* target) {
  if (m_children.find_by_name(name))
    return fk::core::Error::PermissionDenied;

  auto target_node = m_children.find_by_name(target);
  if (!target_node)
    return fk::core::Error::NotFound;

  m_children.add(Child(name, target_node));
  return {};
}

fk::core::Result<void, fk::core::Error> TmpFsDirectoryNode::rename(const char* old_name,
                                                                   const char* new_name) {
  auto node = m_children.find_by_name(old_name);
  if (!node)
    return fk::core::Error::NotFound;

  if (m_children.find_by_name(new_name))
    return fk::core::Error::PermissionDenied;

  m_children.remove_by_name(old_name);
  node->set_name(new_name);
  m_children.add(Child(new_name, node));

  return {};
}
