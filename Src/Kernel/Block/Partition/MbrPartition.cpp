#include <Kernel/Block/Partition/MbrPartition.h>
#include <LibC/string.h> // For memcpy
#include <LibFK/Algorithms/log.h>

bool MbrPartitionStrategy::are_arguments_valid(const void* sector512, const PartitionEntry* output_partitions, int max_partitions) const {
    if (!sector512 || !output_partitions || max_partitions <= 0) {
        fk::algorithms::kwarn("MBR", "Invalid arguments to parser");
        return false;
    }
    return true;
}

bool MbrPartitionStrategy::has_valid_signature(const uint8_t* data) const {
    if (!(data[510] == 0x55 && data[511] == 0xAA)) {
        fk::algorithms::klog("MBR", "Invalid MBR signature.");
        return false;
    }
    return true;
}

bool MbrPartitionStrategy::is_entry_valid(const MbrEntry& entry) const {
    return entry.partition_type != 0 && entry.sectors_count != 0;
}

void MbrPartitionStrategy::convert_mbr_entry(const MbrEntry& mbr_entry, PartitionEntry& partition_entry) const {
    partition_entry.type = static_cast<PartitionType>(mbr_entry.partition_type);
    partition_entry.lba_start = mbr_entry.start_lba;
    partition_entry.lba_count = mbr_entry.sectors_count;
    partition_entry.is_bootable = (mbr_entry.boot_indicator == 0x80);
    
    // MBR entries always have CHS, even if LBA is used primarily
    partition_entry.has_chs = true;
    memcpy(partition_entry.chs_start, mbr_entry.start_chs, sizeof(mbr_entry.start_chs));
    memcpy(partition_entry.chs_end, mbr_entry.end_chs, sizeof(mbr_entry.end_chs));
}

int MbrPartitionStrategy::populate_entries(const MbrEntry* entries, PartitionEntry* output_partitions, int max_partitions) const {
    int out_count = 0;
    for (int i = 0; i < 4 && out_count < max_partitions; i++) {
        if (!is_entry_valid(entries[i])) {
            fk::algorithms::klog("MBR", "Skipping invalid or empty entry %d.", i);
            continue;
        }
        
        fk::algorithms::klog("MBR", "Found valid partition at entry %d. Type: 0x%x, LBA start: %u, Size: %u sectors.", i, entries[i].partition_type, entries[i].start_lba, entries[i].sectors_count);
        convert_mbr_entry(entries[i], output_partitions[out_count++]);
    }
    return out_count;
}

int MbrPartitionStrategy::parse(const void *sector512, PartitionEntry *output_partitions, int max_partitions) {
    fk::algorithms::klog("MBR", "Attempting to parse MBR partition table.");

    if (!are_arguments_valid(sector512, output_partitions, max_partitions))
        return 0;

    const uint8_t *data = static_cast<const uint8_t *>(sector512);

    if (!has_valid_signature(data))
        return 0;

    fk::algorithms::klog("MBR", "MBR signature is valid.");

    const MbrEntry *entries = reinterpret_cast<const MbrEntry *>(data + 446);
    int out_count = populate_entries(entries, output_partitions, max_partitions);

    if (out_count > 0) {
        fk::algorithms::klog("MBR", "Successfully parsed %d MBR partition(s).", out_count);
    } else {
        fk::algorithms::klog("MBR", "No valid MBR partitions found.");
    }

    return out_count;
}
