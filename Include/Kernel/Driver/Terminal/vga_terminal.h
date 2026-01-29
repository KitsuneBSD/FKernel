#pragma once

#include <Kernel/Driver/Terminal/terminal.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Container/circular_buffer.h>
#include <LibFK/Terminal/ansi_parser.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace terminal {

/// @brief VGA Terminal implementation using VGA adapter and PS/2 keyboard
class VGATerminal final : public Terminal, public fk::terminal::AnsiDelegate {
public:
  explicit VGATerminal(int index);
  virtual ~VGATerminal() override = default;

  static VGATerminal &the();
  static void set_active(VGATerminal* terminal);

  void on_char(char c);

  virtual fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t *buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t *buffer) override;
  virtual fk::core::Result<int, fk::core::Error> ioctl(uint64_t request,
                                                       uint64_t arg) override;
  virtual size_t size() const override { return 0; }

  // Terminal interface
  virtual fk::core::Result<void, fk::core::Error>
  attach_input(InputDevice *device) override;
  virtual fk::core::Result<void, fk::core::Error>
  attach_output(OutputDevice *device) override;
  virtual TerminalCapabilities capabilities() const override;
  virtual fk::core::Result<void, fk::core::Error>
  set_size(uint16_t rows, uint16_t cols) override;
  virtual void get_size(uint16_t &rows, uint16_t &cols) const override;
  virtual const char *type_name() const override;

  // AnsiDelegate interface
  virtual void put_char(char c) override;
  virtual void move_cursor(uint16_t row, uint16_t col) override;
  virtual void move_cursor_up(uint16_t rows) override;
  virtual void move_cursor_down(uint16_t rows) override;
  virtual void move_cursor_forward(uint16_t cols) override;
  virtual void move_cursor_back(uint16_t cols) override;
  virtual void set_colors(uint8_t fg, uint8_t bg) override;
  virtual void clear_screen(uint8_t mode) override;
  virtual void clear_line(uint8_t mode) override;
  virtual void set_scroll_region(uint16_t top, uint16_t bottom) override;
  virtual void save_cursor() override;
  virtual void restore_cursor() override;
  virtual void show_cursor(bool visible) override;

  // Public access to index
  int index() const { return m_index; }

  // Clear screen
  void clear();

private:
  [[maybe_unused]] int m_index;

  // Input queue for keys
  static constexpr size_t INPUT_QUEUE_SIZE = 1024;
  fk::containers::CircularBuffer<char, INPUT_QUEUE_SIZE> m_input_queue;

  // ANSI Parser and Synchronization
  fk::terminal::AnsiParser m_ansi_parser;
  fk::synchronization::Spinlock m_lock;

  // Terminal state
  bool m_raw_mode{false};
  bool m_echo_enabled{true};
  size_t m_line_chars{0};
  uint16_t m_rows{25};
  uint16_t m_cols{80};

  // Cursor state for save/restore
  uint16_t m_saved_cursor_x{0};
  uint16_t m_saved_cursor_y{0};
};

} // namespace terminal
} // namespace fkernel
