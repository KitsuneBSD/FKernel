#pragma once

#include <Kernel/Driver/Storage/Nvme/nvme_async_operation.h>
#include <LibFK/Container/array.h>

namespace fkernel {

class NvmePendingOperations {
public:
  NvmePendingOperations() = default;

  void add(NvmeAsyncOperation* operation) {
    uint16_t id = operation->command_id();
    if (id < 256)
      m_operations[id] = operation;
  }

  void remove(uint16_t id) {
    if (id < 256)
      m_operations[id] = nullptr;
  }

  NvmeAsyncOperation* find(uint16_t id) const {
    if (id >= 256)
      return nullptr;
    return m_operations[id];
  }

  bool contains(uint16_t id) const {
    if (id >= 256)
      return false;
    return m_operations[id] != nullptr;
  }

  void clear() {
    for (auto& op : m_operations)
      op = nullptr;
  }

private:
  fk::containers::array<NvmeAsyncOperation*, 256> m_operations{};
};

} // namespace fkernel
