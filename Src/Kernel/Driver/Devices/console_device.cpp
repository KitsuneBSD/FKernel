#include <Kernel/Driver/Devices/console_device.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#include <LibFK/Algorithms/log.h>

int ConsoleDevice::open(VNode *vnode, FileDescriptor *fd, int flags) {
  (void)vnode;
  (void)fd;
  (void)flags;
  fk::algorithms::klog("CONSOLE", "Opened /dev/console");
  return 0;
}

int ConsoleDevice::close(VNode *vnode, FileDescriptor *fd) {
  (void)vnode;
  (void)fd;
  fk::algorithms::klog("CONSOLE", "Closed /dev/console");
  return 0;
}

int ConsoleDevice::write(VNode *vnode, FileDescriptor *fd, const void *buffer,
                         size_t size, size_t offset) {
  (void)vnode;
  (void)fd;
  (void)offset;

  const char *str = reinterpret_cast<const char *>(buffer);
  auto &vga = vga::the();

  for (size_t i = 0; i < size; ++i)
    vga.put_char(str[i]);

  return static_cast<int>(size);
}

int ConsoleDevice::read(VNode *vnode, FileDescriptor *fd, void *buffer,
                        size_t size, size_t offset) {
  (void)vnode;
  (void)fd;
  (void)offset;

  char *buf = reinterpret_cast<char *>(buffer);
  size_t bytes = 0;

  while (bytes < size && PS2Keyboard::the().has_key()) {
    buf[bytes++] = PS2Keyboard::the().pop_key();
  }

  return static_cast<int>(bytes);
}

VNodeOps ConsoleDevice::ops = {
    .read = &ConsoleDevice::read,
    .write = &ConsoleDevice::write,
    .open = &ConsoleDevice::open,
    .close = &ConsoleDevice::close,
    .lookup = nullptr,
    .create = nullptr,
    .readdir = nullptr,
    .unlink = nullptr,
};
