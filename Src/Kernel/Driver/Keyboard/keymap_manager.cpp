#include <Kernel/Driver/Keyboard/keymap_manager.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Fs/Vfs/definitions.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Result.h>
#include <LibC/string.h>

namespace fkernel::drivers {

// Row helper: 16 entries
#define R16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p

KeymapManager::KeymapManager() {
    load_default_abnt2();
}

KeymapManager& KeymapManager::the() {
    static KeymapManager instance;
    return instance;
}

void KeymapManager::load_default_abnt2() {
    static const uint8_t abnt2_normal[128] = {
        R16(0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t'),
        R16('q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0, '[', '\n', 0, 'a', 's'),
        R16('d', 'f', 'g', 'h', 'j', 'k', 'l', 'c', '~', '\'', 0, ']', 'z', 'x', 'c', 'v'),
        R16('b', 'n', 'm', ',', '.', ';', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };

    static const uint8_t abnt2_shift[128] = {
        R16(0, 27, '!', '@', '#', '$', '%', 0, '&', '*', '(', ')', '_', '+', '\b', '\t'),
        R16('Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0, '{', '\n', 0, 'A', 'S'),
        R16('D', 'F', 'G', 'H', 'J', 'K', 'L', 'C', '^', '\"', 0, '}', 'Z', 'X', 'C', 'V'),
        R16('B', 'N', 'M', '<', '>', ':', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, '|', 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        R16(0, 0, 0, '?', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    };

    memcpy(m_map_normal, abnt2_normal, 128);
    memcpy(m_map_shift, abnt2_shift, 128);
    memset(m_map_alt, 0, 128);
}

fk::core::Result<void, fk::core::Error> KeymapManager::load_from_file(const char* path) {
    auto file_or_error = VirtualFileSystem::the().open(path, 0); // O_RDONLY
    if (file_or_error.is_error()) {
        return file_or_error.error();
    }

    auto file = file_or_error.value();
    
    uint8_t header[8];
    TRY(file->seek(0, SeekMode::Set));
    TRY(file->read(8, header));

    if (memcmp(header, "FKMAP", 5) != 0) {
        fk::algorithms::klog("KEYMAP", "Invalid keymap header in %s", path);
        return fk::core::Error::InvalidParameter;
    }

    // Load tables: Normal, Shift, Alt (128 bytes each)
    TRY(file->seek(8, SeekMode::Set));
    TRY(file->read(128, m_map_normal));
    
    TRY(file->seek(136, SeekMode::Set));
    TRY(file->read(128, m_map_shift));
    
    TRY(file->seek(264, SeekMode::Set));
    TRY(file->read(128, m_map_alt));

    fk::algorithms::klog("KEYMAP", "Loaded keymap from %s", path);
    return {};
}

char KeymapManager::translate(uint8_t keycode, bool shift, bool alt) const {
    if (keycode >= 128)
        return 0;

    // Special hardcoded combination: Alt + Q -> / (only if alt map doesn't have it)
    if (alt && keycode == 0x10 && m_map_alt[keycode] == 0) {
        return '/';
    }

    if (alt && m_map_alt[keycode] != 0) {
        return (char)m_map_alt[keycode];
    }

    if (shift) {
        return (char)m_map_shift[keycode];
    }

    return (char)m_map_normal[keycode];
}

} // namespace fkernel::drivers
