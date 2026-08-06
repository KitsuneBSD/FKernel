#include <Kernel/Driver/Terminal/terminal_factory.h>
#include <Kernel/Driver/Terminal/serial_terminal.h>

namespace fkernel {
namespace terminal {

fk::core::Result<fk::memory::OwnPtr<VGATerminal>, fk::core::Error> TerminalFactory::create_vga_terminal(int index) {
    auto terminal = fk::make_owned<VGATerminal>(index);
    if (!terminal) {
        return fk::core::Error::OutOfMemory;
    }
    return terminal;
}

fk::core::Result<fk::memory::OwnPtr<Terminal>, fk::core::Error> TerminalFactory::create_serial_terminal(const char* port_config) {
    auto terminal = fk::make_owned<SerialTerminal>(port_config);
    if (!terminal)
        return fk::core::Error::OutOfMemory;
    return fk::memory::OwnPtr<Terminal>(terminal.leak_ptr());
}

fk::core::Result<fk::memory::OwnPtr<Terminal>, fk::core::Error> TerminalFactory::create_pty_terminal() {
    // TODO: Implement PTY terminal creation
    return fk::core::Error::NotImplemented;
}

fk::core::Result<fk::memory::OwnPtr<Terminal>, fk::core::Error> TerminalFactory::create_terminal(TerminalType type, int index, const char* config) {
    switch (type) {
        case TerminalType::VGA: {
            auto result = create_vga_terminal(index);
            if (result.is_error()) {
                return result.error();
            }
            // Convert OwnPtr<VGATerminal> to OwnPtr<Terminal>
            return fk::memory::OwnPtr<Terminal>(result.value().leak_ptr());
        }
        
        case TerminalType::Serial:
            return create_serial_terminal(config);
            
        case TerminalType::PTY:
            return create_pty_terminal();
    }
    
    return fk::core::Error::InvalidParameter;
}

} // namespace terminal
} // namespace fkernel