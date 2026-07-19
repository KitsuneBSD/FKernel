#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Driver/Pty/pty_buffer.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

// Master side of a pseudo-terminal pair.
// Writing to master delivers data to slave's read; reading from master
// returns data written by the slave.
class PtyMaster final : public CharacterDevice {
public:
  PtyMaster(fk::RefPtr<PtyBuffer> to_slave, fk::RefPtr<PtyBuffer> from_slave);
  virtual ~PtyMaster() override = default;

  fk::core::Result<size_t, fk::core::Error>
  read(uint64_t offset, size_t size, uint8_t* buf) override;

  fk::core::Result<size_t, fk::core::Error>
  write(uint64_t offset, size_t size, const uint8_t* buf) override;

  size_t size() const override { return 0; }
  bool is_directory() const override { return false; }
  bool is_character_device() const override { return true; }

private:
  fk::RefPtr<PtyBuffer> m_to_slave;
  fk::RefPtr<PtyBuffer> m_from_slave;
};

} // namespace fkernel
