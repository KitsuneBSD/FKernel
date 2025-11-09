#pragma once

#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#include <LibC/stdarg.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write formatted data to a string using a variable argument list.
 *
 * @param str  Pointer to the buffer where the formatted string will be stored.
 * @param size Maximum number of characters to write, including the null
 * terminator.
 * @param fmt  Format string specifying how to format the output.
 * @param args Variable argument list initialized with ::va_start.
 * @return The number of characters that would have been written if @p size was
 * unlimited, not including the terminating null byte. If the return value is >=
 * @p size, the output was truncated.
 *
 * @note This is the low-level implementation used by snprintf and kprintf.
 */
int vsnprintf(char *str, size_t size, const char *fmt, va_list args);

/**
 * @brief Write formatted data to a string.
 *
 * @param str  Pointer to the buffer where the formatted string will be stored.
 * @param size Maximum number of characters to write, including the null
 * terminator.
 * @param fmt  Format string specifying how to format the output.
 * @param ...  Variable arguments corresponding to @p fmt.
 * @return The number of characters that would have been written if @p size was
 * unlimited, not including the terminating null byte. If the return value is >=
 * @p size, the output was truncated.
 *
 * @note This is a safer version of sprintf since it requires a buffer size.
 */
int snprintf(char *str, size_t size, const char *fmt, ...);

/**
 * @brief Print formatted output to the kernel console.
 *
 * The output is written to both the VGA text buffer and the serial port,
 * making it useful for debugging even when VGA is unavailable.
 *
 * @param fmt Format string specifying how to format the output.
 * @param ... Variable arguments corresponding to @p fmt.
 */
void kprintf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
