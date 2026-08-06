#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Driver/Terminal/serial_terminal.h>
#include <Kernel/Driver/Terminal/vga_terminal.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/Virtual/DevFs/dev_fs.h>

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

    TRY(m_vga_terminals.push_back(fk::types::move(terminal)));
    fk::algorithms::klog("TERMINAL_MANAGER",
                         "Created VGA terminal tty%d on-demand", tty_index);
    return id;
  }

  case TerminalType::Serial: {
    const char* cfg = name_hint ? name_hint : "com1";
    auto terminal = fk::make_owned<SerialTerminal>(cfg);
    if (!terminal)
        return fk::core::Error::OutOfMemory;
    auto* ptr = terminal.get();
    fkernel::DriverManager::the().register_device(fk::RefPtr<Node>(ptr));
    TRY(m_serial_terminals.push_back(fk::types::move(terminal)));
    fk::algorithms::klog("TERMINAL_MANAGER", "Created serial terminal %s", cfg);
    return id;
  }

  case TerminalType::PTY:
    // TODO: Implement pseudo-terminals (requires PTY master/slave pair)
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

  // Remove serial terminal by sequential order (id - vga_count - 1)
  size_t serial_idx = id.value() - 1 - m_vga_terminals.size();
  if (serial_idx < m_serial_terminals.size()) {
      fkernel::DriverManager::the().unregister_device(
          fk::RefPtr<Node>(m_serial_terminals[serial_idx].get()));
      if (serial_idx < m_serial_terminals.size() - 1) {
          auto tmp = fk::types::move(m_serial_terminals[serial_idx]);
          m_serial_terminals[serial_idx] =
              fk::types::move(m_serial_terminals[m_serial_terminals.size() - 1]);
          m_serial_terminals[m_serial_terminals.size() - 1] = fk::types::move(tmp);
      }
      m_serial_terminals.pop_back();
      fk::algorithms::klog("TERMINAL_MANAGER", "Deleted serial terminal");
      return {};
  }
  return fk::core::Error::NotFound;
}

fk::memory::optional<Terminal *> TerminalManager::find_terminal(TerminalId id) {
  // Check VGA terminals first
  auto vga_term = find_vga_terminal(id);
  if (vga_term.has_value()) {
    return vga_term.value();
  }

  // Check serial terminals
  size_t serial_idx = id.value() - 1 - m_vga_terminals.size();
  if (serial_idx < m_serial_terminals.size())
      return m_serial_terminals[serial_idx].get();
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
