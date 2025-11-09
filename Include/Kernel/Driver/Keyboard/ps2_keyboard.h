#pragma once

#include <LibFK/Types/types.h>

static constexpr uint16_t PS2_DATA_PORT = 0x60;     ///< PS/2 data port
static constexpr uint16_t PS2_STATUS_PORT = 0x64;   ///< PS/2 status port
static constexpr size_t KEYBOARD_BUFFER_SIZE = 256; ///< Size of the key buffer

/**
 * @brief PS/2 keyboard controller
 *
 * Handles PS/2 keyboard input, manages an internal key buffer, and
 * provides a singleton interface for reading key presses.
 */
class PS2Keyboard {
private:
  char buffer[KEYBOARD_BUFFER_SIZE]; ///< Circular buffer for pressed keys
  size_t head = 0;                   ///< Head index of the buffer
  size_t tail = 0;                   ///< Tail index of the buffer
  bool shift_pressed = false;        ///< Track shift key state

  PS2Keyboard() = default; ///< Private constructor for singleton

  /**
   * @brief Push a character into the internal buffer
   * @param c Character to push
   */
  void push_char(char c);

  /**
   * @brief Handle an incoming scancode from the PS/2 controller
   * @param scancode Scancode byte
   */
  void handle_scancode(uint8_t scancode);

public:
  /**
   * @brief Get the singleton instance of the PS2Keyboard
   * @return Reference to the keyboard instance
   */
  static PS2Keyboard &the() {
    static PS2Keyboard instance;
    return instance;
  }

  /**
   * @brief Initialize the PS/2 keyboard
   */
  void initialize();

  /**
   * @brief PS/2 IRQ handler to be called on keyboard interrupts
   */
  void irq_handler();

  /**
   * @brief Check if a key is available in the buffer
   * @return true if a key is available, false otherwise
   */
  bool has_key() const;

  /**
   * @brief Pop a key from the buffer
   * @return The next available character
   */
  char pop_key();
};
