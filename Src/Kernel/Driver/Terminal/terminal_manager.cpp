#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace terminal {

TerminalManager& TerminalManager::the() {
    static TerminalManager instance;
    return instance;
}

fk::core::Result<TerminalId, fk::core::Error> TerminalManager::create_terminal(TerminalType type, const char* name_hint) {
    TerminalId id(m_next_id.value());
    m_next_id = TerminalId(m_next_id.value() + 1);
    
    switch (type) {
        case TerminalType::VGA: {
            int tty_index = (name_hint) ? atoi(name_hint) : (id.value() - 1);
            
            // Check if terminal with this index already exists
            for (size_t i = 0; i < m_vga_terminals.size(); ++i) {
                if (m_vga_terminals[i]->index() == tty_index) {
                    return fk::core::Error::AlreadyExists;
                }
            }
            
            auto terminal = fk::make_owned<VGATerminal>(tty_index);
            if (!terminal) {
                return fk::core::Error::OutOfMemory;
            }
            
            // Register with DevFs
            char name_buf[16];
            snprintf(name_buf, sizeof(name_buf), "tty%d", tty_index);
            DevFs::the().register_device(terminal.get(), name_buf);
            
            m_vga_terminals.push_back(fk::types::move(terminal));
            fk::algorithms::klog("TerminalManager", "Created VGA terminal tty%d on-demand", tty_index);
            return id;
        }
        
        case TerminalType::Serial:
            // TODO: Implement serial terminals
            return fk::core::Error::NotImplemented;
            
        case TerminalType::PTY:
            // TODO: Implement pseudo-terminals
            return fk::core::Error::NotImplemented;
    }
    
    return fk::core::Error::InvalidParameter;
}



fk::memory::optional<VGATerminal*> TerminalManager::find_vga_terminal(TerminalId id) {
    for (size_t i = 0; i < m_vga_terminals.size(); ++i) {
        if (m_vga_terminals[i]->index() == static_cast<int>(id.value() - 1)) {
            return m_vga_terminals[i].get();
        }
    }
    return {};
}

fk::core::Result<void, fk::core::Error> TerminalManager::delete_terminal(TerminalId id) {
    // Find and remove VGA terminal
    for (size_t i = 0; i < m_vga_terminals.size(); ++i) {
        if (m_vga_terminals[i]->index() == static_cast<int>(id.value() - 1)) {
            // Unregister from DevFs
            char name_buf[16];
            snprintf(name_buf, sizeof(name_buf), "tty%d", id.value() - 1);
            DevFs::the().unregister_device(name_buf);
            
            // Remove from vector (manual swap and pop for efficiency)
            if (i < m_vga_terminals.size() - 1) {
                auto temp = fk::types::move(m_vga_terminals[i]);
                m_vga_terminals[i] = fk::types::move(m_vga_terminals[m_vga_terminals.size() - 1]);
                m_vga_terminals[m_vga_terminals.size() - 1] = fk::types::move(temp);
            }
            m_vga_terminals.pop_back();
            fk::algorithms::klog("TerminalManager", "Deleted VGA terminal tty%d", id.value() - 1);
            return {};
        }
    }
    
    // TODO: Handle other terminal types when implemented
    return fk::core::Error::NotFound;
}

fk::memory::optional<Terminal*> TerminalManager::find_terminal(TerminalId id) {
    // Check VGA terminals first
    auto vga_term = find_vga_terminal(id);
    if (vga_term.has_value()) {
        return vga_term.value();
    }
    
    // TODO: Check other terminal types when implemented
    return {};
}

void TerminalManager::initialize() {
    // Create default VGA terminals
    for (int i = 0; i < 6; ++i) {
        auto result = create_terminal(TerminalType::VGA);
        if (result.is_error()) {
            fk::algorithms::klog("TerminalManager", "Failed to create default VGA terminal %d", i);
        }
    }
}

} // namespace terminal
} // namespace fkernel