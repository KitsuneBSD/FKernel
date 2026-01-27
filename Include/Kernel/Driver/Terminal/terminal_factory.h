#pragma once

#include <Kernel/Driver/Terminal/terminal.h>
#include <Kernel/Driver/Terminal/vga_terminal.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Core/Result.h>

namespace fkernel {
namespace terminal {

/// @brief Factory class for creating different types of terminals
class TerminalFactory {
public:
    /// @brief Create a VGA terminal
    static fk::core::Result<fk::memory::OwnPtr<VGATerminal>, fk::core::Error> create_vga_terminal(int index);
    
    /// @brief Create a serial terminal (TODO: implement)
    static fk::core::Result<fk::memory::OwnPtr<Terminal>, fk::core::Error> create_serial_terminal(const char* port_config);
    
    /// @brief Create a pseudo-terminal (PTY) (TODO: implement)
    static fk::core::Result<fk::memory::OwnPtr<Terminal>, fk::core::Error> create_pty_terminal();
    
    /// @brief Create terminal by type
    static fk::core::Result<fk::memory::OwnPtr<Terminal>, fk::core::Error> create_terminal(TerminalType type, int index = -1, const char* config = nullptr);

private:
    TerminalFactory() = delete;
    ~TerminalFactory() = delete;
    TerminalFactory(const TerminalFactory&) = delete;
    TerminalFactory& operator=(const TerminalFactory&) = delete;
};

} // namespace terminal
} // namespace fkernel