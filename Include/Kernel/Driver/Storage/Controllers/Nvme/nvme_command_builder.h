#pragma once

#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_command.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class NvmeCommandBuilder {
public:
  static NvmeCommand build_read(uint64_t lba, uint32_t block_count, uint64_t prp1,
                                uint64_t prp2 = 0, uint32_t ns_id = 1);

  static NvmeCommand build_write(uint64_t lba, uint32_t block_count, uint64_t prp1,
                                 uint64_t prp2 = 0, uint32_t ns_id = 1);

  static NvmeCommand build_flush(uint32_t ns_id = 1);

  static NvmeCommand create_submission_queue(uint16_t qid, uint16_t size, uint16_t cqid,
                                             uint64_t prp1, uint32_t ns_id = 1);

  static NvmeCommand create_completion_queue(uint16_t qid, uint16_t size, uint64_t prp1,
                                             uint32_t ns_id = 1);

  static NvmeCommand identify(uint32_t ns_id, uint64_t prp1);
};

} // namespace fkernel
