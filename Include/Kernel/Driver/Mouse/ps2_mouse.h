#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

static constexpr size_t MOUSE_BUFFER_SIZE = 256;

struct MouseEvent {
    int8_t  dx;
    int8_t  dy;
    int8_t  scroll;
    uint8_t buttons; // bit0=left, bit1=right, bit2=middle
};

class PS2Mouse final : public fkernel::CharacterDevice {
  uint8_t m_packet[4]{};
  uint8_t m_packet_byte{0};
  uint8_t m_packet_size{3};
  bool m_hw_initialized{false};
  fk::containers::Vector<MouseEvent> m_events;
  fk::synchronization::Spinlock m_lock;

public:
  static PS2Mouse& the();

  PS2Mouse();
  virtual ~PS2Mouse() override = default;

  void initialize();
  void irq_handler();

  virtual fk::core::Result<void, fk::core::Error> on_open() override;

  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t, uint8_t*) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t, const uint8_t*) override {
      return fk::core::Error::NotImplemented;
  }
  virtual size_t size() const override { return 0; }

private:
  void send_command(uint8_t cmd);
  void process_packet();
};
