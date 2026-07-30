#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

// UFS on-disk values are always little-endian on x86 images.
// These helpers exist so the intent is explicit and BSD-origin code is easy
// to adapt if big-endian media is ever needed.
inline uint32_t ufs_le32(uint32_t v) { return v; }
inline uint64_t ufs_le64(uint64_t v) { return v; }
inline int32_t  ufs_le32s(int32_t v) { return v; }
inline int64_t  ufs_le64s(int64_t v) { return v; }

} // namespace fkernel
