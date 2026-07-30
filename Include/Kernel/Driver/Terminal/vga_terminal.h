#pragma once

#include <Kernel/Driver/Terminal/terminal.h>
#include <Kernel/Driver/Terminal/terminal_renderer.h>
#include <Kernel/Ipc/endpoint.h>
#include <LibFK/Core/result.h>
#include <LibFK/Container/circular_buffer.h>
#include <LibFK/Terminal/ansi_parser.h>
#include <LibFK/Synchronization/spinlock.h>

namespace fkernel {
namespace terminal {

struct TerminalState {
    bool raw_mode{false};
    bool echo_enabled{true};
    bool isig_enabled{true};
    bool line_drawing_mode{false};
    bool eof_pending{false};
    size_t line_chars{0};
    uint16_t rows{25};
    uint16_t cols{80};
    int foreground_pgid{0};
};

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
  virtual void set_colors_rgb(uint32_t fg, uint32_t bg) override;
  virtual void clear_screen(uint8_t mode) override;
  virtual void clear_line(uint8_t mode) override;
  virtual void set_scroll_region(uint16_t top, uint16_t bottom) override;
  virtual void save_cursor() override;
  virtual void restore_cursor() override;
  virtual void save_screen() override;
  virtual void restore_screen() override;
  virtual void show_cursor(bool visible) override;
  virtual uint32_t get_cursor_x() override;
  virtual uint32_t get_cursor_y() override;
  virtual uint16_t get_width() override;
  virtual uint16_t get_height() override;
  virtual void respond(const char* data) override;
  
  virtual void insert_chars(uint16_t count) override;
  virtual void delete_chars(uint16_t count) override;
  virtual void erase_chars(uint16_t count) override;
  virtual void insert_lines(uint16_t count) override;
  virtual void delete_lines(uint16_t count) override;
  virtual void scroll_up(uint16_t count) override;
  virtual void scroll_down(uint16_t count) override;
  virtual void set_line_drawing_mode(bool enabled) override;

  int index() const { return m_index; }
  void clear();

private:
  int m_index;
  static constexpr size_t INPUT_QUEUE_SIZE = 1024;
  fk::containers::CircularBuffer<char, INPUT_QUEUE_SIZE> m_input_queue;
  fkernel::ipc::Endpoint m_read_endpoint;
  fk::terminal::AnsiParser m_ansi_parser;
  fk::synchronization::Spinlock m_lock;
  
  TerminalState m_state;
  TerminalRenderer m_renderer;
};

} // namespace terminal
} // namespace fkernel