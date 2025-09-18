#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupt/pic.h>
#include <Kernel/Arch/x86_64/io.h>

void Pic8259::remap(uint8_t offset1, uint8_t offset2) {
  uint8_t mask1 = inb(PIC1_DATA);
  uint8_t mask2 = inb(PIC2_DATA);

  outb(PIC1_CMD, 0x11);
  io_wait();
  outb(PIC2_CMD, 0x11);
  io_wait();

  outb(PIC1_DATA, offset1);
  io_wait();
  outb(PIC2_DATA, offset2);
  io_wait();

  outb(PIC1_DATA, 0x04); // escravo ligado no IRQ2
  io_wait();
  outb(PIC2_DATA, 0x02);
  io_wait();

  outb(PIC1_DATA, 0x01);
  io_wait();
  outb(PIC2_DATA, 0x01);
  io_wait();

  outb(PIC1_DATA, mask1);
  outb(PIC2_DATA, mask2);
}

void Pic8259::sendEOI(uint8_t irq) {
  if (irq >= 8)
    outb(PIC2_CMD, EOI);
  outb(PIC1_CMD, EOI);
}

void Pic8259::set_mask(uint8_t irq) {
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  irq = (irq < 8) ? irq : irq - 8;
  uint8_t value = inb(port) | (1 << irq);
  outb(port, value);
}

void Pic8259::clear_mask(uint8_t irq) {
  uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
  irq = (irq < 8) ? irq : irq - 8;
  uint8_t value = inb(port) & ~(1 << irq);
  outb(port, value);
}

uint8_t Pic8259::get_mask_master() { return inb(PIC1_DATA); }

uint8_t Pic8259::get_mask_slave() { return inb(PIC2_DATA); }

void Pic8259::set_mask_master(uint8_t mask) { outb(PIC1_DATA, mask); }

void Pic8259::set_mask_slave(uint8_t mask) { outb(PIC2_DATA, mask); }
