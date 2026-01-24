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

    static fk::text::String bsd_ata(int index) {
        return generate("ad", index);
    }

    static fk::text::String bsd_ahci(int index) {
        return generate("ada", index);
    }
};
