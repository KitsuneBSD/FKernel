#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Driver/Terminal/terminal_manager.h>
#include <Kernel/Driver/Keyboard/keymap_manager.h>
#include <LibFK/Algorithms/log.h>

fk::core::Result<size_t, fk::core::Error> PS2Keyboard::read([[maybe_unused]] uint64_t offset, [[maybe_unused]] size_t size, [[maybe_unused]] uint8_t* out_buffer) {
    return fk::core::Error::PermissionDenied;
}

fk::core::Result<size_t, fk::core::Error> PS2Keyboard::write(uint64_t, size_t, const uint8_t*) {
    return fk::core::Error::PermissionDenied;
}

void PS2Keyboard::push_char(char c) {
  size_t next = (head + 1) % KEYBOARD_BUFFER_SIZE;
  if (next != tail) {
    buffer[head] = c;
    head = next;
  }
}

bool PS2Keyboard::has_key() {
  return head != tail;
}

char PS2Keyboard::pop_key() {
  if (head == tail)
    return 0;

  char c = buffer[tail];
  tail = (tail + 1) % KEYBOARD_BUFFER_SIZE;
  return c;
}

void PS2Keyboard::handle_scancode(uint8_t scancode) {
  bool key_released = scancode & 0x80;
  uint8_t keycode = scancode & 0x7F;

  if (keycode == 42 || keycode == 54) { // shift
    shift_pressed = !key_released;
    fk::algorithms::kdebug("KBD", "Shift %s", shift_pressed ? "PRESS" : "release");
    return;
  }

  if (keycode == 56) { // alt
    alt_pressed = !key_released;
    fk::algorithms::kdebug("KBD", "Alt %s", alt_pressed ? "PRESS" : "release");
    return;
  }

  if (keycode == 29) { // ctrl
    ctrl_pressed = !key_released;
    fk::algorithms::kdebug("KBD", "Ctrl %s", ctrl_pressed ? "PRESS" : "release");
    return;
  }

  // Handle TTY switching (F1-F6)
  if (!key_released && (keycode >= 0x3B && keycode <= 0x40)) {
    int tty_index = keycode - 0x3B;
    fkernel::terminal::TerminalManager::the().switch_to(tty_index);
    return;
  }

  if (key_released)
    return;

  // Delegate translation to the KeymapManager
  char c = fkernel::drivers::KeymapManager::the().translate(keycode, shift_pressed, alt_pressed, ctrl_pressed);

  if (c) {
    fk::algorithms::kdebug("KBD", "Scancode 0x%02X -> char 0x%02X (ctrl=%d shift=%d alt=%d)", scancode, (unsigned char)c, ctrl_pressed, shift_pressed, alt_pressed);
    fkernel::terminal::TerminalManager::the().handle_input(c);
  }
}

void PS2Keyboard::irq_handler() {
  uint8_t scancode = inb(PS2_DATA_PORT);
  handle_scancode(scancode);
}

void PS2Keyboard::initialize() {
  set_layout(fkernel::drivers::KeyboardLayout::US_INTL);
  HardwareInterruptManager::the().unmask_interrupt(1);
  fk::algorithms::klog("KEYBOARD", "PS/2 keyboard initialized on IRQ1 (Unmasked, US_INTL, Compose ON)");
}
