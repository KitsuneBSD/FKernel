#pragma once

#include <Kernel/Driver/SerialPort/serial.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#include <LibC/stdarg.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int snprintf(char *str, size_t size, const char *fmt, ...);
void kprintf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
