#pragma once

#include <Kernel/Driver/Storage/Nvme/nvme_controller_state.h>
#include <LibFK/Core/Result.h>

namespace fkernel {

class NvmeQueueSetup {
public:
  explicit NvmeQueueSetup(NvmeControllerState& state) : m_state(state) {}

  fk::core::Result<void, fk::core::Error> setup_queues() { return setup_admin_queue(); }

  fk::core::Result<void, fk::core::Error> setup_admin_queue() {
    return m_state.queue_manager().setup_admin_queue();
  }

private:
  NvmeControllerState& m_state;
};

} // namespace fkernel