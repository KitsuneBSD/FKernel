#pragma once

#include <LibFK/Core/result.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// Unix-style indirect block resolution: a file's blocks live in a small
// direct array followed by single/double/triple indirect index blocks.
//
//   resolve_pointer(block_number, index) -> Result<T, Error>
//
// reads one pointer entry (`index`) from the index block `block_number` and
// returns the physical block (or fragment) number it holds. `T` is the pointer
// width (uint32_t or uint64_t). `roots` is the file's direct-array followed by
// the single/double/triple indirect root slots (e.g. ext2 i_block[15] with 12
// direct, or UFS di_db[12] + di_ib[3]).
//
// Returns the resolved physical block, or Error::InvalidData when `logical`
// exceeds the addressable range.

template <typename T, typename ResolvePointer>
fk::core::Result<T, fk::core::Error>
resolve_indirect(uint32_t logical, uint32_t nindir,
                 const T* roots, uint32_t ndirect, uint32_t nroots,
                 ResolvePointer resolve_pointer) {
    if (logical < ndirect)
        return roots[logical];

    uint32_t idx = logical - ndirect;

    // Single indirect
    if (idx < nindir)
        return resolve_pointer(roots[ndirect], idx);

    idx -= nindir;

    // Double indirect
    uint32_t sq = nindir * nindir;
    if (idx < sq) {
        uint32_t l1 = idx / nindir;
        uint32_t l2 = idx % nindir;
        T dind = TRY(resolve_pointer(roots[ndirect + 1], l1));
        if (dind == 0) return fk::core::Error::InvalidData;
        return resolve_pointer(dind, l2);
    }
    idx -= sq;

    // Triple indirect
    if (idx < sq * nindir) {
        uint32_t l1 = idx / sq;
        uint32_t l2 = (idx % sq) / nindir;
        uint32_t l3 = idx % nindir;
        T t = TRY(resolve_pointer(roots[ndirect + 2], l1));
        if (t == 0) return fk::core::Error::InvalidData;
        T d = TRY(resolve_pointer(t, l2));
        if (d == 0) return fk::core::Error::InvalidData;
        return resolve_pointer(d, l3);
    }

    (void)nroots;
    return fk::core::Error::InvalidData;
}

} // namespace algorithms
} // namespace fk
