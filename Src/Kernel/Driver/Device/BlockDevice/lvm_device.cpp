#include <Kernel/Driver/Device/BlockDevice/lvm_device.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

fk::RefPtr<LvmDevice> LvmDevice::create() {
    return fk::make_ref<LvmDevice>().value();
}

void LvmDevice::add_segment(uint64_t lv_start, uint64_t length,
                             size_t pv_index, uint64_t pv_start) {
    LvmSegment seg;
    seg.lv_start  = lv_start;
    seg.lv_end    = lv_start + length;
    seg.pv_index  = pv_index;
    seg.pv_start  = pv_start;
    m_segments.push_back(seg);
    if (seg.lv_end > m_total_sectors) m_total_sectors = seg.lv_end;
}

void LvmDevice::add_stripe_segments(uint64_t lv_start_sectors, uint64_t total_sectors,
                                    size_t pv_count, const uint64_t* pv_base_starts,
                                    uint64_t extent_size_sectors) {
    uint64_t lv_pos = lv_start_sectors;
    uint64_t remaining = total_sectors;
    size_t   pv_idx = 0;
    fk::containers::Vector<uint64_t> pv_offsets;
    pv_offsets.resize(pv_count);
    for (size_t i = 0; i < pv_count; ++i) pv_offsets[i] = pv_base_starts[i];

    while (remaining > 0) {
        uint64_t chunk = remaining < extent_size_sectors ? remaining : extent_size_sectors;
        add_segment(lv_pos, chunk, pv_idx % pv_count, pv_offsets[pv_idx % pv_count]);
        pv_offsets[pv_idx % pv_count] += chunk;
        lv_pos   += chunk;
        remaining -= chunk;
        ++pv_idx;
    }
}

const LvmSegment* LvmDevice::find_segment(uint64_t lv_sector) const {
    // Linear scan; segments are typically few dozens — acceptable.
    for (size_t i = 0; i < m_segments.size(); ++i) {
        const auto& s = m_segments[i];
        if (lv_sector >= s.lv_start && lv_sector < s.lv_end)
            return &s;
    }
    return nullptr;
}

SectorSize LvmDevice::sector_size() const {
    if (m_children.is_empty()) return SectorSize(512);
    return m_children[0]->sector_size();
}

SectorCount LvmDevice::sector_count() const {
    return SectorCount(m_total_sectors);
}

fk::core::Result<size_t, fk::core::Error>
LvmDevice::read_sectors(uint64_t start, size_t count, uint8_t* buf) {
    size_t ss = sector_size().value();
    size_t done = 0;

    while (done < count) {
        uint64_t lv_sector = start + done;
        const LvmSegment* seg = find_segment(lv_sector);
        if (!seg) return fk::core::Error::IOError;
        if (seg->pv_index >= m_children.size()) return fk::core::Error::InvalidParameter;

        uint64_t offset_in_seg  = lv_sector - seg->lv_start;
        uint64_t pv_sector      = seg->pv_start + offset_in_seg;
        uint64_t avail_in_seg   = seg->lv_end - lv_sector;
        size_t   to_read        = count - done;
        if (static_cast<uint64_t>(to_read) > avail_in_seg)
            to_read = static_cast<size_t>(avail_in_seg);

        auto res = m_children[seg->pv_index]->read_sectors(pv_sector, to_read,
                                                            buf + done * ss);
        if (res.is_error()) return res.error();
        done += res.value();
    }
    return done;
}

fk::core::Result<size_t, fk::core::Error>
LvmDevice::write_sectors(uint64_t start, size_t count, const uint8_t* buf) {
    size_t ss = sector_size().value();
    size_t done = 0;

    while (done < count) {
        uint64_t lv_sector = start + done;
        const LvmSegment* seg = find_segment(lv_sector);
        if (!seg) return fk::core::Error::IOError;
        if (seg->pv_index >= m_children.size()) return fk::core::Error::InvalidParameter;

        uint64_t offset_in_seg  = lv_sector - seg->lv_start;
        uint64_t pv_sector      = seg->pv_start + offset_in_seg;
        uint64_t avail_in_seg   = seg->lv_end - lv_sector;
        size_t   to_write       = count - done;
        if (static_cast<uint64_t>(to_write) > avail_in_seg)
            to_write = static_cast<size_t>(avail_in_seg);

        auto res = m_children[seg->pv_index]->write_sectors(pv_sector, to_write,
                                                             buf + done * ss);
        if (res.is_error()) return res.error();
        done += res.value();
    }
    return done;
}

} // namespace fkernel
