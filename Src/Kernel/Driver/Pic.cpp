#include <Driver/Pic.h>

void remap(int offset1, int offset2) {
  uint8_t mask1 = inb(PIC1_DATA);
  uint8_t mask2 = inb(PIC2_DATA);

  outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
  outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

  outb(PIC1_DATA, offset1); // PIC1 offset
  outb(PIC2_DATA, offset2); // PIC2 offset

  outb(PIC1_DATA, 4); // PIC1 tem PIC2 na IRQ2
  outb(PIC2_DATA, 2); // PIC2 está conectado na IRQ2 do PIC1

  outb(PIC1_DATA, ICW4_8086);
  outb(PIC2_DATA, ICW4_8086);

  outb(PIC1_DATA, mask1);
  outb(PIC2_DATA, mask2);
}

void send_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC2_CMD, 0x20);
  }
  outb(PIC1_CMD, 0x20);
}

void set_mask(uint8_t mask) { outb(PIC1_DATA, mask); }

uint8_t get_mask() { return inb(PIC1_DATA); }

void disable_pic() {
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);
}
