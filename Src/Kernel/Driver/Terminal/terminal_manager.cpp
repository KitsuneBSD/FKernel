#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace terminal {

TerminalManager& TerminalManager::the() {
    static TerminalManager instance;
    return instance;
}

fk::core::Result<TerminalId, fk::core::Error> TerminalManager::create_terminal(TerminalType type) {
    TerminalId id(m_next_id.value());
    m_next_id = TerminalId(m_next_id.value() + 1);
    
    switch (type) {
        case TerminalType::VGA: {
            auto terminal = fk::make_owned<VGATerminal>(id.value() - 1);
            if (!terminal) {
                return fk::core::Error::OutOfMemory;
            }
            
            // Register with DevFs
            char name_buf[16];
            snprintf(name_buf, sizeof(name_buf), "tty%d", id.value() - 1);
            DevFs::the().register_device(terminal.get(), name_buf);
            
            m_vga_terminals.push_back(fk::types::move(terminal));
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