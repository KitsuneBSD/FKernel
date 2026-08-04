#pragma once

#include <Kernel/Driver/Device/BlockDevice/luks_header.h>
#include <Kernel/Driver/Device/BlockDevice/stackable_block_device.h>
#include <LibFK/Algorithms/Crypto/aes_xts.h>
#include <LibFK/Types/types.h>

namespace fkernel {

// dm-crypt block device: AES-256-XTS over a LUKS v1 container.
// Wraps one child BlockDevice (the raw ciphertext device).
class DmCryptDevice : public StackableBlockDevice {
    fk::algorithms::AesXtsKey m_xts;
    uint32_t m_payload_offset{0};
    uint32_t m_sector_size{512};
    bool     m_unlocked{false};

    fk::core::Result<void, fk::core::Error> read_header(LuksHeader& hdr);
    fk::core::Result<bool, fk::core::Error> try_key_slot(const LuksHeader& hdr,
                                                          uint32_t slot,
                                                          const uint8_t* pw, size_t pwlen);

public:
    DmCryptDevice() = default;

    // Unlock the container with `password`. Must be called before I/O.
    fk::core::Result<void, fk::core::Error> unlock(const uint8_t* pw, size_t pwlen);

    fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start, size_t count, uint8_t* buf) override;

    fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start, size_t count, const uint8_t* buf) override;

    SectorSize  sector_size()  const override { return SectorSize(m_sector_size); }
    SectorCount sector_count() const override;
};

} // namespace fkernel
