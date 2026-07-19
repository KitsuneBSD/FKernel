#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

static constexpr size_t MOUSE_BUFFER_SIZE = 256;

struct MouseEvent {
    int8_t  dx;
    int8_t  dy;
    uint8_t buttons; // bit0=left, bit1=right, bit2=middle
};

class PS2Mouse final : public fkernel::CharacterDevice {
  uint8_t m_packet[3]{};
  uint8_t m_packet_byte{0};
  fk::containers::Vector<MouseEvent> m_events;
  fk::synchronization::Spinlock m_lock;

public:
  static PS2Mouse& the();

  PS2Mouse();
  virtual ~PS2Mouse() override = default;

  void initialize();
  void irq_handler();

  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
      return fk::core::Error::NotImplemented;
  }
  virtual size_t size() const override { return 0; }

private:
  void send_command(uint8_t cmd);
  void process_packet();
};
