#include <Kernel/Fs/Vfs/fstab.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>

namespace fkernel {
namespace fs {

static const char* skip_spaces(const char* p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

static const char* skip_to_space_or_newline(const char* p) {
    while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
    return p;
}

static fk::text::String extract_field(const char*& p) {
    p = skip_spaces(p);
    const char* start = p;
    p = skip_to_space_or_newline(p);
    size_t len = static_cast<size_t>(p - start);
    if (len >= 256) len = 255;
    char buf[256];
    for (size_t i = 0; i < len; ++i) buf[i] = start[i];
    buf[len] = '\0';
    return fk::text::String(buf);
}

fk::core::Result<fk::containers::Vector<FstabEntry>, fk::core::Error>
Fstab::parse(const char* content) {
    if (!content) return fk::core::Error::InvalidParameter;

    fk::containers::Vector<FstabEntry> entries;
    const char* p = content;

    while (*p) {
        p = skip_spaces(p);
        if (*p == '#' || *p == '\n') {
            while (*p && *p != '\n') p++;
            if (*p == '\n') p++;
            continue;
        }
        if (!*p) break;

        FstabEntry entry;
        entry.device     = extract_field(p);
        entry.mountpoint = extract_field(p);
        entry.type       = extract_field(p);
        entry.options    = extract_field(p);

        p = skip_spaces(p);
        entry.dump = 0;
        while (*p >= '0' && *p <= '9') { entry.dump = entry.dump * 10 + (*p - '0'); p++; }
        p = skip_spaces(p);
        entry.pass = 0;
        while (*p >= '0' && *p <= '9') { entry.pass = entry.pass * 10 + (*p - '0'); p++; }

        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;

        if (!entry.device.is_empty() && !entry.mountpoint.is_empty())
            entries.push_back(entry);
    }

    return entries;
}

} // namespace fs
} // namespace fkernel
