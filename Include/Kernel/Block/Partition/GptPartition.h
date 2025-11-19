#pragma once

#include <Kernel/Block/Partition/PartitionParsingStrategy.h>

struct GptHeader {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t partition_entries_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
    uint32_t pe_crc32;
} __attribute__((packed));

struct GptEntry {
    uint8_t  type_guid[16];
    uint8_t  unique_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint16_t name[36];
} __attribute__((packed));

class GptPartitionStrategy : public PartitionParsingStrategy {
public:
  int parse(const void *sector512, PartitionEntry *out, int max_out) override;

private:
    bool are_arguments_valid(const void* sector512, const PartitionEntry* out, int max_out) const;
    bool is_header_valid(const GptHeader* header) const;
    bool is_partition_array_valid(const GptHeader* header, const uint8_t* partition_array_start) const;
    int populate_entries(const GptHeader* header, const uint8_t* partition_array_start, PartitionEntry* out, int max_out) const;
    bool is_entry_empty(const GptEntry& entry) const;
    void convert_gpt_entry(const GptEntry& gpt_entry, PartitionEntry& partition_entry) const;
};