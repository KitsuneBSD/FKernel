#include "../../../../Include/Kernel/Driver/pic.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

void pic_init() {
  print_str("Init PIC\n");
  // Desabilitar interrupções (envia 0xFF nas portas de dados)
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);

  // Enviar ICW1 para iniciar a sequência de inicialização
  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();

  // Enviar ICW2 para definir os vetores de interrupção
  outb(PIC1_DATA,
       PIC1_OFFSET); // Definir o vetor de interrupção para o PIC mestre
  io_wait();
  outb(PIC2_DATA,
       PIC2_OFFSET); // Definir o vetor de interrupção para o PIC escravo
  io_wait();

  // Enviar ICW3 para configurar o mapeamento mestre/escravo
  outb(PIC1_DATA, 0x04); // O PIC mestre controla a linha 2
  io_wait();
  outb(PIC2_DATA, 0x02); // O PIC escravo está na linha 2 do PIC mestre
  io_wait();

  // Enviar ICW4 para configurar o modo 8086
  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  // Habilitar as interrupções no PIC mestre e escravo
  outb(PIC1_DATA, 0x00); // Habilitar todas as interrupções no PIC mestre
  outb(PIC2_DATA, 0x00); // Habilitar todas as interrupções no PIC escravo
  print_str("PIC Loaded\n");
}

void PIC_remap(int offset1, int offset2) {
  outb(PIC1_COMMAND,
       ICW1_INIT | ICW1_ICW4); // Inicia a sequência de inicialização
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC1_DATA, offset1); // Offset do PIC mestre
  io_wait();
  outb(PIC2_DATA, offset2); // Offset do PIC escravo
  io_wait();
  outb(PIC1_DATA, 4); // ICW3: PIC mestre conectado ao PIC escravo via IRQ2
  io_wait();
  outb(PIC2_DATA, 2); // ICW3: PIC escravo no IRQ2
  io_wait();
  outb(PIC1_DATA, ICW4_8086); // Modo 8086
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();
  outb(PIC1_DATA, 0); // Desmascarar todas as interrupções
  outb(PIC2_DATA, 0); // Desmascarar todas as interrupções
}

void IRQ_set_mask(uint8_t IRQline) {
  uint16_t port;
  uint8_t value;
  if (IRQline < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    IRQline -= 8;
  }
  value = inb(port) | (1 << IRQline);
  outb(port, value);
}

void IRQ_clear_mask(uint8_t IRQline) {
  uint16_t port;
  uint8_t value;
  if (IRQline < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    IRQline -= 8;
  }
  value = inb(port) & ~(1 << IRQline);
  outb(port, value);
}

void pic_mask(uint8_t irq) {
  uint16_t port;
  uint8_t value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq -= 8;
  }

  value = inb(port) | (1 << irq);
  outb(port, value);
}

void pic_unmask(uint8_t irq) {
  uint16_t port;
  uint8_t value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq -= 8;
  }

  value = inb(port) & ~(1 << irq);
  outb(port, value);
}

void pic_send_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC2_COMMAND, 0x20); // Enviar EOI para o PIC escravo
  }
  outb(PIC1_COMMAND, 0x20); // Enviar EOI para o PIC mestre
}

uint16_t pic_get_irr(void) { return __pic_get_irq_reg(PIC_READ_IRR); }

uint16_t pic_get_isr(void) { return __pic_get_irq_reg(PIC_READ_ISR); }

void pic_disable() {
  print_str("Disable PIC\n");
  outb(PIC1_DATA, 0xff); // Mascarar todas as interrupções no PIC mestre
  outb(PIC2_DATA, 0xff); // Mascarar todas as interrupções no PIC escravo
  print_str("Pic Disabled\n");
}
