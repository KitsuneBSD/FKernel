#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Text/string_builder.h>

class StorageDeviceName {
public:
    static fk::text::String generate(const char* prefix, int index) {
        fk::text::StringBuilder builder;
        builder.append(prefix);
        builder.append_decimal(index);
        return builder.to_string();
    }

    // Partition name: <disk>p<N> (e.g., "ad0p1", "ada1p2")
    static fk::text::String partition(const char* disk_name, int part_index) {
        fk::text::StringBuilder builder;
        builder.append(disk_name);
        builder.append("p");
        builder.append_decimal(part_index);
        return builder.to_string();
    }

    static fk::text::String bsd_ata(int index) {
        return generate("ad", index);
    }

    static fk::text::String bsd_ahci(int index) {
        return generate("ada", index);
    }
};
