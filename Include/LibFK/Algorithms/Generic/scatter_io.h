#pragma once

#include <LibFK/Algorithms/Generic/math.h>
#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>
#include <LibFK/Utilities/memory.h>

namespace fk {
namespace algorithms {

// Chunked block I/O over a logical file whose blocks map to physical blocks
// through a caller-supplied resolver. `resolve_block(lblock)` returns the
// physical block index, or 0 for a sparse hole. `read_block(phys, scratch)`
// reads one full block (block_size bytes) into scratch. `scratch` must hold
// at least block_size bytes.
//
// The resolver and I/O callbacks are per-filesystem; the chunking math, slice
// copy, and sparse zero-fill are shared across all filesystems.

// Reads `size` bytes at `offset` into `buf`. Returns bytes read.
template <typename ResolveBlock, typename ReadBlock>
fk::core::Result<size_t, fk::core::Error>
scatter_read(uint64_t offset, size_t size, uint8_t* buf,
             size_t block_size, uint8_t* scratch,
             ResolveBlock resolve_block, ReadBlock read_block) {
    if (size == 0) return static_cast<size_t>(0);

    size_t done = 0;
    while (done < size) {
        uint64_t abs     = offset + done;
        uint32_t lblock  = static_cast<uint32_t>(abs / block_size);
        uint32_t blk_off = static_cast<uint32_t>(abs % block_size);
        size_t   chunk   = block_size - blk_off;
        if (chunk > size - done) chunk = size - done;

        auto res = resolve_block(lblock);
        if (res.is_error()) return res.error();
        uint64_t phys = res.value();

        if (phys == 0) {
            fk::memory::set(buf + done, 0, chunk);
        } else {
            auto rr = read_block(phys, scratch);
            if (rr.is_error()) return rr.error();
            fk::memory::copy(buf + done, scratch + blk_off, chunk);
        }
        done += chunk;
    }
    return done;
}

// Writes `size` bytes from `buf` at `offset`. Full-block writes bypass the
// scratch; partial blocks are read-modify-written. Returns bytes written.
template <typename ResolveBlock, typename ReadBlock, typename WriteBlock>
fk::core::Result<size_t, fk::core::Error>
scatter_write(uint64_t offset, size_t size, const uint8_t* buf,
              size_t block_size, uint8_t* scratch,
              ResolveBlock resolve_block, ReadBlock read_block,
              WriteBlock write_block) {
    if (size == 0) return static_cast<size_t>(0);

    size_t done = 0;
    while (done < size) {
        uint64_t abs     = offset + done;
        uint32_t lblock  = static_cast<uint32_t>(abs / block_size);
        uint32_t blk_off = static_cast<uint32_t>(abs % block_size);
        size_t   chunk   = block_size - blk_off;
        if (chunk > size - done) chunk = size - done;

        auto res = resolve_block(lblock);
        if (res.is_error()) return res.error();
        uint64_t phys = res.value();
        if (phys == 0) return fk::core::Error::InvalidData;

        if (blk_off == 0 && chunk == block_size) {
            auto wr = write_block(phys, buf + done);
            if (wr.is_error()) return wr.error();
        } else {
            auto rd = read_block(phys, scratch);
            if (rd.is_error()) return rd.error();
            fk::memory::copy(scratch + blk_off, buf + done, chunk);
            auto wr = write_block(phys, scratch);
            if (wr.is_error()) return wr.error();
        }
        done += chunk;
    }
    return done;
}

} // namespace algorithms
} // namespace fk
