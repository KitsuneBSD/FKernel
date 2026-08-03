#include <Kernel/Fs/Vfs/Core/dentry_node_stack.h>

namespace fkernel {

void DentryNodeStack::push(fk::RefPtr<Node> node) {
  m_nodes.push_back(node);
}

void DentryNodeStack::pop() {
  if (!m_nodes.is_empty())
    m_nodes.pop_back();
}

fk::RefPtr<Node> DentryNodeStack::top() const {
  if (m_nodes.is_empty())
    return nullptr;
  return m_nodes[m_nodes.size() - 1];
}

} // namespace fkernel
