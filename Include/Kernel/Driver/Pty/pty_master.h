#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Pty/pty_buffer.h>
#include <Kernel/Driver/Pty/pty_line_discipline.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

class PtyMaster final : public CharacterDevice {
public:
  PtyMaster(fk::RefPtr<PtyBuffer> to_slave, fk::RefPtr<PtyBuffer> from_slave,
            uint32_t index = 0);
  virtual ~PtyMaster() override = default;

  fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t* buf) override;

  fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t* buf) override;

  fk::core::Result<int, fk::core::Error> ioctl(uint64_t request, uint64_t arg) override;

  PtyLineDiscipline& ldisc() { return m_ldisc; }

  size_t size() const override { return 0; }
  bool is_directory() const override { return false; }
  bool is_character_device() const override { return true; }
  short poll() const override {
    short r = POLLOUT;
    if (m_from_slave && !m_from_slave->is_empty()) r |= POLLIN;
    return r;
  }

private:
  fk::RefPtr<PtyBuffer> m_to_slave;
  fk::RefPtr<PtyBuffer> m_from_slave;
  PtyLineDiscipline m_ldisc;
  uint16_t m_rows{24};
  uint16_t m_cols{80};
};

}
