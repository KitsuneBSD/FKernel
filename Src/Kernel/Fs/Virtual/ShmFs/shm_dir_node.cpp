#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/ShmFs/shm_dir_node.h>
#include <Kernel/Fs/Virtual/ShmFs/shm_node.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
ShmDirNode::lookup(const char* name) {
  for (auto& child : m_children) {
    if (fk::memory::compare(child.first.c_str(), name) == 0)
      return child.second;
  }
  return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error>
ShmDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  for (auto& child : m_children) {
    DirectoryEntry de{};
    fk::memory::copy_n(de.name, child.first.c_str(), sizeof(de.name) - 1);
    de.type = 0;
    TRY(entries.push_back(de));
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
ShmDirNode::create_child(const char* name, [[maybe_unused]] int mode) {
  for (auto& child : m_children) {
    if (fk::memory::compare(child.first.c_str(), name) == 0)
      return child.second;
  }

  auto shm_res = ShmNode::create();
  if (shm_res.is_error()) return shm_res.error();
  auto shm = shm_res.value();
  shm->set_name(name);
  shm->set_parent(this);

  TRY(m_children.push_back({fk::text::String(name), fk::RefPtr<Node>(shm)}));
  fk::algorithms::kdebug("SHMFS", "Created shm: %s", name);
  return fk::RefPtr<Node>(shm);
}

bool ShmDirNode::remove_child(const char* name) {
  for (size_t i = 0; i < m_children.size(); ++i) {
    if (fk::memory::compare(m_children[i].first.c_str(), name) == 0) {
      m_children[i] = m_children[m_children.size() - 1];
      m_children.pop_back();
      fk::algorithms::kdebug("SHMFS", "Removed shm: %s", name);
      return true;
    }
  }
  return false;
}

}
