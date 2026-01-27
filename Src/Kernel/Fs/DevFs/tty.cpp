#include <Kernel/Fs/DevFs/tty.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

TTYDevice::TTYDevice(int index) : m_index(index) {
    char name_buf[16];
    if (index == -1) {
        set_name("tty");
    } else {
        snprintf(name_buf, sizeof(name_buf), "tty%d", index);
        set_name(name_buf);
    }
}

fk::core::Result<size_t, fk::core::Error> TTYDevice::read([[maybe_unused]] uint64_t offset, [[maybe_unused]] size_t size, [[maybe_unused]] uint8_t* buffer) {
    // TODO: Implement keyboard input buffer
    return fk::core::Error::NotImplemented;
}

fk::core::Result<size_t, fk::core::Error> TTYDevice::write([[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buffer) {
    if (!buffer) return fk::core::Error::InvalidParameter;

    for (size_t i = 0; i < size; ++i) {
        vga::the().put_char(static_cast<char>(buffer[i]));
    }

    return size;
}

} // namespace fkernel
