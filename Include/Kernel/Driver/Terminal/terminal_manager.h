#pragma once

#include <Kernel/Driver/Terminal/vga_terminal.h>
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

class TerminalId {
    uint32_t m_value;
    
public:
    explicit TerminalId(uint32_t id) : m_value(id) {}
    
    bool is_valid() const { return m_value > 0; }
    uint32_t value() const { return m_value; }
    
    bool operator==(const TerminalId& other) const { return m_value == other.m_value; }
    bool operator!=(const TerminalId& other) const { return !(*this == other); }
};

class TerminalManager {
public:
    static TerminalManager& the();
    
    // Create terminal on-demand
    fk::core::Result<TerminalId, fk::core::Error> create_terminal(TerminalType type);
    
    // Find existing terminal
    fk::memory::optional<VGATerminal*> find_vga_terminal(TerminalId id);
    
    // Get all terminals
    const fk::containers::Vector<fk::OwnPtr<VGATerminal>>& vga_terminals() const { return m_vga_terminals; }
    
    // Initialize default terminals
    void initialize();
    
private:
    TerminalManager() = default;
    ~TerminalManager() = default;
    
    TerminalId m_next_id{1};
    fk::containers::Vector<fk::OwnPtr<VGATerminal>> m_vga_terminals;
    
    // Non-copyable, non-movable
    TerminalManager(const TerminalManager&) = delete;
    TerminalManager& operator=(const TerminalManager&) = delete;
    TerminalManager(TerminalManager&&) = delete;
    TerminalManager& operator=(TerminalManager&&) = delete;
};

} // namespace terminal
} // namespace fkernel