#include <Kernel/Driver/Storage/Nvme/nvme_command_builder.h>

namespace fkernel {

NvmeCommand NvmeCommandBuilder::build_read(uint64_t lba, uint32_t block_count, uint64_t prp1,
                                           uint64_t prp2, uint32_t ns_id) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x02;
  cmd.nsid = ns_id;
  cmd.prp1 = prp1;
  cmd.prp2 = prp2;
  cmd.cdw10 = static_cast<uint32_t>(lba);
  cmd.cdw11 = static_cast<uint32_t>(lba >> 32);
  cmd.cdw12 = block_count - 1;
  return cmd;
}

NvmeCommand NvmeCommandBuilder::build_write(uint64_t lba, uint32_t block_count, uint64_t prp1,
                                            uint64_t prp2, uint32_t ns_id) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x01;
  cmd.nsid = ns_id;
  cmd.prp1 = prp1;
  cmd.prp2 = prp2;
  cmd.cdw10 = static_cast<uint32_t>(lba);
  cmd.cdw11 = static_cast<uint32_t>(lba >> 32);
  cmd.cdw12 = block_count - 1;
  return cmd;
}

NvmeCommand NvmeCommandBuilder::build_flush(uint32_t ns_id) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x00;
  cmd.nsid = ns_id;
  return cmd;
}

NvmeCommand NvmeCommandBuilder::create_submission_queue(uint16_t qid, uint16_t size, uint16_t cqid,
                                                        uint64_t prp1, uint32_t ns_id) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x01;
  cmd.nsid = ns_id;
  cmd.prp1 = prp1;
  cmd.cdw10 = qid;
  cmd.cdw11 = size | (static_cast<uint32_t>(cqid) << 16);
  return cmd;
}

NvmeCommand NvmeCommandBuilder::create_completion_queue(uint16_t qid, uint16_t size, uint64_t prp1,
                                                        uint32_t ns_id) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x05;
  cmd.nsid = ns_id;
  cmd.prp1 = prp1;
  cmd.cdw10 = qid;
  cmd.cdw11 = size - 1;
  return cmd;
}

NvmeCommand NvmeCommandBuilder::identify(uint32_t ns_id, uint64_t prp1) {
  NvmeCommand cmd = {};
  cmd.cdw0 = 0x06;
  cmd.nsid = ns_id;
  cmd.prp1 = prp1;
  cmd.cdw10 = 0x00;
  return cmd;
}

} // namespace fkernel
