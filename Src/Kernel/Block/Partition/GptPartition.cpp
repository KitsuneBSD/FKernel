#include <Kernel/Block/Partition/GptPartition.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/crc32.h>
#include <LibFK/Algorithms/log.h>

bool GptPartitionStrategy::are_arguments_valid(const void* sector512, const PartitionEntry* output_partitions, int max_partitions) const {
    if (!sector512 || !output_partitions || max_partitions <= 0) {
        fk::algorithms::kwarn("GPT", "Invalid parser arguments");
        return false;
    }
    return true;
}

bool GptPartitionStrategy::is_header_valid(const GptHeader* hdr) const {
    if (hdr->signature != 0x5452415020494645ULL) {
        fk::algorithms::klog("GPT", "Invalid signature.");
        return false;
    }

    if (hdr->header_size < 92 || hdr->header_size > 512) {
        fk::algorithms::kwarn("GPT", "GPT Header size invalid: %u", hdr->header_size);
        return false;
    }

    uint8_t header_buffer[512];
    memcpy(header_buffer, hdr, hdr->header_size);
    *reinterpret_cast<uint32_t*>(header_buffer + 16) = 0;

    uint32_t calculated_crc32 = fk::algorithms::crc32(header_buffer, hdr->header_size);
    if (calculated_crc32 != hdr->header_crc32) {
        fk::algorithms::kwarn("GPT", "GPT Header CRC32 mismatch: expected %08x, got %08x", hdr->header_crc32, calculated_crc32);
        return false;
    }

    if (hdr->partition_entry_size != sizeof(GptEntry)) {
        fk::algorithms::kwarn("GPT", "Unexpected GPT entry size: expected %zu, got %u", sizeof(GptEntry), hdr->partition_entry_size);
        return false;
    }

    return true;
}

bool GptPartitionStrategy::is_partition_array_valid(const GptHeader* hdr, const uint8_t* partition_array_start) const {
    size_t array_size = (size_t)hdr->num_partition_entries * hdr->partition_entry_size;
    fk::algorithms::klog("GPT", "Partition entry array size: %u bytes.", array_size);
    
    uint32_t calculated_crc32 = fk::algorithms::crc32(partition_array_start, array_size);

    if (calculated_crc32 != hdr->pe_crc32) {
        fk::algorithms::kwarn("GPT", "GPT Partition Entry Array CRC32 mismatch: expected %08x, got %08x", hdr->pe_crc32, calculated_crc32);
        return false;
    }
    return true;
}

bool GptPartitionStrategy::is_entry_empty(const GptEntry& entry) const {
    for (int i = 0; i < 16; ++i) {
        if (entry.type_guid[i] != 0)
            return false;
    }
    return true;
}

void GptPartitionStrategy::convert_gpt_entry(const GptEntry& gpt_entry, PartitionEntry& partition_entry) const {
    partition_entry.lba_start = gpt_entry.first_lba;
    partition_entry.lba_count = (gpt_entry.last_lba - gpt_entry.first_lba) + 1;
    partition_entry.type = 0;
    partition_entry.is_bootable = false;
    partition_entry.has_chs = false;
}

int GptPartitionStrategy::populate_entries(const GptHeader* hdr, const uint8_t* partition_array_start, PartitionEntry* output_partitions, int max_partitions) const {
    int written = 0;
    size_t entry_size = hdr->partition_entry_size;
    int entries_to_read = hdr->num_partition_entries < (uint32_t)max_partitions ? hdr->num_partition_entries : (uint32_t)max_partitions;
    fk::algorithms::klog("GPT", "Parsing %d GPT partition entries.", entries_to_read);

    for (int i = 0; i < entries_to_read; i++) {
        const GptEntry& entry = *reinterpret_cast<const GptEntry*>(partition_array_start + i * entry_size);
        if (is_entry_empty(entry)) {
            fk::algorithms::klog("GPT", "Skipping empty entry %d.", i);
            continue;
        }
        
        fk::algorithms::klog("GPT", "Found partition at entry %d. LBA start: %lu, Size: %lu sectors.", i, entry.first_lba, (entry.last_lba - entry.first_lba) + 1);
        convert_gpt_entry(entry, output_partitions[written++]);
    }

    return written;
}

int GptPartitionStrategy::parse(const void *sector512, PartitionEntry *output_partitions, int max_partitions) {
    fk::algorithms::klog("GPT", "Attempting to parse GPT partition table.");

    if (!are_arguments_valid(sector512, output_partitions, max_partitions))
        return 0;

    const uint8_t* base = static_cast<const uint8_t*>(sector512);
    const GptHeader* hdr = reinterpret_cast<const GptHeader*>(base);

    if (!is_header_valid(hdr))
        return 0;

    fk::algorithms::klog("GPT", "GPT header is valid. LBA of partition entries: %lu", hdr->partition_entries_lba);

    // This implementation assumes that the buffer 'sector512' contains not only the GPT header (from LBA 1)
    // but also the partition entry array that follows it. The partition entries should start at the LBA
    // specified by hdr->partition_entries_lba, which is typically LBA 2.
    // The following line assumes the entry array starts right after the first 512-byte sector.
    // This will only work if the input buffer is prepared accordingly.
    const uint8_t* partition_entry_array_start = base + 512;
    if (!is_partition_array_valid(hdr, partition_entry_array_start))
        return 0;

    fk::algorithms::klog("GPT", "GPT partition entry array is valid.");

    int written = populate_entries(hdr, partition_entry_array_start, output_partitions, max_partitions);

    if (written > 0) {
        fk::algorithms::klog("GPT", "Successfully parsed %d GPT partition(s).", written);
    } else {
        fk::algorithms::klog("GPT", "No valid GPT partitions found.");
    }
    
    return written;
}
