#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/MqueueFs/mqueue_dir_node.h>
#include <Kernel/Fs/Virtual/MqueueFs/mqueue_node.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
MqueueDirNode::lookup(const char* name) {
  for (auto& child : m_children) {
    if (fk::memory::compare(child.first.c_str(), name) == 0)
      return child.second;
  }
  return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error>
MqueueDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  for (auto& child : m_children) {
    DirectoryEntry de{};
    fk::memory::copy_n(de.name, child.first.c_str(), sizeof(de.name) - 1);
    de.type = 0;
    TRY(entries.push_back(de));
  }
  return {};
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
MqueueDirNode::create_child(const char* name, [[maybe_unused]] int mode) {
  for (auto& child : m_children) {
    if (fk::memory::compare(child.first.c_str(), name) == 0)
      return child.second;
  }

  auto mq_res = MqueueNode::create(10, 8192);
  if (mq_res.is_error()) return mq_res.error();
  auto mq = mq_res.value();
  mq->set_name(name);
  mq->set_parent(this);

  TRY(m_children.push_back({fk::text::String(name), fk::RefPtr<Node>(mq)}));
  fk::algorithms::kdebug("MQUEUEFS", "Created mqueue: %s", name);
  return fk::RefPtr<Node>(mq);
}

bool MqueueDirNode::remove_child(const char* name) {
  for (size_t i = 0; i < m_children.size(); ++i) {
    if (fk::memory::compare(m_children[i].first.c_str(), name) == 0) {
      if (auto* mq = static_cast<MqueueNode*>(m_children[i].second.get()))
        mq->revoke();
      m_children[i] = m_children[m_children.size() - 1];
      m_children.pop_back();
      fk::algorithms::kdebug("MQUEUEFS", "Removed mqueue: %s", name);
      return true;
    }
  }
  return false;
}

}
