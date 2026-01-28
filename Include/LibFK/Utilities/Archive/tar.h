#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace archive {

struct TarHeader {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} __attribute__((packed));

class TarParser {
public:
    static size_t octal_to_int(const char* str, size_t max_len);
};

} // namespace archive
} // namespace fk
