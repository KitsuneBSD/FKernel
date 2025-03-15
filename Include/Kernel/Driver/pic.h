#include "../../../Include/Kernel/LibK/port_io.h"
#include "../../../Include/Kernel/LibK/stdint.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21

#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28

#define PIC_READ_IRR 0x0a
#define PIC_READ_ISR 0x0b

static uint16_t __pic_get_irq_reg(int ocw3) {
  outb(PIC1_COMMAND, ocw3);
  outb(PIC2_COMMAND, ocw3);
  return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

void pic_init();
void pic_disable();

void pic_remask(int offset1, int offset2);

void irq_set_mask(uint8_t irqline);
void irq_clear_mask(uint8_t irqline);

void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);

void pic_sendeoi(uint8_t);
