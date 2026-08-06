#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/new.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Driver/Device/BlockDevice/raid_device.h>

namespace fkernel {

fk::RefPtr<RaidDevice> RaidDevice::create_raid0(size_t stripe_sectors) {
    return fk::make_ref<RaidDevice>(RaidMode::Stripe, stripe_sectors).value();
}

fk::RefPtr<RaidDevice> RaidDevice::create_raid1() {
    return fk::make_ref<RaidDevice>(RaidMode::Mirror, 0).value();
}

SectorSize RaidDevice::sector_size() const {
    if (m_children.is_empty()) return SectorSize(512);
    return m_children[0]->sector_size();
}

SectorCount RaidDevice::sector_count() const {
    if (m_children.is_empty()) return SectorCount(0);

    uint64_t min_sectors = m_children[0]->sector_count().value();
    for (size_t i = 1; i < m_children.size(); ++i) {
        uint64_t n = m_children[i]->sector_count().value();
        if (n < min_sectors) min_sectors = n;
    }

    if (m_mode == RaidMode::Stripe)
        return SectorCount(min_sectors * m_children.size());
    return SectorCount(min_sectors); // RAID 1: mirror capacity
}

fk::core::Result<size_t, fk::core::Error>
RaidDevice::read_sectors(uint64_t start, size_t count, uint8_t* buf) {
    if (m_children.is_empty()) return fk::core::Error::NotFound;

    size_t ss = sector_size().value();
    size_t done = 0;

    if (m_mode == RaidMode::Mirror) {
        size_t idx = m_read_idx % m_children.size();
        m_read_idx++;
        return m_children[idx]->read_sectors(start, count, buf);
    }

    // RAID 0 stripe read: may span chunk boundaries and child boundaries.
    while (done < count) {
        uint64_t sector = start + done;
        size_t n = m_children.size();
        size_t cs = m_stripe_sectors;

        uint64_t chunk_in_stripe = sector / cs;
        size_t   child           = static_cast<size_t>(chunk_in_stripe % n);
        uint64_t child_chunk     = chunk_in_stripe / n;
        size_t   offset_in_chunk = static_cast<size_t>(sector % cs);

        uint64_t child_sector = child_chunk * cs + offset_in_chunk;
        size_t   avail        = cs - offset_in_chunk; // sectors left in this chunk
        size_t   to_read      = count - done;
        if (to_read > avail) to_read = avail;

        auto res = m_children[child]->read_sectors(child_sector, to_read,
                                                    buf + done * ss);
        if (res.is_error()) return res.error();
        done += res.value();
    }
    return done;
}

fk::core::Result<size_t, fk::core::Error>
RaidDevice::write_sectors(uint64_t start, size_t count, const uint8_t* buf) {
    if (m_children.is_empty()) return fk::core::Error::NotFound;

    size_t ss = sector_size().value();
    size_t done = 0;

    if (m_mode == RaidMode::Mirror) {
        size_t written = 0;
        for (size_t i = 0; i < m_children.size(); ++i) {
            auto res = m_children[i]->write_sectors(start, count, buf);
            if (res.is_error()) {
                fk::algorithms::kdebug("RAID1", "mirror child %zu write failed", i);
                continue; // degraded mode: skip failed child
            }
            if (i == 0) written = res.value();
        }
        return written;
    }

    // RAID 0 stripe write
    while (done < count) {
        uint64_t sector = start + done;
        size_t n = m_children.size();
        size_t cs = m_stripe_sectors;

        uint64_t chunk_in_stripe = sector / cs;
        size_t   child           = static_cast<size_t>(chunk_in_stripe % n);
        uint64_t child_chunk     = chunk_in_stripe / n;
        size_t   offset_in_chunk = static_cast<size_t>(sector % cs);

        uint64_t child_sector = child_chunk * cs + offset_in_chunk;
        size_t   avail        = cs - offset_in_chunk;
        size_t   to_write     = count - done;
        if (to_write > avail) to_write = avail;

        auto res = m_children[child]->write_sectors(child_sector, to_write,
                                                     buf + done * ss);
        if (res.is_error()) return res.error();
        done += res.value();
    }
    return done;
}

} // namespace fkernel
