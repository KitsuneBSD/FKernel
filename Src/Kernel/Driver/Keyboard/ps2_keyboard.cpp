#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Hardware/Cpu.h>

// Layout US QWERTY simplificado
static const char scancode_set1[128] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0,
};

static const char scancode_set1_shift[128] = {
    0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ', 0,
};

void PS2Keyboard::push_char(char c) {
  size_t next = (head + 1) % KEYBOARD_BUFFER_SIZE;
  if (next != tail) {
    buffer[head] = c;
    head = next;
  }
}

bool PS2Keyboard::has_key() const { return head != tail; }

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
    return;
  }

  if (key_released)
    return;

  char c =
      shift_pressed ? scancode_set1_shift[keycode] : scancode_set1[keycode];
  if (c)
    push_char(c);
}

void PS2Keyboard::irq_handler() {
  uint8_t scancode = inb(PS2_DATA_PORT);
  handle_scancode(scancode);
}

void PS2Keyboard::initialize() {
  klog("Keyboard", "PS/2 keyboard initialized on IRQ1");
}
