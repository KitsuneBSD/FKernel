#pragma once
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <LibFK/Memory/retain_ptr.h>

/**
 * @brief Virtual TTY interface for a serial port
 *
 * Implements a simple VFS-compatible interface for reading and writing
 * to a serial port. Writing sends data directly to the serial device.
 * Reading is not implemented yet.
 */
struct SerialTTY {
  /**
   * @brief Open the serial TTY device
   *
   * @param vnode VNode representing the serial device
   * @param fd File descriptor
   * @param flags Open flags
   * @return Always returns 0 (success)
   */
  static int open(VNode *vnode, FileDescriptor *fd, int flags) {
    (void)vnode;
    (void)fd;
    (void)flags;
    return 0;
  }

  /**
   * @brief Close the serial TTY device
   *
   * @param vnode VNode representing the serial device
   * @param fd File descriptor
   * @return Always returns 0 (success)
   */
  static int close(VNode *vnode, FileDescriptor *fd) {
    (void)fd;
    (void)vnode;
    return 0;
  }

  /**
   * @brief Write data to the serial port
   *
   * @param vnode VNode representing the serial device
   * @param fd File descriptor
   * @param buf Pointer to data to write
   * @param size Number of bytes to write
   * @param offset File offset (ignored)
   * @return Number of bytes written
   */
  static int write(VNode *vnode, FileDescriptor *fd, const void *buf,
                   size_t size, [[maybe_unused]] size_t offset) {
    (void)fd;
    (void)vnode;
    const char *str = reinterpret_cast<const char *>(buf);
    for (size_t i = 0; i < size; ++i)
      serial::write_char(str[i]);
    return static_cast<int>(size);
  }

  /**
   * @brief Read data from the serial port
   *
   * @param vnode VNode representing the serial device
   * @param fd File descriptor
   * @param buf Destination buffer
   * @param size Number of bytes to read
   * @param offset File offset (ignored)
   * @return Always returns 0 (not implemented)
   */
  static int read(VNode *vnode, FileDescriptor *fd, void *buf, size_t size,
                  size_t offset) {
    (void)vnode;
    (void)fd;
    (void)buf;
    (void)size;
    (void)offset;
    return 0; // Not implemented yet, could use UART circular buffer
  }

  /// Virtual node operations table for the serial TTY device
  static VNodeOps ops;
};

