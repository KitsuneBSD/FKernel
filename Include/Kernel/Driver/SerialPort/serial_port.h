#pragma once

#include <LibFK/Types/types.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/io.h>
#endif // __x86_64__

/**
 * @brief Basic serial port interface (COM1)
 *
 * Provides low-level access to the serial port for debugging and
 * kernel output. Supports character output, string output,
 * and numeric formatting (decimal and hexadecimal).
 */
class serial {
private:
  static constexpr uint16_t COM1 = 0x3F8; ///< COM1 base I/O port

  /**
   * @brief Check if the transmit buffer is empty
   * @return true if ready to transmit, false otherwise
   */
  static inline bool is_transmit_empty() { return inb(COM1 + 5) & 0x20; }

public:
  /**
   * @brief Initialize the serial port
   *
   * Configures baud rate, line control, and enables interrupts if needed.
   */
  static void init();

  /**
   * @brief Write a single character to the serial port
   * @param c Character to write
   */
  static void write_char(char c);

  /**
   * @brief Write a null-terminated string to the serial port
   * @param str String to write
   */
  static void write(const char *str);

  /**
   * @brief Write a signed integer in decimal format to the serial port
   * @param value Integer value to write
   */
  static void write_dec(int64_t value);

  /**
   * @brief Write an unsigned integer in hexadecimal format to the serial port
   * @param value Integer value to write
   */
  static void write_hex(uint64_t value);
};
