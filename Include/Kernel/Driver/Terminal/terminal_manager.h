#pragma once

#include "Kernel/Driver/Terminal/vga_terminal.h"
#include "Kernel/Driver/Terminal/terminal_id.h"
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Memory/optional.h>

namespace fkernel {
namespace terminal {

enum class TerminalType {
    VGA,
    Serial,
    PTY
};

class TerminalManager {
public:
    static TerminalManager& the();
    
    // Create terminal on-demand
    fk::core::Result<TerminalId, fk::core::Error> create_terminal(TerminalType type, const char* name_hint = nullptr);
    
    // Find existing terminal
    fk::memory::optional<VGATerminal*> find_vga_terminal(TerminalId id);
    
    // Delete terminal
    fk::core::Result<void, fk::core::Error> delete_terminal(TerminalId id);
    
    // Get terminal by ID (generic)
    fk::memory::optional<Terminal*> find_terminal(TerminalId id);
    
    // Get all terminals
    const fk::containers::Vector<fk::OwnPtr<VGATerminal>>& vga_terminals() const { return m_vga_terminals; }
    
    // Initialize default terminals
    void initialize();

    bool is_initialized() const { return m_is_initialized; }

    // Get terminal count by type
    size_t vga_terminal_count() const { return m_vga_terminals.size(); }

    // Input handling from keyboard driver
    void handle_input(char c);

     // Active terminal management
     void switch_to(int index);
     VGATerminal* active_terminal() const;
     
     // Force terminal 0 to be active (for userspace visibility)
     void force_tty0_active();
    
private:
    TerminalManager() = default;
    ~TerminalManager() = default;
    
    bool m_is_initialized{false};
    TerminalId m_next_id{1};
    fk::containers::Vector<fk::OwnPtr<VGATerminal>> m_vga_terminals;
    int m_active_terminal_index{0};
    
    // Non-copyable, non-movable
    TerminalManager(const TerminalManager&) = delete;
    TerminalManager& operator=(const TerminalManager&) = delete;
    TerminalManager(TerminalManager&&) = delete;
    TerminalManager& operator=(TerminalManager&&) = delete;
};

} // namespace terminal
} // namespace fkernel