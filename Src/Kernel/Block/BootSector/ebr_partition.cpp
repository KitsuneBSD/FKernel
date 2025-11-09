#include <Kernel/Block/BootSector/ebr_partition.h>
#include <Kernel/Block/BootSector/mbr_partition.h>

int parse_ebr_chain(const AtaDeviceInfo &device, uint32_t ebr_lba_first, PartitionEntry *out, int max_out) {
    (void) device;
    (void) ebr_lba_first;
    (void) out;
    (void) max_out;
    kwarn("EBR Partition","We need implement this");
    return 0;
}
