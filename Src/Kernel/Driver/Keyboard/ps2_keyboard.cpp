#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>

// Row helper: 16 entries
#define R16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p

fk::core::Result<size_t, fk::core::Error> PS2Keyboard::read([[maybe_unused]] uint64_t offset, size_t size, uint8_t* out_buffer) {
    if (size == 0) return 0;
    if (!out_buffer) return fk::core::Error::InvalidParameter;

    size_t count = 0;
    while (count < size) {
        if (has_key()) {
            out_buffer[count++] = static_cast<uint8_t>(pop_key());
            // Return immediately if we have at least one character
            if (count > 0) return count;
        } else {
            if (count > 0) return count;
            SchedulerManager::the().yield();
        }
    }
    return count;
}

fk::core::Result<size_t, fk::core::Error> PS2Keyboard::write(uint64_t, size_t, const uint8_t*) {
    return fk::core::Error::PermissionDenied;
}

// Layout US QWERTY
static const char us_set1[128] = {
    R16(0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t'), // 0x00
    R16('q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's'),  // 0x10
    R16('d', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v'), // 0x20
    R16('b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),               // 0x30
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x40
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x50
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x60
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)                                // 0x70
};

static const char us_set1_shift[128] = {
    R16(0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t'), // 0x00
    R16('Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S'),  // 0x10
    R16('D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V'),  // 0x20
    R16('B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),               // 0x30
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x40
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x50
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x60
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)                                // 0x70
};

// Layout ABNT2
static const char abnt2_set1[128] = {
    R16(0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t'), // 0x00
    R16('q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0, '[', '\n', 0, 'a', 's'),   // 0x10 (0x1A: acute)
    R16('d', 'f', 'g', 'h', 'j', 'k', 'l', 'c', '~', '\'', 0, ']', 'z', 'x', 'c', 'v'), // 0x20 (0x27: ç -> c, 0x2B: ])
    R16('b', 'n', 'm', ',', '.', ';', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),               // 0x30 (0x35: ;)
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x40
    R16(0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, 0, 0, 0, 0, 0, 0),                            // 0x50 (0x56: \)
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x60
    R16(0, 0, 0, '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)                               // 0x70 (0x73: /)
};

static const char abnt2_set1_shift[128] = {
    R16(0, 27, '!', '@', '#', '$', '%', 0, '&', '*', '(', ')', '_', '+', '\b', '\t'),   // 0x00
    R16('Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0, '{', '\n', 0, 'A', 'S'),   // 0x10 (0x1A: grave)
    R16('D', 'F', 'G', 'H', 'J', 'K', 'L', 'C', '^', '"', 0, '}', 'Z', 'X', 'C', 'V'),  // 0x20 (0x27: Ç -> C, 0x2B: })
    R16('B', 'N', 'M', '<', '>', ':', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0),               // 0x30 (0x35: :)
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x40
    R16(0, 0, 0, 0, 0, 0, '|', 0, 0, 0, 0, 0, 0, 0, 0, 0),                             // 0x50 (0x56: |)
    R16(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                               // 0x60
    R16(0, 0, 0, '?', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)                               // 0x70 (0x73: ?)
};

void PS2Keyboard::push_char(char c) {
  size_t next = (head + 1) % KEYBOARD_BUFFER_SIZE;
  if (next != tail) {
    buffer[head] = c;
    head = next;
  }
}

bool PS2Keyboard::has_key() {
  if (head == tail) {
    // Polling fallback if interrupts are disabled/slow
    if (inb(PS2_STATUS_PORT) & 1) {
      uint8_t scancode = inb(PS2_DATA_PORT);
      handle_scancode(scancode);
    }
  }
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
    return;
  }

  if (key_released)
    return;

  char c = 0;
  if (m_layout == KeyboardLayout::ABNT2) {
      c = shift_pressed ? abnt2_set1_shift[keycode] : abnt2_set1[keycode];
  } else {
      c = shift_pressed ? us_set1_shift[keycode] : us_set1[keycode];
  }

  if (c) {
    // fk::algorithms::klog("KEYBOARD", "Key: %c (keycode: 0x%X)", c, keycode);
    push_char(c);
  } else {
    // fk::algorithms::kdebug("KEYBOARD", "Unmapped keycode: 0x%X", keycode);
  }
}

void PS2Keyboard::irq_handler() {
  uint8_t scancode = inb(PS2_DATA_PORT);
  handle_scancode(scancode);
}

void PS2Keyboard::initialize() {
  m_layout = KeyboardLayout::ABNT2;
  HardwareInterruptManager::the().unmask_interrupt(1);
  fk::algorithms::klog("KEYBOARD", "PS/2 keyboard initialized on IRQ1 (Unmasked, ABNT2, Polling Fallback)");
}