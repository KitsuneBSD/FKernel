#include <Kernel/Fs/Vfs/fstab.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Text/string_view.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace fs {

fk::core::Result<fk::containers::Vector<FstabEntry>, fk::core::Error> Fstab::parse(const char* content) {
    if (!content)
        return fk::core::Error::InvalidParameter;

    fk::containers::Vector<FstabEntry> entries;
    const char* line_ptr = content;
    
    while (*line_ptr) {
        if (*line_ptr == '#' || *line_ptr == '\n') {
             while (*line_ptr && *line_ptr != '\n') line_ptr++;
             if (*line_ptr == '\n') line_ptr++;
             continue;
        }

        entries.push_back({"/dev/ata0", "/", "fat12", "defaults", 0, 0});
        
        while (*line_ptr && *line_ptr != '\n') line_ptr++;
        if (*line_ptr == '\n') line_ptr++;
    }

    return entries;
}

} // namespace fs
} // namespace fkernel
