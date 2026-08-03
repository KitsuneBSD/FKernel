#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Pty/pty_buffer.h>
#include <Kernel/Driver/Pty/pty_line_discipline.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

namespace fkernel {

// Slave side of a pseudo-terminal pair.
// Reading from slave delivers data written by the master; writing to slave
// delivers data to the master's read.
class PtySlave final : public CharacterDevice {
public:
  PtySlave(fk::RefPtr<PtyBuffer> from_master, fk::RefPtr<PtyBuffer> to_master,
           uint32_t index, PtyLineDiscipline* ldisc = nullptr);
  virtual ~PtySlave() override = default;

  fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t* buf) override;

  fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t* buf) override;

  size_t size() const override { return 0; }
  bool is_directory() const override { return false; }
  bool is_character_device() const override { return true; }
  short poll() const override {
    short r = POLLOUT;
    if (m_from_master && !m_from_master->is_empty()) r |= POLLIN;
    return r;
  }

private:
  fk::RefPtr<PtyBuffer> m_from_master;
  fk::RefPtr<PtyBuffer> m_to_master;
  PtyLineDiscipline* m_ldisc{nullptr};
};

} // namespace fkernel
