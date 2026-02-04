#include <Kernel/Driver/Storage/Nvme/nvme_command_id_manager.h>

namespace fkernel {

uint16_t NvmeCommandIdManager::allocate() {
  for (uint16_t id = 1; id < MAX_COMMANDS; ++id) {
    if (!m_used_ids[id]) {
      m_used_ids[id] = true;
      return id;
    }
  }
  return 0xFFFF;
}

void NvmeCommandIdManager::release(uint16_t id) {
  if (id < MAX_COMMANDS)
    m_used_ids[id] = false;
}

bool NvmeCommandIdManager::is_allocated(uint16_t id) const {
  return id < MAX_COMMANDS && m_used_ids[id];
}

} // namespace fkernel
