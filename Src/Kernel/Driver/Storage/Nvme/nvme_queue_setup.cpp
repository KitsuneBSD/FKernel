#include <Kernel/Driver/Storage/Nvme/nvme_controller_state.h>
#include <Kernel/Driver/Storage/Nvme/nvme_queue_setup.h>
#include <LibFK/Core/Result.h>

namespace fkernel {

fk::core::Result<void, fk::core::Error> NvmeQueueSetup::setup_queues() {
  return setup_admin_queue();
}

fk::core::Result<void, fk::core::Error> NvmeQueueSetup::setup_admin_queue() {
  return m_state.queue_manager().setup_admin_queue();
}

} // namespace fkernel