#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Driver/Terminal/vga_terminal.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/Virtual/DevFs/dev_fs.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {
namespace terminal {

TerminalManager &TerminalManager::the() {
  static TerminalManager instance;
  return instance;
}

fk::core::Result<TerminalId, fk::core::Error>
TerminalManager::create_terminal(TerminalType type, const char *name_hint) {
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

    // Register with DriverManager (which also handles DevFs)
    auto* term_ptr = terminal.get();
    fkernel::DriverManager::the().register_device(fk::RefPtr<Node>(term_ptr));

    m_vga_terminals.push_back(fk::types::move(terminal));
    fk::algorithms::klog("TERMINAL_MANAGER",
                         "Created VGA terminal tty%d on-demand", tty_index);
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

fk::memory::optional<VGATerminal *>
TerminalManager::find_vga_terminal(TerminalId id) {
  for (size_t i = 0; i < m_vga_terminals.size(); ++i) {
    if (m_vga_terminals[i]->index() == static_cast<int>(id.value() - 1)) {
      return m_vga_terminals[i].get();
    }
  }
  return {};
}

fk::core::Result<void, fk::core::Error>
TerminalManager::delete_terminal(TerminalId id) {
  // Find and remove VGA terminal
  for (size_t i = 0; i < m_vga_terminals.size(); ++i) {
    if (m_vga_terminals[i]->index() == static_cast<int>(id.value() - 1)) {
      // Unregister from DriverManager
      fkernel::DriverManager::the().unregister_device(fk::RefPtr<Node>(m_vga_terminals[i].get()));

      // Remove from vector (manual swap and pop for efficiency)
      if (i < m_vga_terminals.size() - 1) {
        auto temp = fk::types::move(m_vga_terminals[i]);
        m_vga_terminals[i] =
            fk::types::move(m_vga_terminals[m_vga_terminals.size() - 1]);
        m_vga_terminals[m_vga_terminals.size() - 1] = fk::types::move(temp);
      }
      m_vga_terminals.pop_back();
      fk::algorithms::klog("TERMINAL_MANAGER", "Deleted VGA terminal tty%d",
                           id.value() - 1);
      return {};
    }
  }

  // TODO: Handle other terminal types when implemented
  return fk::core::Error::NotFound;
}

fk::memory::optional<Terminal *> TerminalManager::find_terminal(TerminalId id) {
  // Check VGA terminals first
  auto vga_term = find_vga_terminal(id);
  if (vga_term.has_value()) {
    return vga_term.value();
  }

  // TODO: Check other terminal types when implemented
  return {};
}

void TerminalManager::initialize() {
  if (m_is_initialized) return;
  // Create default VGA terminals
  for (int i = 0; i < 6; ++i) {
    auto result = create_terminal(TerminalType::VGA);
    if (result.is_error()) {
      fk::algorithms::klog("TERMINAL_MANAGER",
                           "Failed to create default VGA terminal %d", i);
    }
  }
  // Set default active terminal
  m_active_terminal_index = 0;
  if (m_vga_terminals.size() > 0)
    VGATerminal::set_active(m_vga_terminals[0].get());
  m_is_initialized = true;
}

void TerminalManager::handle_input(char c) {
  if (auto* active = active_terminal()) {
    active->on_char(c);
  }
}

void TerminalManager::switch_to(int index) {
  if (index < 0 || static_cast<size_t>(index) >= m_vga_terminals.size()) {
    return;
  }
  
  if (m_active_terminal_index == index) return;

  // Unmap previous terminal if needed
  if (m_active_terminal_index != -1) {
      // In a more complex system, we would save the state of the current terminal
  }

  m_active_terminal_index = index;
  auto* terminal = m_vga_terminals[index].get();
  
  // Update the global active TTY for legacy reasons/convenience
  VGATerminal::set_active(terminal);
  
  fk::algorithms::klog("TERMINAL_MANAGER", "Switched to tty%d", index);
  
  // 1. Redraw/Sync Display: Clear screen and show terminal state
  // In a real system, we'd restore the character buffer of this terminal to the VGA adapter.
  terminal->clear();
  char msg[64];
  snprintf(msg, sizeof(msg), "\n--- Switched to TTY%d ---\n", index);
  terminal->write(0, fk::memory::length(msg), reinterpret_cast<const uint8_t*>(msg));
}

VGATerminal* TerminalManager::active_terminal() const {
  if (m_vga_terminals.is_empty()) return nullptr;
  return m_vga_terminals[m_active_terminal_index].get();
}

// Global function for external access
void force_tty0_active() {
  TerminalManager::the().force_tty0_active();
}

void TerminalManager::force_tty0_active() {
  if (m_vga_terminals.is_empty()) return;
  if (m_vga_terminals.size() > 0) {
    m_active_terminal_index = 0;
    auto* terminal = m_vga_terminals[0].get();
    VGATerminal::set_active(terminal);
    fk::algorithms::klog("TERMINAL_MANAGER", "Forced tty0 to be active for userspace visibility");
  }
}

} // namespace terminal
} // namespace fkernel
