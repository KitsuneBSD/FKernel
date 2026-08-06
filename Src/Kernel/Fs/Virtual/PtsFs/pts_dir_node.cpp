#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/PtsFs/pts_dir_node.h>

namespace fkernel {

static uint32_t parse_pty_index(const char* name) {
  uint32_t n = 0;
  for (const char* p = name; *p >= '0' && *p <= '9'; ++p)
    n = n * 10 + (uint32_t)(*p - '0');
  return n;
}

void PtsDirNode::register_slave(uint32_t index, fk::RefPtr<Node> slave) {
  for (auto& e : m_slaves) {
    if (e.first == index) { e.second = slave; return; }
  }
  fk::utilities::Pair<uint32_t, fk::RefPtr<Node>> entry{index, slave};
  TRY_OR_FATAL(m_slaves.push_back(entry));
}

void PtsDirNode::unregister_slave(uint32_t index) {
  for (size_t i = 0; i < m_slaves.size(); ++i) {
    if (m_slaves[i].first != index) continue;
    m_slaves[i] = m_slaves[m_slaves.size() - 1];
    m_slaves.pop_back();
    return;
  }
}

fk::core::Result<fk::RefPtr<Node>, fk::core::Error>
PtsDirNode::lookup(const char* name) {
  uint32_t index = parse_pty_index(name);
  for (auto& e : m_slaves) {
    if (e.first == index) return e.second;
  }
  return fk::core::Error::NotFound;
}

fk::core::Result<void, fk::core::Error>
PtsDirNode::list_dir(fk::containers::Vector<DirectoryEntry>& entries) {
  for (auto& e : m_slaves) {
    DirectoryEntry de{};
    if (e.second) {
      fk::memory::copy_n(de.name, e.second->name().c_str(), sizeof(de.name) - 1);
    }
    de.type = 0;
    TRY(entries.push_back(de));
  }
  return {};
}

} // namespace fkernel
