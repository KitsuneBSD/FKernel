#pragma once

#include <Kernel/Driver/Device/BlockDevice/block_device.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

namespace fkernel {

class StackableBlockDevice : public BlockDevice {
protected:
  fk::containers::Vector<fk::RefPtr<BlockDevice>> m_children;

  StackableBlockDevice() = default;

public:
  void add_child(fk::RefPtr<BlockDevice> child) {
    if (child) TRY_OR_FATAL(m_children.push_back(fk::types::move(child)));
  }

  size_t child_count() const { return m_children.size(); }
};

} // namespace fkernel
