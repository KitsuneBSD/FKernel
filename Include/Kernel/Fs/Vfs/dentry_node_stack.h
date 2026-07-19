#pragma once

#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

class DentryNodeStack {
public:
  void push(fk::RefPtr<Node> node);
  void pop();
  fk::RefPtr<Node> top() const;
  const fk::containers::Vector<fk::RefPtr<Node>>& all() const { return m_nodes; }

private:
  fk::containers::Vector<fk::RefPtr<Node>> m_nodes;
};

} // namespace fkernel
