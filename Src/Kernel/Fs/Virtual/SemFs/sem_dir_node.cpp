#include <Kernel/Fs/Virtual/SemFs/sem_dir_node.h>
#include <Kernel/Fs/Virtual/SemFs/sem_node.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
SemDirNode::lookup(const char* name) {
  for (auto& child : m_children) {
    if (fk::memory::compare(child.first.c_str(), name) == 0)
      return child.second;
  }
  return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error>
SemDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  for (auto& child : m_children) {
    DirectoryEntry de{};
    fk::memory::copy_n(de.name, child.first.c_str(), sizeof(de.name) - 1);
    de.type = 0;
    entries.push_back(de);
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
SemDirNode::create_child(const char* name, [[maybe_unused]] int mode) {
  for (auto& child : m_children) {
    if (fk::memory::compare(child.first.c_str(), name) == 0)
      return child.second;
  }

  auto sem_res = SemNode::create(0, ~0u);
  if (sem_res.is_error()) return sem_res.error();
  auto sem = sem_res.value();
  sem->set_name(name);
  sem->set_parent(this);

  m_children.push_back({fk::text::String(name), fk::RefPtr<Node>(sem)});
  fk::algorithms::kdebug("SEMFS", "Created semaphore: %s", name);
  return fk::RefPtr<Node>(sem);
}

bool SemDirNode::remove_child(const char* name) {
  for (size_t i = 0; i < m_children.size(); ++i) {
    if (fk::memory::compare(m_children[i].first.c_str(), name) == 0) {
      if (auto* sem = static_cast<SemNode*>(m_children[i].second.get()))
        sem->revoke();
      m_children[i] = m_children[m_children.size() - 1];
      m_children.pop_back();
      fk::algorithms::kdebug("SEMFS", "Removed semaphore: %s", name);
      return true;
    }
  }
  return false;
}

}
